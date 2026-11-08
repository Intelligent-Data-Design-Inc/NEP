/**
 * @file bfloat16_io_perf.c
 * @brief Benchmark NC_FLOAT vs NC_BFLOAT16 write and read time on data
 *        already in bfloat16, with no conversion.
 *
 * Unlike bfloat16_perf.c, which times the float32 to bfloat16
 * conversion as part of the write and read, this benchmark measures
 * pure I/O: it writes and reads the four GDAS surface fields
 * (@c tmp2m, @c prate_ave, @c soilw1, @c land) in four
 * configurations: (float, contiguous), (bfloat16, contiguous),
 * (float, deflate level 1 with shuffle) and (bfloat16, deflate level
 * 1 with shuffle). The bfloat16 data come from
 * @c test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc, a bfloat16 copy
 * of the GDAS sample made by make_bfloat16_sample.c, so the timed
 * regions contain no conversion at all. Each configuration is
 * repeated 3 times and the minimum times are kept.
 *
 * The program first checks that the bfloat16 sample's stored bits are
 * exactly float_to_bfloat16() of the float sample's values, fill
 * points included, and fails if they are not.
 *
 * Write time runs from nc_create() through nc_close(). Read time runs
 * from nc_open() through nc_close(). Timing uses
 * clock_gettime(CLOCK_MONOTONIC). Every read is checked bit-for-bit
 * against the data written.
 *
 * **Output:** CSV written to the path given on the command line, with
 * columns:
 *   @code
 *   field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_speedup,read_speedup
 *   @endcode
 *   @c ratio_vs_float is the (float, same storage) @c file_bytes
 *   divided by this row's; @c write_speedup and @c read_speedup are
 *   the (float, same storage) times divided by this row's. They are
 *   1.000 for the float rows. Each row is also printed to stdout.
 *
 * **Typical workflow:**
 * @code
 *   ./bfloat16_io_perf gdas_sfc_sample.nc gdas_sfc_sample_bfloat16.nc bfloat16_io_results.csv
 *   python3 plot_bfloat16_io.py bfloat16_io_results.csv bfloat16
 * @endcode
 *
 * **Learning Objectives:**
 * - Measure the I/O benefit of bfloat16 storage with no conversion
 * - Compare contiguous and shuffle+deflate storage for both types
 * - Check a bfloat16 file against a float32 reference
 *
 * **Key Concepts:**
 * - **Data already in bfloat16, no conversion**: the timed regions
 *   are pure I/O through nc_put_vara_bfloat16()/nc_get_vara_bfloat16()
 *   on raw uint16_t bits
 * - **Minimum of 3 runs**: reduces timing noise
 * - **Gates**: the sample bits must be float_to_bfloat16() of the
 *   float sample, and every contiguous bfloat16 file must be smaller
 *   than its float file
 *
 * **Related Examples:**
 * - bfloat16_perf.c - Benchmark that includes the conversion cost
 * - bfloat16.c      - Tutorial that stores tmp2m as NC_BFLOAT16
 *
 * @note The program is intended for local performance profiling. It is
 *       registered as a ctest test only when NEP_ENABLE_BENCHMARKS=ON.
 *       Reference results are measured with
 *       `-DCMAKE_BUILD_TYPE=RelWithDebInfo` (-O2).
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <nep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "bfloat16_conv.h"

/** Number of GDAS fields benchmarked. */
#define NFIELDS 4
/** Field names in the GDAS sample. */
static const char *FIELDS[NFIELDS] = {"tmp2m", "prate_ave", "soilw1", "land"};
/** Length of the grid_yt dimension of the GDAS sample. */
#define NY 768
/** Length of the grid_xt dimension of the GDAS sample. */
#define NX 1536
/** Number of values in one field. */
#define NVALS ((size_t)NY * NX)
/** Repetitions per configuration; the minimum time is kept. */
#define NREPS 3
/** Temporary NetCDF file created for each measurement. */
#define TMP_FILE "bfloat16_io_perf_tmp.nc"

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/**
 * @brief Return current wall-clock time in seconds.
 * @return Seconds from CLOCK_MONOTONIC as a double.
 */
static double
get_time(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/**
 * @brief One benchmark row for one field/type/storage configuration.
 */
typedef struct {
    const char *field;       /**< Field name. */
    const char *type;        /**< "float" or "bfloat16". */
    const char *storage;     /**< "contiguous" or "deflate1". */
    size_t uncompressed;     /**< NY*NX*sizeof(element). */
    long long file_bytes;    /**< File size from the last repetition. */
    double write_s;          /**< Minimum write time. */
    double read_s;           /**< Minimum read time. */
} row_t;

/**
 * @brief Write and read one field in one configuration NREPS times.
 *
 * For each repetition: creates @c TMP_FILE with
 * nc_create(NC_NEXTCDF4 | NC_CLOBBER), defines fixed dims
 * @c time = 1, @c grid_yt = NY, @c grid_xt = NX and one variable of
 * the field name with no fill value (nc_def_var_fill NC_NOFILL for
 * both types, so the float and bfloat16 files match), optionally
 * chunked 1x768x1536 with shuffle and deflate level 1, writes the
 * data with nc_put_vara_float() or
 * nc_put_vara_bfloat16(), closes the file, stats it, reopens it,
 * reads the data back and checks the bits with memcmp(). There is no
 * conversion in either timed region. The minimum write and read times
 * and the last repetition's file size are returned in @p row.
 *
 * @param field   Field name (used as the variable name).
 * @param data    Input data (NVALS floats or uint16_t bfloat16 bits).
 * @param is_bf16 Non-zero for an NC_BFLOAT16 variable.
 * @param deflate Non-zero for shuffle+deflate level 1.
 * @param row     Result row to fill in.
 * @param rbuf    Scratch buffer of at least NVALS*4 bytes.
 * @return 0 on success, 1 on verification failure, exits on NetCDF error.
 */
static int
run_config(const char *field, const void *data, int is_bf16, int deflate,
           row_t *row, void *rbuf)
{
    int rep;
    struct stat st;

    row->field = field;
    row->type = is_bf16 ? "bfloat16" : "float";
    row->storage = deflate ? "deflate1" : "contiguous";
    row->uncompressed = NVALS * (is_bf16 ? sizeof(uint16_t) : sizeof(float));
    row->write_s = 1e30;
    row->read_s = 1e30;
    row->file_bytes = 0;

    for (rep = 0; rep < NREPS; rep++)
    {
        int ncid, varid, dimids[3];
        size_t chunksizes[3] = {1, NY, NX};
        size_t start[3] = {0, 0, 0};
        size_t count[3] = {1, NY, NX};
        double t0, write_s, read_s;
        int ret;

        /* --- Write pass: pure I/O, no conversion. ----------------- */
        t0 = get_time();

        if ((ret = nc_create(TMP_FILE, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "time", 1, &dimids[0]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_yt", NY, &dimids[1]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_xt", NX, &dimids[2]))) ERR(ret);
        if ((ret = nc_def_var(ncid, field, is_bf16 ? NC_BFLOAT16 : NC_FLOAT,
                             3, dimids, &varid))) ERR(ret);
        if ((ret = nc_def_var_fill(ncid, varid, NC_NOFILL, NULL))) ERR(ret);
        if (deflate)
        {
            if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
            if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, 1))) ERR(ret);
        }
        if ((ret = nc_enddef(ncid))) ERR(ret);
        if (is_bf16)
        {
            if ((ret = nc_put_vara_bfloat16(ncid, varid, start, count,
                                            (nc_bfloat16_t *)(void *)data))) ERR(ret);
        }
        else
        {
            if ((ret = nc_put_vara_float(ncid, varid, start, count, data))) ERR(ret);
        }
        if ((ret = nc_close(ncid))) ERR(ret);

        write_s = get_time() - t0;
        if (write_s < row->write_s)
            row->write_s = write_s;

        if (stat(TMP_FILE, &st) != 0)
        {
            perror("stat");
            return 1;
        }
        row->file_bytes = (long long)st.st_size;

        /* --- Read pass: pure I/O, no conversion. ------------------ */
        t0 = get_time();

        if ((ret = nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, field, &varid))) ERR(ret);
        if (is_bf16)
        {
            if ((ret = nc_get_vara_bfloat16(ncid, varid, start, count,
                                            (nc_bfloat16_t *)(void *)rbuf))) ERR(ret);
        }
        else
        {
            if ((ret = nc_get_vara_float(ncid, varid, start, count, rbuf))) ERR(ret);
        }
        if ((ret = nc_close(ncid))) ERR(ret);

        read_s = get_time() - t0;
        if (read_s < row->read_s)
            row->read_s = read_s;

        /* Check the bits read against the bits written. */
        if (memcmp(data, rbuf, NVALS * (is_bf16 ? 2 : 4)))
        {
            fprintf(stderr, "%s: %s %s bits did not round-trip\n",
                    field, row->type, row->storage);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Main entry point.
 *
 * For each of the four GDAS fields, reads the float32 data from the
 * float sample and the bfloat16 bits from the bfloat16 sample,
 * verifies the bits are float_to_bfloat16() of the floats, runs the
 * four configurations (float/bfloat16 x contiguous/deflate1) and
 * appends one CSV row per configuration to the output file, printing
 * each to stdout.
 *
 * @param argc Argument count; must be 4.
 * @param argv argv[1] is gdas_sfc_sample.nc, argv[2]
 *             gdas_sfc_sample_bfloat16.nc, argv[3] the output CSV path.
 * @return 0 on success, 1 on gate or verification failure, 2 on usage
 *         or NetCDF error.
 */
int
main(int argc, char **argv)
{
    float *fdata;
    uint16_t *bdata;
    void *rbuf;
    row_t rows[4];
    FILE *fp;
    int f, c;
    int fail = 0;

    if (argc != 4)
    {
        fprintf(stderr, "usage: bfloat16_io_perf <gdas_sfc_sample.nc> <gdas_sfc_sample_bfloat16.nc> <results.csv>\n");
        return 2;
    }

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

#ifndef __OPTIMIZE__
    fprintf(stderr, "warning: built without compiler optimization; "
            "timings are not representative (use -DCMAKE_BUILD_TYPE=RelWithDebInfo)\n");
#endif

    fdata = (float *)malloc(NVALS * sizeof(float));
    bdata = (uint16_t *)malloc(NVALS * sizeof(uint16_t));
    rbuf = malloc(NVALS * sizeof(float));
    if (!fdata || !bdata || !rbuf)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    fp = fopen(argv[3], "w");
    if (!fp)
    {
        fprintf(stderr, "cannot open %s for writing\n", argv[3]);
        return 1;
    }
    fprintf(fp, "field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_speedup,read_speedup\n");
    printf("field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_speedup,read_speedup\n");

    for (f = 0; f < NFIELDS; f++)
    {
        int ncid, varid;
        long long float_bytes[2];
        double float_write_s[2], float_read_s[2];
        size_t i;
        int ret;

        /* Read the field as float with the built-in backend. */
        if ((ret = nc_open(argv[1], NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, FIELDS[f], &varid))) ERR(ret);
        if ((ret = nc_get_var_float(ncid, varid, fdata))) ERR(ret);
        if ((ret = nc_close(ncid))) ERR(ret);

        /* Read the same field's raw bits from the bfloat16 sample. */
        if ((ret = nc_open(argv[2], NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, FIELDS[f], &varid))) ERR(ret);
        if ((ret = nc_get_var_bfloat16(ncid, varid,
                                      (nc_bfloat16_t *)(void *)bdata))) ERR(ret);
        if ((ret = nc_close(ncid))) ERR(ret);

        /* The sample must hold exactly float_to_bfloat16() of the
         * float data, fill points included. */
        for (i = 0; i < NVALS; i++)
        {
            if (bdata[i] != float_to_bfloat16(fdata[i]))
            {
                fprintf(stderr, "%s: bfloat16 sample does not match "
                        "float_to_bfloat16 of the float sample at %zu\n",
                        FIELDS[f], i);
                fclose(fp);
                return 1;
            }
        }

        /* Run (float, contiguous), (bfloat16, contiguous), (float,
         * deflate1), (bfloat16, deflate1) in that order. */
        for (c = 0; c < 4; c++)
        {
            int is_bf16 = c % 2;
            int deflate = c >= 2;
            const void *data = is_bf16 ? (const void *)bdata : (const void *)fdata;
            double ratio, write_speedup, read_speedup;
            char line[256];

            if (run_config(FIELDS[f], data, is_bf16, deflate, &rows[c], rbuf))
            {
                fclose(fp);
                return 1;
            }
            if (!is_bf16)
            {
                float_bytes[deflate] = rows[c].file_bytes;
                float_write_s[deflate] = rows[c].write_s;
                float_read_s[deflate] = rows[c].read_s;
            }

            ratio = (double)float_bytes[deflate] / (double)rows[c].file_bytes;
            write_speedup = float_write_s[deflate] / rows[c].write_s;
            read_speedup = float_read_s[deflate] / rows[c].read_s;
            snprintf(line, sizeof(line),
                     "%s,%s,%s,%zu,%lld,%.3f,%.6f,%.6f,%.3f,%.3f",
                     rows[c].field, rows[c].type, rows[c].storage,
                     rows[c].uncompressed, rows[c].file_bytes, ratio,
                     rows[c].write_s, rows[c].read_s,
                     write_speedup, read_speedup);
            fprintf(fp, "%s\n", line);
            printf("%s\n", line);

            /* Gate: contiguous bfloat16 must be smaller than
             * contiguous float. Deflate rows are only reported. */
            if (is_bf16 && !deflate && rows[c].file_bytes >= float_bytes[0])
                fail = 1;
        }
    }

    fclose(fp);
    remove(TMP_FILE);

    free(fdata);
    free(bdata);
    free(rbuf);

    if (fail)
        fprintf(stderr, "bfloat16_io_perf: contiguous-size gate failed\n");
    return fail;
}

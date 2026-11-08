/**
 * @file bfloat16_perf.c
 * @brief Benchmark NC_FLOAT vs NC_BFLOAT16 file size and I/O
 *        throughput on four GDAS surface fields.
 *
 * Reads @c tmp2m, @c prate_ave, @c soilw1 and @c land from the GDAS
 * sample file @c test/data/netCDF-4/gdas_sfc_sample.nc and, for each
 * field, writes and reads a NEXTCDF-4 file in six configurations:
 * (float, contiguous), (bfloat16, contiguous), (bfloat16_hdf5,
 * contiguous), (float, deflate level 1 with shuffle), (bfloat16,
 * deflate level 1 with shuffle) and (bfloat16_hdf5, deflate level 1
 * with shuffle). Each configuration is repeated 3 times and the
 * minimum times are kept.
 *
 * The @c bfloat16 rows convert float32 to bfloat16 in user code and
 * exchange raw uint16_t bits through nc_put_vara_bfloat16() and
 * nc_get_vara_bfloat16(). The
 * @c bfloat16_hdf5 rows define the same NC_BFLOAT16 variable but write
 * the float data with nc_put_vara_float() and read with
 * nc_get_vara_float(), so the conversion happens inside HDF5 and
 * @c wconv_s and @c rconv_s are 0. For each bfloat16_hdf5 row a
 * @c #-prefixed line is printed to stdout (not to the CSV) with the
 * number of stored values that differ from round-to-nearest-even.
 *
 * Write time runs from just before the float32 to bfloat16 conversion
 * (bfloat16 only) through nc_close(). Read time runs from nc_open()
 * through nc_close() plus the conversion back to float for bfloat16.
 * Timing uses clock_gettime(CLOCK_MONOTONIC). Throughput is reported in
 * MB/s of float32 data for every row so the speeds compare the same
 * user data.
 *
 * **Output:** CSV written to the path given on the command line, with
 * columns:
 *   @code
 *   field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s
 *   @endcode
 *   @c wconv_s is the minimum float32-to-bfloat16 conversion time (part
 *   of @c write_s) and @c rconv_s the minimum bfloat16-to-float32
 *   conversion time (part of @c read_s); both are 0 for float and
 *   bfloat16_hdf5 rows.
 *   Each row is also printed to stdout.
 *
 * **Typical workflow:**
 * @code
 *   ./bfloat16_perf gdas_sfc_sample.nc bfloat16_results.csv
 *   python3 plot_bfloat16.py bfloat16_results.csv bfloat16
 * @endcode
 *
 * **Learning Objectives:**
 * - Measure the file-size benefit of bfloat16 storage
 * - Compare contiguous and shuffle+deflate storage for both types
 * - Quantify the maximum conversion error of bfloat16
 *
 * **Key Concepts:**
 * - **Raw bit patterns**: bfloat16 data is written and read as
 *   uint16_t through nc_put_vara_bfloat16()/nc_get_vara_bfloat16(), or
 *   converted inside HDF5 by the typed float calls
 * - **Minimum of 3 runs**: reduces timing noise
 * - **Gates**: every contiguous bfloat16 file must be smaller than its
 *   float file, and the maximum relative error must not exceed 2^-8
 *
 * **Related Examples:**
 * - bfloat16.c - Tutorial that stores tmp2m as NC_BFLOAT16
 * - deflate.c  - Deflate/shuffle benchmark for NetCDF-4 files
 *
 * @note The program is intended for local performance profiling. It is
 *       registered as a ctest test only when NEP_ENABLE_BENCHMARKS=ON.
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
#define TMP_FILE "bfloat16_perf_tmp.nc"

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

/** Which conversion path a configuration uses. */
typedef enum {
    CFG_FLOAT,     /**< NC_FLOAT variable, no conversion. */
    CFG_BF16_USER, /**< NC_BFLOAT16 variable, conversion in user code. */
    CFG_BF16_HDF5  /**< NC_BFLOAT16 variable, conversion inside HDF5. */
} cfg_type_t;

/**
 * @brief One benchmark row for one field/type/storage configuration.
 */
typedef struct {
    const char *field;       /**< Field name. */
    const char *type;        /**< "float", "bfloat16" or "bfloat16_hdf5". */
    const char *storage;     /**< "contiguous" or "deflate1". */
    size_t uncompressed;     /**< NY*NX*sizeof(element). */
    long long file_bytes;    /**< File size from the last repetition. */
    double write_s;          /**< Minimum write time. */
    double read_s;           /**< Minimum read time. */
    double max_abs_err;      /**< Max absolute error (non-fill points). */
    double max_rel_err;      /**< Max relative error (non-fill points). */
    double wconv_s;          /**< Minimum float->bfloat16 conversion time. */
    double rconv_s;          /**< Minimum bfloat16->float conversion time. */
} row_t;

/**
 * @brief Write and read one field in one configuration NREPS times.
 *
 * For each repetition: creates @c TMP_FILE with
 * nc_create(NC_NEXTCDF4 | NC_CLOBBER), defines fixed dims
 * @c time = 1, @c grid_yt = NY, @c grid_xt = NX and one variable of the
 * field name, optionally chunked 1x768x1536 with shuffle and deflate
 * level 1, writes the data, closes the file, stats it, reopens it,
 * reads the data back and checks it. @c CFG_FLOAT and @c CFG_BF16_USER
 * use the untyped calls and check the raw bits; @c CFG_BF16_HDF5 uses
 * the typed float calls and checks the read floats against the stored
 * raw bits on the last repetition. The minimum write and read times
 * and the last repetition's file size and error metrics are returned
 * in @p row.
 *
 * @param field   Field name (used as the variable name).
 * @param fdata   Input float data (NVALS elements).
 * @param type    Conversion path (CFG_FLOAT, CFG_BF16_USER or
 *                CFG_BF16_HDF5).
 * @param deflate  Non-zero for shuffle+deflate level 1, 0 for contiguous.
 * @param row     Result row to fill in.
 * @param bdata   Scratch buffer of NVALS uint16_t.
 * @param rdata   Scratch buffer of at least NVALS*4 bytes.
 * @param back    Scratch buffer of NVALS floats for the bfloat16 to
 *                float32 conversion.
 * @return 0 on success, 1 on verification failure, exits on NetCDF error.
 */
static int
run_config(const char *field, const float *fdata, cfg_type_t type, int deflate,
           row_t *row, uint16_t *bdata, uint16_t *rdata, float *back)
{
    int rep;
    int is_bf16 = (type != CFG_FLOAT);
    struct stat st;

    row->field = field;
    row->type = type == CFG_FLOAT ? "float"
              : type == CFG_BF16_USER ? "bfloat16" : "bfloat16_hdf5";
    row->storage = deflate ? "deflate1" : "contiguous";
    row->uncompressed = NVALS * (is_bf16 ? sizeof(uint16_t) : sizeof(float));
    row->write_s = 1e30;
    row->read_s = 1e30;
    row->file_bytes = 0;
    row->max_abs_err = 0.0;
    row->max_rel_err = 0.0;
    row->wconv_s = 0.0;
    row->rconv_s = 0.0;

    for (rep = 0; rep < NREPS; rep++)
    {
        int ncid, varid, dimids[3];
        size_t chunksizes[3] = {1, NY, NX};
        size_t start[3] = {0, 0, 0};
        size_t count[3] = {1, NY, NX};
        const void *wdata;
        void *rdata_typed;
        double t0, tc0, write_s, read_s, wconv_s = 0.0, rconv_s = 0.0;
        size_t i;
        int ret;

        /* --- Write pass: user conversion (bfloat16 only) is timed. - */
        t0 = get_time();

        if (type == CFG_BF16_USER)
        {
            tc0 = get_time();
            for (i = 0; i < NVALS; i++)
                bdata[i] = float_to_bfloat16(fdata[i]);
            wconv_s = get_time() - tc0;
            wdata = bdata;
            rdata_typed = rdata;
        }
        else
        {
            wdata = fdata;
            rdata_typed = rdata;
        }

        if ((ret = nc_create(TMP_FILE, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "time", 1, &dimids[0]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_yt", NY, &dimids[1]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_xt", NX, &dimids[2]))) ERR(ret);
        if ((ret = nc_def_var(ncid, field, is_bf16 ? NC_BFLOAT16 : NC_FLOAT,
                             3, dimids, &varid))) ERR(ret);
        if (deflate)
        {
            if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
            if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, 1))) ERR(ret);
        }
        if ((ret = nc_enddef(ncid))) ERR(ret);
        /* nc_put_vara_bfloat16 stores the raw bits on the NC_BFLOAT16
         * variable; nc_put_vara_float (CFG_BF16_HDF5) lets HDF5
         * convert, which rounds ties away from zero while
         * float_to_bfloat16() rounds ties to even. */
        if (type == CFG_BF16_USER)
        {
            if ((ret = nc_put_vara_bfloat16(ncid, varid, start, count,
                                            (nc_bfloat16_t *)(void *)bdata))) ERR(ret);
        }
        else
        {
            if ((ret = nc_put_vara_float(ncid, varid, start, count, wdata))) ERR(ret);
        }
        if ((ret = nc_close(ncid))) ERR(ret);

        write_s = get_time() - t0;
        if (write_s < row->write_s)
            row->write_s = write_s;
        if (type == CFG_BF16_USER && (rep == 0 || wconv_s < row->wconv_s))
            row->wconv_s = wconv_s;

        if (stat(TMP_FILE, &st) != 0)
        {
            perror("stat");
            return 1;
        }
        row->file_bytes = (long long)st.st_size;

        /* --- Read pass: includes conversion back to float. -------- */
        t0 = get_time();

        if ((ret = nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, field, &varid))) ERR(ret);
        /* nc_get_vara_bfloat16 returns the raw stored bits.
         * nc_get_vara_float on the NC_BFLOAT16 variable
         * (CFG_BF16_HDF5) returns the same values converted to float
         * by HDF5. */
        if (type == CFG_BF16_HDF5)
        {
            if ((ret = nc_get_vara_float(ncid, varid, start, count, back))) ERR(ret);
        }
        else if (type == CFG_BF16_USER)
        {
            if ((ret = nc_get_vara_bfloat16(ncid, varid, start, count,
                                            (nc_bfloat16_t *)(void *)rdata_typed))) ERR(ret);
        }
        else
        {
            if ((ret = nc_get_vara_float(ncid, varid, start, count, rdata_typed))) ERR(ret);
        }
        if ((ret = nc_close(ncid))) ERR(ret);

        if (type == CFG_BF16_USER)
        {
            tc0 = get_time();
            for (i = 0; i < NVALS; i++)
                back[i] = bfloat16_to_float(rdata[i]);
            rconv_s = get_time() - tc0;
        }

        read_s = get_time() - t0;
        if (read_s < row->read_s)
            row->read_s = read_s;
        if (type == CFG_BF16_USER && (rep == 0 || rconv_s < row->rconv_s))
            row->rconv_s = rconv_s;

        /* For CFG_BF16_HDF5, reopen and read the raw bits so the typed
         * read can be checked and compared to round-to-nearest-even. */
        if (type == CFG_BF16_HDF5 && rep == NREPS - 1)
        {
            size_t n_diff = 0;

            if ((ret = nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
            if ((ret = nc_inq_varid(ncid, field, &varid))) ERR(ret);
            if ((ret = nc_get_vara_bfloat16(ncid, varid, start, count,
                                            (nc_bfloat16_t *)(void *)rdata))) ERR(ret);
            if ((ret = nc_close(ncid))) ERR(ret);
            for (i = 0; i < NVALS; i++)
            {
                uint32_t fbits;

                memcpy(&fbits, &back[i], sizeof(fbits));
                if (fbits != (uint32_t)rdata[i] << 16)
                {
                    fprintf(stderr, "%s: bfloat16_hdf5 %s typed read does not match stored bits\n",
                            field, row->storage);
                    return 1;
                }
                if (rdata[i] != float_to_bfloat16(fdata[i]))
                    n_diff++;
            }
            printf("# %s %s: %zu of %zu values differ from round-to-nearest-even\n",
                   field, row->storage, n_diff, NVALS);
        }

        /* Error metrics are computed outside the timed regions. */
        if (is_bf16 && rep == NREPS - 1)
        {
            row->max_abs_err = 0.0;
            row->max_rel_err = 0.0;
            for (i = 0; i < NVALS; i++)
            {
                double err, rel;

                if (fdata[i] == BF16_FILL_IN)
                    continue;
                err = (double)back[i] - (double)fdata[i];
                if (err < 0.0)
                    err = -err;
                if (err > row->max_abs_err)
                    row->max_abs_err = err;
                if (fdata[i] != 0.0f)
                {
                    rel = err / (double)(fdata[i] < 0.0f ? -fdata[i] : fdata[i]);
                    if (rel > row->max_rel_err)
                        row->max_rel_err = rel;
                }
            }
        }

        /* Check the bits read against the bits written. */
        if (type != CFG_BF16_HDF5 &&
            memcmp(wdata, rdata_typed, NVALS * (is_bf16 ? 2 : 4)))
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
 * For each of the four GDAS fields, runs the six configurations
 * (float/bfloat16/bfloat16_hdf5 x contiguous/deflate1), appends one CSV row per
 * configuration to the output file and prints it to stdout.
 *
 * @param argc Argument count; must be 3.
 * @param argv argv[1] is the input gdas_sfc_sample.nc path, argv[2]
 *             the output CSV path.
 * @return 0 on success, 1 on gate or verification failure, 2 on usage
 *         or NetCDF error.
 */
int
main(int argc, char **argv)
{
    float *fdata, *back;
    uint16_t *bdata, *rdata;
    row_t rows[6];
    FILE *fp;
    int f, c;
    int fail = 0;

    if (argc != 3)
    {
        fprintf(stderr, "usage: bfloat16_perf <gdas_sfc_sample.nc> <results.csv>\n");
        return 2;
    }

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

    fdata = (float *)malloc(NVALS * sizeof(float));
    bdata = (uint16_t *)malloc(NVALS * sizeof(uint16_t));
    rdata = (uint16_t *)malloc(NVALS * sizeof(float));
    back = (float *)malloc(NVALS * sizeof(float));
    if (!fdata || !bdata || !rdata || !back)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    fp = fopen(argv[2], "w");
    if (!fp)
    {
        fprintf(stderr, "cannot open %s for writing\n", argv[2]);
        return 1;
    }
    fprintf(fp, "field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s\n");
    printf("field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s\n");

    for (f = 0; f < NFIELDS; f++)
    {
        int ncid, varid;
        long long float_bytes[2];
        int ret;

        /* Read the field with the built-in NetCDF-4 backend. */
        if ((ret = nc_open(argv[1], NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, FIELDS[f], &varid))) ERR(ret);
        if ((ret = nc_get_var_float(ncid, varid, fdata))) ERR(ret);
        if ((ret = nc_close(ncid))) ERR(ret);

        /* Run (float, contiguous), (bfloat16, contiguous),
         * (bfloat16_hdf5, contiguous), (float, deflate1), (bfloat16,
         * deflate1), (bfloat16_hdf5, deflate1) in that order. */
        for (c = 0; c < 6; c++)
        {
            cfg_type_t type = (cfg_type_t)(c % 3);
            int deflate = (c >= 3);
            double ratio, write_mbps, read_mbps;
            char line[256];

            if (run_config(FIELDS[f], fdata, type, deflate, &rows[c],
                           bdata, rdata, back))
            {
                fclose(fp);
                return 1;
            }
            if (type == CFG_FLOAT)
                float_bytes[deflate] = rows[c].file_bytes;

            ratio = (double)float_bytes[deflate] / (double)rows[c].file_bytes;
            write_mbps = (double)NVALS * 4.0 / 1e6 / rows[c].write_s;
            read_mbps = (double)NVALS * 4.0 / 1e6 / rows[c].read_s;
            snprintf(line, sizeof(line),
                     "%s,%s,%s,%zu,%lld,%.3f,%.6f,%.6f,%.3f,%.3f,%.6g,%.6g,%.6f,%.6f",
                     rows[c].field, rows[c].type, rows[c].storage,
                     rows[c].uncompressed, rows[c].file_bytes, ratio,
                     rows[c].write_s, rows[c].read_s, write_mbps, read_mbps,
                     rows[c].max_abs_err, rows[c].max_rel_err,
                     rows[c].wconv_s, rows[c].rconv_s);
            fprintf(fp, "%s\n", line);
            printf("%s\n", line);

            /* Gates: contiguous bfloat16 must be smaller than
             * contiguous float; relative error must not exceed 2^-8.
             * Deflate rows are only reported. */
            if (type != CFG_FLOAT && !deflate &&
                rows[c].file_bytes >= float_bytes[0])
                fail = 1;
            if (type != CFG_FLOAT && rows[c].max_rel_err > BF16_REL_ERR_MAX)
                fail = 1;
        }
    }

    fclose(fp);
    remove(TMP_FILE);

    free(fdata);
    free(back);
    free(bdata);
    free(rdata);

    if (fail)
        fprintf(stderr, "bfloat16_perf: size or relative-error gate failed\n");
    return fail;
}

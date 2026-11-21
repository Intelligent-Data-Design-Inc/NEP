/**
 * @file float16_perf.c
 * @brief Benchmark NC_FLOAT vs NC_FLOAT16 file size and I/O
 *        throughput on four GDAS surface fields.
 *
 * Reads @c tmp2m, @c prate_ave, @c soilw1 and @c land from the GDAS
 * sample file @c test/data/netCDF-4/gdas_sfc_sample.nc and, for each
 * field, writes and reads a NEXTCDF-4 file in six configurations:
 * (float, contiguous), (float16, contiguous), (float16_typed,
 * contiguous), (float, deflate level 1 with shuffle), (float16,
 * deflate level 1 with shuffle) and (float16_typed, deflate level 1
 * with shuffle). Each file holds @c NCOPIES (10) copies of the field
 * as time steps, written and read one step at a time, so the fixed
 * cost of creating, opening and closing a file is spread over more
 * data. Each configuration is repeated 10 times and the minimum times
 * are kept.
 *
 * The @c float16 rows convert float32 to binary16 in user code and
 * exchange raw uint16_t bits through nc_put_vara_float16() and
 * nc_get_vara_float16(). The @c float16_typed rows define the same
 * NC_FLOAT16 variable but write the float data with
 * nc_put_vara_float() and read with nc_get_vara_float(), so the
 * internal conversion engine does the conversion; @c wconv_s and
 * @c rconv_s are 0. Both paths use NEP round-to-nearest-even, so they
 * produce identical stored bits - unlike the historical bfloat16 HDF5
 * rows, there is no differing-rounding report. The typed path's read
 * is the engine's convert-out path, which (issue #386) currently
 * takes an extra full-buffer pass over the converted data.
 *
 * Write time runs from just before the float32 to binary16 conversion
 * (float16 only) through nc_close() and an fsync() of the file
 * (nep_perf_sync_file()), so the data has reached the storage device.
 * Before each read the file is dropped from the page cache
 * (nep_perf_drop_cache(), untimed), so every read comes from the
 * device. Read time runs from nc_open() through nc_close() plus the
 * conversion back to float for float16. Both times cover the whole
 * file (all @c NCOPIES steps). Timing uses
 * clock_gettime(CLOCK_MONOTONIC). Throughput is reported in MB/s of
 * float32 data (@c NCOPIES x 4718592 bytes) for every row so the
 * speeds compare the same user data.
 *
 * **Output:** CSV written to the path given on the command line, with
 * columns:
 *   @code
 *   field,type,storage,copies,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s
 *   @endcode
 *   @c copies is @c NCOPIES; @c uncompressed_bytes, @c file_bytes,
 *   @c write_s and @c read_s cover all copies.
 *   @c wconv_s is the minimum float32-to-binary16 conversion time (part
 *   of @c write_s) and @c rconv_s the minimum binary16-to-float32
 *   conversion time (part of @c read_s); both are 0 for float and
 *   float16_typed rows.
 *   Each row is also printed to stdout.
 *
 * **Typical workflow:**
 * @code
 *   ./float16_perf gdas_sfc_sample.nc float16_results.csv
 *   python3 plot_float16.py float16_results.csv float16
 * @endcode
 *
 * **Learning Objectives:**
 * - Measure the file-size benefit of binary16 storage
 * - Compare contiguous and shuffle+deflate storage for both types
 * - Quantify the maximum conversion error of binary16, including
 *   subnormal underflow on small @c prate_ave values
 *
 * **Key Concepts:**
 * - **Raw bit patterns**: float16 data is written and read as
 *   uint16_t through nc_put_vara_float16()/nc_get_vara_float16(), or
 *   converted by the internal engine in the typed float calls
 * - **Minimum of 10 runs**: reduces timing noise
 * - **Gates**: every contiguous float16 file must be smaller than its
 *   float file, and every non-fill point must be within the error
 *   gate: relative error at most 2^-11, or absolute error below half
 *   the smallest binary16 subnormal (subnormal underflow)
 *
 * **Related Examples:**
 * - float16.c - Tutorial that stores tmp2m as NC_FLOAT16
 * - bfloat16_perf.c - The bfloat16 version of this benchmark
 * - deflate.c  - Deflate/shuffle benchmark for NetCDF-4 files
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
#include "nep_meta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "float16_conv.h"
#include "perf_disk.h"

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
#define NREPS 10
/** Copies of the field written to (and read from) each file, as time
 * steps, so the fixed cost of opening and closing a file is spread over
 * more data. */
#define NCOPIES 10
/** Temporary NetCDF file created for each measurement. */
#define TMP_FILE "float16_perf_tmp.nc"

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
    CFG_FLOAT,      /**< NC_FLOAT variable, no conversion. */
    CFG_F16_USER,   /**< NC_FLOAT16 variable, conversion in user code. */
    CFG_F16_TYPED   /**< NC_FLOAT16 variable, conversion by the internal engine. */
} cfg_type_t;

/**
 * @brief One benchmark row for one field/type/storage configuration.
 */
typedef struct {
    const char *field;       /**< Field name. */
    const char *type;        /**< "float", "float16" or "float16_typed". */
    const char *storage;     /**< "contiguous" or "deflate1". */
    size_t uncompressed;     /**< NY*NX*sizeof(element). */
    long long file_bytes;    /**< File size from the last repetition. */
    double write_s;          /**< Minimum write time. */
    double read_s;           /**< Minimum read time. */
    double max_abs_err;      /**< Max absolute error (non-fill points). */
    double max_rel_err;      /**< Max relative error (non-fill points). */
    size_t gate_bad;         /**< Points outside the error gate. */
    double wconv_s;          /**< Minimum float->float16 conversion time. */
    double rconv_s;          /**< Minimum float16->float conversion time. */
} row_t;

/**
 * @brief Write and read one field in one configuration NREPS times.
 *
 * For each repetition: creates @c TMP_FILE with
 * nc_create(NC_NEXTCDF4 | NC_CLOBBER), defines fixed dims
 * @c time = NCOPIES, @c grid_yt = NY, @c grid_xt = NX and one variable
 * of the field name, optionally chunked 1x768x1536 with shuffle and
 * deflate level 1, writes the field once per time step (converting it
 * again for each step in the @c CFG_F16_USER case), closes and fsyncs
 * the file (timed), stats it, drops it from the page cache (untimed),
 * reopens it, reads every time step back and checks it. @c CFG_FLOAT
 * and @c CFG_F16_USER use nc_put_vara_float()/nc_put_vara_float16()
 * and check the raw bits; @c CFG_F16_TYPED uses the typed float calls
 * and checks the read floats and the stored raw bits on the last
 * repetition. The minimum write and read times and the last
 * repetition's file size and error metrics are returned in @p row.
 *
 * @param field   Field name (used as the variable name).
 * @param fdata   Input float data (NVALS elements).
 * @param type    Conversion path (CFG_FLOAT, CFG_F16_USER or
 *                CFG_F16_TYPED).
 * @param deflate  Non-zero for shuffle+deflate level 1, 0 for contiguous.
 * @param row     Result row to fill in.
 * @param hdata   Scratch buffer of NVALS uint16_t.
 * @param rdata   Scratch buffer of at least NCOPIES*NVALS*4 bytes.
 * @param back    Scratch buffer of NCOPIES*NVALS floats for the
 *                binary16 to float32 conversion.
 * @return 0 on success, 1 on verification failure, exits on NetCDF error.
 */
static int
run_config(const char *field, const float *fdata, cfg_type_t type, int deflate,
           row_t *row, uint16_t *hdata, uint16_t *rdata, float *back)
{
    int rep;
    int is_f16 = (type != CFG_FLOAT);
    size_t esize = is_f16 ? sizeof(uint16_t) : sizeof(float);
    struct stat st;

    row->field = field;
    row->type = type == CFG_FLOAT ? "float"
              : type == CFG_F16_USER ? "float16" : "float16_typed";
    row->storage = deflate ? "deflate1" : "contiguous";
    row->uncompressed = NCOPIES * NVALS * esize;
    row->write_s = 1e30;
    row->read_s = 1e30;
    row->file_bytes = 0;
    row->max_abs_err = 0.0;
    row->max_rel_err = 0.0;
    row->gate_bad = 0;
    row->wconv_s = 0.0;
    row->rconv_s = 0.0;

    for (rep = 0; rep < NREPS; rep++)
    {
        int ncid, varid, dimids[3];
        size_t chunksizes[3] = {1, NY, NX};
        size_t start[3] = {0, 0, 0};
        size_t count[3] = {1, NY, NX};
        double t0, tc0, write_s, read_s, wconv_s = 0.0, rconv_s = 0.0;
        size_t i, step;
        float *last;
        int ret;

        /* --- Write pass: user conversion (float16 only) is timed. -- */
        t0 = get_time();

        if ((ret = nc_create(TMP_FILE, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "time", NCOPIES, &dimids[0]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_yt", NY, &dimids[1]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "grid_xt", NX, &dimids[2]))) ERR(ret);
        if ((ret = nc_def_var(ncid, field, is_f16 ? NC_FLOAT16 : NC_FLOAT,
                             3, dimids, &varid))) ERR(ret);
        if (deflate)
        {
            if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
            if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, 1))) ERR(ret);
        }
        if ((ret = nc_enddef(ncid))) ERR(ret);
        for (step = 0; step < NCOPIES; step++)
        {
            start[0] = step;
            /* nc_put_vara_float16 stores the raw bits on the
             * NC_FLOAT16 variable; nc_put_vara_float (CFG_F16_TYPED)
             * lets the internal engine convert, which uses the same
             * round-to-nearest-even as float_to_float16(). */
            if (type == CFG_F16_USER)
            {
                tc0 = get_time();
                for (i = 0; i < NVALS; i++)
                    hdata[i] = float_to_float16(fdata[i]);
                wconv_s += get_time() - tc0;
                if ((ret = nc_put_vara_float16(ncid, varid, start, count,
                                               (nc_float16_t *)(void *)hdata))) ERR(ret);
            }
            else
            {
                if ((ret = nc_put_vara_float(ncid, varid, start, count, fdata))) ERR(ret);
            }
        }
        if ((ret = nc_close(ncid))) ERR(ret);
        if (nep_perf_sync_file(TMP_FILE))
        {
            perror("fsync");
            return 1;
        }

        write_s = get_time() - t0;
        if (write_s < row->write_s)
            row->write_s = write_s;
        if (type == CFG_F16_USER && (rep == 0 || wconv_s < row->wconv_s))
            row->wconv_s = wconv_s;

        if (stat(TMP_FILE, &st) != 0)
        {
            perror("stat");
            return 1;
        }
        row->file_bytes = (long long)st.st_size;

        /* Start the read from the device, not the page cache. */
        if (nep_perf_drop_cache(TMP_FILE))
        {
            perror("drop cache");
            return 1;
        }

        /* --- Read pass: includes conversion back to float. -------- */
        t0 = get_time();

        if ((ret = nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, field, &varid))) ERR(ret);
        for (step = 0; step < NCOPIES; step++)
        {
            start[0] = step;
            /* nc_get_vara_float16 returns the raw stored bits.
             * nc_get_vara_float on the NC_FLOAT16 variable
             * (CFG_F16_TYPED) returns the same values decoded to float. */
            if (type == CFG_F16_TYPED)
            {
                if ((ret = nc_get_vara_float(ncid, varid, start, count,
                                             back + step * NVALS))) ERR(ret);
            }
            else if (type == CFG_F16_USER)
            {
                uint16_t *r = rdata + step * NVALS;

                if ((ret = nc_get_vara_float16(ncid, varid, start, count,
                                               (nc_float16_t *)(void *)r))) ERR(ret);
                tc0 = get_time();
                for (i = 0; i < NVALS; i++)
                    back[step * NVALS + i] = float16_to_float(r[i]);
                rconv_s += get_time() - tc0;
            }
            else
            {
                if ((ret = nc_get_vara_float(ncid, varid, start, count,
                                             (float *)(void *)rdata + step * NVALS))) ERR(ret);
            }
        }
        if ((ret = nc_close(ncid))) ERR(ret);

        read_s = get_time() - t0;
        if (read_s < row->read_s)
            row->read_s = read_s;
        if (type == CFG_F16_USER && (rep == 0 || rconv_s < row->rconv_s))
            row->rconv_s = rconv_s;

        /* For CFG_F16_TYPED, reopen and read the raw bits so the typed
         * read can be checked. The engine and float_to_float16() both
         * use round-to-nearest-even, so a mismatch at any non-fill
         * point is a failure, not an informational report. */
        if (type == CFG_F16_TYPED && rep == NREPS - 1)
        {
            if ((ret = nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
            if ((ret = nc_inq_varid(ncid, field, &varid))) ERR(ret);
            for (step = 0; step < NCOPIES; step++)
            {
                start[0] = step;
                if ((ret = nc_get_vara_float16(ncid, varid, start, count,
                                               (nc_float16_t *)(void *)(rdata + step * NVALS)))) ERR(ret);
            }
            if ((ret = nc_close(ncid))) ERR(ret);
            for (i = 0; i < NCOPIES * NVALS; i++)
            {
                uint32_t fbits, ebits;
                float expected = float16_to_float(rdata[i]);

                memcpy(&fbits, &back[i], sizeof(fbits));
                memcpy(&ebits, &expected, sizeof(ebits));
                if (fbits != ebits)
                {
                    fprintf(stderr, "%s: float16_typed %s typed read does not match stored bits\n",
                            field, row->storage);
                    return 1;
                }
            }
            for (i = 0; i < NVALS; i++)
            {
                if (fdata[i] == F16_FILL_IN)
                    continue;
                if (rdata[i] != float_to_float16(fdata[i]))
                {
                    fprintf(stderr, "%s: float16_typed %s stored bits differ from round-to-nearest-even\n",
                            field, row->storage);
                    return 1;
                }
            }
        }

        /* Error metrics are computed outside the timed regions, on the
         * last time step (every step holds the same data). */
        last = back + (NCOPIES - 1) * NVALS;
        if (is_f16 && rep == NREPS - 1)
        {
            row->max_abs_err = 0.0;
            row->max_rel_err = 0.0;
            row->gate_bad = 0;
            for (i = 0; i < NVALS; i++)
            {
                double err, rel;

                if (fdata[i] == F16_FILL_IN)
                    continue;
                err = (double)last[i] - (double)fdata[i];
                if (err < 0.0)
                    err = -err;
                if (err > row->max_abs_err)
                    row->max_abs_err = err;
                if (fdata[i] != 0.0f)
                {
                    rel = err / (double)(fdata[i] < 0.0f ? -fdata[i] : fdata[i]);
                    if (rel > row->max_rel_err)
                        row->max_rel_err = rel;
                    if (rel > F16_REL_ERR_MAX && err > F16_ABS_ERR_MAX)
                        row->gate_bad++;
                }
            }
        }

        /* Check the bits read against the bits written, every step. */
        if (type != CFG_F16_TYPED)
        {
            const void *wdata = fdata;

            if (type == CFG_F16_USER)
                wdata = hdata;
            for (step = 0; step < NCOPIES; step++)
            {
                if (memcmp(wdata, (const char *)rdata + step * NVALS * esize,
                           NVALS * esize))
                {
                    fprintf(stderr, "%s: %s %s bits did not round-trip\n",
                            field, row->type, row->storage);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Main entry point.
 *
 * For each of the four GDAS fields, runs the six configurations
 * (float/float16/float16_typed x contiguous/deflate1), appends one CSV
 * row per configuration to the output file and prints it to stdout.
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
    uint16_t *hdata, *rdata;
    row_t rows[6];
    FILE *fp;
    int f, c;
    int fail = 0;

    if (argc != 3)
    {
        fprintf(stderr, "usage: float16_perf <gdas_sfc_sample.nc> <results.csv>\n");
        return 2;
    }

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

#ifndef __OPTIMIZE__
    fprintf(stderr, "warning: built without compiler optimization; "
            "timings are not representative (use -DCMAKE_BUILD_TYPE=RelWithDebInfo)\n");
#endif

    fdata = (float *)malloc(NVALS * sizeof(float));
    hdata = (uint16_t *)malloc(NVALS * sizeof(uint16_t));
    rdata = (uint16_t *)malloc(NCOPIES * NVALS * sizeof(float));
    back = (float *)malloc(NCOPIES * NVALS * sizeof(float));
    if (!fdata || !hdata || !rdata || !back)
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
    fprintf(fp, "field,type,storage,copies,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s\n");
    printf("field,type,storage,copies,uncompressed_bytes,file_bytes,ratio_vs_float,write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,wconv_s,rconv_s\n");

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

        /* Run (float, contiguous), (float16, contiguous),
         * (float16_typed, contiguous), (float, deflate1), (float16,
         * deflate1), (float16_typed, deflate1) in that order. */
        for (c = 0; c < 6; c++)
        {
            cfg_type_t type = (cfg_type_t)(c % 3);
            int deflate = (c >= 3);
            double ratio, write_mbps, read_mbps;
            char line[256];

            if (run_config(FIELDS[f], fdata, type, deflate, &rows[c],
                           hdata, rdata, back))
            {
                fclose(fp);
                return 1;
            }
            if (type == CFG_FLOAT)
                float_bytes[deflate] = rows[c].file_bytes;

            ratio = (double)float_bytes[deflate] / (double)rows[c].file_bytes;
            write_mbps = (double)NCOPIES * NVALS * 4.0 / 1e6 / rows[c].write_s;
            read_mbps = (double)NCOPIES * NVALS * 4.0 / 1e6 / rows[c].read_s;
            snprintf(line, sizeof(line),
                     "%s,%s,%s,%d,%zu,%lld,%.3f,%.6f,%.6f,%.3f,%.3f,%.6g,%.6g,%.6f,%.6f",
                     rows[c].field, rows[c].type, rows[c].storage, NCOPIES,
                     rows[c].uncompressed, rows[c].file_bytes, ratio,
                     rows[c].write_s, rows[c].read_s, write_mbps, read_mbps,
                     rows[c].max_abs_err, rows[c].max_rel_err,
                     rows[c].wconv_s, rows[c].rconv_s);
            fprintf(fp, "%s\n", line);
            printf("%s\n", line);

            /* Gates: contiguous float16 must be smaller than
             * contiguous float; every non-fill point must be within
             * the error gate (relative error at most 2^-11, or
             * absolute error under the subnormal floor). Deflate rows
             * are only reported. */
            if (type != CFG_FLOAT && !deflate &&
                rows[c].file_bytes >= float_bytes[0])
                fail = 1;
            if (type != CFG_FLOAT && rows[c].gate_bad)
                fail = 1;
        }
    }

    fclose(fp);
    remove(TMP_FILE);

    free(fdata);
    free(back);
    free(hdata);
    free(rdata);

    if (fail)
        fprintf(stderr, "float16_perf: size or error gate failed\n");
    return fail;
}

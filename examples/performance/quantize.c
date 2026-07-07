/**
 * @file quantize.c
 * @brief Demonstrates how lossy quantization pre-filters interact with lossless
 *        compression to affect compression ratio, I/O throughput, and precision
 *        loss on a realistic scientific dataset.
 *
 * Quantization is a lossy pre-filter that zeroes excess bits of the IEEE 754
 * floating-point significand before a lossless compression step. Because
 * adjacent quantized values share identical trailing bits, lossless compressors
 * achieve substantially higher compression ratios. Unlike the shuffle filter,
 * quantization introduces a bounded but irreversible precision loss.
 *
 * Three quantization algorithms are tested:
 *
 * - **BitGroom** (NC_QUANTIZE_BITGROOM): applies a uniform bit mask derived
 *   from the requested number of significant decimal digits (NSD), alternating
 *   excess bits between 0 and 1 to preserve the array mean.
 * - **GranularBitRound** (NC_QUANTIZE_GRANULARBR): computes the minimum
 *   required bits per value using IEEE rounding, achieving better compression
 *   at the same NSD than BitGroom.
 * - **BitRound** (NC_QUANTIZE_BITROUND): user specifies the number of binary
 *   significand bits (NSB) to retain directly; free from BitGroom multipoint
 *   statistics artifacts. NSB values {3,6,9,13,16,19,23} correspond to the
 *   same worst-case decimal precision as BitGroom/GranularBitRound NSD 1–7.
 *
 * Each quantize algorithm × NSD/NSB level is paired with all five lossless
 * filters at their sprint-10 optimal settings (all with shuffle enabled).
 * Baseline rows (lossless only, no quantization) are printed first.
 *
 * The program measures max absolute error by reading back written data and
 * comparing element-wise to the original array. Baseline rows report 0.000000.
 *
 * **Call order**: shuffle → lossless filter → nc_def_var_quantize() → nc_enddef()
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   quantize_alg,nsd_or_nsb,filter,compressed_bytes,ratio,write_s,read_s,max_abs_err
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./quantize > quantize_results.csv
 *   python3 plot_quantize.py   # produces quantize_performance.jpg
 * @endcode
 *
 * **Learning Objectives:**
 * - Understand lossy quantization as a pre-filter that improves lossless compression
 * - Learn the three quantization algorithms: BitGroom, GranularBitRound, BitRound
 * - See how NSD (Number of Significant Digits) controls precision vs compression
 * - Measure the interaction between quantization and each lossless filter
 * - Quantify precision loss (max absolute error) to make informed trade-offs
 * - Recognize that quantization + lossless can achieve 10–50× compression ratios
 *   with bounded, predictable precision loss
 *
 * **Key Concepts:**
 * - **Quantization**: Lossy pre-filter that zeroes excess significand bits of
 *   IEEE 754 floats; the zeroed bits compress dramatically better because
 *   adjacent values share identical trailing bit patterns
 * - **NSD (Significant Digits)**: Number of significant decimal digits preserved;
 *   higher NSD = less compression but less precision loss
 * - **NSB (Significant Bits)**: Number of binary significand bits retained
 *   (BitRound only); provides finer control than NSD
 * - **BitGroom**: Alternates excess bits between 0 and 1 to preserve array mean;
 *   simplest algorithm, well-established
 * - **GranularBitRound**: Computes minimum required bits per value using IEEE
 *   rounding; better compression than BitGroom at same NSD
 * - **BitRound**: User specifies NSB directly; free from multipoint statistics
 *   artifacts; most control over precision/compression balance
 * - **Call Order**: shuffle → lossless filter → nc_def_var_quantize() → nc_enddef()
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking.c - Understanding chunked storage
 * - lossless.c - Understanding the lossless filter landscape
 * - deflate.c - Understanding baseline lossless performance
 *
 * **Related Examples:**
 * - lossless.c - Lossless-only comparison (baseline for quantize improvements)
 * - deflate.c - Deflate performance without quantization
 * - zstandard.c - Zstandard performance without quantization
 *
 * **Key API functions:**
 * - nc_def_var_quantize()    Enable quantization pre-filter (BitGroom/GranularBitRound/BitRound)
 * - nc_inq_var_quantize()    Query quantization settings on an open variable
 * - nc_def_var_deflate()     Enable shuffle and/or DEFLATE compression
 * - nc_def_var_zstandard()   Enable Zstandard compression
 * - nc_def_var_szip()        Enable SZIP compression
 * - nc_def_var_lz4()         Enable LZ4 compression
 * - nc_def_var_bzip2()       Enable BZIP2 compression
 * - nc_put_var_float()       Write entire variable in one call
 * - nc_get_var_float()       Read entire variable in one call
 *
 * @note The program is intended for local performance profiling.
 *       Build with ENABLE_BENCHMARKS=ON (CMake) or --enable-benchmarks
 *       (Autotools); it is excluded from regular CI.
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
 * High-Performance Programs for Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <netcdf_filter.h>
#include "nep.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

/** Time dimension: 500 time steps. */
#define NZ 500
/** Latitude dimension: 180 degrees. */
#define NY 180
/** Longitude dimension: 360 degrees. */
#define NX 360

/** Temporary NetCDF file created and removed for each measurement. */
#define TMP_FILE "quantize_tmp.nc"

/** Chunk shape: matches all v1.10.0 performance examples for consistency. */
#define CHUNK_Z 10
#define CHUNK_Y 45
#define CHUNK_X 90

/** Total number of float values in the dataset. */
#define NVALS ((size_t)NZ * NY * NX)

/** Uncompressed size in bytes. */
#define UNCOMPRESSED_BYTES ((double)NVALS * sizeof(float))

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/** NSD range for BitGroom and GranularBitRound (decimal digits).
 *  NC_FLOAT has ~6.92 significant decimal digits (23-bit significand).
 *  NC_QUANTIZE_MAX_FLOAT_NSD is 7, but NSD=7 triggers a boundary condition
 *  in BitGroom where bits_to_zero=0 and the alternating mask produces NaN
 *  on odd indices. Cap at 6 to stay within meaningful float precision. */
#define NSD_MIN 1
#define NSD_MAX 6

/** NSB values for BitRound, equivalent in decimal precision to NSD 1–6.
 *  NSB=23 (NSD=7 equivalent) is excluded for NC_FLOAT: it retains all 23
 *  significand bits and is indistinguishable from no quantization. */
static const int nsb_values[] = {3, 6, 9, 13, 16, 19};
#define N_NSB 6

/** Filter configuration structure (same as lossless.c). */
typedef struct {
    const char *name;        /** Filter name for CSV output. */
    const char *filter_type; /** Internal filter type identifier. */
    int level_or_pixels;     /** Compression level or pixels_per_block. */
} filter_config_t;

/** Optimal lossless filter configurations (from sprint 10). */
static const filter_config_t filters[] = {
    {"deflate",   "DEFLATE",   1},
    {"zstandard", "ZSTANDARD", 1},
    {"szip",      "SZIP",      2},
    {"lz4",       "LZ4",       1},
    {"bzip2",     "BZIP2",     1},
};

#define NUM_FILTERS (sizeof(filters) / sizeof(filters[0]))

/**
 * @brief Return current wall-clock time in seconds.
 * @return Seconds since the epoch as a double.
 */
static double
get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

/**
 * @brief Apply the specified lossless filter to the variable.
 *
 * For DEFLATE, shuffle is embedded in nc_def_var_deflate(). For all other
 * filters, the caller has already applied the shuffle-only call before
 * invoking this function.
 *
 * @param ncid    NetCDF file ID.
 * @param varid   Variable ID.
 * @param filter  Lossless filter configuration.
 * @return 0 on success, 1 on error.
 */
static int
apply_filter(int ncid, int varid, const filter_config_t *filter)
{
    int ret;

    if (strcmp(filter->filter_type, "DEFLATE") == 0)
    {
        if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "ZSTANDARD") == 0)
    {
        if ((ret = nc_def_var_zstandard(ncid, varid, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "SZIP") == 0)
    {
        if ((ret = nc_def_var_szip(ncid, varid, NC_SZIP_NN, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "LZ4") == 0)
    {
        if ((ret = nc_def_var_lz4(ncid, varid, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "BZIP2") == 0)
    {
        if ((ret = nc_def_var_bzip2(ncid, varid, filter->level_or_pixels))) ERR(ret);
    }
    else
    {
        fprintf(stderr, "Unknown filter type: %s\n", filter->filter_type);
        return 1;
    }

    return 0;
}

/**
 * @brief Run one quantize × lossless filter combination and print a CSV row.
 *
 * Creates @c quantize_tmp.nc with shuffle + lossless filter + (optionally)
 * quantization, writes a 500×180×360 NC_FLOAT temperature variable, stats
 * the file to obtain the compressed size, reads the variable back to compute
 * max absolute error, then removes the file.
 *
 * When @p quantize_mode is -1 (baseline), no quantization is applied and
 * @c max_abs_err is reported as 0.000000.
 *
 * @param orig          Pre-allocated buffer of NVALS floats (original data).
 * @param read_buf      Pre-allocated buffer of NVALS floats (read-back buffer).
 * @param q_alg         Quantize algorithm name for CSV ("none", "bitgroom",
 *                      "granularbr", or "bitround").
 * @param quantize_mode NC_QUANTIZE_BITGROOM, NC_QUANTIZE_GRANULARBR,
 *                      NC_QUANTIZE_BITROUND, or -1 for no quantization.
 * @param nsd_or_nsb    NSD (BitGroom/GranularBitRound) or NSB (BitRound)
 *                      value; 0 when quantize_mode is -1.
 * @param filter        Lossless filter configuration.
 * @return 0 on success, 1 on any error.
 */
static int
run_one(float *orig, float *read_buf,
        const char *q_alg, int quantize_mode, int nsd_or_nsb,
        const filter_config_t *filter)
{
    int ncid, varid, dimids[3], ret;
    size_t chunksizes[3] = {CHUNK_Z, CHUNK_Y, CHUNK_X};
    double t_write_start, write_s, t_read_start, read_s;
    struct stat st;
    long long compressed_bytes;
    double ratio, max_abs_err;
    size_t i;

    /* --- Write pass ---------------------------------------------------- */
    t_write_start = get_time();

    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat",  NY, &dimids[1]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon",  NX, &dimids[2]))) ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid))) ERR(ret);
    if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);

    /* Enable shuffle for non-DEFLATE filters (DEFLATE embeds shuffle itself). */
    if (strcmp(filter->filter_type, "DEFLATE") != 0)
    {
        if ((ret = nc_def_var_deflate(ncid, varid, 1, 0, 0))) ERR(ret);
    }

    /* Apply lossless filter. */
    if (apply_filter(ncid, varid, filter)) return 1;

    /* Apply quantization after lossless filter, before enddef. */
    if (quantize_mode >= 0)
    {
        if ((ret = nc_def_var_quantize(ncid, varid, quantize_mode, nsd_or_nsb))) ERR(ret);
    }

    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify quantize settings took effect. */
    if (quantize_mode >= 0)
    {
        int mode_out, nsd_out;
        if ((ret = nc_inq_var_quantize(ncid, varid, &mode_out, &nsd_out))) ERR(ret);
        if (mode_out != quantize_mode || nsd_out != nsd_or_nsb)
        {
            fprintf(stderr, "quantize settings mismatch: expected mode=%d nsd=%d, "
                    "got mode=%d nsd=%d\n", quantize_mode, nsd_or_nsb, mode_out, nsd_out);
            nc_close(ncid);
            return 1;
        }
    }

    if ((ret = nc_put_var_float(ncid, varid, orig))) ERR(ret);
    if ((ret = nc_close(ncid))) ERR(ret);

    write_s = get_time() - t_write_start;

    /* --- Get compressed size ------------------------------------------- */
    if (stat(TMP_FILE, &st) != 0)
    {
        perror("stat");
        return 1;
    }
    compressed_bytes = (long long)st.st_size;
    ratio = UNCOMPRESSED_BYTES / (double)compressed_bytes;

    /* --- Read pass ----------------------------------------------------- */
    t_read_start = get_time();

    if ((ret = nc_open(TMP_FILE, NC_NOWRITE, &ncid))) ERR(ret);
    if ((ret = nc_inq_varid(ncid, "temperature", &varid))) ERR(ret);
    if ((ret = nc_get_var_float(ncid, varid, read_buf))) ERR(ret);
    if ((ret = nc_close(ncid))) ERR(ret);

    read_s = get_time() - t_read_start;

    /* --- Compute max absolute error ------------------------------------ */
    max_abs_err = 0.0;
    if (quantize_mode >= 0)
    {
        for (i = 0; i < NVALS; i++)
        {
            double err = fabs((double)read_buf[i] - (double)orig[i]);
            if (err > max_abs_err)
                max_abs_err = err;
        }
    }

    /* --- Cleanup ------------------------------------------------------- */
    remove(TMP_FILE);

    /* --- Output -------------------------------------------------------- */
    printf("%s,%d,%s,%lld,%.3f,%.3f,%.3f,%f\n",
           q_alg, nsd_or_nsb, filter->name,
           compressed_bytes, ratio, write_s, read_s, max_abs_err);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Allocates two 500×180×360 NC_FLOAT buffers (original data and read-back
 * buffer), then iterates:
 *
 * 1. Baseline rows: no quantization, each lossless filter at optimal setting
 *    (5 rows).
 * 2. BitGroom NSD 1–7 × 5 filters (35 rows).
 * 3. GranularBitRound NSD 1–7 × 5 filters (35 rows).
 * 4. BitRound NSB {3,6,9,13,16,19,23} × 5 filters (35 rows).
 *
 * Total: 110 CSV data rows + 1 header row.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    float *orig, *read_buf;
    size_t i, f;
    int nsd, nsb_idx;

    /* Allocate and fill synthetic temperature data. */
    orig = (float *)malloc(NVALS * sizeof(float));
    if (!orig)
    {
        fprintf(stderr, "Memory allocation failed (%zu bytes)\n",
                NVALS * sizeof(float));
        return 1;
    }
    read_buf = (float *)malloc(NVALS * sizeof(float));
    if (!read_buf)
    {
        fprintf(stderr, "Memory allocation failed (%zu bytes)\n",
                NVALS * sizeof(float));
        free(orig);
        return 1;
    }

    for (i = 0; i < NVALS; i++)
    {
        size_t t = i / ((size_t)NY * NX);
        size_t y = (i / NX) % NY;
        size_t x = i % NX;
        orig[i] = 280.0f + (float)t * 0.1f + (float)y * 0.01f + (float)x * 0.001f;
    }

    printf("quantize_alg,nsd_or_nsb,filter,compressed_bytes,ratio,write_s,read_s,max_abs_err\n");

    /* --- Baseline: no quantization, each lossless filter (5 rows) ------ */
    for (f = 0; f < NUM_FILTERS; f++)
    {
        if (run_one(orig, read_buf, "none", -1, 0, &filters[f]))
        {
            free(orig); free(read_buf); return 1;
        }
    }

    /* --- BitGroom NSD 1–7 × 5 filters (35 rows) ------------------------ */
    for (nsd = NSD_MIN; nsd <= NSD_MAX; nsd++)
    {
        for (f = 0; f < NUM_FILTERS; f++)
        {
            if (run_one(orig, read_buf, "bitgroom",
                        NC_QUANTIZE_BITGROOM, nsd, &filters[f]))
            {
                free(orig); free(read_buf); return 1;
            }
        }
    }

    /* --- GranularBitRound NSD 1–7 × 5 filters (35 rows) ---------------- */
    for (nsd = NSD_MIN; nsd <= NSD_MAX; nsd++)
    {
        for (f = 0; f < NUM_FILTERS; f++)
        {
            if (run_one(orig, read_buf, "granularbr",
                        NC_QUANTIZE_GRANULARBR, nsd, &filters[f]))
            {
                free(orig); free(read_buf); return 1;
            }
        }
    }

    /* --- BitRound NSB {3,6,9,13,16,19,23} × 5 filters (35 rows) -------- */
    for (nsb_idx = 0; nsb_idx < N_NSB; nsb_idx++)
    {
        for (f = 0; f < NUM_FILTERS; f++)
        {
            if (run_one(orig, read_buf, "bitround",
                        NC_QUANTIZE_BITROUND, nsb_values[nsb_idx], &filters[f]))
            {
                free(orig); free(read_buf); return 1;
            }
        }
    }

    free(orig);
    free(read_buf);
    return 0;
}

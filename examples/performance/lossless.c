/**
 * @file lossless.c
 * @brief Compare best-performing settings of all lossless compression filters.
 *
 * This program evaluates the optimal settings for each available lossless
 * compression filter (DEFLATE, Zstandard, SZIP, LZ4, BZIP2) with the shuffle
 * filter enabled, providing a unified comparison for scientific floating-point
 * data. Results help users choose the right compression based on their
 * priorities: maximum compression ratio or fastest I/O throughput.
 *
 * Filters tested (all with shuffle enabled):
 * - DEFLATE (gzip): level 1 (good ratio, fast compression)
 * - Zstandard: level 1 (optimal ratio vs speed tradeoff)
 * - SZIP: NC_SZIP_NN with pixels_per_block=2 (best for NC_FLOAT)
 * - LZ4: level 1 (best ratio with shuffle)
 * - BZIP2: level 1 (optimal for this data type)
 *
 * The shuffle filter is enabled for all tests via
 * nc_def_var_deflate(ncid, varid, 1, 0, 0) before applying the primary
 * compression filter. This has been empirically determined to significantly
 * improve compression ratios for IEEE 754 floating-point data.
 *
 * This program iterates over the 5 filters (1 row per filter) over a
 * 500×180×360 NC_FLOAT temperature dataset (~129 MB uncompressed) and
 * reports:
 *
 *   - Filter name and optimal setting (level or pixels_per_block)
 *   - Compressed file size on disk (bytes via stat())
 *   - Compression ratio (uncompressed / compressed)
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   filter,level_or_pixels,compressed_bytes,ratio,write_s,read_s
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./lossless > lossless_results.csv
 *   python3 plot_lossless.py   # produces lossless_performance.jpg
 * @endcode
 *
 * **Learning Objectives:**
 * - Compare all available lossless compression filters at their optimal settings
 * - Understand the performance trade-offs: ratio vs write speed vs read speed
 * - Learn which filter is best for different priorities (archival vs real-time)
 * - See the consistent benefit of the shuffle filter across all lossless compressors
 * - Make informed filter selection decisions based on empirical measurements
 *
 * **Key Concepts:**
 * - **Filter Comparison**: Each lossless filter has different strengths:
 *   - DEFLATE: universal compatibility, moderate speed and ratio
 *   - Zstandard: best speed/ratio tradeoff for most workloads
 *   - SZIP: designed for scientific data, good for correlated integers
 *   - LZ4: fastest decompression, lowest ratio
 *   - BZIP2: highest ratio, slowest speed
 * - **Optimal Settings**: Each filter tested at its empirically best single setting
 *   (determined from individual filter benchmarks), not all possible levels
 * - **Shuffle Baseline**: All filters tested with shuffle enabled, since shuffle
 *   universally improves ratio for IEEE 754 float data at minimal cost
 * - **Unified Methodology**: Same dataset, chunk shape, and measurement approach
 *   across all filters for fair comparison
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking.c - Understanding chunked storage
 * - deflate.c - Understanding individual filter benchmarks
 *
 * **Related Examples:**
 * - deflate.c - Detailed deflate level sweep
 * - zstandard.c - Detailed Zstandard level sweep
 * - bzip2.c - Detailed BZIP2 level sweep
 * - lz4.c - Detailed LZ4 level sweep
 * - szip.c - Detailed SZIP pixels_per_block sweep
 * - quantize.c - Lossy quantization combined with lossless filters
 *
 * **Key API functions:**
 * - nc_def_var_deflate()      Enable shuffle and/or DEFLATE compression
 * - nc_def_var_zstandard()    Enable Zstandard compression
 * - nc_def_var_szip()         Enable SZIP compression
 * - nc_def_var_lz4()          Enable LZ4 compression
 * - nc_def_var_bzip2()        Enable BZIP2 compression
 * - nc_inq_var_*()            Query filter settings for verification
 * - nc_put_var_float()        Write entire variable in one call
 * - nc_get_var_float()        Read entire variable in one call
 *
 * @note The program is intended for local performance profiling.
 *       Build with ENABLE_BENCHMARKS=ON (CMake) or --enable-benchmarks
 *       (Autotools); it is excluded from regular CI.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <netcdf_filter.h>
#include "nep.h"
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
#define TMP_FILE "lossless_tmp.nc"

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

/** Filter configuration structure. */
typedef struct {
    const char *name;        /** Filter name for CSV output. */
    const char *filter_type; /** Internal filter type identifier. */
    int level_or_pixels;     /** Compression level or pixels_per_block. */
} filter_config_t;

/** Optimal filter configurations (determined empirically in prior sprints). */
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
 * @brief Apply the specified filter to the variable.
 *
 * @param ncid       NetCDF file ID.
 * @param varid      Variable ID.
 * @param filter     Filter configuration.
 * @return 0 on success, non-zero on error.
 */
static int
apply_filter(int ncid, int varid, const filter_config_t *filter)
{
    int ret;

    if (strcmp(filter->filter_type, "DEFLATE") == 0)
    {
        /* Enable deflate with shuffle (shuffle=1, deflate=1). */
        if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "ZSTANDARD") == 0)
    {
        /* Enable zstandard compression. */
        if ((ret = nc_def_var_zstandard(ncid, varid, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "SZIP") == 0)
    {
        /* Enable szip with NN coding method. */
        if ((ret = nc_def_var_szip(ncid, varid, NC_SZIP_NN, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "LZ4") == 0)
    {
        /* Enable lz4 compression using NEP function. */
        if ((ret = nc_def_var_lz4(ncid, varid, filter->level_or_pixels))) ERR(ret);
    }
    else if (strcmp(filter->filter_type, "BZIP2") == 0)
    {
        /* Enable bzip2 compression. */
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
 * @brief Verify that the filter was applied correctly.
 *
 * @param ncid       NetCDF file ID.
 * @param varid      Variable ID.
 * @param filter     Filter configuration.
 * @return 0 on success, 1 on verification failure.
 */
static int
verify_filter(int ncid, int varid, const filter_config_t *filter)
{
    int ret;

    if (strcmp(filter->filter_type, "DEFLATE") == 0)
    {
        int shuffle_out, deflate_out, level_out;
        if ((ret = nc_inq_var_deflate(ncid, varid, &shuffle_out, &deflate_out, &level_out))) ERR(ret);
        if (shuffle_out != 1 || deflate_out != 1 || level_out != filter->level_or_pixels)
        {
            fprintf(stderr, "DEFLATE settings mismatch for %s\n", filter->name);
            return 1;
        }
    }
    else if (strcmp(filter->filter_type, "ZSTANDARD") == 0)
    {
        int zstandard_out, level_out;
        if ((ret = nc_inq_var_zstandard(ncid, varid, &zstandard_out, &level_out))) ERR(ret);
        if (zstandard_out != 1 || level_out != filter->level_or_pixels)
        {
            fprintf(stderr, "ZSTANDARD settings mismatch for %s\n", filter->name);
            return 1;
        }
    }
    else if (strcmp(filter->filter_type, "SZIP") == 0)
    {
        int options_mask_out, pixels_per_block_out;
        if ((ret = nc_inq_var_szip(ncid, varid, &options_mask_out, &pixels_per_block_out))) ERR(ret);
        if (options_mask_out != NC_SZIP_NN || pixels_per_block_out != filter->level_or_pixels)
        {
            fprintf(stderr, "SZIP settings mismatch for %s\n", filter->name);
            return 1;
        }
    }
    else if (strcmp(filter->filter_type, "LZ4") == 0)
    {
        /* Verify LZ4 filter using generic filter inquiry (handles multiple filters). */
        size_t nfilters;
        unsigned int params[1];
        size_t nparams = 1;
        int found_lz4 = 0;

        if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, NULL))) ERR(ret);
        for (size_t f = 0; f < nfilters; f++)
        {
            unsigned int fid;
            if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, &fid))) ERR(ret);
            if ((ret = nc_inq_var_filter_info(ncid, varid, fid, &nparams, params))) ERR(ret);
            if (fid == 32004) /* H5Z_FILTER_LZ4 */
            {
                found_lz4 = 1;
                if ((int)params[0] != filter->level_or_pixels)
                {
                    fprintf(stderr, "LZ4 level mismatch for %s\n", filter->name);
                    return 1;
                }
                break;
            }
        }
        if (!found_lz4)
        {
            fprintf(stderr, "LZ4 filter not found for %s\n", filter->name);
            return 1;
        }
    }
    else if (strcmp(filter->filter_type, "BZIP2") == 0)
    {
        int bzip2_out, level_out;
        if ((ret = nc_inq_var_bzip2(ncid, varid, &bzip2_out, &level_out))) ERR(ret);
        if (bzip2_out != 1 || level_out != filter->level_or_pixels)
        {
            fprintf(stderr, "BZIP2 settings mismatch for %s\n", filter->name);
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Run one filter test and print a CSV row.
 *
 * Creates @c lossless_tmp.nc with the given filter at its optimal setting
 * with shuffle enabled, writes a 500×180×360 NC_FLOAT temperature variable,
 * stats the file to obtain the compressed size, reads the variable back, then
 * removes the file.
 *
 * @param data       Pre-allocated buffer of NVALS floats (synthetic data).
 * @param filter     Filter configuration to test.
 * @return 0 on success, 1 on any error.
 */
static int
run_one_filter(float *data, const filter_config_t *filter)
{
    int ncid, varid, dimids[3], ret;
    size_t chunksizes[3] = {CHUNK_Z, CHUNK_Y, CHUNK_X};
    double t_write_start, write_s, t_read_start, read_s;
    struct stat st;
    long long compressed_bytes;
    double ratio;

    /* --- Write pass ---------------------------------------------------- */
    t_write_start = get_time();

    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat",  NY, &dimids[1]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon",  NX, &dimids[2]))) ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid))) ERR(ret);
    if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);

    /* Enable shuffle for all filters (except DEFLATE which handles it internally). */
    if (strcmp(filter->filter_type, "DEFLATE") != 0)
    {
        if ((ret = nc_def_var_deflate(ncid, varid, 1, 0, 0))) ERR(ret);
    }

    /* Apply the primary compression filter. */
    if (apply_filter(ncid, varid, filter)) return 1;

    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify filter settings took effect before writing. */
    if (verify_filter(ncid, varid, filter)) return 1;

    if ((ret = nc_put_var_float(ncid, varid, data))) ERR(ret);
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
    if ((ret = nc_get_var_float(ncid, varid, data))) ERR(ret);
    if ((ret = nc_close(ncid))) ERR(ret);

    read_s = get_time() - t_read_start;

    /* --- Cleanup ------------------------------------------------------- */
    remove(TMP_FILE);

    /* --- Output -------------------------------------------------------- */
    printf("%s,%d,%lld,%.3f,%.3f,%.3f\n",
           filter->name, filter->level_or_pixels, compressed_bytes, ratio, write_s, read_s);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Allocates a 500×180×360 NC_FLOAT buffer with synthetic temperature data,
 * then iterates over the 5 filters at their optimal settings (5 rows total),
 * printing one CSV row per filter.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    float *data;
    size_t i;
    size_t f;

    /* Allocate and fill synthetic temperature data. */
    data = (float *)malloc(NVALS * sizeof(float));
    if (!data)
    {
        fprintf(stderr, "Memory allocation failed (%zu bytes)\n",
                NVALS * sizeof(float));
        return 1;
    }

    for (i = 0; i < NVALS; i++)
    {
        size_t t = i / ((size_t)NY * NX);
        size_t y = (i / NX) % NY;
        size_t x = i % NX;
        data[i] = 280.0f + (float)t * 0.1f + (float)y * 0.01f + (float)x * 0.001f;
    }

    printf("filter,level_or_pixels,compressed_bytes,ratio,write_s,read_s\n");

    for (f = 0; f < NUM_FILTERS; f++)
    {
        if (run_one_filter(data, &filters[f]))
        {
            free(data);
            return 1;
        }
    }

    free(data);
    return 0;
}

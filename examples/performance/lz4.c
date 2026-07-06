/**
 * @file lz4.c
 * @brief Demonstrates how LZ4 compression levels and the shuffle filter
 *        affect NetCDF-4 compression ratio and I/O throughput.
 *
 * NetCDF-4/HDF5 supports LZ4 compression via the HDF5 LZ4 filter (ID 32004)
 * using the generic nc_def_var_filter() API. LZ4 is a fast lossless
 * compression algorithm focused on compression and decompression speed.
 * LZ4 offers a level range (1–9) where higher levels achieve better
 * compression ratios at the cost of compression speed. Decompression speed
 * remains consistently fast across all levels.
 *
 * The shuffle filter (@p shuffle = 1) reorders bytes before compression:
 * for IEEE 754 floating-point data the most-significant bytes of adjacent
 * values are grouped together, which typically increases the compression ratio
 * by 3–5× at little additional cost. The shuffle filter is enabled by calling
 * nc_def_var_deflate(ncid, varid, 1, 0, 0) (shuffle only, no deflate) before
 * applying the LZ4 filter via nc_def_var_filter().
 *
 * This program iterates LZ4 levels 1–9 × shuffle {off, on}
 * (18 combinations total) over a 500×180×360 NC_FLOAT temperature dataset
 * (~129 MB uncompressed) and reports:
 *
 *   - Compressed file size on disk (bytes via stat())
 *   - Compression ratio (uncompressed / compressed)
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   lz4_level,shuffle,compressed_bytes,ratio,write_s,read_s
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./lz4 > lz4_results.csv
 *   python3 plot_lz4.py   # produces lz4_performance.jpg
 * @endcode
 *
 * **Learning Objectives:**
 * - Understand LZ4 compression characteristics: fastest decompression of all filters
 * - Learn how to use the generic nc_def_var_filter() API with HDF5 filter IDs
 * - See how LZ4 levels (1–9) trade compression speed for ratio
 * - Recognize LZ4's strength: consistently fast decompression regardless of level
 * - Determine when LZ4 is preferred (read-heavy workloads, real-time access)
 *
 * **Key Concepts:**
 * - **LZ4**: A fast lossless compression algorithm optimized for decompression
 *   speed; achieves lower ratios than deflate or zstd but decompresses 2–4× faster
 * - **Generic Filter API**: nc_def_var_filter(ncid, varid, id, nparams, params)
 *   enables any registered HDF5 filter; LZ4 uses filter ID 32004
 * - **Decompression Speed**: LZ4's main advantage — constant high-speed decode
 *   regardless of compression level; ideal for read-intensive applications
 * - **Level Range (1–9)**: Higher levels spend more CPU on compression for
 *   marginally better ratios; decompression speed is unaffected
 * - **Shuffle + LZ4**: Shuffle pre-groups IEEE 754 bytes, improving LZ4's
 *   ratio by 3–5× at minimal cost to decompression speed
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking.c - Understanding chunked storage (required for compression)
 * - deflate.c - Comparison baseline (deflate is the traditional default)
 *
 * **Related Examples:**
 * - deflate.c - Traditional deflate/zlib compression (better ratio, slower decompress)
 * - zstandard.c - Zstandard compression (better ratio, slightly slower decompress)
 * - bzip2.c - BZIP2 compression (best ratio, slowest)
 * - lossless.c - Unified comparison of all lossless filters
 *
 * **Key API functions:**
 * - nc_def_var_filter()     Enable LZ4 compression using HDF5 filter ID 32004
 * - nc_inq_var_filter()     Query filter settings on an open variable
 * - nc_def_var_deflate()    Enable shuffle filter without deflate (deflate=0)
 * - nc_def_var_chunking()   Required before any compression filter
 * - nc_put_var_float()      Write entire variable in one call
 * - nc_get_var_float()      Read entire variable in one call
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

/** HDF5 LZ4 filter ID. */
#define H5Z_FILTER_LZ4 32004

/** Time dimension: 500 time steps. */
#define NZ 500
/** Latitude dimension: 180 degrees. */
#define NY 180
/** Longitude dimension: 360 degrees. */
#define NX 360

/** Temporary NetCDF file created and removed for each measurement. */
#define TMP_FILE "lz4_tmp.nc"

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

/** Minimum and maximum LZ4 levels. */
#define MIN_LEVEL 1
#define MAX_LEVEL 9

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
 * @brief Run one lz4/shuffle combination and print a CSV row.
 *
 * Creates @c lz4_tmp.nc with the given lz4 level and shuffle
 * setting, writes a 500×180×360 NC_FLOAT temperature variable, stats
 * the file to obtain the compressed size, reads the variable back, then
 * removes the file.
 *
 * When @p shuffle is 1, nc_def_var_deflate() is called with shuffle=1 and
 * deflate=0 to activate the byte-shuffle filter without zlib compression;
 * nc_def_var_lz4() is then called to add lz4 on top.
 *
 * @param data       Pre-allocated buffer of NVALS floats (synthetic data).
 * @param level      LZ4 compression level (1 to 9).
 * @param shuffle    1 to enable the shuffle filter, 0 to disable.
 * @return 0 on success, 1 on any error.
 */
static int
run_one(float *data, int level, int shuffle)
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
    if (shuffle)
    {
        /* Enable shuffle only (no deflate compression). */
        if ((ret = nc_def_var_deflate(ncid, varid, 1, 0, 0))) ERR(ret);
    }
    /* Apply LZ4 filter using the generic filter API. */
    {
        unsigned int params[1] = {(unsigned int)level};
        if ((ret = nc_def_var_filter(ncid, varid, H5Z_FILTER_LZ4, 1, params))) ERR(ret);
    }
    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify lz4 settings took effect before writing. */
    {
        size_t nparams = 1;
        unsigned int params[1];
        size_t nfilters;
        
        /* Get number of filters first. */
        if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, NULL))) ERR(ret);
        
        /* Query each filter to find LZ4. */
        int found_lz4 = 0;
        for (size_t f = 0; f < nfilters; f++)
        {
            unsigned int fid;
            if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, &fid))) ERR(ret);
            if ((ret = nc_inq_var_filter_info(ncid, varid, fid, &nparams, params))) ERR(ret);
            if (fid == H5Z_FILTER_LZ4)
            {
                found_lz4 = 1;
                if ((int)params[0] != level)
                {
                    fprintf(stderr, "lz4 level mismatch: expected %d, got %u\n",
                            level, params[0]);
                    nc_close(ncid);
                    return 1;
                }
                break;
            }
        }
        if (!found_lz4)
        {
            fprintf(stderr, "lz4 filter not found on variable\n");
            nc_close(ncid);
            return 1;
        }
    }

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
    printf("%d,%d,%lld,%.3f,%.3f,%.3f\n",
           level, shuffle, compressed_bytes, ratio, write_s, read_s);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Allocates a 500×180×360 NC_FLOAT buffer with synthetic temperature data,
 * then iterates LZ4 levels 1–9 × shuffle {0, 1} (18 rows
 * total), printing one CSV row per combination.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    float *data;
    size_t i;
    int level, shuffle;

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

    printf("lz4_level,shuffle,compressed_bytes,ratio,write_s,read_s\n");

    for (shuffle = 0; shuffle <= 1; shuffle++)
    {
        for (level = MIN_LEVEL; level <= MAX_LEVEL; level++)
        {
            if (run_one(data, level, shuffle))
            {
                free(data);
                return 1;
            }
        }
    }

    free(data);
    return 0;
}

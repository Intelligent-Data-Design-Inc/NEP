/**
 * @file deflate.c
 * @brief Demonstrates how zlib deflate levels and the shuffle filter affect
 *        NetCDF-4 compression ratio and I/O throughput.
 *
 * NetCDF-4/HDF5 supports per-variable deflate compression via
 * nc_def_var_deflate().  The @p level parameter (0 = no compression,
 * 9 = maximum compression effort) trades CPU time for smaller files.
 * The shuffle filter (@p shuffle = 1) reorders bytes before compression:
 * for IEEE 754 floating-point data the most-significant bytes of adjacent
 * values are grouped together, which typically increases the deflate ratio
 * by 2–5× at little additional cost.
 *
 * This program iterates all 20 combinations of deflate level (0–9) and
 * shuffle (off/on) over a 500×180×360 NC_FLOAT temperature dataset
 * (~129 MB uncompressed) and reports:
 *
 *   - Compressed file size on disk (bytes via stat())
 *   - Compression ratio (uncompressed / compressed)
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   deflate_level,shuffle,compressed_bytes,ratio,write_s,read_s
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./deflate > deflate_results.csv
 *   python3 plot_deflate.py   # produces deflate_performance.jpg
 * @endcode
 *
 * **Learning Objectives:**
 * - Understand the deflate (zlib/gzip) compression algorithm in NetCDF-4
 * - Learn how compression level (0–9) trades CPU time for file size reduction
 * - See how the shuffle filter improves compression for floating-point data
 * - Measure and compare write time, read time, and compression ratio
 * - Make data-driven decisions about compression settings for production use
 *
 * **Key Concepts:**
 * - **Deflate (zlib)**: The default lossless compression in HDF5/NetCDF-4;
 *   uses LZ77 + Huffman coding; level 0 = store only, level 9 = max effort
 * - **Shuffle Filter**: Byte transposition that groups corresponding bytes of
 *   adjacent values (e.g., all MSBs together), exploiting IEEE 754 structure
 * - **Level 1 Sweet Spot**: For scientific float data, level 1 typically achieves
 *   90–95% of level 9's compression ratio at 3–5× faster write speed
 * - **Chunked Storage**: Compression requires chunked layout; each chunk is
 *   compressed independently as a single block
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking.c - Understanding chunked storage (required for compression)
 *
 * **Related Examples:**
 * - zstandard.c - Zstandard compression (wider level range, often faster)
 * - bzip2.c - BZIP2 compression (higher ratio, slower)
 * - lz4.c - LZ4 compression (fastest decompression)
 * - lossless.c - Unified comparison of all lossless filters
 * - compression.c - NetCDF-4 compression tutorial (non-benchmark)
 *
 * **Key API functions:**
 * - nc_def_var_deflate()   Enable deflate and/or shuffle filter on a variable
 * - nc_inq_var_deflate()   Query deflate and shuffle settings
 * - nc_put_var_float()     Write entire variable in one call
 * - nc_get_var_float()     Read entire variable in one call
 *
 * @note The program is intended for local performance profiling.
 *       Build with ENABLE_BENCHMARKS=ON (CMake) or --enable-benchmarks
 *       (Autotools); it is excluded from regular CI.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
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
#define TMP_FILE "deflate_tmp.nc"

/** Chunk shape: one time slab per chunk (good general-purpose shape for deflate). */
#define CHUNK_Z 10
#define CHUNK_Y 45
#define CHUNK_X 90

/** Total number of float values in the dataset. */
#define NVALS ((size_t)NZ * NY * NX)

/** Uncompressed size in bytes. */
#define UNCOMPRESSED_BYTES ((double)NVALS * sizeof(float))

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

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
 * @brief Run one deflate/shuffle combination and print a CSV row.
 *
 * Creates @c deflate_tmp.nc with the given deflate level and shuffle
 * setting, writes a 500×180×360 NC_FLOAT temperature variable, stats
 * the file to obtain the compressed size, reads the variable back, then
 * removes the file.
 *
 * @param data       Pre-allocated buffer of NVALS floats (synthetic data).
 * @param level      Deflate level (0–9).
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
    int shuffle_out, deflate_out, level_out;

    /* --- Write pass ---------------------------------------------------- */
    t_write_start = get_time();

    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat",  NY, &dimids[1]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon",  NX, &dimids[2]))) ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid))) ERR(ret);
    if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
    if ((ret = nc_def_var_deflate(ncid, varid, shuffle, 1, level))) ERR(ret);
    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify settings took effect before writing.
     * Note: NetCDF-C reports deflate_out=0 when level=0 (no-op compression);
     * only check level and shuffle when deflate is actually active. */
    if ((ret = nc_inq_var_deflate(ncid, varid, &shuffle_out, &deflate_out, &level_out))) ERR(ret);
    if (level > 0 && (deflate_out != 1 || level_out != level || shuffle_out != shuffle))
    {
        fprintf(stderr, "deflate settings mismatch: expected level=%d shuffle=%d, "
                "got deflate=%d level=%d shuffle=%d\n",
                level, shuffle, deflate_out, level_out, shuffle_out);
        nc_close(ncid);
        return 1;
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
 * then iterates all 20 combinations of shuffle (0 and 1) and deflate level
 * (0–9), printing one CSV row per combination.
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

    printf("deflate_level,shuffle,compressed_bytes,ratio,write_s,read_s\n");

    for (shuffle = 0; shuffle <= 1; shuffle++)
    {
        for (level = 0; level <= 9; level++)
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

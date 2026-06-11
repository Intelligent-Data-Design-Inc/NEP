/**
 * @file zstandard.c
 * @brief Demonstrates how Zstandard compression levels and the shuffle filter
 *        affect NetCDF-4 compression ratio and I/O throughput.
 *
 * NetCDF-4/HDF5 supports Zstandard (zstd) compression via
 * nc_def_var_zstandard().  Unlike zlib deflate (levels 0–9), zstd supports a
 * much wider range: negative levels (-7 to -1) trade compression ratio for
 * extreme write speed, level 1 is the fast default, and levels up to 22 push
 * toward maximum compression at increasing CPU cost.
 *
 * The shuffle filter (@p shuffle = 1) reorders bytes before compression:
 * for IEEE 754 floating-point data the most-significant bytes of adjacent
 * values are grouped together, which typically increases the compression ratio
 * by 3–5× at little additional cost.  Because nc_def_var_zstandard() does not
 * accept a shuffle parameter directly, shuffle is enabled by calling
 * nc_def_var_deflate(ncid, varid, 1, 0, 0) (shuffle only, no deflate) before
 * nc_def_var_zstandard().
 *
 * This program iterates a representative subset of 12 levels
 * {-7, -3, -1, 0, 1, 3, 6, 9, 12, 15, 19, 22} × shuffle {off, on}
 * (44 combinations total) over a 500×180×360 NC_FLOAT temperature dataset
 * (~129 MB uncompressed) and reports:
 *
 *   - Compressed file size on disk (bytes via stat())
 *   - Compression ratio (uncompressed / compressed)
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   zstd_level,shuffle,compressed_bytes,ratio,write_s,read_s
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./zstandard > zstandard_results.csv
 *   python3 plot_zstandard.py   # produces zstandard_performance.jpg
 * @endcode
 *
 * **Key API functions:**
 * - nc_def_var_zstandard()  Enable Zstandard compression at a given level
 * - nc_inq_var_zstandard()  Query Zstandard settings on an open variable
 * - nc_def_var_deflate()    Enable shuffle filter without deflate (deflate=0)
 * - nc_def_var_chunking()   Required before any compression filter
 * - nc_put_var_float()      Write entire variable in one call
 * - nc_get_var_float()      Read entire variable in one call
 *
 * @note The program is intended for local performance profiling.
 *       Build with ENABLE_BENCHMARKS=ON (CMake) or --enable-benchmarks
 *       (Autotools); it is excluded from regular CI.
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

/** Time dimension: 500 time steps. */
#define NZ 500
/** Latitude dimension: 180 degrees. */
#define NY 180
/** Longitude dimension: 360 degrees. */
#define NX 360

/** Temporary NetCDF file created and removed for each measurement. */
#define TMP_FILE "zstandard_tmp.nc"

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

/** Representative zstd levels covering the full range -7 to 22. */
static const int LEVELS[] = {-7, -3, -1, 0, 1, 3, 6, 9, 12, 15, 19, 22};
#define NLEVELS (int)(sizeof(LEVELS) / sizeof(LEVELS[0]))

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
 * @brief Run one zstd/shuffle combination and print a CSV row.
 *
 * Creates @c zstandard_tmp.nc with the given zstd level and shuffle
 * setting, writes a 500×180×360 NC_FLOAT temperature variable, stats
 * the file to obtain the compressed size, reads the variable back, then
 * removes the file.
 *
 * When @p shuffle is 1, nc_def_var_deflate() is called with shuffle=1 and
 * deflate=0 to activate the byte-shuffle filter without zlib compression;
 * nc_def_var_zstandard() is then called to add zstd on top.
 *
 * @param data       Pre-allocated buffer of NVALS floats (synthetic data).
 * @param level      Zstandard compression level (-7 to 22).
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
    int hasfilter_out, level_out;

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
    if ((ret = nc_def_var_zstandard(ncid, varid, level))) ERR(ret);
    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify zstd settings took effect before writing. */
    if ((ret = nc_inq_var_zstandard(ncid, varid, &hasfilter_out, &level_out))) ERR(ret);
    if (hasfilter_out != 1 || level_out != level)
    {
        fprintf(stderr, "zstandard settings mismatch: expected level=%d, "
                "got hasfilter=%d level=%d\n", level, hasfilter_out, level_out);
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
 * then iterates 12 representative zstd levels × shuffle {0, 1} (44 rows
 * total), printing one CSV row per combination.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    float *data;
    size_t i;
    int li, shuffle;

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

    printf("zstd_level,shuffle,compressed_bytes,ratio,write_s,read_s\n");

    for (shuffle = 0; shuffle <= 1; shuffle++)
    {
        for (li = 0; li < NLEVELS; li++)
        {
            if (run_one(data, LEVELS[li], shuffle))
            {
                free(data);
                return 1;
            }
        }
    }

    free(data);
    return 0;
}

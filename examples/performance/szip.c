/**
 * @file szip.c
 * @brief Demonstrates how SZIP options_mask and pixels_per_block affect
 *        NetCDF-4 compression ratio and I/O throughput.
 *
 * SZIP (Lossless Scientific Data Compression) is a lossless compression
 * algorithm developed at JPL for scientific floating-point data and integrated
 * into HDF5 as filter ID 4.  NetCDF-4 exposes it via nc_def_var_szip().
 *
 * SZIP supports two coding methods: nearest-neighbor (@c NC_SZIP_NN) and
 * entropy coding (@c NC_SZIP_EC).  However, @c NC_SZIP_EC requires integer
 * data with a defined bit depth and does not work with NC_FLOAT.  This
 * program therefore uses @c NC_SZIP_NN exclusively, which is the correct
 * and commonly used mode for floating-point scientific data.
 *
 * The @p pixels_per_block parameter (even values 2 to NC_MAX_PIXELS_PER_BLOCK
 * = 32) controls the block size used by the SZIP encoder.  Larger values give
 * the encoder more context and often improve ratio, at some increase in CPU
 * cost.  For chunked storage, @p pixels_per_block must divide the chunk's
 * fastest-varying dimension (CHUNK_X).  With CHUNK_X=90, valid values are
 * 2 (90/2=45) and any divisor of 90.
 *
 * This program iterates 5 pixels_per_block values over a 500x180x360 NC_FLOAT
 * temperature dataset (~129 MB uncompressed) and reports:
 *
 *   - Compressed file size on disk (bytes via stat())
 *   - Compression ratio (uncompressed / compressed)
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   pixels_per_block,compressed_bytes,ratio,write_s,read_s
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./szip > szip_results.csv
 *   python3 plot_szip.py   # produces szip_performance.jpg
 * @endcode
 *
 * **Key API functions:**
 * - nc_def_var_szip()     Enable SZIP compression (NC_SZIP_NN) with given block size
 * - nc_inq_var_szip()     Query SZIP settings on an open variable
 * - nc_def_var_chunking() Set chunked storage (required before any filter)
 * - nc_put_var_float()    Write entire variable in one call
 * - nc_get_var_float()    Read entire variable in one call
 *
 * @note SZIP write support requires a NetCDF-C/HDF5 build compiled with szlib.
 * @note NC_SZIP_EC is not used because it does not support NC_FLOAT data.
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
#define TMP_FILE "szip_tmp.nc"

/** Chunk shape: CHUNK_X=90 is divisible by all tested pixels_per_block values
 *  {2, 6, 10, 18, 30}: the even divisors of 90 up to NC_MAX_PIXELS_PER_BLOCK.
 *  pixels_per_block must be even and divide CHUNK_X for NC_SZIP_NN. */
#define CHUNK_Z 10
#define CHUNK_Y 45
#define CHUNK_X 90

/** Total number of float values in the dataset. */
#define NVALS ((size_t)NZ * NY * NX)

/** Uncompressed size in bytes. */
#define UNCOMPRESSED_BYTES ((double)NVALS * sizeof(float))

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/** pixels_per_block values to test: even divisors of CHUNK_X=90. */
static const int PPB_VALUES[] = {2, 6, 10, 18, 30};
#define N_PPB 5

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
 * @brief Run one SZIP combination and print a CSV row.
 *
 * Creates @c szip_tmp.nc with the given options_mask and pixels_per_block,
 * writes a 500x180x360 NC_FLOAT temperature variable, stats the file to
 * obtain the compressed size, reads the variable back, then removes the file.
 *
 * @param data            Pre-allocated buffer of NVALS floats (synthetic data).
 * @param pixels_per_block Value that divides CHUNK_X=90.
 * @return 0 on success, 1 on any error.
 */
static int
run_one(float *data, int pixels_per_block)
{
    int ncid, varid, dimids[3], ret;
    size_t chunksizes[3] = {CHUNK_Z, CHUNK_Y, CHUNK_X};
    double t_write_start, write_s, t_read_start, read_s;
    struct stat st;
    long long compressed_bytes;
    double ratio;
    int mask_out, ppb_out;

    /* --- Write pass ---------------------------------------------------- */
    t_write_start = get_time();

    if ((ret = nc_create(TMP_FILE, NC_NETCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat",  NY, &dimids[1]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon",  NX, &dimids[2]))) ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid))) ERR(ret);
    if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
    if ((ret = nc_def_var_szip(ncid, varid, NC_SZIP_NN, pixels_per_block))) ERR(ret);
    if ((ret = nc_enddef(ncid))) ERR(ret);

    /* Verify szip settings took effect before writing. */
    if ((ret = nc_inq_var_szip(ncid, varid, &mask_out, &ppb_out))) ERR(ret);
    if (ppb_out != pixels_per_block)
    {
        fprintf(stderr, "szip settings mismatch: expected ppb=%d, got ppb=%d\n",
                pixels_per_block, ppb_out);
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
    printf("%d,%lld,%.3f,%.3f,%.3f\n",
           pixels_per_block, compressed_bytes, ratio, write_s, read_s);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Allocates a 500x180x360 NC_FLOAT buffer with synthetic temperature data,
 * then iterates 5 pixels_per_block values (5 rows total), printing one CSV
 * row per value.  All rows use NC_SZIP_NN coding.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    float *data;
    size_t i;
    int pi;

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

    printf("pixels_per_block,compressed_bytes,ratio,write_s,read_s\n");

    for (pi = 0; pi < N_PPB; pi++)
    {
        if (run_one(data, PPB_VALUES[pi]))
        {
            free(data);
            return 1;
        }
    }

    free(data);
    return 0;
}

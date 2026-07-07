/**
 * @file endianness.c
 * @brief Demonstrates byte order (endianness) handling performance in NetCDF-4/HDF5.
 *
 * NetCDF-4 supports explicit byte order control via nc_def_var_endian().
 * By default, data is stored in the platform's native byte order
 * (NC_ENDIAN_NATIVE). However, users can specify NC_ENDIAN_LITTLE or
 * NC_ENDIAN_BIG to ensure cross-platform compatibility or match requirements
 * of downstream tools.
 *
 * This program creates a 500×180×360 NC_FLOAT temperature dataset
 * (~129 MB uncompressed) three times with different byte orders and reports:
 *
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *   - File size on disk (bytes via stat())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   endian_mode,write_s,read_s,file_bytes
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./endianness > endianness_results.csv
 *   python3 plot_endianness.py   # produces endianness_performance.jpg
 * @endcode
 *
 * **Learning Objectives:**
 * - Understand byte order (endianness) in scientific data storage
 * - Learn to control byte order with nc_def_var_endian()
 * - Measure the performance cost of byte-swapping on non-native endianness
 * - Recognize when explicit endianness control is beneficial for portability
 * - Make informed decisions about endianness for cross-platform data exchange
 *
 * **Key Concepts:**
 * - **Endianness**: Byte order of multi-byte values; little-endian (LE) stores
 *   least-significant byte first (x86/ARM), big-endian (BE) stores MSB first
 * - **NC_ENDIAN_NATIVE**: Store in the platform's native byte order (default);
 *   fastest I/O since no byte-swapping is needed on the writing platform
 * - **NC_ENDIAN_LITTLE**: Force little-endian storage regardless of platform;
 *   ensures portability and fast reads on x86/ARM systems
 * - **NC_ENDIAN_BIG**: Force big-endian storage; matches legacy formats and
 *   network byte order (useful for interoperability with older tools)
 * - **Byte-Swapping Cost**: When reading non-native endianness, HDF5 must swap
 *   bytes for every value — typically adds 5–15% overhead for large arrays
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking.c - Understanding chunked storage in NetCDF-4
 *
 * **Related Examples:**
 * - deflate.c - Compression performance (endianness affects compressibility slightly)
 * - fill_values.c - Another storage property that affects write performance
 * - chunking.c - Chunk shape performance (orthogonal to endianness)
 *
 * **Key API functions:**
 * - nc_def_var_endian()    Set byte order for a variable
 * - nc_inq_var_endian()    Query current byte order setting
 * - nc_put_var_float()     Write entire variable in one call
 * - nc_get_var_float()     Read entire variable in one call
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

/** Total number of float values in the dataset. */
#define NVALS ((size_t)NZ * NY * NX)

/** Uncompressed size in bytes. */
#define UNCOMPRESSED_BYTES ((double)NVALS * sizeof(float))

/** Error exit code. */
#define ERRCODE 2
/** Error handling macro. */
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
 * @brief Generate synthetic temperature data.
 *
 * Fills the buffer with synthetic data: 280.0 + t*0.1 + y*0.01 + x*0.001
 *
 * @param data Buffer of NVALS floats to fill.
 */
static void
generate_data(float *data)
{
    size_t t, y, x, idx = 0;
    for (t = 0; t < NZ; t++)
        for (y = 0; y < NY; y++)
            for (x = 0; x < NX; x++)
                data[idx++] = 280.0f + (float)t * 0.1f + (float)y * 0.01f + (float)x * 0.001f;
}

/**
 * @brief Run one endianness test and print a CSV row.
 *
 * Creates a file with the given byte order, writes a
 * 500×180×360 NC_FLOAT temperature variable, stats the file to
 * obtain the on-disk size, reads the variable back, then removes
 * the file.
 *
 * @param path       Path for temporary file (will be created and removed).
 * @param endian     Byte order: NC_ENDIAN_NATIVE, NC_ENDIAN_LITTLE, or NC_ENDIAN_BIG.
 * @param endian_name String for CSV output ("native", "little", or "big").
 * @return 0 on success, 1 on any error.
 */
static int
run_test(const char *path, int endian, const char *endian_name)
{
    int ncid, varid, dimids[3], ret;
    double t_write_start, write_s, t_read_start, read_s;
    struct stat st;
    long long file_bytes;
    float *data;

    /* Allocate data buffer */
    data = (float *)malloc(NVALS * sizeof(float));
    if (!data)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    generate_data(data);

    /* --- Write pass ---------------------------------------------------- */
    t_write_start = get_time();

    if ((ret = nc_create(path, NC_NETCDF4 | NC_CLOBBER, &ncid)))
        ERR(ret);

    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat", NY, &dimids[1])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon", NX, &dimids[2])))
        ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid)))
        ERR(ret);

    /* Set byte order for the variable */
    if ((ret = nc_def_var_endian(ncid, varid, endian)))
        ERR(ret);

    if ((ret = nc_enddef(ncid)))
        ERR(ret);
    if ((ret = nc_put_var_float(ncid, varid, data)))
        ERR(ret);
    if ((ret = nc_close(ncid)))
        ERR(ret);

    write_s = get_time() - t_write_start;

    /* --- Get file size ------------------------------------------------- */
    if (stat(path, &st) != 0)
    {
        perror("stat");
        free(data);
        return 1;
    }
    file_bytes = (long long)st.st_size;

    /* --- Read pass ----------------------------------------------------- */
    t_read_start = get_time();

    if ((ret = nc_open(path, NC_NOWRITE, &ncid)))
        ERR(ret);
    if ((ret = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(ret);
    if ((ret = nc_get_var_float(ncid, varid, data)))
        ERR(ret);
    if ((ret = nc_close(ncid)))
        ERR(ret);

    read_s = get_time() - t_read_start;

    /* --- Output -------------------------------------------------------- */
    printf("%s,%.3f,%.3f,%lld\n",
           endian_name, write_s, read_s, file_bytes);

    /* Cleanup */
    free(data);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Runs three endianness tests:
 *   1. NC_ENDIAN_NATIVE (platform native byte order)
 *   2. NC_ENDIAN_LITTLE (explicit little-endian)
 *   3. NC_ENDIAN_BIG (explicit big-endian)
 *
 * Prints CSV header followed by one data row per test.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    int ret;

    printf("endian_mode,write_s,read_s,file_bytes\n");

    /* Native byte order test */
    ret = run_test("endian_native.nc", NC_ENDIAN_NATIVE, "native");
    if (ret) return 1;
    remove("endian_native.nc");

    /* Little-endian test */
    ret = run_test("endian_little.nc", NC_ENDIAN_LITTLE, "little");
    if (ret) return 1;
    remove("endian_little.nc");

    /* Big-endian test */
    ret = run_test("endian_big.nc", NC_ENDIAN_BIG, "big");
    if (ret) return 1;
    remove("endian_big.nc");

    return 0;
}

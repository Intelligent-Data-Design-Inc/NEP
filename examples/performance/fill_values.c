/**
 * @file fill_values.c
 * @brief Demonstrates fill value handling performance across classic and NetCDF-4 formats.
 *
 * NetCDF supports fill values — a special value indicating missing or unwritten
 * data. When fill mode is enabled (NC_FILL, the default), NetCDF automatically
 * writes fill values to any data not explicitly written. This benchmark measures
 * the performance impact of fill mode (ON vs OFF) across both classic (netCDF-3)
 * and NetCDF-4/HDF5 formats.
 *
 * This program creates a 500×180×360 NC_FLOAT temperature dataset (~129 MB)
 * four times with different configurations and reports:
 *
 *   - Write time (seconds)
 *   - Read time (seconds, full variable via nc_get_var_float())
 *   - File size on disk (bytes via stat())
 *
 * **Output:** CSV printed to stdout with columns:
 *   @code
 *   format,fill_mode,write_s,read_s,file_bytes
 *   @endcode
 *
 * **Typical workflow:**
 * @code
 *   ./fill_values > fill_values_results.csv
 *   python3 plot_fill_values.py   # produces fill_values_performance.jpg
 * @endcode
 *
 * **Key API functions:**
 * - nc_set_fill()           Enable/disable fill value mode
 * - nc_inq_fill()           Query current fill mode setting
 * - nc_put_att_float()      Set _FillValue attribute for classic files
 * - nc_def_var_fill()       Define fill value for netCDF-4 variables
 * - nc_get_att_float()      Query variable fill value
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

/** Total number of float values in the dataset. */
#define NVALS ((size_t)NZ * NY * NX)

/** Uncompressed size in bytes. */
#define UNCOMPRESSED_BYTES ((double)NVALS * sizeof(float))

/** NetCDF fill value for floats. */
#define FILL_VALUE (9.9692099683868690e+36f)

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
 * @brief Run one fill value test and print a CSV row.
 *
 * Creates a file with the given format and fill mode, writes a
 * 500×180×360 NC_FLOAT temperature variable, stats the file to
 * obtain the on-disk size, reads the variable back, then removes
 * the file.
 *
 * @param path       Path for temporary file (will be created and removed).
 * @param format     File format: NC_CLOBBER (classic) or NC_NETCDF4|NC_CLOBBER.
 * @param fill_mode  1 for NC_FILL, 0 for NC_NOFILL.
 * @param format_name String for CSV output ("classic" or "nc4").
 * @return 0 on success, 1 on any error.
 */
static int
run_test(const char *path, int format, int fill_mode, const char *format_name)
{
    int ncid, varid, dimids[3], ret;
    int old_fill_mode;
    double t_write_start, write_s, t_read_start, read_s;
    struct stat st;
    long long file_bytes;
    float *data;
    float fill_val = FILL_VALUE;

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

    if ((ret = nc_create(path, format, &ncid)))
        ERR(ret);

    /* Set fill mode before defining dimensions/variables */
    if ((ret = nc_set_fill(ncid, fill_mode ? NC_FILL : NC_NOFILL, &old_fill_mode)))
        ERR(ret);

    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat", NY, &dimids[1])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon", NX, &dimids[2])))
        ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid)))
        ERR(ret);

    /* Set _FillValue attribute */
    if (format & NC_NETCDF4)
    {
        /* For netCDF-4, use nc_def_var_fill */
        int no_fill = fill_mode ? 0 : 1;
        if ((ret = nc_def_var_fill(ncid, varid, no_fill, &fill_val)))
            ERR(ret);
    }
    else
    {
        /* For classic, use nc_put_att */
        if ((ret = nc_put_att_float(ncid, varid, "_FillValue", NC_FLOAT, 1, &fill_val)))
            ERR(ret);
    }

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
    printf("%s,%d,%.3f,%.3f,%lld\n",
           format_name, fill_mode, write_s, read_s, file_bytes);

    /* Cleanup */
    free(data);

    return 0;
}

/**
 * @brief Main entry point.
 *
 * Runs four fill value tests:
 *   1. Classic format with NC_FILL
 *   2. Classic format with NC_NOFILL
 *   3. NetCDF-4 format with NC_FILL
 *   4. NetCDF-4 format with NC_NOFILL
 *
 * Prints CSV header followed by one data row per test.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    int ret;

    printf("format,fill_mode,write_s,read_s,file_bytes\n");

    /* Classic format tests */
    ret = run_test("fill_classic_fillon.nc", NC_CLOBBER, 1, "classic");
    if (ret) return 1;
    remove("fill_classic_fillon.nc");

    ret = run_test("fill_classic_filloff.nc", NC_CLOBBER, 0, "classic");
    if (ret) return 1;
    remove("fill_classic_filloff.nc");

    /* NetCDF-4 format tests */
    ret = run_test("fill_nc4_fillon.nc", NC_NETCDF4 | NC_CLOBBER, 1, "nc4");
    if (ret) return 1;
    remove("fill_nc4_fillon.nc");

    ret = run_test("fill_nc4_filloff.nc", NC_NETCDF4 | NC_CLOBBER, 0, "nc4");
    if (ret) return 1;
    remove("fill_nc4_filloff.nc");

    return 0;
}

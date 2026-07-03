/**
 * @file chunking.c
 * @brief Demonstrates how chunk shape selection affects NetCDF-4 I/O performance
 *
 * Chunk shape is one of the most impactful performance decisions when creating
 * a NetCDF-4 file. The optimal shape depends entirely on how the data will be
 * accessed: a layout tuned for one access pattern is often worst-case for another.
 *
 * This program creates a meteorologically realistic 3D temperature dataset
 * (100 time steps × 36 latitudes × 72 longitudes) with two different chunk
 * shapes and measures read performance across two contrasting access patterns:
 *
 * **Chunk shapes tested:**
 * - Time-optimized (1×36×72): one complete horizontal field per chunk.
 *   Reading a time step loads exactly one chunk — ideal for NWP model output.
 * - Column-optimized (100×1×1): one complete time series per grid point.
 *   Extracting a station time series loads exactly one chunk — ideal for
 *   observation-style access.
 *
 * **Access patterns measured:**
 * - Time slab: read all 100 horizontal fields sequentially (typical forecast output)
 * - Column profile: read all NY×NX = 2,592 point time series (one per grid point)
 *
 * **Output:** CSV printed to stdout with columns:
 *   chunk_shape, access_pattern, elapsed_s, MB_per_s
 *
 * The 2×2 contrast (two shapes × two patterns) demonstrates that a 10–100×
 * performance difference is common between well-matched and mismatched configurations.
 *
 * **Plotting the output:**
 * @code
 *   ./chunking > results.csv
 *   # Import results.csv into any spreadsheet or Python/R plotting tool
 * @endcode
 *
 * **Learning Objectives:**
 * - Understand how chunk shape determines which access patterns are fast or slow
 * - Learn to use nc_def_var_chunking() to set chunk shape at creation time
 * - Use nc_inq_var_chunking() to verify chunk shape on an existing variable
 * - Measure read throughput with gettimeofday() wall-clock timing
 * - See that neither chunk shape is universally better — match shape to access
 *
 * **Key Concepts:**
 * - **Chunk Shape**: The dimensions of each storage block; determines how many
 *   chunks must be read for a given access pattern (fewer = faster)
 * - **Time-Optimized Chunks (1×NY×NX)**: Each chunk holds one complete time slab;
 *   reading a time step loads exactly one chunk — ideal for spatial analysis
 * - **Column-Optimized Chunks (NZ×1×1)**: Each chunk holds one complete time series;
 *   extracting a station record loads exactly one chunk — ideal for point analysis
 * - **Access Pattern Mismatch**: When chunk shape conflicts with access pattern,
 *   many extra chunks must be read and mostly discarded — 10–100× slower
 * - **No Universal Best**: The optimal chunk shape depends entirely on how the
 *   data will be read; design for the dominant access pattern
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 file creation basics
 * - chunking_performance.c - Broader chunking strategies overview
 *
 * **Related Examples:**
 * - chunking_performance.c - Three chunking strategies compared (netcdf-4 tutorial)
 * - deflate.c - Compression performance (depends on chunk shape)
 * - cache_tuning.c - Chunk cache tuning (mitigates mismatch overhead)
 *
 * **Key API functions:**
 * - nc_def_var_chunking()     Set chunk shape at variable definition time
 * - nc_inq_var_chunking()     Query chunk shape on an open variable
 * - nc_get_vara_float()       Read a hyperslab (both access patterns)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Dataset dimensions: 100 time steps × 36 lats × 72 lons (~10 MB total) */
#define NZ 100   /**< Time dimension size */
#define NY 36    /**< Latitude dimension size */
#define NX 72    /**< Longitude dimension size */

/* Chunk shape A: time-optimized — one complete horizontal field per chunk (~10 KB) */
#define CHUNK_Z_TIME 1
#define CHUNK_Y_TIME 36
#define CHUNK_X_TIME 72

/* Chunk shape B: column-optimized — one complete time series per point (~0.4 KB) */
#define CHUNK_Z_COL 100
#define CHUNK_Y_COL 1
#define CHUNK_X_COL 1

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
 * @brief Create a NZ×NY×NX temperature file with the given chunk shape.
 *
 * For time-optimized chunks (1×NY×NX) the file is written one time slab at a time,
 * which aligns with the chunk boundaries. For column-optimized chunks (NZ×1×1) the
 * file is written one column at a time, which aligns with those chunk boundaries and
 * avoids the extreme slowness of writing column chunks via horizontal slabs.
 *
 * @param path       Path to the NetCDF file to create (overwritten if it exists).
 * @param chunk_z    Chunk size along the time dimension.
 * @param chunk_y    Chunk size along the latitude dimension.
 * @param chunk_x    Chunk size along the longitude dimension.
 * @return 0 on success, 1 on error.
 */
static int
write_file(const char *path, size_t chunk_z, size_t chunk_y, size_t chunk_x)
{
    int ncid, varid, dimids[3], ret;
    size_t chunksizes[3];
    size_t start[3], count[3];
    float *buf;
    int i;

    chunksizes[0] = chunk_z;
    chunksizes[1] = chunk_y;
    chunksizes[2] = chunk_x;

    if ((ret = nc_create(path, NC_NETCDF4 | NC_CLOBBER, &ncid)))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", NZ, &dimids[0])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lat",  NY, &dimids[1])))
        ERR(ret);
    if ((ret = nc_def_dim(ncid, "lon",  NX, &dimids[2])))
        ERR(ret);
    if ((ret = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &varid)))
        ERR(ret);
    if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)))
        ERR(ret);
    if ((ret = nc_enddef(ncid)))
        ERR(ret);

    if (chunk_y == 1 && chunk_x == 1)
    {
        /* Column-optimized: write one column (NZ×1×1) at a time to match chunks */
        int y, x;
        buf = (float *)malloc(NZ * sizeof(float));
        if (!buf) { fprintf(stderr, "Memory allocation failed\n"); nc_close(ncid); return 1; }
        count[0] = NZ; count[1] = 1; count[2] = 1;
        for (y = 0; y < NY; y++)
        {
            for (x = 0; x < NX; x++)
            {
                for (i = 0; i < NZ; i++)
                    buf[i] = 280.0f + i * 0.1f;
                start[0] = 0; start[1] = y; start[2] = x;
                if ((ret = nc_put_vara_float(ncid, varid, start, count, buf)))
                    ERR(ret);
            }
        }
    }
    else
    {
        /* Time-optimized: write one horizontal slab (1×NY×NX) at a time */
        int t, j;
        buf = (float *)malloc(NY * NX * sizeof(float));
        if (!buf) { fprintf(stderr, "Memory allocation failed\n"); nc_close(ncid); return 1; }
        count[0] = 1; count[1] = NY; count[2] = NX;
        for (t = 0; t < NZ; t++)
        {
            float base = 280.0f + t * 0.1f;
            for (j = 0; j < NY * NX; j++)
                buf[j] = base;
            start[0] = t; start[1] = 0; start[2] = 0;
            if ((ret = nc_put_vara_float(ncid, varid, start, count, buf)))
                ERR(ret);
        }
    }

    free(buf);
    if ((ret = nc_close(ncid)))
        ERR(ret);
    return 0;
}

/**
 * @brief Time the "time slab" access pattern: read all NZ horizontal fields.
 *
 * Issues NZ separate nc_get_vara_float() calls, each requesting one complete
 * [1, NY, NX] slab. With time-optimized chunks this loads one chunk per call;
 * with column-optimized chunks each call must load NY×NX chunks.
 *
 * @param path        Path to the NetCDF file to read.
 * @param shape_label Short string identifying the chunk shape (for CSV output).
 * @return 0 on success, 1 on error.
 */
static int
time_access_time_slab(const char *path, const char *shape_label)
{
    int ncid, varid, ret;
    size_t start[3], count[3];
    float *slab;
    double t0, elapsed, mb_per_s;
    double total_mb;
    int t;

    slab = (float *)malloc(NY * NX * sizeof(float));
    if (!slab)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    if ((ret = nc_open(path, NC_NOWRITE, &ncid)))
        ERR(ret);
    if ((ret = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(ret);

    count[0] = 1; count[1] = NY; count[2] = NX;

    t0 = get_time();
    for (t = 0; t < NZ; t++)
    {
        start[0] = t; start[1] = 0; start[2] = 0;
        if ((ret = nc_get_vara_float(ncid, varid, start, count, slab)))
            ERR(ret);
    }
    elapsed = get_time() - t0;

    if ((ret = nc_close(ncid)))
        ERR(ret);
    free(slab);

    total_mb = (double)NZ * NY * NX * sizeof(float) / (1024.0 * 1024.0);
    mb_per_s = (elapsed > 0.0) ? total_mb / elapsed : 0.0;

    printf("%s,time_slab,%.3f,%.1f\n", shape_label, elapsed, mb_per_s);
    return 0;
}

/**
 * @brief Time the "column profile" access pattern: read all NY×NX point time series.
 *
 * Issues NY×NX nc_get_vara_float() calls, each requesting one complete [NZ, 1, 1]
 * column. With column-optimized chunks this loads one chunk per call; with
 * time-optimized chunks each call must touch NZ chunks.
 *
 * @param path        Path to the NetCDF file to read.
 * @param shape_label Short string identifying the chunk shape (for CSV output).
 * @return 0 on success, 1 on error.
 */
static int
time_access_column(const char *path, const char *shape_label)
{
    int ncid, varid, ret;
    size_t start[3], count[3];
    float *col;
    double t0, elapsed, mb_per_s;
    double total_mb;
    int y, x;

    col = (float *)malloc(NZ * sizeof(float));
    if (!col)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    if ((ret = nc_open(path, NC_NOWRITE, &ncid)))
        ERR(ret);
    if ((ret = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(ret);

    count[0] = NZ; count[1] = 1; count[2] = 1;

    t0 = get_time();
    for (y = 0; y < NY; y++)
    {
        for (x = 0; x < NX; x++)
        {
            start[0] = 0; start[1] = y; start[2] = x;
            if ((ret = nc_get_vara_float(ncid, varid, start, count, col)))
                ERR(ret);
        }
    }
    elapsed = get_time() - t0;

    if ((ret = nc_close(ncid)))
        ERR(ret);
    free(col);

    total_mb = (double)NY * NX * NZ * sizeof(float) / (1024.0 * 1024.0);
    mb_per_s = (elapsed > 0.0) ? total_mb / elapsed : 0.0;

    printf("%s,column_profile,%.3f,%.1f\n", shape_label, elapsed, mb_per_s);
    return 0;
}

/**
 * @brief Main entry point.
 *
 * Creates two NetCDF files with contrasting chunk shapes, runs two access
 * patterns on each, and prints CSV results to stdout.
 *
 * @return 0 on success, 1 on any error.
 */
int
main(void)
{
    printf("chunk_shape,access_pattern,elapsed_s,MB_per_s\n");

    /* File 1: time-optimized chunks (1 × 180 × 360) */
    if (write_file("chunking_time.nc",
                   CHUNK_Z_TIME, CHUNK_Y_TIME, CHUNK_X_TIME))
        return 1;
    if (time_access_time_slab("chunking_time.nc", "time_optimized"))
        return 1;
    if (time_access_column("chunking_time.nc", "time_optimized"))
        return 1;
    remove("chunking_time.nc");

    /* File 2: column-optimized chunks (500 × 1 × 1) */
    if (write_file("chunking_col.nc",
                   CHUNK_Z_COL, CHUNK_Y_COL, CHUNK_X_COL))
        return 1;
    if (time_access_time_slab("chunking_col.nc", "column_optimized"))
        return 1;
    if (time_access_column("chunking_col.nc", "column_optimized"))
        return 1;
    remove("chunking_col.nc");

    return 0;
}

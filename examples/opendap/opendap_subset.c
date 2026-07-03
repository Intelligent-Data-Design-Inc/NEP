/* Copyright 2026, Intelligent Data Design Inc. All rights reserved. */
/* See the COPYRIGHT file for copyright and license information. */

/**
 * @file opendap_subset.c
 * @brief OPeNDAP client-side subsetting example.
 *
 * This example demonstrates client-side subsetting: opening the full remote
 * dataset once, then making multiple targeted data requests using start/count
 * arrays. This approach offers more flexibility than server-side constraints.
 *
 * What this example does:
 * 1. Opens the full remote dataset (no constraint expression)
 * 2. Queries the variable dimensions to understand the full dataset shape
 * 3. Demonstrates four different data access patterns:
 *    - Single time slice: Read all lat/lon for one time step
 *    - Time series: Read all time points at a single lat/lon location
 *    - Regional subset: Read a small 3D cube (3 time steps, 10x10 spatial)
 *    - Multiple scattered requests: Show overhead of many small requests
 * 4. Closes the dataset
 *
 * Client-side vs Server-side subsetting trade-offs:
 *
 * Client-side (this example):
 *   + Can make multiple different subset requests from one open
 *   + Flexible exploration - change start/count without reopening
 *   - Transfers more data over network for each request
 *   - Server still reads full data then extracts subset
 *
 * Server-side (constraint expressions):
 *   + Server only sends the requested data
 *   + Most bandwidth-efficient for single known subset
 *   - Must reopen to change the subset
 *   - Dimension queries return constrained sizes only
 *
 * The 3D array layout is [time][lat][lon] with dimensions approximately
 * [time~200][lat=89][lon=180] depending on the dataset version.
 *
 * **Learning Objectives:**
 * - Open a full remote dataset without constraint expressions
 * - Use start[] and count[] arrays for multiple targeted data requests
 * - Implement four different access patterns (time slice, time series, regional, scattered)
 * - Understand the trade-offs between client-side and server-side subsetting
 * - Recognize the overhead of many small requests vs fewer large requests
 *
 * **Key Concepts:**
 * - **Client-Side Subsetting**: Open the full dataset once, then use start/count
 *   in nc_get_vara_*() calls to select different subregions on each read
 * - **Time Slice Access**: Read all spatial points for one time step
 *   (start=[t,0,0], count=[1,nlat,nlon])
 * - **Time Series Access**: Read all time points at one spatial location
 *   (start=[0,lat,lon], count=[ntime,1,1])
 * - **Regional Subset**: Read a small 3D cube from the full dataset
 *   (start=[t0,lat0,lon0], count=[nt,nlat_sub,nlon_sub])
 * - **Multiple Requests**: One nc_open() supports unlimited nc_get_vara_*() calls
 *   with different start/count values — no need to reopen for each subset
 *
 * **Prerequisites:**
 * - opendap_simple.c - Basic OPeNDAP access and metadata query
 * - simple_2D.c - Understanding start/count subsetting for local files
 * - A NetCDF-C library built with OPeNDAP support (NC_HAS_DAP2 or NC_HAS_DAP4)
 *
 * **Related Examples:**
 * - opendap_simple.c - Simplest OPeNDAP access (single subset)
 * - opendap_constraint.c - Server-side subsetting via URL constraints
 *
 * **Compilation:**
 * @code
 * gcc -o opendap_subset opendap_subset.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./opendap_subset
 * @endcode
 *
 * **Expected Output:**
 * - Opens remote SST dataset and prints full dimension sizes
 * - Demonstrates four access patterns with timing and data summaries:
 *   single time slice, time series at a point, regional 3D cube,
 *   and multiple scattered requests
 *
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

/* Error handling macro like coord.c */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    int ncid, varid, retval;
    int var_ndims;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t time_len, lat_len, lon_len;
    
    /* URL without constraint - we'll subset client-side */
    const char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    printf("OPeNDAP Client-Side Subsetting: %s\n\n", url);

    /* Open and get dimensions */
    if ((retval = nc_open(url, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "sst", &varid)))
        ERR(retval);
    if ((retval = nc_inq_varndims(ncid, varid, &var_ndims)))
        ERR(retval);
    if ((retval = nc_inq_vardimid(ncid, varid, var_dimids)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, var_dimids[0], &time_len)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, var_dimids[1], &lat_len)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, var_dimids[2], &lon_len)))
        ERR(retval);

    printf("Dimensions: time=%zu, lat=%zu, lon=%zu\n\n", time_len, lat_len, lon_len);
    
    /* Request 1: Single time slice */
    printf("1. Single time slice (all lat/lon for t=0):\n");
    {
        size_t start[3] = {0, 0, 0};
        size_t count[3] = {1, lat_len, lon_len};
        float *data = malloc(lat_len * lon_len * sizeof(float));
        if (!data) { fprintf(stderr, "Memory allocation failed\n"); return 1; }

        if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
            ERR(retval);
        printf("   Shape: %zux%zu, sample[45,90]=%.2f\n", lat_len, lon_len, data[45 * lon_len + 90]);
        free(data);
    }
    
    /* Request 2: Time series at one location */
    printf("\n2. Time series at (lat=45, lon=90):\n");
    {
        size_t start[3] = {0, 45, 90};
        size_t count[3] = {time_len, 1, 1};
        float *data = malloc(time_len * sizeof(float));
        if (!data) { fprintf(stderr, "Memory allocation failed\n"); return 1; }

        if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
            ERR(retval);
        printf("   %zu time points, first 5: ", time_len);
        for (int i = 0; i < 5 && i < (int)time_len; i++)
            printf("%.2f ", data[i]);
        printf("\n");
        free(data);
    }
    
    /* Request 3: Regional subset */
    printf("\n3. Regional subset (3x10x10 at t=0, lat=40, lon=85):\n");
    {
        size_t start[3] = {0, 40, 85};
        size_t count[3] = {3, 10, 10};
        float data[3][10][10];

        if ((retval = nc_get_vara_float(ncid, varid, start, count, &data[0][0][0])))
            ERR(retval);
        printf("   Shape 3x10x10, value[0,5,5]=%.2f\n", data[0][5][5]);
    }
    
    /* Request 4: Multiple scattered reads */
    printf("\n4. Multiple scattered reads (3 separate 10x10 slices):\n");
    {
        float data[10][10];

        for (int t = 0; t < 3; t++) {
            size_t start[3] = {(size_t)t, 40, 85};
            size_t count[3] = {1, 10, 10};
            if ((retval = nc_get_vara_float(ncid, varid, start, count, &data[0][0])))
                ERR(retval);
        }
        printf("   Complete\n");
    }
    
    /* Close */
    printf("\n");
    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("Done.\n");
    
    return 0;
}

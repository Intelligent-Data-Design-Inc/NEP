/* Copyright 2026, Intelligent Data Design Inc. All rights reserved. */
/* See the COPYRIGHT file for copyright and license information. */

/**
 * @file opendap_subset.c
 * @brief OPeNDAP client-side subsetting example with performance timing.
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
 * 4. Times each operation to demonstrate performance characteristics
 * 5. Closes the dataset
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
 * Performance lessons demonstrated:
 * - Open once, read multiple times (amortize connection overhead)
 * - Fewer large requests are more efficient than many small requests
 * - The TIMEIT macro shows actual transfer time for each pattern
 *
 * The 3D array layout is [time][lat][lon] with dimensions approximately
 * [time~200][lat=89][lon=180] depending on the dataset version.
 *
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Error handling macro */
#define NC_CHK(ret) do { \
    if ((ret) != NC_NOERR) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(ret), __LINE__); \
        return 1; \
    } \
} while (0)

/* Simple timing macro */
#define TIMEIT(code, msg) do { \
    clock_t start = clock(); \
    code; \
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC; \
    printf("  %s: %.3f seconds\n", msg, elapsed); \
} while (0)

int main(void)
{
    int ncid, varid;
    int var_ndims;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t time_len, lat_len, lon_len;
    
    /* URL without constraint - we'll subset client-side */
    const char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    printf("OPeNDAP Client-Side Subsetting Example\n");
    printf("======================================\n\n");
    printf("Opening full dataset:\n  %s\n\n", url);
    
    /* Open the dataset once */
    TIMEIT(NC_CHK(nc_open(url, NC_NOWRITE, &ncid)), "Open dataset");
    
    /* Get variable ID */
    NC_CHK(nc_inq_varid(ncid, "sst", &varid));
    
    /* Get dimension sizes */
    NC_CHK(nc_inq_varndims(ncid, varid, &var_ndims));
    NC_CHK(nc_inq_vardimid(ncid, varid, var_dimids));
    NC_CHK(nc_inq_dimlen(ncid, var_dimids[0], &time_len));
    NC_CHK(nc_inq_dimlen(ncid, var_dimids[1], &lat_len));
    NC_CHK(nc_inq_dimlen(ncid, var_dimids[2], &lon_len));
    
    printf("\nFull dataset dimensions:\n");
    printf("  Time: %zu\n", time_len);
    printf("  Latitude: %zu\n", lat_len);
    printf("  Longitude: %zu\n", lon_len);
    
    /* Read 1: Single time slice (all lat/lon for one time step) */
    printf("\n--- Request 1: Single time slice ---\n");
    {
        size_t start[3] = {0, 0, 0};  /* First time step */
        size_t count[3] = {1, lat_len, lon_len};
        float *data = malloc(lat_len * lon_len * sizeof(float));
        if (!data) { fprintf(stderr, "Memory allocation failed\n"); return 1; }
        
        TIMEIT(NC_CHK(nc_get_vara_float(ncid, varid, start, count, data)),
               "Read full spatial slice (1 time step)");
        
        printf("  Data shape: %zux%zu\n", lat_len, lon_len);
        printf("  Sample value [45,90]: %.2f\n", data[45 * lon_len + 90]);
        
        free(data);
    }
    
    /* Read 2: Time series at a specific location */
    printf("\n--- Request 2: Time series at single location ---\n");
    {
        size_t start[3] = {0, 45, 90};  /* Start at time 0, lat 45, lon 90 */
        size_t count[3] = {time_len, 1, 1};
        float *data = malloc(time_len * sizeof(float));
        if (!data) { fprintf(stderr, "Memory allocation failed\n"); return 1; }
        
        TIMEIT(NC_CHK(nc_get_vara_float(ncid, varid, start, count, data)),
               "Read time series (all time steps, 1 location)");
        
        printf("  Data shape: %zu time points\n", time_len);
        printf("  First 5 values: ");
        for (int i = 0; i < 5 && i < (int)time_len; i++) {
            printf("%.2f ", data[i]);
        }
        printf("\n");
        
        free(data);
    }
    
    /* Read 3: Small regional subset */
    printf("\n--- Request 3: Regional subset ---\n");
    {
        size_t start[3] = {0, 40, 85};  /* Start coordinates */
        size_t count[3] = {3, 10, 10};  /* 3 time steps, 10x10 spatial */
        float data[3][10][10];
        
        TIMEIT(NC_CHK(nc_get_vara_float(ncid, varid, start, count, &data[0][0][0])),
               "Read regional subset (3x10x10)");
        
        printf("  Data shape: 3x10x10\n");
        printf("  Value at [0,5,5]: %.2f\n", data[0][5][5]);
    }
    
    /* Read 4: Multiple scattered requests (demonstrating flexibility) */
    printf("\n--- Request 4: Multiple scattered reads ---\n");
    {
        float data[10][10];
        clock_t start_time = clock();
        
        /* Make 3 separate requests for different regions */
        for (int t = 0; t < 3; t++) {
            size_t start[3] = {(size_t)t, 40, 85};
            size_t count[3] = {1, 10, 10};
            NC_CHK(nc_get_vara_float(ncid, varid, start, count, &data[0][0]));
        }
        
        double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
        printf("  Read 3 separate 10x10 slices: %.3f seconds\n", elapsed);
        printf("  (Note: Multiple requests have overhead)\n");
    }
    
    /* Close the dataset */
    printf("\n");
    TIMEIT(NC_CHK(nc_close(ncid)), "Close dataset");
    
    printf("\nKey takeaways:\n");
    printf("- Open once, read multiple times for efficiency\n");
    printf("- Use start/count to request only needed data\n");
    printf("- Fewer large requests are more efficient than many small ones\n");
    
    return 0;
}

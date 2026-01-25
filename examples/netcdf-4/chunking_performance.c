/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates the impact of chunking on I/O performance with NetCDF-4.
 * Creates 3D datasets with different chunking strategies and measures performance.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netcdf.h>

#define NTIME 100
#define NLAT 180
#define NLON 360
#define NDIMS 3
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* Timing utility */
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

/* Create and write a file with specified chunking */
double create_chunked_file(const char *filename, const char *strategy_name,
                           int storage, size_t *chunksizes) {
    int ncid, varid;
    int time_dimid, lat_dimid, lon_dimid;
    int dimids[NDIMS];
    int retval;
    struct timespec start, end;
    
    float *data = malloc(NTIME * NLAT * NLON * sizeof(float));
    if (!data) {
        printf("Error: Memory allocation failed\n");
        exit(ERRCODE);
    }
    
    /* Initialize data with simple pattern */
    for (int t = 0; t < NTIME; t++)
        for (int lat = 0; lat < NLAT; lat++)
            for (int lon = 0; lon < NLON; lon++)
                data[t * NLAT * NLON + lat * NLON + lon] = 
                    (float)(t * 1000 + lat * 10 + lon);
    
    printf("\n=== %s ===\n", strategy_name);
    
    /* Start timing */
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Create file */
    if ((retval = nc_create(filename, NC_CLOBBER|NC_NETCDF4, &ncid)))
        ERR(retval);
    
    /* Define dimensions */
    if ((retval = nc_def_dim(ncid, "time", NTIME, &time_dimid)))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
        ERR(retval);
    
    /* Define variable */
    dimids[0] = time_dimid;
    dimids[1] = lat_dimid;
    dimids[2] = lon_dimid;
    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
        ERR(retval);
    
    /* Set chunking */
    if ((retval = nc_def_var_chunking(ncid, varid, storage, chunksizes)))
        ERR(retval);
    
    /* End define mode */
    if ((retval = nc_enddef(ncid)))
        ERR(retval);
    
    /* Write data */
    if ((retval = nc_put_var_float(ncid, varid, data)))
        ERR(retval);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* End timing */
    clock_gettime(CLOCK_MONOTONIC, &end);
    double write_time = get_time_diff(start, end);
    
    /* Get file size */
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fclose(fp);
        printf("File size: %ld bytes (%.2f MB)\n", file_size, file_size / 1048576.0);
    }
    
    printf("Write time: %.3f seconds\n", write_time);
    
    if (storage == NC_CHUNKED) {
        printf("Chunk sizes: [%zu, %zu, %zu]\n", 
               chunksizes[0], chunksizes[1], chunksizes[2]);
    } else {
        printf("Storage: Contiguous (no chunking)\n");
    }
    
    free(data);
    return write_time;
}

/* Read and validate file */
double read_and_validate(const char *filename, const char *test_name) {
    int ncid, varid;
    int retval;
    struct timespec start, end;
    
    float *data = malloc(NTIME * NLAT * NLON * sizeof(float));
    if (!data) {
        printf("Error: Memory allocation failed\n");
        exit(ERRCODE);
    }
    
    /* Start timing */
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Open file */
    if ((retval = nc_open(filename, NC_NOWRITE, &ncid)))
        ERR(retval);
    
    /* Get variable ID */
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    
    /* Verify chunking settings */
    int storage;
    size_t chunksizes[NDIMS];
    if ((retval = nc_inq_var_chunking(ncid, varid, &storage, chunksizes)))
        ERR(retval);
    
    /* Read all data */
    if ((retval = nc_get_var_float(ncid, varid, data)))
        ERR(retval);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* End timing */
    clock_gettime(CLOCK_MONOTONIC, &end);
    double read_time = get_time_diff(start, end);
    
    /* Validate a few data points */
    int errors = 0;
    for (int t = 0; t < 5; t++) {
        for (int lat = 0; lat < 5; lat++) {
            for (int lon = 0; lon < 5; lon++) {
                float expected = (float)(t * 1000 + lat * 10 + lon);
                float actual = data[t * NLAT * NLON + lat * NLON + lon];
                if (actual != expected) {
                    printf("Error: data[%d][%d][%d] = %f, expected %f\n",
                           t, lat, lon, actual, expected);
                    errors++;
                }
            }
        }
    }
    
    if (errors > 0) {
        printf("*** FAILED: %d validation errors\n", errors);
        exit(ERRCODE);
    }
    
    printf("%s read time: %.3f seconds\n", test_name, read_time);
    
    free(data);
    return read_time;
}

int main() {
    printf("Chunking Performance Demonstration\n");
    printf("===================================\n");
    printf("Dataset dimensions: [time=%d, lat=%d, lon=%d]\n", NTIME, NLAT, NLON);
    printf("Total data points: %d\n", NTIME * NLAT * NLON);
    printf("Total data size: %.2f MB\n", 
           (NTIME * NLAT * NLON * sizeof(float)) / 1048576.0);
    
    size_t chunksizes[NDIMS];
    double write_times[4];
    double read_times[4];
    
    /* Strategy 1: Contiguous storage (no chunking) */
    write_times[0] = create_chunked_file("chunk_contiguous.nc", 
                                         "Contiguous Storage",
                                         NC_CONTIGUOUS, NULL);
    
    /* Strategy 2: Time-optimized chunks (good for time-series access) */
    chunksizes[0] = 100;  /* All time steps */
    chunksizes[1] = 1;    /* Single lat */
    chunksizes[2] = 1;    /* Single lon */
    write_times[1] = create_chunked_file("chunk_time_optimized.nc",
                                         "Time-Optimized Chunking",
                                         NC_CHUNKED, chunksizes);
    
    /* Strategy 3: Spatial-optimized chunks (good for spatial slicing) */
    chunksizes[0] = 1;    /* Single time step */
    chunksizes[1] = 180;  /* All lats */
    chunksizes[2] = 360;  /* All lons */
    write_times[2] = create_chunked_file("chunk_spatial_optimized.nc",
                                         "Spatial-Optimized Chunking",
                                         NC_CHUNKED, chunksizes);
    
    /* Strategy 4: Balanced chunks (moderate sizes for mixed access) */
    chunksizes[0] = 10;   /* 10 time steps */
    chunksizes[1] = 45;   /* 45 lats */
    chunksizes[2] = 90;   /* 90 lons */
    write_times[3] = create_chunked_file("chunk_balanced.nc",
                                         "Balanced Chunking",
                                         NC_CHUNKED, chunksizes);
    
    /* Read and validate all files */
    printf("\n=== Reading and Validating Files ===\n");
    read_times[0] = read_and_validate("chunk_contiguous.nc", "Contiguous");
    read_times[1] = read_and_validate("chunk_time_optimized.nc", "Time-optimized");
    read_times[2] = read_and_validate("chunk_spatial_optimized.nc", "Spatial-optimized");
    read_times[3] = read_and_validate("chunk_balanced.nc", "Balanced");
    
    /* Performance summary */
    printf("\n=== Performance Summary ===\n");
    printf("%-25s %12s %12s\n", "Strategy", "Write (s)", "Read (s)");
    printf("%-25s %12.3f %12.3f\n", "Contiguous", write_times[0], read_times[0]);
    printf("%-25s %12.3f %12.3f\n", "Time-optimized", write_times[1], read_times[1]);
    printf("%-25s %12.3f %12.3f\n", "Spatial-optimized", write_times[2], read_times[2]);
    printf("%-25s %12.3f %12.3f\n", "Balanced", write_times[3], read_times[3]);
    
    printf("\n=== Recommendations ===\n");
    printf("- Contiguous storage: Best for small datasets or sequential access\n");
    printf("- Time-optimized: Best for time-series analysis at specific locations\n");
    printf("- Spatial-optimized: Best for spatial analysis at specific times\n");
    printf("- Balanced: Good compromise for mixed access patterns\n");
    printf("- Chunk size should align with typical access patterns\n");
    printf("- Larger chunks reduce metadata overhead but may waste I/O\n");
    printf("- Consider compression when using chunking (see compression.c)\n");
    
    printf("\n*** SUCCESS: All chunking strategies tested!\n");
    return 0;
}

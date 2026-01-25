/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates compression filters in NetCDF-4 with performance analysis.
 * Tests deflate, shuffle, and their combinations with realistic data.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <netcdf.h>

#define NTIME 50
#define NLAT 90
#define NLON 180
#define NDIMS 3
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

typedef struct {
    char name[64];
    char filename[128];
    int shuffle;
    int deflate;
    int deflate_level;
    double write_time;
    double read_time;
    long file_size;
    double compression_ratio;
} CompressionTest;

/* Generate realistic temperature data with spatial/temporal patterns */
void generate_temperature_data(float *data) {
    for (int t = 0; t < NTIME; t++) {
        for (int lat = 0; lat < NLAT; lat++) {
            for (int lon = 0; lon < NLON; lon++) {
                int idx = t * NLAT * NLON + lat * NLON + lon;
                
                /* Base temperature with latitude gradient */
                float base_temp = 15.0 - (lat - NLAT/2) * 0.5;
                
                /* Seasonal variation */
                float seasonal = 10.0 * sin(2.0 * M_PI * t / NTIME);
                
                /* Spatial variation */
                float spatial = 5.0 * sin(2.0 * M_PI * lon / NLON) * 
                               cos(2.0 * M_PI * lat / NLAT);
                
                data[idx] = base_temp + seasonal + spatial;
            }
        }
    }
}

/* Get file size */
long get_file_size(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

/* Create compressed file and measure performance */
void create_compressed_file(CompressionTest *test, float *data) {
    int ncid, varid;
    int time_dimid, lat_dimid, lon_dimid;
    int dimids[NDIMS];
    int retval;
    struct timespec start, end;
    
    printf("\n=== %s ===\n", test->name);
    
    /* Start timing */
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Create file */
    if ((retval = nc_create(test->filename, NC_CLOBBER|NC_NETCDF4, &ncid)))
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
    
    /* Set compression */
    if (test->deflate || test->shuffle) {
        if ((retval = nc_def_var_deflate(ncid, varid, test->shuffle, 
                                         test->deflate, test->deflate_level)))
            ERR(retval);
    }
    
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
    test->write_time = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    /* Get file size */
    test->file_size = get_file_size(test->filename);
    
    printf("Write time: %.3f seconds\n", test->write_time);
    printf("File size: %ld bytes (%.2f MB)\n", 
           test->file_size, test->file_size / 1048576.0);
    
    if (test->shuffle) printf("Shuffle: enabled\n");
    if (test->deflate) printf("Deflate: level %d\n", test->deflate_level);
}

/* Read and validate compressed file */
void read_compressed_file(CompressionTest *test, float *original_data) {
    int ncid, varid;
    int retval;
    struct timespec start, end;
    int shuffle, deflate, deflate_level;
    
    float *data = malloc(NTIME * NLAT * NLON * sizeof(float));
    if (!data) {
        printf("Error: Memory allocation failed\n");
        exit(ERRCODE);
    }
    
    /* Start timing */
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Open file */
    if ((retval = nc_open(test->filename, NC_NOWRITE, &ncid)))
        ERR(retval);
    
    /* Get variable ID */
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    
    /* Verify compression settings */
    if ((retval = nc_inq_var_deflate(ncid, varid, &shuffle, &deflate, &deflate_level)))
        ERR(retval);
    
    if (shuffle != test->shuffle || deflate != test->deflate || 
        (deflate && deflate_level != test->deflate_level)) {
        printf("Error: Compression settings mismatch\n");
        exit(ERRCODE);
    }
    
    /* Read data */
    if ((retval = nc_get_var_float(ncid, varid, data)))
        ERR(retval);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* End timing */
    clock_gettime(CLOCK_MONOTONIC, &end);
    test->read_time = (end.tv_sec - start.tv_sec) + 
                      (end.tv_nsec - start.tv_nsec) / 1e9;
    
    /* Validate data (check first 100 points) */
    int errors = 0;
    for (int i = 0; i < 100 && i < NTIME * NLAT * NLON; i++) {
        if (fabs(data[i] - original_data[i]) > 0.001) {
            printf("Error: data[%d] = %f, expected %f\n", i, data[i], original_data[i]);
            errors++;
        }
    }
    
    if (errors > 0) {
        printf("*** FAILED: %d validation errors\n", errors);
        exit(ERRCODE);
    }
    
    printf("Read time: %.3f seconds\n", test->read_time);
    printf("Data validated successfully\n");
    
    free(data);
}

int main() {
    printf("Compression Filter Demonstration\n");
    printf("=================================\n");
    printf("Dataset dimensions: [time=%d, lat=%d, lon=%d]\n", NTIME, NLAT, NLON);
    printf("Total data points: %d\n", NTIME * NLAT * NLON);
    printf("Total data size: %.2f MB\n", 
           (NTIME * NLAT * NLON * sizeof(float)) / 1048576.0);
    
    /* Generate realistic temperature data */
    float *data = malloc(NTIME * NLAT * NLON * sizeof(float));
    if (!data) {
        printf("Error: Memory allocation failed\n");
        return ERRCODE;
    }
    generate_temperature_data(data);
    
    /* Define compression tests */
    CompressionTest tests[] = {
        {"Uncompressed (baseline)", "compress_none.nc", 0, 0, 0, 0, 0, 0, 0},
        {"Shuffle only", "compress_shuffle.nc", 1, 0, 0, 0, 0, 0, 0},
        {"Deflate level 1", "compress_deflate1.nc", 0, 1, 1, 0, 0, 0, 0},
        {"Deflate level 5", "compress_deflate5.nc", 0, 1, 5, 0, 0, 0, 0},
        {"Deflate level 9", "compress_deflate9.nc", 0, 1, 9, 0, 0, 0, 0},
        {"Shuffle + Deflate 5 (recommended)", "compress_shuffle_deflate5.nc", 1, 1, 5, 0, 0, 0, 0}
    };
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    /* Run all tests */
    for (int i = 0; i < num_tests; i++) {
        create_compressed_file(&tests[i], data);
        read_compressed_file(&tests[i], data);
    }
    
    /* Calculate compression ratios */
    long baseline_size = tests[0].file_size;
    for (int i = 0; i < num_tests; i++) {
        tests[i].compression_ratio = (double)baseline_size / tests[i].file_size;
    }
    
    /* Print summary table */
    printf("\n=== Performance Summary ===\n");
    printf("%-35s %12s %12s %12s %10s\n", 
           "Strategy", "Write (s)", "Read (s)", "Size (MB)", "Ratio");
    printf("%-35s %12s %12s %12s %10s\n",
           "--------", "---------", "--------", "---------", "-----");
    
    for (int i = 0; i < num_tests; i++) {
        printf("%-35s %12.3f %12.3f %12.2f %10.2fx\n",
               tests[i].name,
               tests[i].write_time,
               tests[i].read_time,
               tests[i].file_size / 1048576.0,
               tests[i].compression_ratio);
    }
    
    /* Print recommendations */
    printf("\n=== Recommendations ===\n");
    printf("- Uncompressed: Fastest I/O but largest files\n");
    printf("- Shuffle only: Reorganizes bytes for better compression (use with deflate)\n");
    printf("- Deflate level 1: Fast compression, moderate space savings\n");
    printf("- Deflate level 5: Good balance of speed and compression\n");
    printf("- Deflate level 9: Maximum compression, slower writes\n");
    printf("- Shuffle + Deflate: RECOMMENDED for scientific data\n");
    printf("- Higher deflate levels increase write time but improve compression\n");
    printf("- Read performance generally similar across compression levels\n");
    printf("- Compression effectiveness depends on data patterns\n");
    
    free(data);
    printf("\n*** SUCCESS: All compression strategies tested!\n");
    return 0;
}

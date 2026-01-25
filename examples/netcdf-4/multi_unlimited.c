/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates multiple unlimited dimensions in NetCDF-4.
 * Shows appending data to both time and station dimensions.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <netcdf.h>

#define FILE_NAME "multi_unlimited.nc"
#define INITIAL_STATIONS 3
#define INITIAL_TIMES 5
#define ADDED_STATIONS 2
#define ADDED_TIMES 3
#define NDIMS 2
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main() {
    int ncid, varid;
    int station_dimid, time_dimid;
    int dimids[NDIMS];
    int retval;
    
    printf("Multiple Unlimited Dimensions Demonstration\n");
    printf("============================================\n");
    
    /* ========== INITIAL WRITE PHASE ========== */
    printf("\n=== Phase 1: Create file with initial data ===\n");
    printf("Initial stations: %d\n", INITIAL_STATIONS);
    printf("Initial times: %d\n", INITIAL_TIMES);
    
    /* Create file */
    if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
        ERR(retval);
    
    /* Define two unlimited dimensions */
    if ((retval = nc_def_dim(ncid, "station", NC_UNLIMITED, &station_dimid)))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dimid)))
        ERR(retval);
    
    /* Define variable using both unlimited dimensions */
    dimids[0] = station_dimid;
    dimids[1] = time_dimid;
    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
        ERR(retval);
    
    /* End define mode */
    if ((retval = nc_enddef(ncid)))
        ERR(retval);
    
    /* Write initial data */
    float initial_data[INITIAL_STATIONS][INITIAL_TIMES];
    for (int s = 0; s < INITIAL_STATIONS; s++) {
        for (int t = 0; t < INITIAL_TIMES; t++) {
            initial_data[s][t] = 20.0 + s * 2.0 + t * 0.5;
        }
    }
    
    size_t start[NDIMS], count[NDIMS];
    start[0] = 0;
    start[1] = 0;
    count[0] = INITIAL_STATIONS;
    count[1] = INITIAL_TIMES;
    
    if ((retval = nc_put_vara_float(ncid, varid, start, count, &initial_data[0][0])))
        ERR(retval);
    
    printf("Wrote initial data: %d stations × %d times\n", 
           INITIAL_STATIONS, INITIAL_TIMES);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* ========== APPEND STATIONS PHASE ========== */
    printf("\n=== Phase 2: Append new stations ===\n");
    printf("Adding %d new stations\n", ADDED_STATIONS);
    
    /* Reopen file in write mode */
    if ((retval = nc_open(FILE_NAME, NC_WRITE, &ncid)))
        ERR(retval);
    
    /* Get variable ID */
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    
    /* Append new stations */
    start[0] = INITIAL_STATIONS;  /* Start after existing stations */
    start[1] = 0;                 /* All times */
    count[0] = ADDED_STATIONS;
    count[1] = INITIAL_TIMES;
    
    float new_station_data[ADDED_STATIONS][INITIAL_TIMES];
    for (int s = 0; s < ADDED_STATIONS; s++) {
        for (int t = 0; t < INITIAL_TIMES; t++) {
            new_station_data[s][t] = 20.0 + (INITIAL_STATIONS + s) * 2.0 + t * 0.5;
        }
    }
    
    if ((retval = nc_put_vara_float(ncid, varid, start, count, &new_station_data[0][0])))
        ERR(retval);
    
    printf("Appended %d stations (now %d total)\n", 
           ADDED_STATIONS, INITIAL_STATIONS + ADDED_STATIONS);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* ========== APPEND TIMES PHASE ========== */
    printf("\n=== Phase 3: Append new times ===\n");
    printf("Adding %d new times\n", ADDED_TIMES);
    
    /* Reopen file */
    if ((retval = nc_open(FILE_NAME, NC_WRITE, &ncid)))
        ERR(retval);
    
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    
    /* Append new times for all stations */
    start[0] = 0;                          /* All stations */
    start[1] = INITIAL_TIMES;              /* Start after existing times */
    count[0] = INITIAL_STATIONS + ADDED_STATIONS;
    count[1] = ADDED_TIMES;
    
    int total_stations = INITIAL_STATIONS + ADDED_STATIONS;
    float new_time_data[total_stations][ADDED_TIMES];
    for (int s = 0; s < total_stations; s++) {
        for (int t = 0; t < ADDED_TIMES; t++) {
            new_time_data[s][t] = 20.0 + s * 2.0 + (INITIAL_TIMES + t) * 0.5;
        }
    }
    
    if ((retval = nc_put_vara_float(ncid, varid, start, count, &new_time_data[0][0])))
        ERR(retval);
    
    printf("Appended %d times (now %d total)\n", 
           ADDED_TIMES, INITIAL_TIMES + ADDED_TIMES);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* ========== READ AND VALIDATE PHASE ========== */
    printf("\n=== Phase 4: Read and validate all data ===\n");
    
    /* Open file for reading */
    if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
        ERR(retval);
    
    /* Verify both dimensions are unlimited */
    int num_unlimdims;
    int unlimdimids[NC_MAX_DIMS];
    if ((retval = nc_inq_unlimdims(ncid, &num_unlimdims, unlimdimids)))
        ERR(retval);
    
    if (num_unlimdims != 2) {
        printf("Error: Expected 2 unlimited dimensions, found %d\n", num_unlimdims);
        exit(ERRCODE);
    }
    printf("Verified: 2 unlimited dimensions detected\n");
    
    /* Get dimension IDs */
    if ((retval = nc_inq_dimid(ncid, "station", &station_dimid)))
        ERR(retval);
    if ((retval = nc_inq_dimid(ncid, "time", &time_dimid)))
        ERR(retval);
    
    /* Get dimension sizes */
    size_t nstations, ntimes;
    if ((retval = nc_inq_dimlen(ncid, station_dimid, &nstations)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, time_dimid, &ntimes)))
        ERR(retval);
    
    int expected_stations = INITIAL_STATIONS + ADDED_STATIONS;
    int expected_times = INITIAL_TIMES + ADDED_TIMES;
    
    if (nstations != expected_stations) {
        printf("Error: Expected %d stations, found %zu\n", expected_stations, nstations);
        exit(ERRCODE);
    }
    if (ntimes != expected_times) {
        printf("Error: Expected %d times, found %zu\n", expected_times, ntimes);
        exit(ERRCODE);
    }
    
    printf("Final dimensions: %zu stations × %zu times\n", nstations, ntimes);
    
    /* Read all data */
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    
    float *all_data = malloc(nstations * ntimes * sizeof(float));
    if (!all_data) {
        printf("Error: Memory allocation failed\n");
        exit(ERRCODE);
    }
    
    if ((retval = nc_get_var_float(ncid, varid, all_data)))
        ERR(retval);
    
    /* Validate data */
    int errors = 0;
    for (size_t s = 0; s < nstations; s++) {
        for (size_t t = 0; t < ntimes; t++) {
            float expected = 20.0 + s * 2.0 + t * 0.5;
            float actual = all_data[s * ntimes + t];
            if (fabs(actual - expected) > 0.001) {
                printf("Error: data[%zu][%zu] = %f, expected %f\n", s, t, actual, expected);
                errors++;
                if (errors >= 10) break;
            }
        }
        if (errors >= 10) break;
    }
    
    if (errors > 0) {
        printf("*** FAILED: %d validation errors\n", errors);
        exit(ERRCODE);
    }
    
    printf("Verified: all %zu data values correct\n", nstations * ntimes);
    
    /* Close file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    free(all_data);
    
    printf("\n=== Use Case ===\n");
    printf("Multiple unlimited dimensions are useful for:\n");
    printf("- Irregular time-series data from multiple stations\n");
    printf("- Adding new stations without knowing total count in advance\n");
    printf("- Appending new time steps as data becomes available\n");
    printf("- Growing datasets in multiple directions\n");
    printf("Note: Classic NetCDF format supports only ONE unlimited dimension\n");
    
    printf("\n*** SUCCESS: Multiple unlimited dimensions demonstrated!\n");
    return 0;
}

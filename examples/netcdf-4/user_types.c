/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates user-defined types in NetCDF-4: compound, vlen, enum, opaque.
 * Shows how to create and use custom data types.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netcdf.h>

#define FILE_NAME "user_types.nc"
#define NOBS 5
#define NDAYS 3
#define CALIB_SIZE 16
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* Compound type: Weather observation */
typedef struct {
    double time;
    float temperature;
    float pressure;
    float humidity;
} WeatherObs;

/* Enum type: Cloud cover categories */
typedef enum {
    CLEAR = 0,
    PARTLY_CLOUDY = 1,
    CLOUDY = 2,
    OVERCAST = 3
} CloudCover;

int main() {
    int ncid, retval;
    
    printf("User-Defined Types Demonstration\n");
    printf("=================================\n");
    
    /* ========== CREATE FILE AND DEFINE TYPES ========== */
    printf("\n=== Phase 1: Create file and define user types ===\n");
    
    if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
        ERR(retval);
    
    /* 1. Define Compound Type */
    printf("\n--- Compound Type (weather observation) ---\n");
    nc_type compound_typeid;
    if ((retval = nc_def_compound(ncid, sizeof(WeatherObs), "weather_obs_t", &compound_typeid)))
        ERR(retval);
    if ((retval = nc_insert_compound(ncid, compound_typeid, "time", 
                                     offsetof(WeatherObs, time), NC_DOUBLE)))
        ERR(retval);
    if ((retval = nc_insert_compound(ncid, compound_typeid, "temperature",
                                     offsetof(WeatherObs, temperature), NC_FLOAT)))
        ERR(retval);
    if ((retval = nc_insert_compound(ncid, compound_typeid, "pressure",
                                     offsetof(WeatherObs, pressure), NC_FLOAT)))
        ERR(retval);
    if ((retval = nc_insert_compound(ncid, compound_typeid, "humidity",
                                     offsetof(WeatherObs, humidity), NC_FLOAT)))
        ERR(retval);
    printf("Defined compound type with 4 fields\n");
    
    /* 2. Define Variable-Length Type */
    printf("\n--- Variable-Length Type (ragged arrays) ---\n");
    nc_type vlen_typeid;
    if ((retval = nc_def_vlen(ncid, "obs_per_day_t", NC_INT, &vlen_typeid)))
        ERR(retval);
    printf("Defined vlen type for variable-length integer arrays\n");
    
    /* 3. Define Enumeration Type */
    printf("\n--- Enumeration Type (cloud cover) ---\n");
    nc_type enum_typeid;
    if ((retval = nc_def_enum(ncid, NC_INT, "cloud_cover_t", &enum_typeid)))
        ERR(retval);
    CloudCover clear = CLEAR;
    CloudCover partly = PARTLY_CLOUDY;
    CloudCover cloudy = CLOUDY;
    CloudCover overcast = OVERCAST;
    if ((retval = nc_insert_enum(ncid, enum_typeid, "CLEAR", &clear)))
        ERR(retval);
    if ((retval = nc_insert_enum(ncid, enum_typeid, "PARTLY_CLOUDY", &partly)))
        ERR(retval);
    if ((retval = nc_insert_enum(ncid, enum_typeid, "CLOUDY", &cloudy)))
        ERR(retval);
    if ((retval = nc_insert_enum(ncid, enum_typeid, "OVERCAST", &overcast)))
        ERR(retval);
    printf("Defined enum type with 4 categories\n");
    
    /* 4. Define Opaque Type */
    printf("\n--- Opaque Type (binary calibration data) ---\n");
    nc_type opaque_typeid;
    if ((retval = nc_def_opaque(ncid, CALIB_SIZE, "calibration_t", &opaque_typeid)))
        ERR(retval);
    printf("Defined opaque type with %d-byte size\n", CALIB_SIZE);
    
    /* ========== DEFINE DIMENSIONS AND VARIABLES ========== */
    printf("\n=== Phase 2: Define dimensions and variables ===\n");
    
    int obs_dimid, day_dimid;
    if ((retval = nc_def_dim(ncid, "obs", NOBS, &obs_dimid)))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "day", NDAYS, &day_dimid)))
        ERR(retval);
    
    /* Variables using custom types */
    int compound_varid, vlen_varid, enum_varid, opaque_varid;
    
    if ((retval = nc_def_var(ncid, "observations", compound_typeid, 1, &obs_dimid, &compound_varid)))
        ERR(retval);
    if ((retval = nc_def_var(ncid, "obs_per_day", vlen_typeid, 1, &day_dimid, &vlen_varid)))
        ERR(retval);
    if ((retval = nc_def_var(ncid, "cloud_cover", enum_typeid, 1, &obs_dimid, &enum_varid)))
        ERR(retval);
    if ((retval = nc_def_var(ncid, "calibration", opaque_typeid, 0, NULL, &opaque_varid)))
        ERR(retval);
    
    if ((retval = nc_enddef(ncid)))
        ERR(retval);
    
    /* ========== WRITE DATA ========== */
    printf("\n=== Phase 3: Write data ===\n");
    
    /* Write compound data */
    WeatherObs obs_data[NOBS];
    for (int i = 0; i < NOBS; i++) {
        obs_data[i].time = 1000.0 + i * 3600.0;
        obs_data[i].temperature = 20.0 + i * 2.0;
        obs_data[i].pressure = 1013.0 + i * 0.5;
        obs_data[i].humidity = 60.0 - i * 5.0;
    }
    if ((retval = nc_put_var(ncid, compound_varid, obs_data)))
        ERR(retval);
    printf("Wrote %d compound observations\n", NOBS);
    
    /* Write vlen data */
    nc_vlen_t vlen_data[NDAYS];
    int day1_obs[] = {10, 15, 20};
    int day2_obs[] = {12, 18, 22, 25};
    int day3_obs[] = {8, 14};
    
    vlen_data[0].len = 3;
    vlen_data[0].p = day1_obs;
    vlen_data[1].len = 4;
    vlen_data[1].p = day2_obs;
    vlen_data[2].len = 2;
    vlen_data[2].p = day3_obs;
    
    if ((retval = nc_put_var(ncid, vlen_varid, vlen_data)))
        ERR(retval);
    printf("Wrote vlen data: day1=%zu obs, day2=%zu obs, day3=%zu obs\n",
           vlen_data[0].len, vlen_data[1].len, vlen_data[2].len);
    
    /* Write enum data */
    CloudCover cloud_data[NOBS] = {CLEAR, PARTLY_CLOUDY, CLOUDY, PARTLY_CLOUDY, OVERCAST};
    if ((retval = nc_put_var(ncid, enum_varid, cloud_data)))
        ERR(retval);
    printf("Wrote %d cloud cover values\n", NOBS);
    
    /* Write opaque data */
    unsigned char calib_data[CALIB_SIZE];
    for (int i = 0; i < CALIB_SIZE; i++) {
        calib_data[i] = (unsigned char)(i * 17);
    }
    if ((retval = nc_put_var(ncid, opaque_varid, calib_data)))
        ERR(retval);
    printf("Wrote %d bytes of opaque calibration data\n", CALIB_SIZE);
    
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* ========== READ AND VALIDATE ========== */
    printf("\n=== Phase 4: Read and validate data ===\n");
    
    if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
        ERR(retval);
    
    /* Verify compound type */
    printf("\n--- Validating Compound Type ---\n");
    if ((retval = nc_inq_varid(ncid, "observations", &compound_varid)))
        ERR(retval);
    
    WeatherObs obs_read[NOBS];
    if ((retval = nc_get_var(ncid, compound_varid, obs_read)))
        ERR(retval);
    
    int errors = 0;
    for (int i = 0; i < NOBS; i++) {
        if (fabs(obs_read[i].time - obs_data[i].time) > 0.001 ||
            fabs(obs_read[i].temperature - obs_data[i].temperature) > 0.001 ||
            fabs(obs_read[i].pressure - obs_data[i].pressure) > 0.001 ||
            fabs(obs_read[i].humidity - obs_data[i].humidity) > 0.001) {
            printf("Error: compound data mismatch at index %d\n", i);
            errors++;
        }
    }
    if (errors == 0) {
        printf("Verified: all %d compound observations correct\n", NOBS);
    }
    
    /* Verify vlen type */
    printf("\n--- Validating Variable-Length Type ---\n");
    if ((retval = nc_inq_varid(ncid, "obs_per_day", &vlen_varid)))
        ERR(retval);
    
    nc_vlen_t vlen_read[NDAYS];
    if ((retval = nc_get_var(ncid, vlen_varid, vlen_read)))
        ERR(retval);
    
    for (int d = 0; d < NDAYS; d++) {
        if (vlen_read[d].len != vlen_data[d].len) {
            printf("Error: vlen length mismatch for day %d\n", d);
            errors++;
        } else {
            int *vals = (int *)vlen_read[d].p;
            int *expected = (int *)vlen_data[d].p;
            for (size_t i = 0; i < vlen_read[d].len; i++) {
                if (vals[i] != expected[i]) {
                    printf("Error: vlen data mismatch day %d, obs %zu\n", d, i);
                    errors++;
                }
            }
        }
    }
    if (errors == 0) {
        printf("Verified: all vlen data correct (lengths: %zu, %zu, %zu)\n",
               vlen_read[0].len, vlen_read[1].len, vlen_read[2].len);
    }
    
    /* Free vlen memory */
    if ((retval = nc_free_vlen(vlen_read)))
        ERR(retval);
    
    /* Verify enum type */
    printf("\n--- Validating Enumeration Type ---\n");
    if ((retval = nc_inq_varid(ncid, "cloud_cover", &enum_varid)))
        ERR(retval);
    
    CloudCover cloud_read[NOBS];
    if ((retval = nc_get_var(ncid, enum_varid, cloud_read)))
        ERR(retval);
    
    for (int i = 0; i < NOBS; i++) {
        if (cloud_read[i] != cloud_data[i]) {
            printf("Error: enum data mismatch at index %d\n", i);
            errors++;
        }
    }
    if (errors == 0) {
        printf("Verified: all %d cloud cover values correct\n", NOBS);
    }
    
    /* Verify opaque type */
    printf("\n--- Validating Opaque Type ---\n");
    if ((retval = nc_inq_varid(ncid, "calibration", &opaque_varid)))
        ERR(retval);
    
    unsigned char calib_read[CALIB_SIZE];
    if ((retval = nc_get_var(ncid, opaque_varid, calib_read)))
        ERR(retval);
    
    for (int i = 0; i < CALIB_SIZE; i++) {
        if (calib_read[i] != calib_data[i]) {
            printf("Error: opaque data mismatch at byte %d\n", i);
            errors++;
        }
    }
    if (errors == 0) {
        printf("Verified: all %d bytes of opaque data correct\n", CALIB_SIZE);
    }
    
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    if (errors > 0) {
        printf("\n*** FAILED: %d validation errors\n", errors);
        return ERRCODE;
    }
    
    printf("\n=== Use Cases ===\n");
    printf("- Compound types: Group related fields (like C structs)\n");
    printf("- Variable-length types: Store ragged arrays efficiently\n");
    printf("- Enumeration types: Categorical data with named values\n");
    printf("- Opaque types: Binary metadata or proprietary formats\n");
    
    printf("\n*** SUCCESS: All user-defined types demonstrated!\n");
    return 0;
}

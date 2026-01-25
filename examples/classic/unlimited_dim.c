/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates unlimited dimensions for time-series data.
 * Creates a file with an unlimited time dimension, writes initial
 * timesteps, then reopens to append additional timesteps.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#define FILE_NAME "unlimited_dim.nc"
#define NLAT 4
#define NLON 5
#define INITIAL_TIMESTEPS 3
#define APPEND_TIMESTEPS 2
#define TOTAL_TIMESTEPS (INITIAL_TIMESTEPS + APPEND_TIMESTEPS)
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
   int ncid, time_varid, temp_varid;
   int time_dimid, lat_dimid, lon_dimid;
   int dimids[3];
   int retval;
   
   float time_data[TOTAL_TIMESTEPS] = {0.0, 1.0, 2.0, 3.0, 4.0};
   float temp_data[TOTAL_TIMESTEPS][NLAT][NLON];
   float temp_in[TOTAL_TIMESTEPS][NLAT][NLON];
   
   /* ========== WRITE PHASE (Initial) ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);
   
   /* Initialize temperature data for all timesteps */
   for (int t = 0; t < TOTAL_TIMESTEPS; t++)
      for (int i = 0; i < NLAT; i++)
         for (int j = 0; j < NLON; j++)
            temp_data[t][i][j] = 273.15 + t * 1.0 + i * 5.0 + j * 2.0;
   
   /* Create the NetCDF file */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
      ERR(retval);
   
   /* Define dimensions - time is unlimited */
   if ((retval = nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
      ERR(retval);
   
   /* Define time coordinate variable */
   if ((retval = nc_def_var(ncid, "time", NC_FLOAT, 1, &time_dimid, &time_varid)))
      ERR(retval);
   
   /* Define temperature variable (time, lat, lon) */
   dimids[0] = time_dimid;
   dimids[1] = lat_dimid;
   dimids[2] = lon_dimid;
   if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &temp_varid)))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write initial timesteps (0, 1, 2) */
   size_t start[3] = {0, 0, 0};
   size_t count[3] = {INITIAL_TIMESTEPS, NLAT, NLON};
   
   if ((retval = nc_put_vara_float(ncid, time_varid, start, &count[0], time_data)))
      ERR(retval);
   if ((retval = nc_put_vara_float(ncid, temp_varid, start, count, &temp_data[0][0][0])))
      ERR(retval);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("*** SUCCESS writing initial %d timesteps!\n", INITIAL_TIMESTEPS);
   
   /* ========== APPEND PHASE ========== */
   printf("\nReopening file to append data...\n");
   
   /* Reopen file in write mode */
   if ((retval = nc_open(FILE_NAME, NC_WRITE, &ncid)))
      ERR(retval);
   
   /* Get variable IDs */
   if ((retval = nc_inq_varid(ncid, "time", &time_varid)))
      ERR(retval);
   if ((retval = nc_inq_varid(ncid, "temperature", &temp_varid)))
      ERR(retval);
   
   /* Query current time dimension size */
   size_t current_size;
   if ((retval = nc_inq_dimlen(ncid, time_dimid, &current_size)))
      ERR(retval);
   
   if (current_size != INITIAL_TIMESTEPS) {
      printf("Error: Expected %d timesteps, found %zu\n", INITIAL_TIMESTEPS, current_size);
      exit(ERRCODE);
   }
   printf("Current time dimension size: %zu\n", current_size);
   
   /* Append additional timesteps (3, 4) */
   start[0] = INITIAL_TIMESTEPS;
   count[0] = APPEND_TIMESTEPS;
   
   if ((retval = nc_put_vara_float(ncid, time_varid, start, &count[0], 
                                     &time_data[INITIAL_TIMESTEPS])))
      ERR(retval);
   if ((retval = nc_put_vara_float(ncid, temp_varid, start, count, 
                                     &temp_data[INITIAL_TIMESTEPS][0][0])))
      ERR(retval);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("*** SUCCESS appending %d timesteps!\n", APPEND_TIMESTEPS);
   
   /* ========== READ PHASE ========== */
   printf("\nReopening file for validation...\n");
   
   /* Open the file for reading */
   if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
      ERR(retval);
   
   /* Verify unlimited dimension */
   int unlimdimid;
   if ((retval = nc_inq_unlimdim(ncid, &unlimdimid)))
      ERR(retval);
   
   if (unlimdimid != time_dimid) {
      printf("Error: time dimension is not unlimited\n");
      exit(ERRCODE);
   }
   printf("Verified: time dimension is unlimited\n");
   
   /* Verify total timesteps */
   size_t final_size;
   if ((retval = nc_inq_dimlen(ncid, time_dimid, &final_size)))
      ERR(retval);
   
   if (final_size != TOTAL_TIMESTEPS) {
      printf("Error: Expected %d total timesteps, found %zu\n", TOTAL_TIMESTEPS, final_size);
      exit(ERRCODE);
   }
   printf("Verified: %zu total timesteps after append\n", final_size);
   
   /* Read all time values */
   float time_in[TOTAL_TIMESTEPS];
   if ((retval = nc_get_var_float(ncid, time_varid, time_in)))
      ERR(retval);
   
   /* Verify time values */
   int errors = 0;
   for (int t = 0; t < TOTAL_TIMESTEPS; t++) {
      if (time_in[t] != time_data[t]) {
         printf("Error: time[%d] = %f, expected %f\n", t, time_in[t], time_data[t]);
         errors++;
      }
   }
   
   if (errors == 0) {
      printf("Verified: time coordinate values correct [");
      for (int t = 0; t < TOTAL_TIMESTEPS; t++) {
         printf("%g", time_in[t]);
         if (t < TOTAL_TIMESTEPS - 1) printf(", ");
      }
      printf("]\n");
   }
   
   /* Read all temperature data */
   if ((retval = nc_get_var_float(ncid, temp_varid, &temp_in[0][0][0])))
      ERR(retval);
   
   /* Verify temperature data continuity */
   for (int t = 0; t < TOTAL_TIMESTEPS; t++) {
      for (int i = 0; i < NLAT; i++) {
         for (int j = 0; j < NLON; j++) {
            if (temp_in[t][i][j] != temp_data[t][i][j]) {
               printf("Error: temperature[%d][%d][%d] = %f, expected %f\n", 
                      t, i, j, temp_in[t][i][j], temp_data[t][i][j]);
               errors++;
            }
         }
      }
   }
   
   if (errors > 0) {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }
   
   printf("Verified: all temperature data correct (%d timesteps x %d x %d = %d values)\n",
          TOTAL_TIMESTEPS, NLAT, NLON, TOTAL_TIMESTEPS * NLAT * NLON);
   printf("  Initial write: timesteps 0-%d\n", INITIAL_TIMESTEPS - 1);
   printf("  Appended: timesteps %d-%d\n", INITIAL_TIMESTEPS, TOTAL_TIMESTEPS - 1);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("\n*** SUCCESS: All validation checks passed!\n");
   return 0;
}

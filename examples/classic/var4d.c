/*
 * This is part of the book: Writing NetCDF Programs.
 *
 * Demonstrates multi-dimensional arrays with 2D, 3D, and 4D variables.
 * Creates a file with 4 dimensions (time, level, lat, lon) and three
 * variables of different dimensionalities to show dimension reuse.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 * Copyright: 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#define FILE_NAME "var4d.nc"
#define NTIME 3
#define NLEVEL 2
#define NLAT 4
#define NLON 5
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
   int ncid, varid_2d, varid_3d, varid_4d;
   int time_dimid, level_dimid, lat_dimid, lon_dimid;
   int dimids_2d[2], dimids_3d[3], dimids_4d[4];
   int retval;
   
   float temp_surface[NLAT][NLON];
   float temp_profile[NTIME][NLAT][NLON];
   float temp_3d[NTIME][NLEVEL][NLAT][NLON];
   
   float temp_surface_in[NLAT][NLON];
   float temp_profile_in[NTIME][NLAT][NLON];
   float temp_3d_in[NTIME][NLEVEL][NLAT][NLON];
   
   /* ========== WRITE PHASE ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);
   
   /* Initialize 2D surface temperature (lat, lon) */
   for (int i = 0; i < NLAT; i++)
      for (int j = 0; j < NLON; j++)
         temp_surface[i][j] = 273.15 + i * 5.0 + j * 2.0;
   
   /* Initialize 3D temperature profile (time, lat, lon) */
   for (int t = 0; t < NTIME; t++)
      for (int i = 0; i < NLAT; i++)
         for (int j = 0; j < NLON; j++)
            temp_profile[t][i][j] = 273.15 + t * 1.0 + i * 5.0 + j * 2.0;
   
   /* Initialize 4D temperature field (time, level, lat, lon) */
   for (int t = 0; t < NTIME; t++)
      for (int k = 0; k < NLEVEL; k++)
         for (int i = 0; i < NLAT; i++)
            for (int j = 0; j < NLON; j++)
               temp_3d[t][k][i][j] = 273.15 + t * 1.0 + k * 10.0 + i * 5.0 + j * 2.0;
   
   /* Create the NetCDF file */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
      ERR(retval);
   
   /* Define dimensions */
   if ((retval = nc_def_dim(ncid, "time", NTIME, &time_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "level", NLEVEL, &level_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
      ERR(retval);
   
   /* Define 2D variable: temp_surface(lat, lon) */
   dimids_2d[0] = lat_dimid;
   dimids_2d[1] = lon_dimid;
   if ((retval = nc_def_var(ncid, "temp_surface", NC_FLOAT, 2, dimids_2d, &varid_2d)))
      ERR(retval);
   
   /* Define 3D variable: temp_profile(time, lat, lon) */
   dimids_3d[0] = time_dimid;
   dimids_3d[1] = lat_dimid;
   dimids_3d[2] = lon_dimid;
   if ((retval = nc_def_var(ncid, "temp_profile", NC_FLOAT, 3, dimids_3d, &varid_3d)))
      ERR(retval);
   
   /* Define 4D variable: temp_3d(time, level, lat, lon) */
   dimids_4d[0] = time_dimid;
   dimids_4d[1] = level_dimid;
   dimids_4d[2] = lat_dimid;
   dimids_4d[3] = lon_dimid;
   if ((retval = nc_def_var(ncid, "temp_3d", NC_FLOAT, 4, dimids_4d, &varid_4d)))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write the data */
   if ((retval = nc_put_var_float(ncid, varid_2d, &temp_surface[0][0])))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, varid_3d, &temp_profile[0][0][0])))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, varid_4d, &temp_3d[0][0][0][0])))
      ERR(retval);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("*** SUCCESS writing file!\n");
   
   /* ========== READ PHASE ========== */
   printf("\nReopening file for validation...\n");
   
   /* Open the file for reading */
   if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
      ERR(retval);
   
   /* Verify metadata: check number of dimensions and variables */
   int ndims_in, nvars_in;
   if ((retval = nc_inq(ncid, &ndims_in, &nvars_in, NULL, NULL)))
      ERR(retval);
   
   if (ndims_in != 4) {
      printf("Error: Expected 4 dimensions, found %d\n", ndims_in);
      exit(ERRCODE);
   }
   printf("Verified: %d dimensions\n", ndims_in);
   
   if (nvars_in != 3) {
      printf("Error: Expected 3 variables, found %d\n", nvars_in);
      exit(ERRCODE);
   }
   printf("Verified: %d variables\n", nvars_in);
   
   /* Verify dimension sizes */
   size_t len_time, len_level, len_lat, len_lon;
   if ((retval = nc_inq_dimlen(ncid, time_dimid, &len_time)))
      ERR(retval);
   if ((retval = nc_inq_dimlen(ncid, level_dimid, &len_level)))
      ERR(retval);
   if ((retval = nc_inq_dimlen(ncid, lat_dimid, &len_lat)))
      ERR(retval);
   if ((retval = nc_inq_dimlen(ncid, lon_dimid, &len_lon)))
      ERR(retval);
   
   if (len_time != NTIME || len_level != NLEVEL || len_lat != NLAT || len_lon != NLON) {
      printf("Error: Dimension sizes incorrect\n");
      exit(ERRCODE);
   }
   printf("Verified: time=%zu, level=%zu, lat=%zu, lon=%zu\n", 
          len_time, len_level, len_lat, len_lon);
   
   /* Verify variable types and dimensions */
   nc_type var_type;
   int var_ndims;
   
   /* Check 2D variable */
   if ((retval = nc_inq_var(ncid, varid_2d, NULL, &var_type, &var_ndims, NULL, NULL)))
      ERR(retval);
   if (var_type != NC_FLOAT || var_ndims != 2) {
      printf("Error: temp_surface has wrong type or dimensions\n");
      exit(ERRCODE);
   }
   printf("Verified: temp_surface is 2D NC_FLOAT\n");
   
   /* Check 3D variable */
   if ((retval = nc_inq_var(ncid, varid_3d, NULL, &var_type, &var_ndims, NULL, NULL)))
      ERR(retval);
   if (var_type != NC_FLOAT || var_ndims != 3) {
      printf("Error: temp_profile has wrong type or dimensions\n");
      exit(ERRCODE);
   }
   printf("Verified: temp_profile is 3D NC_FLOAT\n");
   
   /* Check 4D variable */
   if ((retval = nc_inq_var(ncid, varid_4d, NULL, &var_type, &var_ndims, NULL, NULL)))
      ERR(retval);
   if (var_type != NC_FLOAT || var_ndims != 4) {
      printf("Error: temp_3d has wrong type or dimensions\n");
      exit(ERRCODE);
   }
   printf("Verified: temp_3d is 4D NC_FLOAT\n");
   
   /* Read the data back */
   if ((retval = nc_get_var_float(ncid, varid_2d, &temp_surface_in[0][0])))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, varid_3d, &temp_profile_in[0][0][0])))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, varid_4d, &temp_3d_in[0][0][0][0])))
      ERR(retval);
   
   /* Verify data correctness */
   int errors = 0;
   
   /* Verify 2D data */
   for (int i = 0; i < NLAT; i++) {
      for (int j = 0; j < NLON; j++) {
         if (temp_surface_in[i][j] != temp_surface[i][j]) {
            printf("Error: temp_surface[%d][%d] mismatch\n", i, j);
            errors++;
         }
      }
   }
   
   /* Verify 3D data */
   for (int t = 0; t < NTIME; t++) {
      for (int i = 0; i < NLAT; i++) {
         for (int j = 0; j < NLON; j++) {
            if (temp_profile_in[t][i][j] != temp_profile[t][i][j]) {
               printf("Error: temp_profile[%d][%d][%d] mismatch\n", t, i, j);
               errors++;
            }
         }
      }
   }
   
   /* Verify 4D data */
   for (int t = 0; t < NTIME; t++) {
      for (int k = 0; k < NLEVEL; k++) {
         for (int i = 0; i < NLAT; i++) {
            for (int j = 0; j < NLON; j++) {
               if (temp_3d_in[t][k][i][j] != temp_3d[t][k][i][j]) {
                  printf("Error: temp_3d[%d][%d][%d][%d] mismatch\n", t, k, i, j);
                  errors++;
               }
            }
         }
      }
   }
   
   if (errors > 0) {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }
   
   printf("Verified: all data values correct\n");
   printf("  2D array: %d x %d = %d values\n", NLAT, NLON, NLAT*NLON);
   printf("  3D array: %d x %d x %d = %d values\n", NTIME, NLAT, NLON, NTIME*NLAT*NLON);
   printf("  4D array: %d x %d x %d x %d = %d values\n", 
          NTIME, NLEVEL, NLAT, NLON, NTIME*NLEVEL*NLAT*NLON);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("\n*** SUCCESS: All validation checks passed!\n");
   return 0;
}

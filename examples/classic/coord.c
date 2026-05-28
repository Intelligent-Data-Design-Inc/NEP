/**
 * @file coord.c
 * @brief Demonstrates 3D surface temperature with time, lat, lon coordinate variables
 *
 * This example builds on coord_vars.c by adding a time dimension, creating a 3D
 * surface temperature field with latitude, longitude, and time coordinate variables.
 * It demonstrates how to work with three-dimensional data and follows Climate and
 * Forecast (CF) conventions for all coordinate metadata, including the time axis.
 *
 * The program creates a surface temperature dataset on a 4x5 lat/lon grid with 3
 * time steps, writes it to a classic NetCDF file, then reopens and validates all
 * contents.
 *
 * **Learning Objectives:**
 * - Work with 3D data (time, lat, lon)
 * - Define time coordinate variables with CF conventions
 * - Use the CF `calendar` attribute for time coordinates
 * - Add the `coordinates` attribute to data variables (CF best practice)
 * - Create classic-format NetCDF files (no HDF5 dependency)
 * - Validate multi-dimensional data after writing
 *
 * **Key Concepts:**
 * - **3D Data**: Surface temperature varying over space and time
 * - **Time Coordinate**: 1D variable with CF time attributes (units, calendar, axis)
 * - **`coordinates` Attribute**: Explicitly links data variables to their coordinate
 *   variables, a CF convention best practice
 * - **Classic Format**: Uses NC_CLOBBER (classic NetCDF) rather than NC_NETCDF4
 *
 * **CF Convention Attributes Used:**
 * - `units`: Physical units (degrees_north, degrees_east, hours since ..., K)
 * - `standard_name`: CF standard name vocabulary
 * - `long_name`: Human-readable descriptive name
 * - `axis`: Coordinate axis identifier (X, Y, T)
 * - `calendar`: Calendar type for time coordinates (standard)
 * - `_FillValue`: Value representing missing data
 * - `coordinates`: Lists coordinate variables for a data variable
 *
 * **Prerequisites:**
 * - coord_vars.c - 2D coordinate variables and CF metadata
 *
 * **Related Examples:**
 * - f_coord.f90 - Fortran equivalent
 * - coord_vars.c - 2D version without time dimension
 * - unlimited_dim.c - Using unlimited dimensions for time series
 * - var4d.c - 4D data with time, level, lat, lon
 *
 * **Compilation:**
 * @code
 * gcc -o coord coord.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./coord
 * ncdump coord.nc
 * @endcode
 *
 * **Expected Output:**
 * Creates coord.nc containing:
 * - 3 dimensions: time(3), lat(4), lon(5)
 * - 4 variables: time(time), lat(lat), lon(lon), sfc_temp(time,lat,lon)
 * - CF-compliant metadata attributes on all variables
 * - Surface temperature data in Kelvin
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "coord.nc"
#define NTIME 3
#define NLAT 4
#define NLON 5
#define NDIMS 3
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
   int ncid, time_varid, lat_varid, lon_varid, temp_varid;
   int time_dimid, lat_dimid, lon_dimid;
   int dimids[NDIMS];
   int retval;

   float time_data[NTIME] = {0.0, 6.0, 12.0};
   float lat[NLAT] = {-45.0, -15.0, 15.0, 45.0};
   float lon[NLON] = {-120.0, -60.0, 0.0, 60.0, 120.0};
   float sfc_temp[NTIME][NLAT][NLON];

   float time_in[NTIME];
   float lat_in[NLAT];
   float lon_in[NLON];
   float sfc_temp_in[NTIME][NLAT][NLON];

   /* ========== WRITE PHASE ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);

   /* Initialize surface temperature data */
   for (int t = 0; t < NTIME; t++)
      for (int i = 0; i < NLAT; i++)
         for (int j = 0; j < NLON; j++)
            sfc_temp[t][i][j] = 280.0 + i * 2.0 + j * 0.5 + t * 1.0;

   /* Create the NetCDF file (classic format) */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER, &ncid)))
      ERR(retval);

   /* Define dimensions */
   if ((retval = nc_def_dim(ncid, "time", NTIME, &time_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
      ERR(retval);

   /* Define time coordinate variable */
   if ((retval = nc_def_var(ncid, "time", NC_FLOAT, 1, &time_dimid, &time_varid)))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, time_varid, "units", 22, "hours since 2026-01-01")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, time_varid, "standard_name", 4, "time")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, time_varid, "long_name", 4, "Time")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, time_varid, "axis", 1, "T")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, time_varid, "calendar", 8, "standard")))
      ERR(retval);

   /* Define latitude coordinate variable */
   if ((retval = nc_def_var(ncid, "lat", NC_FLOAT, 1, &lat_dimid, &lat_varid)))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "units", 13, "degrees_north")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "standard_name", 8, "latitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "long_name", 8, "Latitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "axis", 1, "Y")))
      ERR(retval);

   /* Define longitude coordinate variable */
   if ((retval = nc_def_var(ncid, "lon", NC_FLOAT, 1, &lon_dimid, &lon_varid)))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "units", 12, "degrees_east")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "standard_name", 9, "longitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "long_name", 9, "Longitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "axis", 1, "X")))
      ERR(retval);

   /* Define surface temperature variable */
   dimids[0] = time_dimid;
   dimids[1] = lat_dimid;
   dimids[2] = lon_dimid;
   if ((retval = nc_def_var(ncid, "sfc_temp", NC_FLOAT, NDIMS, dimids, &temp_varid)))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "units", 1, "K")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "standard_name", 19, "surface_temperature")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "long_name", 19, "Surface Temperature")))
      ERR(retval);

   float fill_value = -999.0;
   if ((retval = nc_put_att_float(ncid, temp_varid, "_FillValue", NC_FLOAT, 1, &fill_value)))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "coordinates", 12, "time lat lon")))
      ERR(retval);

   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);

   /* Write coordinate variables */
   if ((retval = nc_put_var_float(ncid, time_varid, time_data)))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, lat_varid, lat)))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, lon_varid, lon)))
      ERR(retval);

   /* Write surface temperature data */
   if ((retval = nc_put_var_float(ncid, temp_varid, &sfc_temp[0][0][0])))
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

   /* Verify metadata */
   int ndims_in, nvars_in;
   if ((retval = nc_inq(ncid, &ndims_in, &nvars_in, NULL, NULL)))
      ERR(retval);

   if (ndims_in != 3) {
      printf("Error: Expected 3 dimensions, found %d\n", ndims_in);
      exit(ERRCODE);
   }
   printf("Verified: %d dimensions\n", ndims_in);

   if (nvars_in != 4) {
      printf("Error: Expected 4 variables, found %d\n", nvars_in);
      exit(ERRCODE);
   }
   printf("Verified: %d variables (time, lat, lon, sfc_temp)\n", nvars_in);

   /* Verify dimension sizes */
   size_t dimlen;
   if ((retval = nc_inq_dimlen(ncid, time_dimid, &dimlen)))
      ERR(retval);
   if (dimlen != NTIME) {
      printf("Error: time dimension = %zu, expected %d\n", dimlen, NTIME);
      exit(ERRCODE);
   }
   printf("Verified: time dimension = %zu\n", dimlen);

   if ((retval = nc_inq_dimlen(ncid, lat_dimid, &dimlen)))
      ERR(retval);
   if (dimlen != NLAT) {
      printf("Error: lat dimension = %zu, expected %d\n", dimlen, NLAT);
      exit(ERRCODE);
   }
   printf("Verified: lat dimension = %zu\n", dimlen);

   if ((retval = nc_inq_dimlen(ncid, lon_dimid, &dimlen)))
      ERR(retval);
   if (dimlen != NLON) {
      printf("Error: lon dimension = %zu, expected %d\n", dimlen, NLON);
      exit(ERRCODE);
   }
   printf("Verified: lon dimension = %zu\n", dimlen);

   /* Verify time coordinate attributes */
   char time_units[256] = {0}, time_stdname[256] = {0}, time_axis[256] = {0}, time_cal[256] = {0};
   size_t time_units_len, time_stdname_len, time_axis_len, time_cal_len;

   if ((retval = nc_inq_attlen(ncid, time_varid, "units", &time_units_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, time_varid, "units", time_units)))
      ERR(retval);
   time_units[time_units_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, time_varid, "standard_name", &time_stdname_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, time_varid, "standard_name", time_stdname)))
      ERR(retval);
   time_stdname[time_stdname_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, time_varid, "axis", &time_axis_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, time_varid, "axis", time_axis)))
      ERR(retval);
   time_axis[time_axis_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, time_varid, "calendar", &time_cal_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, time_varid, "calendar", time_cal)))
      ERR(retval);
   time_cal[time_cal_len] = '\0';

   if (strcmp(time_units, "hours since 2026-01-01") != 0 ||
       strcmp(time_stdname, "time") != 0 ||
       strcmp(time_axis, "T") != 0 ||
       strcmp(time_cal, "standard") != 0) {
      printf("Error: time coordinate attributes incorrect\n");
      exit(ERRCODE);
   }
   printf("Verified: all time coordinate attributes correct\n");

   /* Verify latitude coordinate attributes */
   char lat_units[256] = {0}, lat_stdname[256] = {0}, lat_axis[256] = {0};
   size_t lat_units_len, lat_stdname_len, lat_axis_len;

   if ((retval = nc_inq_attlen(ncid, lat_varid, "units", &lat_units_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "units", lat_units)))
      ERR(retval);
   lat_units[lat_units_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, lat_varid, "standard_name", &lat_stdname_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "standard_name", lat_stdname)))
      ERR(retval);
   lat_stdname[lat_stdname_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, lat_varid, "axis", &lat_axis_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "axis", lat_axis)))
      ERR(retval);
   lat_axis[lat_axis_len] = '\0';

   if (strcmp(lat_units, "degrees_north") != 0 ||
       strcmp(lat_stdname, "latitude") != 0 ||
       strcmp(lat_axis, "Y") != 0) {
      printf("Error: latitude coordinate attributes incorrect\n");
      exit(ERRCODE);
   }
   printf("Verified: all latitude coordinate attributes correct\n");

   /* Verify longitude coordinate attributes */
   char lon_units[256] = {0}, lon_stdname[256] = {0};
   size_t lon_units_len, lon_stdname_len;

   if ((retval = nc_inq_attlen(ncid, lon_varid, "units", &lon_units_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lon_varid, "units", lon_units)))
      ERR(retval);
   lon_units[lon_units_len] = '\0';

   if ((retval = nc_inq_attlen(ncid, lon_varid, "standard_name", &lon_stdname_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lon_varid, "standard_name", lon_stdname)))
      ERR(retval);
   lon_stdname[lon_stdname_len] = '\0';

   if (strcmp(lon_units, "degrees_east") != 0 ||
       strcmp(lon_stdname, "longitude") != 0) {
      printf("Error: longitude coordinate attributes incorrect\n");
      exit(ERRCODE);
   }
   printf("Verified: all longitude coordinate attributes correct\n");

   /* Verify sfc_temp variable attributes */
   char temp_units[256] = {0}, temp_coords[256] = {0};
   size_t temp_units_len, temp_coords_len;
   float fill_value_in;

   if ((retval = nc_inq_attlen(ncid, temp_varid, "units", &temp_units_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, temp_varid, "units", temp_units)))
      ERR(retval);
   temp_units[temp_units_len] = '\0';

   if ((retval = nc_get_att_float(ncid, temp_varid, "_FillValue", &fill_value_in)))
      ERR(retval);

   if ((retval = nc_inq_attlen(ncid, temp_varid, "coordinates", &temp_coords_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, temp_varid, "coordinates", temp_coords)))
      ERR(retval);
   temp_coords[temp_coords_len] = '\0';

   if (strcmp(temp_units, "K") != 0 ||
       fill_value_in != fill_value ||
       strcmp(temp_coords, "time lat lon") != 0) {
      printf("Error: sfc_temp variable attributes incorrect\n");
      exit(ERRCODE);
   }
   printf("Verified: all sfc_temp variable attributes correct\n");

   /* Read coordinate variables */
   if ((retval = nc_get_var_float(ncid, time_varid, time_in)))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, lat_varid, lat_in)))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, lon_varid, lon_in)))
      ERR(retval);

   /* Verify coordinate data */
   int errors = 0;
   for (int t = 0; t < NTIME; t++) {
      if (time_in[t] != time_data[t]) {
         printf("Error: time[%d] = %f, expected %f\n", t, time_in[t], time_data[t]);
         errors++;
      }
   }

   for (int i = 0; i < NLAT; i++) {
      if (lat_in[i] != lat[i]) {
         printf("Error: lat[%d] = %f, expected %f\n", i, lat_in[i], lat[i]);
         errors++;
      }
   }

   for (int j = 0; j < NLON; j++) {
      if (lon_in[j] != lon[j]) {
         printf("Error: lon[%d] = %f, expected %f\n", j, lon_in[j], lon[j]);
         errors++;
      }
   }

   if (errors == 0) {
      printf("Verified: coordinate arrays correct\n");
      printf("  time: [%g, %g, %g]\n", time_data[0], time_data[1], time_data[2]);
      printf("  lat: [%g, %g, %g, %g]\n", lat[0], lat[1], lat[2], lat[3]);
      printf("  lon: [%g, %g, %g, %g, %g]\n", lon[0], lon[1], lon[2], lon[3], lon[4]);
   }

   /* Read surface temperature data */
   if ((retval = nc_get_var_float(ncid, temp_varid, &sfc_temp_in[0][0][0])))
      ERR(retval);

   /* Verify surface temperature data */
   for (int t = 0; t < NTIME; t++) {
      for (int i = 0; i < NLAT; i++) {
         for (int j = 0; j < NLON; j++) {
            if (sfc_temp_in[t][i][j] != sfc_temp[t][i][j]) {
               printf("Error: sfc_temp[%d][%d][%d] = %f, expected %f\n",
                      t, i, j, sfc_temp_in[t][i][j], sfc_temp[t][i][j]);
               errors++;
            }
         }
      }
   }

   if (errors > 0) {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }

   printf("Verified: all surface temperature data correct (%d values)\n", NTIME * NLAT * NLON);

   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);

   printf("\n*** SUCCESS: All validation checks passed!\n");
   return 0;
}

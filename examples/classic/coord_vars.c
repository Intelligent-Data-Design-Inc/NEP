/**
 * @file coord_vars.c
 * @brief Demonstrates coordinate variables and CF convention metadata
 *
 * This example introduces the concept of coordinate variables - special 1D variables
 * that share the same name as their dimension and provide values along that axis.
 * Coordinate variables are essential for geospatial data, defining latitude, longitude,
 * time, or other dimensional coordinates.
 *
 * The program creates a 2D temperature field (4x5 grid) with latitude and longitude
 * coordinate variables following Climate and Forecast (CF) conventions. CF conventions
 * are the standard metadata conventions for climate and forecast data.
 *
 * **Learning Objectives:**
 * - Understand coordinate variables and their relationship to dimensions
 * - Learn CF convention metadata attributes (units, standard_name, long_name, axis)
 * - Master attribute definition and retrieval (nc_put_att, nc_get_att)
 * - Work with multi-dimensional geospatial data
 * - Implement proper metadata for scientific data interoperability
 *
 * **Key Concepts:**
 * - **Coordinate Variable**: 1D variable with same name as its dimension (e.g., lat(lat))
 * - **CF Conventions**: Standardized metadata for climate/forecast data
 * - **Attributes**: Metadata attached to variables (units, standard_name, etc.)
 * - **_FillValue**: Special attribute indicating missing/undefined data values
 * - **Geospatial Grid**: Regular lat/lon grid for spatial data
 *
 * **CF Convention Attributes Used:**
 * - `units`: Physical units (degrees_north, degrees_east, K)
 * - `standard_name`: CF standard name vocabulary (latitude, longitude, air_temperature)
 * - `long_name`: Human-readable descriptive name
 * - `axis`: Coordinate axis identifier (X, Y, Z, T)
 * - `_FillValue`: Value representing missing data
 *
 * **Prerequisites:** 
 * - simple_2D.c - Basic NetCDF file operations
 *
 * **Related Examples:**
 * - f_coord_vars.f90 - Fortran equivalent
 * - unlimited_dim.c - Adds time dimension with unlimited size
 * - var4d.c - 4D data with time, level, lat, lon coordinates
 *
 * **Compilation:**
 * @code
 * gcc -o coord_vars coord_vars.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./coord_vars
 * ncdump coord_vars.nc
 * @endcode
 *
 * **Expected Output:**
 * Creates coord_vars.nc containing:
 * - 2 dimensions: lat(4), lon(5)
 * - 3 variables: lat(lat), lon(lon), temperature(lat,lon)
 * - CF-compliant metadata attributes
 * - Temperature data in Kelvin with lat/lon coordinates
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "coord_vars.nc"
#define NLAT 4
#define NLON 5
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
   int ncid, lat_varid, lon_varid, temp_varid;
   int lat_dimid, lon_dimid;
   int dimids[2];
   int retval;
   
   float lat[NLAT] = {-45.0, -15.0, 15.0, 45.0};
   float lon[NLON] = {-120.0, -60.0, 0.0, 60.0, 120.0};
   float temperature[NLAT][NLON];
   
   float lat_in[NLAT];
   float lon_in[NLON];
   float temperature_in[NLAT][NLON];
   
   /* ========== WRITE PHASE ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);
   
   /* Initialize temperature data (synthetic: varies with lat and lon) */
   for (int i = 0; i < NLAT; i++)
      for (int j = 0; j < NLON; j++)
         temperature[i][j] = 273.15 + i * 5.0 + j * 2.0;
   
   /* Create the NetCDF file */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
      ERR(retval);
   
   /* Define dimensions */
   if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
      ERR(retval);
   
   /* Define coordinate variables (same name as dimension) */
   if ((retval = nc_def_var(ncid, "lat", NC_FLOAT, 1, &lat_dimid, &lat_varid)))
      ERR(retval);
   if ((retval = nc_def_var(ncid, "lon", NC_FLOAT, 1, &lon_dimid, &lon_varid)))
      ERR(retval);
   
   /* Add CF convention attributes to latitude */
   if ((retval = nc_put_att_text(ncid, lat_varid, "units", 13, "degrees_north")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "standard_name", 8, "latitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "long_name", 8, "Latitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lat_varid, "axis", 1, "Y")))
      ERR(retval);
   
   /* Add CF convention attributes to longitude */
   if ((retval = nc_put_att_text(ncid, lon_varid, "units", 12, "degrees_east")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "standard_name", 9, "longitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "long_name", 9, "Longitude")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, lon_varid, "axis", 1, "X")))
      ERR(retval);
   
   /* Define temperature variable */
   dimids[0] = lat_dimid;
   dimids[1] = lon_dimid;
   if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, 2, dimids, &temp_varid)))
      ERR(retval);
   
   /* Add CF convention attributes to temperature */
   if ((retval = nc_put_att_text(ncid, temp_varid, "units", 1, "K")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "standard_name", 15, "air_temperature")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, temp_varid, "long_name", 15, "Air Temperature")))
      ERR(retval);
   
   float fill_value = -999.0;
   if ((retval = nc_put_att_float(ncid, temp_varid, "_FillValue", NC_FLOAT, 1, &fill_value)))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write coordinate variables */
   if ((retval = nc_put_var_float(ncid, lat_varid, lat)))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, lon_varid, lon)))
      ERR(retval);
   
   /* Write temperature data */
   if ((retval = nc_put_var_float(ncid, temp_varid, &temperature[0][0])))
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
   
   if (ndims_in != 2) {
      printf("Error: Expected 2 dimensions, found %d\n", ndims_in);
      exit(ERRCODE);
   }
   printf("Verified: %d dimensions\n", ndims_in);
   
   if (nvars_in != 3) {
      printf("Error: Expected 3 variables, found %d\n", nvars_in);
      exit(ERRCODE);
   }
   printf("Verified: %d variables (lat, lon, temperature)\n", nvars_in);
   
   /* Verify latitude attributes */
   char att_text[256];
   size_t att_len;
   
   if ((retval = nc_inq_attlen(ncid, lat_varid, "units", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "units", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "degrees_north") != 0) {
      printf("Error: lat units = '%s', expected 'degrees_north'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: lat units = '%s'\n", att_text);
   
   if ((retval = nc_inq_attlen(ncid, lat_varid, "standard_name", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "standard_name", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "latitude") != 0) {
      printf("Error: lat standard_name = '%s', expected 'latitude'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: lat standard_name = '%s'\n", att_text);
   
   if ((retval = nc_inq_attlen(ncid, lat_varid, "axis", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lat_varid, "axis", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "Y") != 0) {
      printf("Error: lat axis = '%s', expected 'Y'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: lat axis = '%s'\n", att_text);
   
   /* Verify longitude attributes */
   if ((retval = nc_inq_attlen(ncid, lon_varid, "units", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lon_varid, "units", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "degrees_east") != 0) {
      printf("Error: lon units = '%s', expected 'degrees_east'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: lon units = '%s'\n", att_text);
   
   if ((retval = nc_inq_attlen(ncid, lon_varid, "standard_name", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, lon_varid, "standard_name", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "longitude") != 0) {
      printf("Error: lon standard_name = '%s', expected 'longitude'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: lon standard_name = '%s'\n", att_text);
   
   /* Verify temperature attributes */
   if ((retval = nc_inq_attlen(ncid, temp_varid, "units", &att_len)))
      ERR(retval);
   if ((retval = nc_get_att_text(ncid, temp_varid, "units", att_text)))
      ERR(retval);
   att_text[att_len] = '\0';
   if (strcmp(att_text, "K") != 0) {
      printf("Error: temperature units = '%s', expected 'K'\n", att_text);
      exit(ERRCODE);
   }
   printf("Verified: temperature units = '%s'\n", att_text);
   
   float fill_value_in;
   if ((retval = nc_get_att_float(ncid, temp_varid, "_FillValue", &fill_value_in)))
      ERR(retval);
   if (fill_value_in != fill_value) {
      printf("Error: temperature _FillValue = %f, expected %f\n", fill_value_in, fill_value);
      exit(ERRCODE);
   }
   printf("Verified: temperature _FillValue = %f\n", fill_value_in);
   
   /* Read coordinate variables */
   if ((retval = nc_get_var_float(ncid, lat_varid, lat_in)))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, lon_varid, lon_in)))
      ERR(retval);
   
   /* Verify coordinate data */
   int errors = 0;
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
      printf("  lat: [%g, %g, %g, %g]\n", lat[0], lat[1], lat[2], lat[3]);
      printf("  lon: [%g, %g, %g, %g, %g]\n", lon[0], lon[1], lon[2], lon[3], lon[4]);
   }
   
   /* Read temperature data */
   if ((retval = nc_get_var_float(ncid, temp_varid, &temperature_in[0][0])))
      ERR(retval);
   
   /* Verify temperature data */
   for (int i = 0; i < NLAT; i++) {
      for (int j = 0; j < NLON; j++) {
         if (temperature_in[i][j] != temperature[i][j]) {
            printf("Error: temperature[%d][%d] = %f, expected %f\n", 
                   i, j, temperature_in[i][j], temperature[i][j]);
            errors++;
         }
      }
   }
   
   if (errors > 0) {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }
   
   printf("Verified: all temperature data correct (%d values)\n", NLAT * NLON);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("\n*** SUCCESS: All validation checks passed!\n");
   return 0;
}

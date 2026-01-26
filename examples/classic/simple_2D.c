/**
 * @file simple_2D.c
 * @brief Basic example demonstrating 2D array creation and reading in NetCDF
 *
 * This example shows the fundamental workflow for working with NetCDF files:
 * - Creating a new NetCDF file
 * - Defining dimensions and variables
 * - Writing data to variables
 * - Closing and reopening the file
 * - Reading and verifying data
 *
 * The program creates a 2D integer array (6x12) with sequential values (0, 1, 2, ..., 71),
 * writes it to a NetCDF-4 file, then reopens the file to verify both metadata and data
 * correctness. This demonstrates the complete read-write cycle that forms the foundation
 * of NetCDF programming.
 *
 * **Learning Objectives:**
 * - Understand basic NetCDF file structure (dimensions, variables, data)
 * - Learn dimension and variable definition workflow
 * - Master data writing and reading operations
 * - Implement error handling patterns with nc_strerror()
 * - Verify metadata and data integrity
 *
 * **Key Concepts:**
 * - **Dimensions**: Named axes that define array shapes (x=6, y=12)
 * - **Variables**: Named data arrays with defined dimensions and types
 * - **NetCDF-4 Format**: HDF5-based format with enhanced features
 * - **Define Mode**: Metadata definition phase before data writing
 * - **Data Mode**: Phase where actual data is written/read
 *
 * **Prerequisites:** None - this is a beginner example
 *
 * **Related Examples:**
 * - coord_vars.c - Adds coordinate variables and CF metadata
 * - f_simple_2D.f90 - Fortran equivalent of this example
 * - simple_nc4.c - NetCDF-4 specific features (compression, chunking)
 *
 * **Compilation:**
 * @code
 * gcc -o simple_2D simple_2D.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./simple_2D
 * ncdump simple_2D.nc
 * @endcode
 *
 * **Expected Output:**
 * Creates simple_2D.nc containing:
 * - 2 dimensions: x(6), y(12)
 * - 1 variable: data(y, x) of type int
 * - Data: sequential integers from 0 to 71
 *
 * @author Edward Hartnett
 * @date 2026-01-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#define FILE_NAME "simple_2D.nc"
#define NDIMS 2
#define NX 6
#define NY 12
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
   int ncid, varid;
   int x_dimid, y_dimid;
   int dimids[NDIMS];
   int retval;
   
   int data_out[NY][NX];
   int data_in[NY][NX];
   
   /* ========== WRITE PHASE ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);
   
   /* Initialize data with sequential integers (0, 1, 2, 3, ...) */
   for (int i = 0; i < NY; i++)
      for (int j = 0; j < NX; j++)
         data_out[i][j] = i * NX + j;
   
   /* Create the NetCDF file (NC_CLOBBER overwrites existing file) */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
      ERR(retval);
   
   /* Define dimensions */
   if ((retval = nc_def_dim(ncid, "x", NX, &x_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "y", NY, &y_dimid)))
      ERR(retval);
   
   /* Define the variable (dimension order: y, x for C row-major) */
   dimids[0] = y_dimid;
   dimids[1] = x_dimid;
   if ((retval = nc_def_var(ncid, "data", NC_INT, NDIMS, dimids, &varid)))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write the data to the file */
   if ((retval = nc_put_var_int(ncid, varid, &data_out[0][0])))
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
   
   if (ndims_in != NDIMS) {
      printf("Error: Expected %d dimensions, found %d\n", NDIMS, ndims_in);
      exit(ERRCODE);
   }
   printf("Verified: %d dimensions\n", ndims_in);
   
   if (nvars_in != 1) {
      printf("Error: Expected 1 variable, found %d\n", nvars_in);
      exit(ERRCODE);
   }
   printf("Verified: %d variable\n", nvars_in);
   
   /* Verify dimension sizes */
   size_t len_x, len_y;
   if ((retval = nc_inq_dimlen(ncid, x_dimid, &len_x)))
      ERR(retval);
   if ((retval = nc_inq_dimlen(ncid, y_dimid, &len_y)))
      ERR(retval);
   
   if (len_x != NX) {
      printf("Error: Expected x dimension = %d, found %zu\n", NX, len_x);
      exit(ERRCODE);
   }
   if (len_y != NY) {
      printf("Error: Expected y dimension = %d, found %zu\n", NY, len_y);
      exit(ERRCODE);
   }
   printf("Verified: x dimension = %zu, y dimension = %zu\n", len_x, len_y);
   
   /* Verify variable type */
   nc_type var_type;
   if ((retval = nc_inq_vartype(ncid, varid, &var_type)))
      ERR(retval);
   
   if (var_type != NC_INT) {
      printf("Error: Expected variable type NC_INT, found %d\n", var_type);
      exit(ERRCODE);
   }
   printf("Verified: variable type is NC_INT\n");
   
   /* Read the data back */
   if ((retval = nc_get_var_int(ncid, varid, &data_in[0][0])))
      ERR(retval);
   
   /* Verify data correctness */
   int errors = 0;
   for (int i = 0; i < NY; i++) {
      for (int j = 0; j < NX; j++) {
         int expected = i * NX + j;
         if (data_in[i][j] != expected) {
            printf("Error: data[%d][%d] = %d, expected %d\n", 
                   i, j, data_in[i][j], expected);
            errors++;
         }
      }
   }
   
   if (errors > 0) {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }
   
   printf("Verified: all %d data values correct (0, 1, 2, ..., %d)\n", 
          NX * NY, NX * NY - 1);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("\n*** SUCCESS: All validation checks passed!\n");
   return 0;
}

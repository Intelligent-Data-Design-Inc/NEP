/**
 * @file simple_nc4.c
 * @brief Basic NetCDF-4/HDF5 format file creation and format detection
 *
 * This example introduces the NetCDF-4 format, which uses HDF5 as the underlying
 * storage layer. NetCDF-4 provides enhanced features including compression, chunking,
 * multiple unlimited dimensions, and user-defined types while maintaining backward
 * compatibility with the NetCDF API.
 *
 * The program creates a simple 2D array using the NC_NETCDF4 flag and demonstrates
 * format detection to verify the file was created in NetCDF-4/HDF5 format rather
 * than classic NetCDF format.
 *
 * **Learning Objectives:**
 * - Understand NetCDF-4 format and its HDF5 foundation
 * - Learn to create NetCDF-4 files with NC_NETCDF4 flag
 * - Master format detection using nc_inq_format() and nc_inq_format_extended()
 * - Recognize differences between classic and NetCDF-4 formats
 * - Prepare for advanced NetCDF-4 features (compression, chunking, groups)
 *
 * **Key Concepts:**
 * - **NetCDF-4 Format**: HDF5-based format with enhanced features
 * - **NC_NETCDF4 Flag**: Creation flag to specify NetCDF-4 format
 * - **Format Detection**: Runtime identification of file format type
 * - **HDF5 Backend**: Underlying storage technology for NetCDF-4
 * - **Backward Compatibility**: NetCDF-4 files readable with NetCDF-4 library
 *
 * **NetCDF-4 vs Classic Comparison:**
 * - **Classic**: Simple format, 2GB limits (CDF-1) or 4GB variable limits (CDF-2/5)
 * - **NetCDF-4**: HDF5-based, compression, chunking, unlimited file/variable sizes
 * - **Classic**: One unlimited dimension maximum
 * - **NetCDF-4**: Multiple unlimited dimensions supported
 * - **Classic**: No compression or chunking
 * - **NetCDF-4**: Built-in compression filters and flexible chunking
 *
 * **Prerequisites:**
 * - simple_2D.c - Basic NetCDF file operations
 * - format_variants.c - Understanding classic format types
 *
 * **Related Examples:**
 * - f_simple_nc4.f90 - Fortran equivalent
 * - compression.c - NetCDF-4 compression features
 * - chunking_performance.c - NetCDF-4 chunking strategies
 * - multi_unlimited.c - Multiple unlimited dimensions
 * - user_types.c - User-defined compound types
 *
 * **Compilation:**
 * @code
 * gcc -o simple_nc4 simple_nc4.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./simple_nc4
 * ncdump simple_nc4.nc
 * h5dump simple_nc4.nc  # View as HDF5 file
 * @endcode
 *
 * **Expected Output:**
 * Creates simple_nc4.nc in NetCDF-4/HDF5 format containing:
 * - 2 dimensions: x(6), y(12)
 * - 1 variable: data(y, x) of type int
 * - Format: NC_FORMAT_NETCDF4 (HDF5-based)
 * - File is readable by both NetCDF-4 and HDF5 tools
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#define FILE_NAME "simple_nc4.nc"
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
   printf("Creating NetCDF-4 file: %s\n", FILE_NAME);
   
   /* Initialize data with sequential integers (0, 1, 2, 3, ...) */
   for (int i = 0; i < NY; i++)
      for (int j = 0; j < NX; j++)
         data_out[i][j] = i * NX + j;
   
   /* Create the NetCDF-4 file with NC_NETCDF4 flag */
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
   
   /* Verify format is NetCDF-4 */
   int format;
   if ((retval = nc_inq_format(ncid, &format)))
      ERR(retval);
   
   if (format != NC_FORMAT_NETCDF4) {
      printf("Error: Expected NC_FORMAT_NETCDF4 (%d), found %d\n", 
             NC_FORMAT_NETCDF4, format);
      exit(ERRCODE);
   }
   printf("Verified: Format is NC_FORMAT_NETCDF4\n");
   
   /* Get extended format information */
   int format_extended, mode;
   if ((retval = nc_inq_format_extended(ncid, &format_extended, &mode)))
      ERR(retval);
   
   printf("Extended format: %d, Mode: %d\n", format_extended, mode);
   
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
   printf("NetCDF-4 format uses HDF5 as storage backend.\n");
   return 0;
}

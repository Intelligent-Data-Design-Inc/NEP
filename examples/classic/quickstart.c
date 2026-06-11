/**
 * @file quickstart.c
 * @brief Minimal introduction to NetCDF - the simplest starting point for new users
 *
 * This is the most basic NetCDF example, demonstrating the essential 6-step pattern:
 *   1. Create file          (nc_create)
 *   2. Define dimensions    (nc_def_dim)
 *   3. Define variables     (nc_def_var)
 *   4. Add attributes       (nc_put_att_*)
 *   5. Write data           (nc_put_var_*)
 *   6. Close file           (nc_close)
 *
 * The program creates a tiny 2D array (2x3) with 6 integer values, adds descriptive
 * attributes, writes it to a file, then reopens and reads the data back to verify
 * the round-trip worked. This demonstrates the complete NetCDF workflow.
 *
 * **Learning Objectives:**
 * - Understand the basic NetCDF workflow (create → define → write → close → read)
 * - Learn how to define dimensions and variables
 * - Master attribute creation for metadata
 * - Implement simple error handling
 * - Verify data integrity
 *
 * **Key Concepts:**
 * - **Dimensions**: Named axes that define array shapes (X=2, Y=3)
 * - **Variables**: Named data arrays with dimensions and types
 * - **Attributes**: Metadata describing the file or variables
 * - **Define Mode**: Phase where structure is defined (dimensions, variables, attributes)
 * - **Data Mode**: Phase where actual data is written/read
 *
 * **Prerequisites:** Basic C programming knowledge
 *
 * **Related Examples:**
 * - simple_2D.c - More detailed 2D array example with validation
 * - f_quickstart.f90 - Fortran equivalent of this example
 *
 * **Compilation:**
 * @code
 * gcc -o quickstart quickstart.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./quickstart
 * ncdump quickstart.nc
 * @endcode
 *
 * **Expected Output:**
 * Creates quickstart.nc containing:
 * - 2 dimensions: X(2), Y(3)
 * - 1 variable: data(X, Y) of type int
 * - 1 global attribute: description = "a quickstart example"
 * - 1 variable attribute: units = "m/s"
 * - Data: 6 sequential integers (1, 2, 3, 4, 5, 6)
 *
 * @author Edward Hartnett
 * @date 2026-01-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "quickstart.nc"
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return ERRCODE;}

int main()
{
   int ncid, x_dimid, y_dimid, data_varid;
   int dimids[2];
   int retval;
   
   int data_out[2][3] = {{1, 2, 3}, {4, 5, 6}};
   int data_in[2][3];
   
   /* ========== WRITE PHASE ========== */
   printf("Creating NetCDF file: %s\n", FILE_NAME);
   
   /* Create the NetCDF file (NC_CLOBBER overwrites existing file) */
   if ((retval = nc_create(FILE_NAME, NC_CLOBBER, &ncid)))
      ERR(retval);
   
   /* Define dimensions: X=2, Y=3 */
   if ((retval = nc_def_dim(ncid, "X", 2, &x_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "Y", 3, &y_dimid)))
      ERR(retval);
   
   /* Define the variable with dimensions X and Y */
   dimids[0] = x_dimid;
   dimids[1] = y_dimid;
   if ((retval = nc_def_var(ncid, "data", NC_INT, 2, dimids, &data_varid)))
      ERR(retval);
   
   /* Add global attribute */
   if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "description", 
                                  strlen("a quickstart example"), "a quickstart example")))
      ERR(retval);
   
   /* Add variable attribute */
   if ((retval = nc_put_att_text(ncid, data_varid, "units", 
                                  strlen("m/s"), "m/s")))
      ERR(retval);
   
   /* End define mode - ready to write data */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write the data to the file */
   if ((retval = nc_put_var_int(ncid, data_varid, &data_out[0][0])))
      ERR(retval);
   
   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("*** SUCCESS writing file!\n");
   
   /* ========== READ PHASE ========== */
   printf("\nReopening file for reading...\n");

   /* Open the file for reading */
   if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
      ERR(retval);

   /* Read the data back */
   if ((retval = nc_get_var_int(ncid, data_varid, &data_in[0][0])))
      ERR(retval);

   /* Verify data correctness */
   int ok = 1;
   for (int i = 0; i < 2 && ok; i++) {
      for (int j = 0; j < 3 && ok; j++) {
         if (data_in[i][j] != data_out[i][j])
            ok = 0;
      }
   }

   /* Close the file */
   if ((retval = nc_close(ncid)))
      ERR(retval);

   if (ok)
      printf("*** SUCCESS: Data read back correctly!\n");
   else
      printf("Error: Data mismatch\n");

   return ok ? 0 : 1;
}

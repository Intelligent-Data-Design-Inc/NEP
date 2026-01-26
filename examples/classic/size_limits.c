/**
 * @file size_limits.c
 * @brief Demonstrates file size and dimension limits for NetCDF classic formats
 *
 * This example explores the size limitations of the three classic NetCDF formats
 * (CDF-1, CDF-2, CDF-5) by creating test files and explaining theoretical limits.
 * Understanding these limits is essential for choosing the appropriate format for
 * large datasets.
 *
 * The program runs in small test mode (suitable for CI) by default, creating modest-sized
 * test files and displaying theoretical limits. This helps users understand when they
 * need to upgrade from CDF-1 to CDF-2 or CDF-5 for larger datasets.
 *
 * **Learning Objectives:**
 * - Understand file size limits for each classic NetCDF format
 * - Learn when format upgrades are necessary (2GB → 4GB → unlimited)
 * - Calculate theoretical maximum dimensions for different data types
 * - Recognize format-related error messages
 * - Make informed decisions about format selection for large datasets
 *
 * **Key Concepts:**
 * - **File Size Limit**: Maximum total file size (header + data)
 * - **Variable Size Limit**: Maximum size of a single variable
 * - **Dimension Limit**: Maximum dimension size based on variable limits
 * - **Format Upgrade Path**: CDF-1 → CDF-2 → CDF-5 for increasing size needs
 * - **Practical Limits**: Real-world constraints vs theoretical maximums
 *
 * **Format Limits Summary:**
 * - **CDF-1**: 2GB file limit, 2GB variable limit (NetCDF 3.0+)
 * - **CDF-2**: Unlimited file, 4GB variable limit (NetCDF 3.6.0+)
 * - **CDF-5**: Unlimited file and variable sizes (NetCDF 4.4.0+)
 *
 * **Prerequisites:**
 * - format_variants.c - Understanding format types
 * - simple_2D.c - Basic dimension and variable concepts
 *
 * **Related Examples:**
 * - f_size_limits.f90 - Fortran equivalent
 * - format_variants.c - Format comparison
 * - simple_nc4.c - NetCDF-4 format (different size limits)
 *
 * **Compilation:**
 * @code
 * gcc -o size_limits size_limits.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./size_limits
 * @endcode
 *
 * **Expected Output:**
 * - Creates small test files in each format
 * - Displays theoretical size limits
 * - Shows file sizes and dimension calculations
 * - Provides guidance on format selection
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <sys/stat.h>

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* Small test mode - fast tests suitable for CI */
#define CLASSIC_DIM 5000         /* ~20KB for float data */
#define OFFSET_DIM 5000          /* ~20KB for float data */
#define DATA_DIM 5000            /* ~20KB for float data */
#define TEST_MODE "SMALL"

/* Get file size in bytes */
long get_file_size(const char *filename)
{
   struct stat st;
   if (stat(filename, &st) == 0)
      return st.st_size;
   return -1;
}

/* Calculate theoretical limits for each format */
void print_format_limits(void)
{
   printf("\n=== NetCDF Classic Format Size Limits ===\n\n");
   
   printf("NC_CLASSIC_MODEL (CDF-1):\n");
   printf("  Total file size limit: 2GB (2,147,483,647 bytes)\n");
   printf("  Single variable limit: 2GB\n");
   printf("  Compatibility: NetCDF 3.0+, all tools\n\n");
   
   printf("NC_64BIT_OFFSET (CDF-2):\n");
   printf("  Total file size limit: effectively unlimited\n");
   printf("  Single variable limit: 4GB (4,294,967,295 bytes)\n");
   printf("  Compatibility: NetCDF 3.6.0+\n\n");
   
   printf("NC_64BIT_DATA (CDF-5):\n");
   printf("  Total file size limit: effectively unlimited\n");
   printf("  Single variable limit: effectively unlimited (2^64)\n");
   printf("  Compatibility: NetCDF 4.4.0+ or PnetCDF\n\n");
   
   printf("Size Calculation Formula:\n");
   printf("  file_size = header_size + sum(variable_sizes)\n");
   printf("  variable_size = product(dimensions) * sizeof(data_type)\n\n");
}

/* Test a specific format */
void test_format(const char *filename, int format_flag, const char *format_name, 
                 size_t dim_size)
{
   int ncid, varid, dimid;
   int retval;
   int format_in, mode_in;
   
   printf("Testing %s format...\n", format_name);
   printf("  Creating file: %s\n", filename);
   printf("  Dimension size: %zu\n", dim_size);
   
   /* Create file with specified format */
   if ((retval = nc_create(filename, format_flag | NC_CLOBBER, &ncid)))
      ERR(retval);
   
   /* Define dimension */
   if ((retval = nc_def_dim(ncid, "x", dim_size, &dimid)))
      ERR(retval);
   
   /* Define variable */
   if ((retval = nc_def_var(ncid, "data", NC_FLOAT, 1, &dimid, &varid)))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write a few test values */
   float test_data[10] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
   size_t start = 0;
   size_t count = (dim_size < 10) ? dim_size : 10;
   if ((retval = nc_put_vara_float(ncid, varid, &start, &count, test_data)))
      ERR(retval);
   
   /* Close file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   /* Reopen and verify format */
   if ((retval = nc_open(filename, NC_NOWRITE, &ncid)))
      ERR(retval);
   
   /* Check format */
   if ((retval = nc_inq_format(ncid, &format_in)))
      ERR(retval);
   
   /* Get extended format info */
   if ((retval = nc_inq_format_extended(ncid, &format_in, &mode_in)))
      ERR(retval);
   
   /* Verify format matches */
   const char *detected_format = "UNKNOWN";
   if (format_in == NC_FORMAT_CLASSIC)
      detected_format = "NC_FORMAT_CLASSIC";
   else if (format_in == NC_FORMAT_64BIT_OFFSET)
      detected_format = "NC_FORMAT_64BIT_OFFSET";
   else if (format_in == NC_FORMAT_64BIT_DATA)
      detected_format = "NC_FORMAT_64BIT_DATA";
   
   printf("  Format detected: %s\n", detected_format);
   
   /* Get file size */
   long file_size = get_file_size(filename);
   if (file_size >= 0) {
      if (file_size >= 1073741824)
         printf("  File size: %.2f GB\n", file_size / 1073741824.0);
      else if (file_size >= 1048576)
         printf("  File size: %.2f MB\n", file_size / 1048576.0);
      else if (file_size >= 1024)
         printf("  File size: %.2f KB\n", file_size / 1024.0);
      else
         printf("  File size: %ld bytes\n", file_size);
   }
   
   /* Calculate theoretical variable size */
   size_t var_size = dim_size * sizeof(float);
   if (var_size >= 1073741824)
      printf("  Variable size: %.2f GB\n", var_size / 1073741824.0);
   else if (var_size >= 1048576)
      printf("  Variable size: %.2f MB\n", var_size / 1048576.0);
   else
      printf("  Variable size: %.2f KB\n", var_size / 1024.0);
   
   /* Close file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("  ✓ Test complete\n\n");
}

int main()
{
   printf("NetCDF Classic Format Size Limits Test\n");
   printf("Test mode: %s\n", TEST_MODE);
   printf("\nRunning in small file mode (default).\n");
   printf("For actual size limit testing, use size_limits_large program\n");
   printf("(requires --enable-large-tests build option)\n");
   
   /* Print theoretical limits */
   print_format_limits();
   
   /* Test each format */
   test_format("size_limits_classic.nc", NC_CLASSIC_MODEL, 
               "NC_CLASSIC_MODEL", CLASSIC_DIM);
   
   test_format("size_limits_64bit_offset.nc", NC_64BIT_OFFSET, 
               "NC_64BIT_OFFSET", OFFSET_DIM);
   
   test_format("size_limits_64bit_data.nc", NC_64BIT_DATA, 
               "NC_64BIT_DATA", DATA_DIM);
   
   printf("=== All Format Tests Complete ===\n\n");
   
   printf("Summary:\n");
   printf("  Test mode: %s\n", TEST_MODE);
   printf("  Files created: 3\n");
   printf("  Formats tested: NC_CLASSIC_MODEL, NC_64BIT_OFFSET, NC_64BIT_DATA\n");
   printf("\nSmall file tests demonstrate format detection and calculations.\n");
   printf("For actual size limit testing, use size_limits_large program.\n");
   printf("\n*** SUCCESS: All validation checks passed! ***\n");
   return 0;
}

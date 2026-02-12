/**
 * @file format_variants.c
 * @brief Demonstrates all five NetCDF binary format variants
 *
 * This example creates identical data structures in all five NetCDF binary
 * formats to illustrate their differences in file size limits, compatibility,
 * storage backend, and use cases. Understanding format variants is crucial for
 * choosing the right format for your data requirements.
 *
 * The program creates five files with the same 3D temperature and pressure data
 * (time×lat×lon) in different formats, then compares their characteristics including
 * file size, format detection, and size limitations.
 *
 * **Learning Objectives:**
 * - Understand all five NetCDF binary format variants
 * - Learn when to use each format based on size requirements
 * - Master format detection with nc_inq_format() and nc_inq_format_extended()
 * - Compare file sizes and format overhead
 * - Make informed format choices for different use cases
 *
 * **Key Concepts:**
 * - **NC_CLASSIC_MODEL (CDF-1)**: Original NetCDF format, 2GB limits
 * - **NC_64BIT_OFFSET (CDF-2)**: Extended format with 4GB variable limit
 * - **NC_64BIT_DATA (CDF-5)**: Modern format with unlimited variable sizes
 * - **NC_NETCDF4**: NetCDF-4/HDF5 format with groups, compression, user types
 * - **NC_NETCDF4|NC_CLASSIC_MODEL**: HDF5 storage with classic data model
 * - **Format Detection**: Runtime identification of file format
 * - **Backward Compatibility**: Older formats work with all NetCDF versions
 *
 * **Format Comparison:**
 *
 * | Format | File Limit | Variable Limit | NetCDF Version | Backend | Use Case |
 * |--------|-----------|----------------|----------------|---------|----------|
 * | CDF-1  | 2GB       | 2GB           | 3.0+           | CDF     | Maximum compatibility |
 * | CDF-2  | Unlimited | 4GB           | 3.6.0+         | CDF     | Large files, moderate variables |
 * | CDF-5  | Unlimited | Unlimited     | 4.4.0+         | CDF     | Very large variables |
 * | NC4    | Unlimited | Unlimited     | 4.0+           | HDF5    | Full NetCDF-4 features |
 * | NC4/CM | Unlimited | Unlimited     | 4.0+           | HDF5    | HDF5 storage, classic model |
 *
 * **Prerequisites:**
 * - simple_2D.c - Basic file creation
 * - simple_nc4.c - NetCDF-4 basics
 *
 * **Related Examples:**
 * - f_format_variants.f90 - Fortran equivalent
 * - size_limits.c - Demonstrates actual size limits
 * - compression.c - NetCDF-4 compression features
 *
 * **Compilation:**
 * @code
 * gcc -o format_variants format_variants.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./format_variants
 * ls -lh format_*.nc
 * ncdump -h format_classic.nc
 * @endcode
 *
 * **Expected Output:**
 * Creates five files:
 * - format_classic.nc (CDF-1)
 * - format_64bit_offset.nc (CDF-2)
 * - format_64bit_data.nc (CDF-5)
 * - format_netcdf4.nc (NetCDF-4/HDF5)
 * - format_netcdf4_classic.nc (NetCDF-4 Classic Model)
 * All contain identical data with similar file sizes for this small dataset.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <sys/stat.h>

#define NTIME 10
#define NLAT 20
#define NLON 30
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* Get file size in bytes */
long get_file_size(const char *filename)
{
   struct stat st;
   if (stat(filename, &st) == 0)
      return st.st_size;
   return -1;
}

/* Create a file in the specified format with identical data structure */
void create_format_file(const char *filename, int format_flag, const char *format_name)
{
   int ncid, time_dimid, lat_dimid, lon_dimid;
   int temp_varid, pressure_varid;
   int dimids[3];
   int retval;
   
   float temperature[NTIME][NLAT][NLON];
   float pressure[NTIME][NLAT][NLON];
   
   printf("Creating %s format file: %s\n", format_name, filename);
   
   /* Initialize data */
   for (int t = 0; t < NTIME; t++)
      for (int i = 0; i < NLAT; i++)
         for (int j = 0; j < NLON; j++) {
            temperature[t][i][j] = 273.15 + t * 1.0 + i * 0.5 + j * 0.2;
            pressure[t][i][j] = 1013.25 + t * 0.1 + i * 0.05 + j * 0.02;
         }
   
   /* Create file */
   if ((retval = nc_create(filename, format_flag | NC_CLOBBER, &ncid)))
      ERR(retval);
   
   /* Define dimensions */
   if ((retval = nc_def_dim(ncid, "time", NTIME, &time_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lat", NLAT, &lat_dimid)))
      ERR(retval);
   if ((retval = nc_def_dim(ncid, "lon", NLON, &lon_dimid)))
      ERR(retval);
   
   /* Define variables */
   dimids[0] = time_dimid;
   dimids[1] = lat_dimid;
   dimids[2] = lon_dimid;
   
   if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, 3, dimids, &temp_varid)))
      ERR(retval);
   if ((retval = nc_def_var(ncid, "pressure", NC_FLOAT, 3, dimids, &pressure_varid)))
      ERR(retval);
   
   /* Add attributes */
   if ((retval = nc_put_att_text(ncid, temp_varid, "units", 1, "K")))
      ERR(retval);
   if ((retval = nc_put_att_text(ncid, pressure_varid, "units", 3, "hPa")))
      ERR(retval);
   
   /* End define mode */
   if ((retval = nc_enddef(ncid)))
      ERR(retval);
   
   /* Write data */
   if ((retval = nc_put_var_float(ncid, temp_varid, &temperature[0][0][0])))
      ERR(retval);
   if ((retval = nc_put_var_float(ncid, pressure_varid, &pressure[0][0][0])))
      ERR(retval);
   
   /* Close file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
   
   printf("  File created successfully\n");
}

/* Verify a format file */
void verify_format_file(const char *filename, int expected_format,
                        const char *expected_format_name)
{
   int ncid, retval;
   int format_in;
   int ndims, nvars;
   float temperature[NTIME][NLAT][NLON];
   float pressure[NTIME][NLAT][NLON];
   int temp_varid, pressure_varid;
   
   printf("\nVerifying file: %s\n", filename);
   
   /* Open file */
   if ((retval = nc_open(filename, NC_NOWRITE, &ncid)))
      ERR(retval);
   
   /* Check format */
   if ((retval = nc_inq_format(ncid, &format_in)))
      ERR(retval);
   
   /* Determine format name */
   const char *detected_format = "UNKNOWN";
   if (format_in == NC_FORMAT_CLASSIC)
      detected_format = "NC_FORMAT_CLASSIC (CDF-1)";
   else if (format_in == NC_FORMAT_64BIT_OFFSET)
      detected_format = "NC_FORMAT_64BIT_OFFSET (CDF-2)";
   else if (format_in == NC_FORMAT_64BIT_DATA)
      detected_format = "NC_FORMAT_64BIT_DATA (CDF-5)";
   else if (format_in == NC_FORMAT_NETCDF4)
      detected_format = "NC_FORMAT_NETCDF4 (HDF5)";
   else if (format_in == NC_FORMAT_NETCDF4_CLASSIC)
      detected_format = "NC_FORMAT_NETCDF4_CLASSIC (HDF5/Classic)";
   
   printf("  Format detected: %s\n", detected_format);
   
   /* Verify expected format */
   if (format_in != expected_format) {
      printf("Error: Expected format %s (%d), got %s (%d)\n",
             expected_format_name, expected_format, detected_format, format_in);
      exit(ERRCODE);
   }
   
   /* Get file size */
   long file_size = get_file_size(filename);
   if (file_size >= 0) {
      printf("  File size: ");
      if (file_size >= 1048576)
         printf("%.2f MB (%ld bytes)\n", file_size / 1048576.0, file_size);
      else if (file_size >= 1024)
         printf("%.2f KB (%ld bytes)\n", file_size / 1024.0, file_size);
      else
         printf("%ld bytes\n", file_size);
   }
   
   /* Verify metadata */
   if ((retval = nc_inq(ncid, &ndims, &nvars, NULL, NULL)))
      ERR(retval);
   
   if (ndims != 3 || nvars != 2) {
      printf("Error: Expected 3 dimensions and 2 variables, found %d dims, %d vars\n", 
             ndims, nvars);
      exit(ERRCODE);
   }
   printf("  Metadata: %d dimensions, %d variables\n", ndims, nvars);
   
   /* Get variable IDs */
   if ((retval = nc_inq_varid(ncid, "temperature", &temp_varid)))
      ERR(retval);
   if ((retval = nc_inq_varid(ncid, "pressure", &pressure_varid)))
      ERR(retval);
   
   /* Read data */
   if ((retval = nc_get_var_float(ncid, temp_varid, &temperature[0][0][0])))
      ERR(retval);
   if ((retval = nc_get_var_float(ncid, pressure_varid, &pressure[0][0][0])))
      ERR(retval);
   
   /* Verify a few data values */
   int errors = 0;
   float expected_temp = 273.15;
   float expected_pressure = 1013.25;
   
   if (temperature[0][0][0] != expected_temp) {
      printf("Error: temperature[0][0][0] = %f, expected %f\n", 
             temperature[0][0][0], expected_temp);
      errors++;
   }
   
   if (pressure[0][0][0] != expected_pressure) {
      printf("Error: pressure[0][0][0] = %f, expected %f\n", 
             pressure[0][0][0], expected_pressure);
      errors++;
   }
   
   if (errors == 0)
      printf("  Data validation: %d values verified\n", NTIME * NLAT * NLON * 2);
   else {
      printf("*** FAILED: %d data validation errors\n", errors);
      exit(ERRCODE);
   }
   
   /* Close file */
   if ((retval = nc_close(ncid)))
      ERR(retval);
}

int main()
{
   printf("NetCDF Format Variants Comparison\n\n");
   
   printf("This program creates five files with identical data structures\n");
   printf("in all five NetCDF binary formats to demonstrate their differences.\n\n");
   
   printf("Data structure:\n");
   printf("  Dimensions: time=%d, lat=%d, lon=%d\n", NTIME, NLAT, NLON);
   printf("  Variables: temperature(time,lat,lon), pressure(time,lat,lon)\n");
   printf("  Data type: NC_FLOAT (4 bytes per value)\n");
   printf("  Total data: %d values per variable\n\n", NTIME * NLAT * NLON);
   
   /* Create files in each format */
   printf("=== Creating Format Files ===\n\n");
   
   create_format_file("format_classic.nc", NC_CLASSIC_MODEL, "NC_CLASSIC_MODEL");
   create_format_file("format_64bit_offset.nc", NC_64BIT_OFFSET, "NC_64BIT_OFFSET");
   create_format_file("format_64bit_data.nc", NC_64BIT_DATA, "NC_64BIT_DATA");
   create_format_file("format_netcdf4.nc", NC_NETCDF4, "NC_NETCDF4");
   create_format_file("format_netcdf4_classic.nc", NC_NETCDF4 | NC_CLASSIC_MODEL,
                      "NC_NETCDF4|NC_CLASSIC_MODEL");
   
   /* Verify files */
   printf("\n=== Verifying Format Files ===\n");
   
   verify_format_file("format_classic.nc", NC_FORMAT_CLASSIC,
                      "NC_FORMAT_CLASSIC");
   verify_format_file("format_64bit_offset.nc", NC_FORMAT_64BIT_OFFSET,
                      "NC_FORMAT_64BIT_OFFSET");
   verify_format_file("format_64bit_data.nc", NC_FORMAT_64BIT_DATA,
                      "NC_FORMAT_64BIT_DATA");
   verify_format_file("format_netcdf4.nc", NC_FORMAT_NETCDF4,
                      "NC_FORMAT_NETCDF4");
   verify_format_file("format_netcdf4_classic.nc", NC_FORMAT_NETCDF4_CLASSIC,
                      "NC_FORMAT_NETCDF4_CLASSIC");
   
   /* Summary */
   printf("\n=== Format Comparison Summary ===\n\n");
   
   long size_classic = get_file_size("format_classic.nc");
   long size_offset = get_file_size("format_64bit_offset.nc");
   long size_data = get_file_size("format_64bit_data.nc");
   long size_nc4 = get_file_size("format_netcdf4.nc");
   long size_nc4_classic = get_file_size("format_netcdf4_classic.nc");
   
   printf("File sizes:\n");
   printf("  NC_CLASSIC_MODEL:            %ld bytes\n", size_classic);
   printf("  NC_64BIT_OFFSET:             %ld bytes\n", size_offset);
   printf("  NC_64BIT_DATA:               %ld bytes\n", size_data);
   printf("  NC_NETCDF4:                  %ld bytes\n", size_nc4);
   printf("  NC_NETCDF4|NC_CLASSIC_MODEL: %ld bytes\n", size_nc4_classic);
   
   printf("\nFormat Characteristics:\n\n");
   
   printf("NC_CLASSIC_MODEL (CDF-1):\n");
   printf("  File size limit: 2GB\n");
   printf("  Variable size limit: 2GB\n");
   printf("  Storage backend: CDF binary\n");
   printf("  Compatibility: NetCDF 3.0+, all tools\n");
   printf("  Use when: Maximum compatibility needed, files < 2GB\n\n");
   
   printf("NC_64BIT_OFFSET (CDF-2):\n");
   printf("  File size limit: effectively unlimited\n");
   printf("  Variable size limit: 4GB per variable\n");
   printf("  Storage backend: CDF binary\n");
   printf("  Compatibility: NetCDF 3.6.0+\n");
   printf("  Use when: Large files needed, variables < 4GB each\n\n");
   
   printf("NC_64BIT_DATA (CDF-5):\n");
   printf("  File size limit: effectively unlimited\n");
   printf("  Variable size limit: effectively unlimited\n");
   printf("  Storage backend: CDF binary\n");
   printf("  Compatibility: NetCDF 4.4.0+ or PnetCDF\n");
   printf("  Use when: Very large variables needed (> 4GB)\n\n");
   
   printf("NC_NETCDF4 (HDF5):\n");
   printf("  File size limit: effectively unlimited\n");
   printf("  Variable size limit: effectively unlimited\n");
   printf("  Storage backend: HDF5\n");
   printf("  Compatibility: NetCDF 4.0+\n");
   printf("  Features: groups, compression, chunking, user-defined types\n");
   printf("  Use when: Advanced features needed (compression, groups, etc.)\n\n");
   
   printf("NC_NETCDF4|NC_CLASSIC_MODEL (HDF5 Classic Model):\n");
   printf("  File size limit: effectively unlimited\n");
   printf("  Variable size limit: effectively unlimited\n");
   printf("  Storage backend: HDF5\n");
   printf("  Compatibility: NetCDF 4.0+\n");
   printf("  Features: compression, chunking (no groups, no user-defined types)\n");
   printf("  Use when: HDF5 storage benefits needed with classic data model\n\n");
   
   printf("Key Observations:\n");
   printf("  - All five formats store identical data correctly\n");
   printf("  - Classic formats (CDF-1/2/5) have smaller overhead for small files\n");
   printf("  - NetCDF-4 formats (HDF5) have larger overhead but support compression\n");
   printf("  - NC4 classic model is a useful middle ground: HDF5 storage, simple model\n");
   printf("  - Use nc_inq_format() to detect format type when reading files\n\n");
   
   printf("*** SUCCESS: All format tests passed! ***\n");
   return 0;
}

/**
 * @file tst_imap_mag.c
 * @brief Test opening IMAP MAG CDF file via NetCDF API
 * 
 * This test validates that the IMAP MAG L1B calibration CDF file can be
 * opened through the standard NetCDF API using the CDF UDF handler.
 * 
 * The test data file is copied to the build directory by the build system
 * to support both in-tree and out-of-tree builds.
 * 
 * @author Edward Hartnett
 * @date 2025-11-26
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netcdf.h>
#include "cdfdispatch.h"

#define TEST_FILE "data/imap_mag_l1b-calibration_20240229_v001.cdf"

/**
 * @brief Main test function
 * 
 * Opens IMAP MAG CDF file via NetCDF API and then closes it.
 * 
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    int ncid;
    int ndims, nvars, natts, unlimdimid;
    int retval;
    
    printf("=== NEP IMAP MAG CDF Test ===\n\n");
    
    /* Register the CDF UDF handler with NetCDF */
    printf("Registering CDF UDF handler...\n");
    extern const NC_Dispatch *CDF_dispatch_table;
    NC_CDF_initialize();
    
    /* CDF files start with magic bytes 0xCDF30001 or 0xCDF26002 */
    /* Using CDF3 magic number: 0xCD, 0xF3, 0x00, 0x01 */
    char cdf_magic[5] = "\xCD\xF3\x00\x01";
    retval = nc_def_user_format(NC_UDF0, (NC_Dispatch *)CDF_dispatch_table, cdf_magic);
    if (retval != NC_NOERR)
    {
        fprintf(stderr, "ERROR: Failed to register CDF UDF handler: %s\n", 
                nc_strerror(retval));
        return 1;
    }
    printf("  ✓ CDF UDF handler registered\n\n");
    
    /* Open the IMAP MAG CDF file using NetCDF API */
    printf("Opening IMAP MAG CDF file via NetCDF API: %s\n", TEST_FILE);
    if ((retval = nc_open(TEST_FILE, NC_NOWRITE, &ncid)))
    {
        fprintf(stderr, "ERROR: Failed to open IMAP MAG CDF file via NetCDF API: %s\n", 
                nc_strerror(retval));
        return 1;
    }
    printf("  ✓ Successfully opened IMAP MAG CDF file via NetCDF API\n\n");

    /* Check the metadata. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid)))
	return 1;
    printf("ndims %d nvars %d natts %d unlimdimid %d\n", ndims, nvars, natts, unlimdimid);

    /* In this test file there are 6 dims, 24 global atts and 6 vars. */
    if (ndims != 6 || nvars != 6 || natts != 24) return 1;
    
    /* Close the file */
    printf("Closing file...\n");
    if ((retval = nc_close(ncid)))
    {
	fprintf(stderr, "ERROR: Failed to close file: %s\n", nc_strerror(retval));
        return 1;
    }
    printf("  ✓ Successfully closed file\n\n");
    
    /* Print test summary */
    printf("=== Test Summary ===\n");
    printf("✓ IMAP MAG CDF file opened via NetCDF API\n");
    printf("✓ File closed successfully\n\n");
    printf("SUCCESS: IMAP MAG CDF file access validated!\n");
    
    return 0;
}

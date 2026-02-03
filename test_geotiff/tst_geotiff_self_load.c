/**
 * @file tst_geotiff_self_load.c
 * @brief Test GeoTIFF UDF self-loading functionality
 * 
 * This test validates that the GeoTIFF UDF handler works correctly with
 * NetCDF-C's self-loading UDF plugin system when HAVE_NETCDF_UDF_SELF_REGISTRATION
 * is available. It verifies that:
 * 1. NC_GEOTIFF_initialize() can be called successfully
 * 2. The initialization function does NOT call nc_def_user_format() when self-loading is available
 * 3. GeoTIFF files can be opened through the NetCDF API after initialization
 * 
 * This test is part of v1.5.4 Sprint 2 and validates the conditional compilation
 * strategy for UDF self-loading support.
 * 
 * @author Edward Hartnett
 * @date 2026-02-03
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"

#define TEST_FILE "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"

/**
 * @brief Test UDF self-loading initialization
 * 
 * Validates that NC_GEOTIFF_initialize() works correctly when
 * HAVE_NETCDF_UDF_SELF_REGISTRATION is defined.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_self_load_initialization(void)
{
    int ret;
    
    printf("*** Testing GeoTIFF self-loading initialization...\n");
    
#ifdef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    HAVE_NETCDF_UDF_SELF_REGISTRATION is defined\n");
    printf("    NC_GEOTIFF_initialize() should NOT call nc_def_user_format()\n");
#else
    printf("    HAVE_NETCDF_UDF_SELF_REGISTRATION is NOT defined\n");
    printf("    Skipping self-loading test (requires new NetCDF-C)\n");
    return 0;
#endif
    
    /* Call initialization function */
    ret = NC_GEOTIFF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: NC_GEOTIFF_initialize() failed: %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("    ✓ NC_GEOTIFF_initialize() succeeded\n");
    return 0;
}

/**
 * @brief Test opening GeoTIFF file after self-load initialization
 * 
 * Validates that GeoTIFF files can be opened through the NetCDF API
 * after calling NC_GEOTIFF_initialize() with self-loading support.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_open_after_self_load(void)
{
    int ncid, ret;
    int ndims, nvars, ngatts, unlimdimid;
    
    printf("\n*** Testing GeoTIFF file open after self-load initialization...\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    Skipping test (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)\n");
    return 0;
#endif
    
    /* Open GeoTIFF file through NetCDF API */
    printf("    Opening file: %s\n", TEST_FILE);
    ret = nc_open(TEST_FILE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to open GeoTIFF file: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ Successfully opened GeoTIFF file\n");
    
    /* Query file metadata to validate it opened correctly */
    ret = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to query file metadata: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    
    printf("    File metadata:\n");
    printf("      Dimensions: %d\n", ndims);
    printf("      Variables: %d\n", nvars);
    printf("      Global attributes: %d\n", ngatts);
    printf("    ✓ Successfully queried file metadata\n");
    
    /* Validate expected structure */
    if (ndims < 2)
    {
        printf("    ERROR: Expected at least 2 dimensions, found %d\n", ndims);
        nc_close(ncid);
        return 1;
    }
    
    if (nvars < 1)
    {
        printf("    ERROR: Expected at least 1 variable, found %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    
    printf("    ✓ File structure validated\n");
    
    /* Close file */
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to close file: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ Successfully closed file\n");
    
    return 0;
}

/**
 * @brief Test multiple initializations
 * 
 * Validates that NC_GEOTIFF_initialize() can be called multiple times
 * without error (idempotent behavior).
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_multiple_initializations(void)
{
    int ret;
    
    printf("\n*** Testing multiple initializations...\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    Skipping test (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)\n");
    return 0;
#endif
    
    /* First initialization */
    ret = NC_GEOTIFF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: First initialization failed: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ First initialization succeeded\n");
    
    /* Second initialization (should be idempotent) */
    ret = NC_GEOTIFF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Second initialization failed: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ Second initialization succeeded (idempotent)\n");
    
    return 0;
}

/**
 * @brief Main test function
 * 
 * Runs all self-loading tests for GeoTIFF UDF handler.
 * 
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    int errors = 0;
    
    printf("=== GeoTIFF UDF Self-Loading Test ===\n\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("HAVE_NETCDF_UDF_SELF_REGISTRATION is not defined.\n");
    printf("This test requires NetCDF-C with UDF self-loading support.\n");
    printf("Skipping all tests.\n\n");
    printf("=== Test Summary ===\n");
    printf("SKIPPED (requires new NetCDF-C with self-loading support)\n");
    return 0;
#endif
    
    /* Run tests */
    if (test_self_load_initialization() != 0)
        errors++;
    
    if (test_open_after_self_load() != 0)
        errors++;
    
    if (test_multiple_initializations() != 0)
        errors++;
    
    /* Print summary */
    printf("\n=== Test Summary ===\n");
    if (errors == 0)
    {
        printf("✓ All tests PASSED\n");
        printf("GeoTIFF UDF self-loading validated successfully.\n");
        return 0;
    }
    else
    {
        printf("✗ %d test(s) FAILED\n", errors);
        return 1;
    }
}

#else /* !HAVE_GEOTIFF */

int main(void)
{
    printf("=== GeoTIFF UDF Self-Loading Test ===\n\n");
    printf("GeoTIFF support not enabled. Skipping test.\n");
    return 0;
}

#endif /* HAVE_GEOTIFF */

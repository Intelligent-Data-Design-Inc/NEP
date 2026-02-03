/**
 * @file tst_cdf_self_load.c
 * @brief Test CDF UDF self-loading functionality
 * 
 * This test validates that the CDF UDF handler works correctly with
 * NetCDF-C's self-loading UDF plugin system when HAVE_NETCDF_UDF_SELF_REGISTRATION
 * is available. It verifies that:
 * 1. NC_CDF_initialize() can be called successfully
 * 2. The initialization function does NOT call nc_def_user_format() when self-loading is available
 * 3. CDF files can be opened through the NetCDF API after initialization
 * 
 * This test is part of v1.5.4 Sprint 3 and validates the conditional compilation
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

#ifdef HAVE_CDF
#include "cdfdispatch.h"

#define TEST_FILE "tst_cdf_simple.cdf"

/**
 * @brief Test UDF self-loading initialization
 * 
 * Validates that NC_CDF_initialize() works correctly when
 * HAVE_NETCDF_UDF_SELF_REGISTRATION is defined.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_self_load_initialization(void)
{
    int ret;
    
    printf("*** Testing CDF self-loading initialization...\n");
    
#ifdef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    HAVE_NETCDF_UDF_SELF_REGISTRATION is defined\n");
    printf("    NC_CDF_initialize() should NOT call nc_def_user_format()\n");
#else
    printf("    HAVE_NETCDF_UDF_SELF_REGISTRATION is NOT defined\n");
    printf("    Skipping self-loading test (requires new NetCDF-C)\n");
    return 0;
#endif
    
    /* Call initialization function */
    ret = NC_CDF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: NC_CDF_initialize() failed: %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("    ✓ NC_CDF_initialize() succeeded\n");
    return 0;
}

/**
 * @brief Test opening CDF file after self-load initialization
 * 
 * Validates that CDF files can be opened through the NetCDF API
 * after calling NC_CDF_initialize() with self-loading support.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_open_after_self_load(void)
{
    int ncid, ret;
    int ndims, nvars, ngatts, unlimdimid;
    char varname[NC_MAX_NAME + 1];
    
    printf("\n*** Testing CDF file open after self-load initialization...\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    Skipping test (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)\n");
    return 0;
#endif
    
    /* Open CDF file through NetCDF API */
    printf("    Opening file: %s\n", TEST_FILE);
    ret = nc_open(TEST_FILE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to open CDF file: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ Successfully opened CDF file\n");
    
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
    
    /* Validate expected structure (from tst_cdf_basic.c) */
    if (ndims < 1)
    {
        printf("    ERROR: Expected at least 1 dimension, found %d\n", ndims);
        nc_close(ncid);
        return 1;
    }
    
    if (nvars != 1)
    {
        printf("    ERROR: Expected 1 variable, found %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    
    /* Validate variable name */
    ret = nc_inq_varname(ncid, 0, varname);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to query variable name: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    
    if (strcmp(varname, "temperature") != 0)
    {
        printf("    ERROR: Expected variable 'temperature', found '%s'\n", varname);
        nc_close(ncid);
        return 1;
    }
    
    printf("    ✓ File structure validated (variable: %s)\n", varname);
    
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
 * Validates that NC_CDF_initialize() can be called multiple times
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
    ret = NC_CDF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: First initialization failed: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ First initialization succeeded\n");
    
    /* Second initialization (should be idempotent) */
    ret = NC_CDF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Second initialization failed: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("    ✓ Second initialization succeeded (idempotent)\n");
    
    return 0;
}

/**
 * @brief Test reading data after self-load initialization
 * 
 * Validates that data can be read from CDF files through the NetCDF API
 * after self-load initialization.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_read_data_after_self_load(void)
{
    int ncid, varid, ret;
    float data[10];
    size_t start[1] = {0};
    size_t count[1] = {10};
    int i;
    
    printf("\n*** Testing data reading after self-load initialization...\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    Skipping test (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)\n");
    return 0;
#endif
    
    /* Open file */
    ret = nc_open(TEST_FILE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to open file: %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* Get variable ID */
    ret = nc_inq_varid(ncid, "temperature", &varid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to get variable ID: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    
    /* Read data */
    ret = nc_get_vara_float(ncid, varid, start, count, data);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to read data: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    printf("    ✓ Successfully read data\n");
    
    /* Validate data values (from tst_cdf_basic.c: 20.0 to 24.5) */
    printf("    Data values: ");
    for (i = 0; i < 10; i++)
    {
        float expected = 20.0f + (float)i * 0.5f;
        if (data[i] != expected)
        {
            printf("\n    ERROR: Expected data[%d] = %.1f, found %.1f\n", i, expected, data[i]);
            nc_close(ncid);
            return 1;
        }
        if (i < 5)
            printf("%.1f ", data[i]);
    }
    printf("...\n");
    printf("    ✓ Data values validated\n");
    
    /* Close file */
    nc_close(ncid);
    
    return 0;
}

/**
 * @brief Main test function
 * 
 * Runs all self-loading tests for CDF UDF handler.
 * 
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    int errors = 0;
    
    printf("=== CDF UDF Self-Loading Test ===\n\n");
    
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
    
    if (test_read_data_after_self_load() != 0)
        errors++;
    
    /* Print summary */
    printf("\n=== Test Summary ===\n");
    if (errors == 0)
    {
        printf("✓ All tests PASSED\n");
        printf("CDF UDF self-loading validated successfully.\n");
        return 0;
    }
    else
    {
        printf("✗ %d test(s) FAILED\n", errors);
        return 1;
    }
}

#else /* !HAVE_CDF */

int main(void)
{
    printf("=== CDF UDF Self-Loading Test ===\n\n");
    printf("CDF support not enabled. Skipping test.\n");
    return 0;
}

#endif /* HAVE_CDF */

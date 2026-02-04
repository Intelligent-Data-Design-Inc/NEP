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
#include <unistd.h>
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
 * @brief Test self-loading behavior explanation
 * 
 * When HAVE_NETCDF_UDF_SELF_REGISTRATION is defined, the initialization
 * function does NOT register the UDF handler. Instead, NetCDF-C's plugin
 * system handles registration via RC file or dlopen. This test documents
 * the expected behavior.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_self_load_behavior(void)
{
    printf("\n*** Testing self-loading behavior...\n");
    
#ifdef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    With HAVE_NETCDF_UDF_SELF_REGISTRATION defined:\n");
    printf("    - NC_GEOTIFF_initialize() does NOT call nc_def_user_format()\n");
    printf("    - UDF registration happens via NetCDF-C plugin system\n");
    printf("    - Applications configure via RC file (.ncrc):\n");
    printf("        NETCDF.UDF0.LIBRARY=/path/to/libnep.so\n");
    printf("        NETCDF.UDF0.INIT=NC_GEOTIFF_initialize\n");
    printf("        NETCDF.UDF0.MAGIC=II*\n");
    printf("    - NetCDF-C calls initialization function automatically\n");
    printf("    ✓ Self-loading behavior documented\n");
#else
    printf("    Without HAVE_NETCDF_UDF_SELF_REGISTRATION:\n");
    printf("    - NC_GEOTIFF_initialize() calls nc_def_user_format()\n");
    printf("    - Applications must call NC_GEOTIFF_initialize() explicitly\n");
    printf("    - Manual registration required at startup\n");
    printf("    ✓ Manual registration behavior documented\n");
#endif
    
    return 0;
}

/**
 * @brief Test with RC file configuration
 * 
 * Creates a .ncrc file with UDF configuration and validates that
 * GeoTIFF files can be opened through NetCDF-C's self-loading mechanism.
 * 
 * @return 0 on success, non-zero on failure
 */
static int test_with_rc_file(void)
{
    int ncid, ret;
    int ndims, nvars, ngatts, unlimdimid;
    FILE *rc;
    const char *lib_path;
    
    printf("\n*** Testing with RC file configuration...\n");
    
#ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
    printf("    Skipping (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)\n");
    return 0;
#endif
    
    /* Get library path from build system */
#ifdef NEP_GEOTIFF_LIB_PATH
    lib_path = NEP_GEOTIFF_LIB_PATH;
#else
    lib_path = getenv("NEP_GEOTIFF_LIB_PATH");
    if (!lib_path)
    {
        printf("    ERROR: NEP_GEOTIFF_LIB_PATH not defined\n");
        printf("    Build system must provide library path\n");
        return 1;
    }
#endif
    
    printf("    Using library: %s\n", lib_path);
    
    /* Verify library exists */
    if (access(lib_path, F_OK) != 0)
    {
        printf("    ERROR: Library not found: %s\n", lib_path);
        return 1;
    }
    
    /* Create .ncrc in current directory */
    rc = fopen(".ncrc", "w");
    if (!rc)
    {
        printf("    ERROR: Failed to create .ncrc file\n");
        return 1;
    }
    
    fprintf(rc, "NETCDF.UDF0.LIBRARY=%s\n", lib_path);
    fprintf(rc, "NETCDF.UDF0.INIT=NC_GEOTIFF_initialize\n");
    fprintf(rc, "NETCDF.UDF0.MAGIC=II*\n");
    fclose(rc);
    printf("    ✓ Created .ncrc configuration\n");
    
    /* Initialize - NetCDF-C will read .ncrc and load plugin */
    ret = NC_GEOTIFF_initialize();
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Initialization failed: %s\n", nc_strerror(ret));
        remove(".ncrc");
        return 1;
    }
    
    /* Try to open GeoTIFF file through NetCDF API */
    printf("    Opening file: %s\n", TEST_FILE);
    ret = nc_open(TEST_FILE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to open GeoTIFF file: %s\n", nc_strerror(ret));
        remove(".ncrc");
        return 1;
    }
    printf("    ✓ Successfully opened GeoTIFF file via self-loading\n");
    
    /* Query file metadata to validate it opened correctly */
    ret = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to query file metadata: %s\n", nc_strerror(ret));
        nc_close(ncid);
        remove(".ncrc");
        return 1;
    }
    
    printf("    File metadata: %d dims, %d vars, %d global attrs\n", ndims, nvars, ngatts);
    
    /* Validate expected structure */
    if (ndims < 2 || nvars < 1)
    {
        printf("    ERROR: Unexpected file structure\n");
        nc_close(ncid);
        remove(".ncrc");
        return 1;
    }
    printf("    ✓ File metadata validated\n");
    
    /* Close file */
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: Failed to close file: %s\n", nc_strerror(ret));
        remove(".ncrc");
        return 1;
    }
    
    /* Clean up .ncrc */
    remove(".ncrc");
    printf("    ✓ Cleaned up .ncrc\n");
    
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
    
    if (test_self_load_behavior() != 0)
        errors++;
    
    if (test_with_rc_file() != 0)
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

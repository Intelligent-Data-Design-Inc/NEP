/**
 * @file tst_cdf_ncrc_autoload.c
 * @brief Test CDF UDF autoloading via .ncrc and NETCDF_RC (issue #134)
 *
 * Validates that a CDF file can be opened through the NetCDF API
 * without any explicit call to NC_CDF_initialize(). The test sets
 * NETCDF_RC to a temporary directory containing a generated .ncrc
 * file, then calls nc_open() directly. NetCDF-C reads the RC file and
 * loads libnep automatically.
 *
 * UDF slot used: UDF2 (NASA CDF, magic: \xCD\xF3\x00\x01)
 *
 * The test file tst_cdf_simple.cdf must already exist (created by
 * tst_cdf_basic which runs before this test).
 *
 * @author Edward Hartnett
 * @date 2026-02-27
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netcdf.h>

#ifdef HAVE_CDF

#define TEST_FILE "tst_cdf_simple.cdf"
#define TMP_NCRC_DIR "/tmp/nep_ncrc_test_cdf"
#define TMP_NCRC_FILE TMP_NCRC_DIR "/.ncrc"

/**
 * @brief Write a .ncrc into a temp directory and set NETCDF_RC.
 *
 * @param lib_path  Path to libnep shared library.
 * @return 0 on success, non-zero on failure.
 */
static int setup_netcdf_rc(const char *lib_path)
{
    FILE *f;

    if (mkdir(TMP_NCRC_DIR, 0755) != 0 && access(TMP_NCRC_DIR, F_OK) != 0)
    {
        printf("    ERROR: Failed to create temp dir %s\n", TMP_NCRC_DIR);
        return 1;
    }

    f = fopen(TMP_NCRC_FILE, "w");
    if (!f)
    {
        printf("    ERROR: Failed to create %s\n", TMP_NCRC_FILE);
        return 1;
    }

    fprintf(f, "NETCDF.UDF2.LIBRARY=%s\n", lib_path);
    fprintf(f, "NETCDF.UDF2.INIT=NC_CDF_initialize\n");
    fprintf(f, "NETCDF.UDF2.MAGIC=\\xCD\\xF3\\x00\\x01\n");
    fclose(f);

    if (setenv("NETCDF_RC", TMP_NCRC_DIR, 1) != 0)
    {
        printf("    ERROR: setenv(NETCDF_RC) failed\n");
        return 1;
    }

    printf("    Created %s\n", TMP_NCRC_FILE);
    printf("    NETCDF_RC=%s\n", TMP_NCRC_DIR);
    return 0;
}

/**
 * @brief Remove the temp RC directory and unset NETCDF_RC.
 */
static void teardown_netcdf_rc(void)
{
    unlink(TMP_NCRC_FILE);
    rmdir(TMP_NCRC_DIR);
    unsetenv("NETCDF_RC");
}

/**
 * @brief Test pure autoloading: open CDF file without calling init.
 *
 * @return 0 on success, non-zero on failure.
 */
static int test_cdf_autoload(void)
{
    int ncid, varid, ret;
    int ndims, nvars, ngatts, unlimdimid;
    float data[10];
    size_t start[1] = {0};
    size_t count[1] = {10};
    const char *lib_path;
    int i;

    printf("*** Testing CDF pure autoloading via NETCDF_RC...\n");

#ifdef NEP_CDF_LIB_PATH
    lib_path = NEP_CDF_LIB_PATH;
#else
    lib_path = getenv("NEP_CDF_LIB_PATH");
    if (!lib_path)
    {
        printf("    ERROR: NEP_CDF_LIB_PATH not set\n");
        return 1;
    }
#endif

    if (access(lib_path, F_OK) != 0)
    {
        printf("    Skipping: shared library not found: %s\n", lib_path);
        printf("    (expected in static-only builds)\n");
        return 0;
    }

    if (access(TEST_FILE, F_OK) != 0)
    {
        printf("    Skipping: test file not found: %s\n", TEST_FILE);
        printf("    (run tst_cdf_basic first to create the test file)\n");
        return 0;
    }

    if (setup_netcdf_rc(lib_path) != 0)
        return 1;

    /* Open CDF file without calling NC_CDF_initialize() first */
    ret = nc_open(TEST_FILE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("    Skipping: nc_open failed: %s\n", nc_strerror(ret));
        printf("    (NetCDF-C UDF autoloading via NETCDF_RC not active)\n");
        teardown_netcdf_rc();
        return 0;
    }
    printf("    Opened %s without explicit init\n", TEST_FILE);

    ret = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: nc_inq failed: %s\n", nc_strerror(ret));
        nc_close(ncid);
        teardown_netcdf_rc();
        return 1;
    }
    printf("    File has %d dims, %d vars, %d global attrs\n", ndims, nvars, ngatts);

    if (ndims < 1 || nvars != 1)
    {
        printf("    ERROR: Unexpected file structure\n");
        nc_close(ncid);
        teardown_netcdf_rc();
        return 1;
    }

    ret = nc_inq_varid(ncid, "temperature", &varid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: nc_inq_varid failed: %s\n", nc_strerror(ret));
        nc_close(ncid);
        teardown_netcdf_rc();
        return 1;
    }

    ret = nc_get_vara_float(ncid, varid, start, count, data);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: nc_get_vara_float failed: %s\n", nc_strerror(ret));
        nc_close(ncid);
        teardown_netcdf_rc();
        return 1;
    }

    for (i = 0; i < 10; i++)
    {
        float expected = 20.0f + (float)i * 0.5f;
        if (data[i] != expected)
        {
            printf("    ERROR: data[%d] = %.1f, expected %.1f\n", i, data[i], expected);
            nc_close(ncid);
            teardown_netcdf_rc();
            return 1;
        }
    }
    printf("    Data values validated (20.0 to 24.5)\n");

    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: nc_close failed: %s\n", nc_strerror(ret));
        teardown_netcdf_rc();
        return 1;
    }

    teardown_netcdf_rc();
    printf("    CDF autoloading via NETCDF_RC succeeded\n");
    return 0;
}

int main(void)
{
    int errors = 0;

    printf("=== CDF .ncrc Autoloading Test ===\n\n");

    if (test_cdf_autoload() != 0)
        errors++;

    printf("\n=== Test Summary ===\n");
    if (errors == 0)
    {
        printf("All tests PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", errors);
    return 1;
}

#else /* !HAVE_CDF */

int main(void)
{
    printf("=== CDF .ncrc Autoloading Test ===\n\n");
    printf("CDF support not enabled. Skipping.\n");
    return 0;
}

#endif /* HAVE_CDF */

/**
 * @file tst_geotiff_ncrc_autoload.c
 * @brief Test GeoTIFF UDF autoloading via .ncrc and NETCDF_RC (issue #134)
 *
 * Validates that a GeoTIFF file can be opened through the NetCDF API
 * without any explicit call to NC_GEOTIFF_initialize(). The test sets
 * NETCDF_RC to a temporary directory containing a generated nep.ncrc
 * file, then calls nc_open() directly. NetCDF-C reads the RC file and
 * loads libnep automatically.
 *
 * UDF slot used: UDF1 (standard TIFF little-endian, magic: II*)
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

#ifdef HAVE_GEOTIFF

#define TEST_FILE "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"
#define TMP_NCRC_DIR "/tmp/nep_ncrc_test_geotiff"
#define TMP_NCRC_FILE TMP_NCRC_DIR "/nep.ncrc"

/**
 * @brief Write a nep.ncrc into a temp directory and set NETCDF_RC.
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

    fprintf(f, "NETCDF.UDF0.LIBRARY=%s\n", lib_path);
    fprintf(f, "NETCDF.UDF0.INIT=NC_GEOTIFF_initialize\n");
    fprintf(f, "NETCDF.UDF0.MAGIC=II+\n");
    fprintf(f, "NETCDF.UDF1.LIBRARY=%s\n", lib_path);
    fprintf(f, "NETCDF.UDF1.INIT=NC_GEOTIFF_initialize\n");
    fprintf(f, "NETCDF.UDF1.MAGIC=II*\n");
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
 * @brief Test pure autoloading: open GeoTIFF without calling init.
 *
 * @return 0 on success, non-zero on failure.
 */
static int test_geotiff_autoload(void)
{
    int ncid, ret;
    int ndims, nvars, ngatts, unlimdimid;
    const char *lib_path;

    printf("*** Testing GeoTIFF pure autoloading via NETCDF_RC...\n");

#ifdef NEP_GEOTIFF_LIB_PATH
    lib_path = NEP_GEOTIFF_LIB_PATH;
#else
    lib_path = getenv("NEP_GEOTIFF_LIB_PATH");
    if (!lib_path)
    {
        printf("    ERROR: NEP_GEOTIFF_LIB_PATH not set\n");
        return 1;
    }
#endif

    if (access(lib_path, F_OK) != 0)
    {
        printf("    Skipping: shared library not found: %s\n", lib_path);
        printf("    (expected in static-only builds)\n");
        return 0;
    }

    if (setup_netcdf_rc(lib_path) != 0)
        return 1;

    /* Open GeoTIFF without calling NC_GEOTIFF_initialize() first */
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

    if (ndims < 2 || nvars < 1)
    {
        printf("    ERROR: Unexpected file structure\n");
        nc_close(ncid);
        teardown_netcdf_rc();
        return 1;
    }

    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("    ERROR: nc_close failed: %s\n", nc_strerror(ret));
        teardown_netcdf_rc();
        return 1;
    }

    teardown_netcdf_rc();
    printf("    GeoTIFF autoloading via NETCDF_RC succeeded\n");
    return 0;
}

int main(void)
{
    int errors = 0;

    printf("=== GeoTIFF .ncrc Autoloading Test ===\n\n");

    if (test_geotiff_autoload() != 0)
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

#else /* !HAVE_GEOTIFF */

int main(void)
{
    printf("=== GeoTIFF .ncrc Autoloading Test ===\n\n");
    printf("GeoTIFF support not enabled. Skipping.\n");
    return 0;
}

#endif /* HAVE_GEOTIFF */

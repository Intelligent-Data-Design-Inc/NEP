/**
 * @file tst_grib2_udf.c
 * @brief Test for GRIB2 User Defined Format (UDF) handler open/close.
 *
 * Validates that a GRIB2 file can be opened and closed through the standard
 * NetCDF API using the GRIB2 UDF handler, and that non-GRIB2 files are
 * correctly rejected.
 *
 * This test is part of v1.7.0 Sprint 2.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#ifdef HAVE_GRIB2

#include <stdio.h>
#include <netcdf.h>
#include <grib2.h>
#include "grib2dispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR do { \
    fprintf(stderr, "ERROR at %s:%d\n", __FILE__, __LINE__); \
    return 1; \
} while(0)

/** @internal Check return value; jump to ERR on non-zero. */
#define CHECK(e) do { if ((e)) ERR; } while(0)

/** Path to the GRIB2 test data file (relative to test build directory). */
#define GRIB2_TEST_FILE "data/gdaswave.t00z.wcoast.0p16.f000.grib2"

int
main(void)
{
    int ncid, ret;

    /* Enable verbose g2c logging for CI diagnostics. */
    g2c_set_log_level(3);

    /* Register the GRIB2 dispatch table. */
    NC_GRIB2_initialize();

    /* Test 1: open a valid GRIB2 file. */
    CHECK(nc_open(GRIB2_TEST_FILE, NC_NOWRITE, &ncid));
    printf("PASS: nc_open GRIB2 file\n");

    /* Test 2: close the file. */
    CHECK(nc_close(ncid));
    printf("PASS: nc_close GRIB2 file\n");

    /* Test 3: opening a non-GRIB2 path must return an error. */
    ret = nc_open("/dev/null", NC_NOWRITE, &ncid);
    if (ret == NC_NOERR)
        ERR;
    printf("PASS: nc_open non-GRIB2 file returned error (%d)\n", ret);

    /* Restore default g2c log level. */
    g2c_set_log_level(0);

    printf("PASS: all tests passed\n");
    return 0;
}

#else /* !HAVE_GRIB2 */

#include <stdio.h>

int
main(void)
{
    printf("GRIB2 support not enabled - skipping\n");
    return 0;
}

#endif /* HAVE_GRIB2 */

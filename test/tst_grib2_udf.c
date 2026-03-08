/**
 * @file tst_grib2_udf.c
 * @brief Test for GRIB2 User Defined Format (UDF) handler.
 *
 * Validates that a GRIB2 file can be opened and closed through the standard
 * NetCDF API using the GRIB2 UDF handler, that non-GRIB2 files are correctly
 * rejected, and that the NetCDF-4 metadata model (dimensions, variables,
 * attributes) is correctly populated from the GRIB2 product inventory.
 *
 * This test covers v1.7.0 Sprint 2 (open/close) and Sprint 3 (metadata).
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

/** Expected values derived from gdaswave.t00z.wcoast.0p16.f000.grib2. */
#define EXPECTED_NUM_PRODUCTS 19   /**< 19 messages x 1 product each */
#define EXPECTED_NX           151  /**< Grid size in X (longitude) */
#define EXPECTED_NY           241  /**< Grid size in Y (latitude) */
/** Variable 0 is WIND: discipline=0, category=2, param=1. */
#define EXPECTED_VAR0_DISC    0
#define EXPECTED_VAR0_CAT     2
#define EXPECTED_VAR0_PARAM   1
#define EXPECTED_VAR0_NAME    "WIND"

int
main(void)
{
    int ncid, ret;
    int ndims, nvars, natts, unlimdim;
    int dimid_x, dimid_y;
    size_t len_x, len_y;
    nc_type xtype;
    int disc, cat, pnum;
    char varname[NC_MAX_NAME + 1];
    char conventions[32];
    int edition;

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

    /* Re-open for metadata tests. */
    CHECK(nc_open(GRIB2_TEST_FILE, NC_NOWRITE, &ncid));

    /* Test 4: dimension and variable counts. */
    CHECK(nc_inq(ncid, &ndims, &nvars, &natts, &unlimdim));
    if (ndims != 2) ERR;
    if (nvars != EXPECTED_NUM_PRODUCTS) ERR;
    printf("PASS: nc_inq ndims=%d nvars=%d\n", ndims, nvars);

    /* Test 5: dimension sizes. */
    CHECK(nc_inq_dimid(ncid, "x", &dimid_x));
    CHECK(nc_inq_dimid(ncid, "y", &dimid_y));
    CHECK(nc_inq_dimlen(ncid, dimid_x, &len_x));
    CHECK(nc_inq_dimlen(ncid, dimid_y, &len_y));
    if (len_x != EXPECTED_NX) ERR;
    if (len_y != EXPECTED_NY) ERR;
    printf("PASS: dimensions x=%zu y=%zu\n", len_x, len_y);

    /* Test 6: first variable type and name. */
    CHECK(nc_inq_vartype(ncid, 0, &xtype));
    if (xtype != NC_FLOAT) ERR;
    CHECK(nc_inq_varname(ncid, 0, varname));
    if (strncmp(varname, EXPECTED_VAR0_NAME,
                sizeof(EXPECTED_VAR0_NAME) - 1) != 0) ERR;
    printf("PASS: variable 0 name=%s type=NC_FLOAT\n", varname);

    /* Test 7: GRIB2 integer attributes on variable 0. */
    CHECK(nc_get_att_int(ncid, 0, "GRIB2_discipline", &disc));
    CHECK(nc_get_att_int(ncid, 0, "GRIB2_category", &cat));
    CHECK(nc_get_att_int(ncid, 0, "GRIB2_param_number", &pnum));
    if (disc != EXPECTED_VAR0_DISC) ERR;
    if (cat != EXPECTED_VAR0_CAT) ERR;
    if (pnum != EXPECTED_VAR0_PARAM) ERR;
    printf("PASS: var 0 discipline=%d category=%d param=%d\n",
           disc, cat, pnum);

    /* Test 8: global attributes. */
    CHECK(nc_get_att_text(ncid, NC_GLOBAL, "Conventions", conventions));
    if (strncmp(conventions, "GRIB2", 5) != 0) ERR;
    CHECK(nc_get_att_int(ncid, NC_GLOBAL, "GRIB2_edition", &edition));
    if (edition != 2) ERR;
    printf("PASS: global Conventions=GRIB2 edition=%d\n", edition);

    CHECK(nc_close(ncid));

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

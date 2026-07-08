/**
 * @file tst_pds4_udf.c
 * @brief Test for PDS4 User Defined Format (UDF) handler.
 *
 * Sprint 4: validates that the PDS4 XML label is converted into the
 * netCDF-4 metadata model. The test opens a real PDS4 XML label and checks
 * global attributes, the file-area group, dimensions, and the array variable.
 * Data reading is still expected to fail/return NC_EINVAL.
 *
 * Test file: test/data/PDS4/test_image.xml
 *   A minimal PDS4 Product_Observational label referencing a 4x4 float32 image.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_PDS4

#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include "pds4dispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the PDS4 test data file (relative to test build directory). */
#define PDS4_TEST_FILE "data/PDS4/test_image.xml"
#define PDS4_TABLE_BINARY_FILE "data/PDS4/test_table_binary.xml"
#define PDS4_TABLE_CHAR_FILE "data/PDS4/Table_Character_Example.xml"

/**
 * @internal Test Sprint 5 binary table metadata.
 *
 * Opens the test_table_binary.xml label and verifies:
 * - A child group named after the data file exists.
 * - A "record" dimension of length 5.
 * - Three variables: Timestamp (NC_DOUBLE), Detector_Counts (NC_INT), Temperature (NC_FLOAT).
 * - Units attributes: "s", "DN", "K".
 */
static int
test_table_binary(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    int dimids[NC_MAX_DIMS];
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    char units[64] = {0};

    printf("\n--- Sprint 5: Table_Binary metadata tests ---\n");

    if ((retval = nc_open(PDS4_TABLE_BINARY_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open(%s)\n", PDS4_TABLE_BINARY_FILE);

    /* Find the file-area child group. */
    if ((retval = nc_inq_ncid(ncid, "test_table_binary.dat", &grp_ncid)))
        ERR(retval);
    printf("PASS: found group 'test_table_binary.dat'\n");

    /* Check group metadata: 1 dimension (record), 3 variables. */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 1)
    {
        fprintf(stderr, "Expected 1 dimension, got %d\n", ndims);
        return 1;
    }
    if (nvars != 3)
    {
        fprintf(stderr, "Expected 3 variables, got %d\n", nvars);
        return 1;
    }
    printf("PASS: group metadata: ndims=%d nvars=%d\n", ndims, nvars);

    /* Verify record dimension has length 5. */
    if ((retval = nc_inq_dimids(grp_ncid, &ndims, dimids, 0)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], name, &len)))
        ERR(retval);
    if (strcmp(name, "record") != 0 || len != 5)
    {
        fprintf(stderr, "Unexpected dim: name=%s len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim 'record' len=%zu\n", len);

    /* Verify Timestamp variable: NC_DOUBLE, units=s */
    if ((retval = nc_inq_varid(grp_ncid, "Timestamp", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE || ndims != 1)
    {
        fprintf(stderr, "Unexpected Timestamp: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", units)))
        ERR(retval);
    if (strcmp(units, "s") != 0)
    {
        fprintf(stderr, "Unexpected Timestamp units: '%s'\n", units);
        return 1;
    }
    printf("PASS: Timestamp is NC_DOUBLE, units='%s'\n", units);

    /* Verify Detector_Counts variable: NC_INT, units=DN */
    if ((retval = nc_inq_varid(grp_ncid, "Detector_Counts", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_INT || ndims != 1)
    {
        fprintf(stderr, "Unexpected Detector_Counts: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    memset(units, 0, sizeof(units));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", units)))
        ERR(retval);
    if (strcmp(units, "DN") != 0)
    {
        fprintf(stderr, "Unexpected Detector_Counts units: '%s'\n", units);
        return 1;
    }
    printf("PASS: Detector_Counts is NC_INT, units='%s'\n", units);

    /* Verify Temperature variable: NC_FLOAT, units=K */
    if ((retval = nc_inq_varid(grp_ncid, "Temperature", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_FLOAT || ndims != 1)
    {
        fprintf(stderr, "Unexpected Temperature: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    memset(units, 0, sizeof(units));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", units)))
        ERR(retval);
    if (strcmp(units, "K") != 0)
    {
        fprintf(stderr, "Unexpected Temperature units: '%s'\n", units);
        return 1;
    }
    printf("PASS: Temperature is NC_FLOAT, units='%s'\n", units);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Table_Binary metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Sprint 5 character table metadata.
 *
 * Opens Table_Character_Example.xml and verifies:
 * - A child group named after the data file exists.
 * - A "record" dimension of length 224.
 * - Three variables: Wavelength (NC_DOUBLE), Reflectance (NC_DOUBLE), Error (NC_DOUBLE).
 * - Units attribute "nm" on Wavelength.
 */
static int
test_table_character(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    int dimids[NC_MAX_DIMS];
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    char units[64] = {0};

    printf("\n--- Sprint 5: Table_Character metadata tests ---\n");

    if ((retval = nc_open(PDS4_TABLE_CHAR_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open(%s)\n", PDS4_TABLE_CHAR_FILE);

    /* Find the file-area child group. */
    if ((retval = nc_inq_ncid(ncid, "Table_Character_Example.tab", &grp_ncid)))
        ERR(retval);
    printf("PASS: found group 'Table_Character_Example.tab'\n");

    /* Check group metadata: 1 dimension (record), 3 variables. */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 1)
    {
        fprintf(stderr, "Expected 1 dimension, got %d\n", ndims);
        return 1;
    }
    if (nvars != 3)
    {
        fprintf(stderr, "Expected 3 variables, got %d\n", nvars);
        return 1;
    }
    printf("PASS: group metadata: ndims=%d nvars=%d\n", ndims, nvars);

    /* Verify record dimension has length 224. */
    if ((retval = nc_inq_dimids(grp_ncid, &ndims, dimids, 0)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], name, &len)))
        ERR(retval);
    if (strcmp(name, "record") != 0 || len != 224)
    {
        fprintf(stderr, "Unexpected dim: name=%s len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim 'record' len=%zu\n", len);

    /* Verify Wavelength variable: NC_DOUBLE, units=nm */
    if ((retval = nc_inq_varid(grp_ncid, "Wavelength", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE || ndims != 1)
    {
        fprintf(stderr, "Unexpected Wavelength: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", units)))
        ERR(retval);
    if (strcmp(units, "nm") != 0)
    {
        fprintf(stderr, "Unexpected Wavelength units: '%s'\n", units);
        return 1;
    }
    printf("PASS: Wavelength is NC_DOUBLE, units='%s'\n", units);

    /* Verify Reflectance variable: NC_DOUBLE, no units. */
    if ((retval = nc_inq_varid(grp_ncid, "Reflectance", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE || ndims != 1)
    {
        fprintf(stderr, "Unexpected Reflectance: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    printf("PASS: Reflectance is NC_DOUBLE\n");

    /* Verify Error variable: NC_DOUBLE. */
    if ((retval = nc_inq_varid(grp_ncid, "Error", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE || ndims != 1)
    {
        fprintf(stderr, "Unexpected Error: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }
    printf("PASS: Error is NC_DOUBLE\n");

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Table_Character metadata tests PASSED.\n");
    return 0;
}

int
main(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid, ngrps;
    int dimids[NC_MAX_DIMS];
    int grpids[1];
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    char title[256] = {0};
    char lid[256] = {0};

    /* Register the PDS4 UDF handler.
     * Accept NC_EINVAL in case it was already registered by a previous test. */
    if ((retval = NC_PDS4_initialize()) && retval != NC_EINVAL)
        ERR(retval);
    printf("PASS: NC_PDS4_initialize\n");

    /* Open the PDS4 label file read-only. */
    if ((retval = nc_open(PDS4_TEST_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open\n");

    /* Verify the ncid is valid (non-negative). */
    if (ncid < 0)
    {
        fprintf(stderr, "Expected valid ncid >= 0, got %d\n", ncid);
        return 1;
    }
    printf("PASS: ncid=%d is valid\n", ncid);

    /* Check root group metadata. The root group should have no dimensions or
     * variables of its own, but should have global attributes from the label. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 0 || nvars != 0 || unlimdimid != -1)
    {
        fprintf(stderr, "Unexpected root metadata: ndims=%d nvars=%d unlimdimid=%d\n",
                ndims, nvars, unlimdimid);
        return 1;
    }
    if (natts < 2)
    {
        fprintf(stderr, "Expected at least 2 global attributes, got %d\n", natts);
        return 1;
    }
    printf("PASS: root group metadata: ndims=%d nvars=%d natts=%d unlimdimid=%d\n",
           ndims, nvars, natts, unlimdimid);

    /* Check global attributes from Identification_Area. */
    if ((retval = nc_get_att_text(ncid, NC_GLOBAL, "title", title)))
        ERR(retval);
    title[255] = '\0';
    if (strcmp(title, "NEP PDS4 UDF Test Image") != 0)
    {
        fprintf(stderr, "Unexpected title attribute: %s\n", title);
        return 1;
    }
    printf("PASS: global attribute title='%s'\n", title);

    if ((retval = nc_get_att_text(ncid, NC_GLOBAL, "logical_identifier", lid)))
        ERR(retval);
    lid[255] = '\0';
    if (strstr(lid, "nep_test") == NULL)
    {
        fprintf(stderr, "Unexpected logical_identifier attribute: %s\n", lid);
        return 1;
    }
    printf("PASS: global attribute logical_identifier='%s'\n", lid);

    /* Check that there is exactly one child group. */
    if ((retval = nc_inq_grps(ncid, &ngrps, grpids)))
        ERR(retval);
    if (ngrps != 1)
    {
        fprintf(stderr, "Expected 1 child group, got %d\n", ngrps);
        return 1;
    }
    if ((retval = nc_inq_grpname(grpids[0], name)))
        ERR(retval);
    if (strcmp(name, "test_image.img") != 0)
    {
        fprintf(stderr, "Unexpected child group name: %s\n", name);
        return 1;
    }
    printf("PASS: found 1 child group '%s'\n", name);

    /* Find the file-area child group by name. */
    if ((retval = nc_inq_ncid(ncid, "test_image.img", &grp_ncid)))
        ERR(retval);
    printf("PASS: group ncid=%d\n", grp_ncid);

    /* Check group metadata: two dimensions and one variable. */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 2 || nvars != 1 || unlimdimid != -1)
    {
        fprintf(stderr, "Unexpected group metadata: ndims=%d nvars=%d unlimdimid=%d\n",
                ndims, nvars, unlimdimid);
        return 1;
    }
    printf("PASS: group metadata: ndims=%d nvars=%d natts=%d unlimdimid=%d\n",
           ndims, nvars, natts, unlimdimid);

    /* Verify the two dimensions. */
    if ((retval = nc_inq_dimids(grp_ncid, &ndims, dimids, 0)))
        ERR(retval);
    if (ndims != 2)
    {
        fprintf(stderr, "Expected 2 dimension IDs, got %d\n", ndims);
        return 1;
    }

    if ((retval = nc_inq_dim(grp_ncid, dimids[0], name, &len)))
        ERR(retval);
    if (strcmp(name, "Line") != 0 || len != 4)
    {
        fprintf(stderr, "Unexpected dim 0: name=%s len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim 0: %s=%zu\n", name, len);

    if ((retval = nc_inq_dim(grp_ncid, dimids[1], name, &len)))
        ERR(retval);
    if (strcmp(name, "Sample") != 0 || len != 4)
    {
        fprintf(stderr, "Unexpected dim 1: name=%s len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim 1: %s=%zu\n", name, len);

    /* Verify the array variable. */
    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (strcmp(name, "data") != 0 || xtype != NC_FLOAT || ndims != 2)
    {
        fprintf(stderr, "Unexpected variable: name=%s xtype=%d ndims=%d\n",
                name, xtype, ndims);
        return 1;
    }
    printf("PASS: variable '%s' is NC_FLOAT with %d dimensions\n", name, ndims);

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close\n");

    printf("All PDS4 UDF Sprint 4 tests PASSED.\n");

    /* Sprint 5: Table metadata tests. */
    if (test_table_binary())
        return 1;
    if (test_table_character())
        return 1;

    printf("\nAll PDS4 UDF Sprint 4 + Sprint 5 tests PASSED.\n");
    return 0;
}

#else

#include <stdio.h>
int
main(void)
{
    printf("SKIP: PDS4 support not compiled in (HAVE_PDS4 not defined).\n");
    return 0;
}

#endif /* HAVE_PDS4 */

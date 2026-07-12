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
#include <math.h>
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
#define PDS4_CASSINI_HRD_FILE "data/PDS4/cassini_hrd/hrd_2000_on_off.xml"
#define PDS4_MESSENGER_TNMAP_FILE "data/PDS4/messenger_tnmap/thermal_neutron_map.xml"
#define PDS4_LCS_9P_FILE "data/PDS4/lcs_9p/20050706_000.xml"
#define PDS4_MAVEN_L1B_FILE "data/PDS4/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml"
#define PDS4_MAVEN_L3_FILE "data/PDS4/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml"
#define PDS4_MAVEN_IUVS_FILE "data/PDS4/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml"

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

/**
 * @internal Test Sprint 6 array data reading.
 *
 * Opens test_image.xml and reads the 4x4 float array.
 * Known values: row-major 0.0, 1.0, 2.0, ..., 15.0
 */
static int
test_array_data_read(void)
{
    int ncid, grp_ncid, varid, retval;
    float data[16];
    size_t start[2] = {0, 0};
    size_t count[2] = {4, 4};
    int i;

    printf("\n--- Sprint 6: Array data reading tests ---\n");

    if ((retval = nc_open(PDS4_TEST_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "test_image.img", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);

    /* Read the full 4x4 array. */
    if ((retval = nc_get_vara_float(grp_ncid, varid, start, count, data)))
        ERR(retval);

    /* Verify all 16 values: 0.0, 1.0, ..., 15.0 */
    for (i = 0; i < 16; i++)
    {
        if (fabs(data[i] - (float)i) > 1e-6f)
        {
            fprintf(stderr, "Array data[%d]: expected %f, got %f\n",
                    i, (float)i, data[i]);
            nc_close(ncid);
            return 1;
        }
    }
    printf("PASS: array full read: 16 values verified\n");

    /* Read a 2x2 sub-hyperslab starting at [1,1]. Expected: 5,6,9,10 */
    {
        float sub[4];
        size_t s2[2] = {1, 1};
        size_t c2[2] = {2, 2};

        if ((retval = nc_get_vara_float(grp_ncid, varid, s2, c2, sub)))
            ERR(retval);

        if (fabs(sub[0] - 5.0f) > 1e-6f || fabs(sub[1] - 6.0f) > 1e-6f ||
            fabs(sub[2] - 9.0f) > 1e-6f || fabs(sub[3] - 10.0f) > 1e-6f)
        {
            fprintf(stderr, "Sub-hyperslab: expected 5,6,9,10 got %f,%f,%f,%f\n",
                    sub[0], sub[1], sub[2], sub[3]);
            nc_close(ncid);
            return 1;
        }
        printf("PASS: array sub-hyperslab [1:2,1:2] = {5,6,9,10}\n");
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Array data reading tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Sprint 6 binary table data reading.
 *
 * Opens test_table_binary.xml and reads field data.
 * Known values (record 0):
 *   Timestamp = 667917639.0 (s)
 *   Detector_Counts = 1024 (DN)
 *   Temperature = 273.15 (K, approx)
 */
static int
test_table_binary_data_read(void)
{
    int ncid, grp_ncid, varid, retval;
    double timestamps[5];
    int counts[5];
    float temps[5];
    size_t start[1] = {0};
    size_t count[1] = {5};

    printf("\n--- Sprint 6: Table_Binary data reading tests ---\n");

    if ((retval = nc_open(PDS4_TABLE_BINARY_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "test_table_binary.dat", &grp_ncid)))
        ERR(retval);

    /* Read Timestamp field (all 5 records). */
    if ((retval = nc_inq_varid(grp_ncid, "Timestamp", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, timestamps)))
        ERR(retval);
    if (fabs(timestamps[0] - 667917639.0) > 0.01 ||
        fabs(timestamps[1] - 667917640.0) > 0.01 ||
        fabs(timestamps[4] - 667917643.0) > 0.01)
    {
        fprintf(stderr, "Timestamp: got %f, %f, ..., %f\n",
                timestamps[0], timestamps[1], timestamps[4]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Timestamp[0..4] = {%.1f, %.1f, ..., %.1f}\n",
           timestamps[0], timestamps[1], timestamps[4]);

    /* Read Detector_Counts field. */
    if ((retval = nc_inq_varid(grp_ncid, "Detector_Counts", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_int(grp_ncid, varid, start, count, counts)))
        ERR(retval);
    if (counts[0] != 1024 || counts[1] != 1087 || counts[4] != 1056)
    {
        fprintf(stderr, "Counts: got %d, %d, ..., %d\n",
                counts[0], counts[1], counts[4]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Detector_Counts[0..4] = {%d, %d, ..., %d}\n",
           counts[0], counts[1], counts[4]);

    /* Read Temperature field. */
    if ((retval = nc_inq_varid(grp_ncid, "Temperature", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, start, count, temps)))
        ERR(retval);
    if (fabs(temps[0] - 273.15f) > 0.01f || fabs(temps[4] - 273.19f) > 0.01f)
    {
        fprintf(stderr, "Temperature: got %f, ..., %f\n", temps[0], temps[4]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Temperature[0..4] = {%.2f, ..., %.2f}\n", temps[0], temps[4]);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Table_Binary data reading tests PASSED.\n");
    return 0;
}

/**
 * @internal Test real Cassini-Huygens HRD engineering table.
 *
 * Opens cassini_hrd/hrd_2000_on_off.xml and verifies:
 * - A child group named hrd_2000_on_off.tab exists.
 * - Two NC_CHAR variables: ON_OFF_TIME and ON_OFF_FLAG.
 * - 11 records can be read.
 */
static int
test_mission_cassini_hrd(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    char on_off_time[22] = {0};
    char on_off_flag[4] = {0};
    size_t start2[2] = {0, 0};
    size_t count_time[2] = {1, 21};
    size_t count_flag[2] = {1, 3};

    printf("\n--- Mission: Cassini-Huygens HRD ---\n");

    if ((retval = nc_open(PDS4_CASSINI_HRD_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "hrd_2000_on_off.tab", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    /* One record dimension plus one strlen dimension per NC_CHAR field. */
    if (ndims != 3 || nvars != 2)
    {
        fprintf(stderr, "Expected ndims=3 nvars=2, got %d %d\n", ndims, nvars);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "record", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 11)
    {
        fprintf(stderr, "Expected 11 records, got %zu\n", len);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "ON_OFF_TIME", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "Expected ON_OFF_TIME xtype=NC_CHAR, got %d\n", xtype);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "ON_OFF_FLAG", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "Expected ON_OFF_FLAG xtype=NC_CHAR, got %d\n", xtype);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "ON_OFF_TIME", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_text(grp_ncid, varid, start2, count_time, on_off_time)))
        ERR(retval);
    on_off_time[21] = '\0';
    if (strstr(on_off_time, "2000") == NULL)
    {
        fprintf(stderr, "Unexpected ON_OFF_TIME: '%s'\n", on_off_time);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "ON_OFF_FLAG", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_text(grp_ncid, varid, start2, count_flag, on_off_flag)))
        ERR(retval);
    on_off_flag[3] = '\0';
    {
        size_t flag_len = strlen(on_off_flag);
        while (flag_len > 0 && (on_off_flag[flag_len - 1] == ' ' || on_off_flag[flag_len - 1] == '\t'))
            on_off_flag[--flag_len] = '\0';
    }
    if (strcmp(on_off_flag, "ON") != 0 && strcmp(on_off_flag, "OFF") != 0)
    {
        fprintf(stderr, "Unexpected ON_OFF_FLAG: '%s'\n", on_off_flag);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Cassini-Huygens HRD table opens and reads.\n");
    return 0;
}

/**
 * @internal Test real MESSENGER Mercury thermal neutron map.
 *
 * Opens messenger_tnmap/thermal_neutron_map.xml and verifies:
 * - A child group named thermal_neutron_map.img exists.
 * - One NC_UBYTE variable with dimensions [Line=360, Sample=720].
 * - A small hyperslab can be read.
 */
static int
test_mission_messenger_tnmap(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    unsigned char data[4];
    size_t start[2] = {0, 0};
    size_t count[2] = {2, 2};

    printf("\n--- Mission: MESSENGER Thermal Neutron Map ---\n");

    if ((retval = nc_open(PDS4_MESSENGER_TNMAP_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "thermal_neutron_map.img", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 2 || nvars != 1)
    {
        fprintf(stderr, "Expected ndims=2 nvars=1, got %d %d\n", ndims, nvars);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "Line", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 360)
    {
        fprintf(stderr, "Expected Line=360, got %zu\n", len);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "Sample", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 720)
    {
        fprintf(stderr, "Expected Sample=720, got %zu\n", len);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "Mercury Thermal Neutron Map", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_UBYTE || ndims != 2)
    {
        fprintf(stderr, "Unexpected variable: xtype=%d ndims=%d\n", xtype, ndims);
        return 1;
    }

    if ((retval = nc_get_vara_uchar(grp_ncid, varid, start, count, data)))
        ERR(retval);
    printf("PASS: MESSENGER map [0:2,0:2] = {%u,%u,%u,%u}\n",
           (unsigned)data[0], (unsigned)data[1], (unsigned)data[2], (unsigned)data[3]);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    return 0;
}

/**
 * @internal Test real LCS-9P comet photometry table.
 *
 * Opens lcs_9p/20050706_000.xml and verifies:
 * - A child group named 20050706_000.tab exists.
 * - Eight variables including ASCII_Integer and ASCII_Real fields.
 * - The first record reads successfully.
 */
static int
test_mission_lcs_9p(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    long long spec_num;
    double ha_pos;
    size_t start[1] = {0};
    size_t count[1] = {1};

    printf("\n--- Mission: LCS-9P Comet Photometry ---\n");

    if ((retval = nc_open(PDS4_LCS_9P_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "20050706_000.tab", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (ndims != 1 || nvars != 8)
    {
        fprintf(stderr, "Expected ndims=1 nvars=8, got %d %d\n", ndims, nvars);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "record", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 118)
    {
        fprintf(stderr, "Expected 118 records, got %zu\n", len);
        return 1;
    }

    /* The PDS4 reader replaces spaces in field names with underscores. */
    if ((retval = nc_inq_varid(grp_ncid, "Spec_Num", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_INT64)
    {
        fprintf(stderr, "Expected Spec_Num xtype=NC_INT64, got %d\n", xtype);
        return 1;
    }

    if ((retval = nc_get_vara_longlong(grp_ncid, varid, start, count, &spec_num)))
        ERR(retval);
    if (spec_num < 1)
    {
        fprintf(stderr, "Unexpected Spec_Num: %lld\n", spec_num);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "HA_Pos", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, &ha_pos)))
        ERR(retval);
    printf("PASS: LCS-9P record 0: Spec_Num=%lld, HA_Pos=%.3f km\n", spec_num, ha_pos);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    return 0;
}

/**
 * @internal Test MAVEN NGIMS L1B housekeeping table (large sparse delimited table).
 *
 * Opens the Maven L1B housekeeping label and verifies:
 * - Child group named after the CSV data file exists.
 * - 324 variables (one per field).
 * - "record" dimension has length 26121.
 * - TIME variable is NC_DOUBLE.
 * - Light data read: TIME[0] and TIME[1] are both > 0.
 */
static int
test_mission_maven_l1b(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    double time_vals[2];
    size_t start[1] = {0};
    size_t count[1] = {2};

    printf("\n--- Mission: MAVEN NGIMS L1B Housekeeping ---\n");

    if ((retval = nc_open(PDS4_MAVEN_L1B_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.csv",
                              &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars != 324)
    {
        fprintf(stderr, "Expected 324 variables, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d\n", nvars);

    if ((retval = nc_inq_dimid(grp_ncid, "record", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 26121)
    {
        fprintf(stderr, "Expected 26121 records, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: record dimension len=%zu\n", len);

    if ((retval = nc_inq_varid(grp_ncid, "TIME", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE || ndims != 1)
    {
        fprintf(stderr, "Expected TIME xtype=NC_DOUBLE ndims=1, got %d %d\n", xtype, ndims);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: TIME is NC_DOUBLE\n");

    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, time_vals)))
        ERR(retval);
    if (time_vals[0] <= 0.0 || time_vals[1] <= 0.0)
    {
        fprintf(stderr, "Expected TIME > 0, got %f, %f\n", time_vals[0], time_vals[1]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: TIME[0..1] = {%.3f, %.3f}\n", time_vals[0], time_vals[1]);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN L1B housekeeping table opens and reads.\n");
    return 0;
}

/**
 * @internal Test MAVEN NGIMS L3 science table (small delimited table with mixed types).
 *
 * Opens the Maven L3 science label and verifies:
 * - Child group named after the CSV data file exists.
 * - 15 variables (one per field), record dimension length 2.
 * - T_UTC is NC_CHAR (ASCII_Date_Time type).
 * - T_UNIX and TEMPERATURE are NC_DOUBLE.
 * - Known data values verified for T_UNIX[0], SCALE_HEIGHT[0], TEMPERATURE[0].
 */
static int
test_mission_maven_l3(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    size_t len;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    double t_unix;
    double scale_height;
    double temperature;
    size_t start[1] = {0};
    size_t count[1] = {1};

    printf("\n--- Mission: MAVEN NGIMS L3 Science ---\n");

    if ((retval = nc_open(PDS4_MAVEN_L3_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv",
                              &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars != 15)
    {
        fprintf(stderr, "Expected 15 variables, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d\n", nvars);

    if ((retval = nc_inq_dimid(grp_ncid, "record", &varid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, varid, name, &len)))
        ERR(retval);
    if (len != 2)
    {
        fprintf(stderr, "Expected 2 records, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: record dimension len=%zu\n", len);

    /* T_UTC: ASCII_Date_Time -> NC_CHAR 2D variable. */
    if ((retval = nc_inq_varid(grp_ncid, "T_UTC", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "Expected T_UTC xtype=NC_CHAR, got %d\n", xtype);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: T_UTC is NC_CHAR (ASCII_Date_Time)\n");

    /* T_UNIX: ASCII_Real -> NC_DOUBLE. */
    if ((retval = nc_inq_varid(grp_ncid, "T_UNIX", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE)
    {
        fprintf(stderr, "Expected T_UNIX xtype=NC_DOUBLE, got %d\n", xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, &t_unix)))
        ERR(retval);
    if (fabs(t_unix - 1735698148.655328) > 0.01)
    {
        fprintf(stderr, "Unexpected T_UNIX[0]: %f\n", t_unix);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: T_UNIX[0] = %.3f\n", t_unix);

    /* SCALE_HEIGHT: ASCII_Real -> NC_DOUBLE. */
    if ((retval = nc_inq_varid(grp_ncid, "SCALE_HEIGHT", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, &scale_height)))
        ERR(retval);
    if (fabs(scale_height - 3.210464) > 0.001)
    {
        fprintf(stderr, "Unexpected SCALE_HEIGHT[0]: %f\n", scale_height);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: SCALE_HEIGHT[0] = %.6f\n", scale_height);

    /* TEMPERATURE: ASCII_Real -> NC_DOUBLE. */
    if ((retval = nc_inq_varid(grp_ncid, "TEMPERATURE", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &ndims, NULL, &natts)))
        ERR(retval);
    if (xtype != NC_DOUBLE)
    {
        fprintf(stderr, "Expected TEMPERATURE xtype=NC_DOUBLE, got %d\n", xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, &temperature)))
        ERR(retval);
    if (fabs(temperature - 51.332605) > 0.01)
    {
        fprintf(stderr, "Unexpected TEMPERATURE[0]: %f\n", temperature);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: TEMPERATURE[0] = %.6f\n", temperature);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN L3 science table opens and reads.\n");
    return 0;
}

/**
 * @internal Test MAVEN IUVS corona metadata (Group_Field_Binary support).
 *
 * Opens the IUVS FITS-backed PDS4 file and verifies:
 * - A child group named after the FITS data file exists.
 * - Total nvars = 116 (scalar + group fields from all 8 tables).
 * - COLUMN is a 2D variable [record, COLUMN_rep] with trailing dim = 2.
 * - V_TANGENT is a 2D variable with trailing dim = 3.
 * - TANGENT_ALT is a 1D NC_FLOAT variable.
 */
static int
test_mission_maven_iuvs_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    int var_ndims, var_dimids[NC_MAX_DIMS];
    size_t dim_len;

    printf("\n--- Mission: MAVEN IUVS Corona (FITS-backed PDS4) ---\n");

    if ((retval = nc_open(PDS4_MAVEN_IUVS_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    /* Verify child group exists with the FITS filename. */
    if ((retval = nc_inq_ncid(ncid,
            "mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits",
            &grp_ncid)))
        ERR(retval);

    /* Check total variable count (scalar fields + group fields). */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars != 116)
    {
        fprintf(stderr, "Expected nvars=116, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d (scalar + group fields)\n", nvars);

    /* Verify COLUMN is 2D with trailing dim = 2 (Group_Field_Binary, repetitions=2). */
    if ((retval = nc_inq_varid(grp_ncid, "COLUMN", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims, var_dimids, NULL)))
        ERR(retval);
    if (var_ndims != 2 || xtype != NC_FLOAT)
    {
        fprintf(stderr, "COLUMN: expected ndims=2 NC_FLOAT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[1], name, &dim_len)))
        ERR(retval);
    if (dim_len != 2)
    {
        fprintf(stderr, "COLUMN trailing dim: expected len=2, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: COLUMN is 2D NC_FLOAT with trailing dim=2\n");

    /* Verify V_TANGENT is 2D with trailing dim = 3 (repetitions=3). */
    if ((retval = nc_inq_varid(grp_ncid, "V_TANGENT", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims, var_dimids, NULL)))
        ERR(retval);
    if (var_ndims != 2)
    {
        fprintf(stderr, "V_TANGENT: expected ndims=2, got %d\n", var_ndims);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[1], name, &dim_len)))
        ERR(retval);
    if (dim_len != 3)
    {
        fprintf(stderr, "V_TANGENT trailing dim: expected len=3, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: V_TANGENT is 2D with trailing dim=3\n");

    /* Verify TANGENT_ALT is 1D NC_FLOAT. */
    if ((retval = nc_inq_varid(grp_ncid, "TANGENT_ALT", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims, NULL, NULL)))
        ERR(retval);
    if (var_ndims != 1 || xtype != NC_FLOAT)
    {
        fprintf(stderr, "TANGENT_ALT: expected 1D NC_FLOAT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: TANGENT_ALT is 1D NC_FLOAT\n");

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN IUVS metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test MAVEN IUVS corona data reading (Group_Field_Binary support).
 *
 * Reads scalar and group field data from the FITS-backed PDS4 file:
 * - TANGENT_ALT[0] should be finite and > 0 (km altitude).
 * - RADIANCE[50,0] should be finite and > 0 (emission feature radiance in kR).
 */
static int
test_mission_maven_iuvs_data(void)
{
    int ncid, grp_ncid, varid, retval;
    float tangent_alt;
    float radiance_vals[2];
    size_t start1[1] = {0};
    size_t count1[1] = {1};
    size_t start2[2] = {50, 0};
    size_t count2[2] = {1, 2};

    printf("\n--- Mission: MAVEN IUVS Corona (data reading) ---\n");

    if ((retval = nc_open(PDS4_MAVEN_IUVS_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid,
            "mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits",
            &grp_ncid)))
        ERR(retval);

    /* Read TANGENT_ALT[0] — scalar field, should be positive altitude in km. */
    if ((retval = nc_inq_varid(grp_ncid, "TANGENT_ALT", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, start1, count1, &tangent_alt)))
        ERR(retval);
    if (!isfinite(tangent_alt) || tangent_alt <= 0)
    {
        fprintf(stderr, "TANGENT_ALT[0] = %f (expected finite > 0)\n", tangent_alt);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: TANGENT_ALT[0] = %.3f km\n", tangent_alt);

    /* Read RADIANCE[50,0..1] — group field, should have non-zero radiance. */
    if ((retval = nc_inq_varid(grp_ncid, "RADIANCE", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, start2, count2, radiance_vals)))
        ERR(retval);
    if (!isfinite(radiance_vals[0]) || radiance_vals[0] <= 0)
    {
        fprintf(stderr, "RADIANCE[50,0] = %e (expected finite > 0)\n", radiance_vals[0]);
        nc_close(ncid);
        return 1;
    }
    if (!isfinite(radiance_vals[1]))
    {
        fprintf(stderr, "RADIANCE[50,1] = %e (expected finite)\n", radiance_vals[1]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: RADIANCE[50,0..1] = {%e, %e} kR\n", radiance_vals[0], radiance_vals[1]);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN IUVS data reading tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Sprint 6 character table data reading.
 *
 * Opens Table_Character_Example.xml and reads field data.
 * Known values (record 0):
 *   Wavelength = 320.0 (nm)
 *   Reflectance = 0.05039
 *   Error = 0.01451
 */
static int
test_table_character_data_read(void)
{
    int ncid, grp_ncid, varid, retval;
    double wavelengths[3];
    double reflectances[3];
    double errors[3];
    size_t start[1] = {0};
    size_t count[1] = {3};

    printf("\n--- Sprint 6: Table_Character data reading tests ---\n");

    if ((retval = nc_open(PDS4_TABLE_CHAR_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid, "Table_Character_Example.tab", &grp_ncid)))
        ERR(retval);

    /* Read Wavelength field (first 3 records). */
    if ((retval = nc_inq_varid(grp_ncid, "Wavelength", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, wavelengths)))
        ERR(retval);
    if (fabs(wavelengths[0] - 320.0) > 0.01 ||
        fabs(wavelengths[1] - 330.0) > 0.01 ||
        fabs(wavelengths[2] - 340.0) > 0.01)
    {
        fprintf(stderr, "Wavelength: got %f, %f, %f\n",
                wavelengths[0], wavelengths[1], wavelengths[2]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Wavelength[0..2] = {%.1f, %.1f, %.1f}\n",
           wavelengths[0], wavelengths[1], wavelengths[2]);

    /* Read Reflectance field. */
    if ((retval = nc_inq_varid(grp_ncid, "Reflectance", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, reflectances)))
        ERR(retval);
    if (fabs(reflectances[0] - 0.05039) > 0.0001 ||
        fabs(reflectances[1] - 0.05898) > 0.0001)
    {
        fprintf(stderr, "Reflectance: got %f, %f\n",
                reflectances[0], reflectances[1]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Reflectance[0..2] = {%f, %f, %f}\n",
           reflectances[0], reflectances[1], reflectances[2]);

    /* Read Error field. */
    if ((retval = nc_inq_varid(grp_ncid, "Error", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(grp_ncid, varid, start, count, errors)))
        ERR(retval);
    if (fabs(errors[0] - 0.01451) > 0.0001 ||
        fabs(errors[1] - 0.01123) > 0.0001)
    {
        fprintf(stderr, "Error: got %f, %f\n", errors[0], errors[1]);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: Error[0..2] = {%f, %f, %f}\n",
           errors[0], errors[1], errors[2]);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Table_Character data reading tests PASSED.\n");
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

    /* Sprint 6: Data reading tests. */
    if (test_array_data_read())
        return 1;
    if (test_table_binary_data_read())
        return 1;
    if (test_table_character_data_read())
        return 1;

    printf("\nAll PDS4 UDF Sprint 4 + Sprint 5 + Sprint 6 tests PASSED.\n");

    /* Mission data validation tests. */
    if (test_mission_cassini_hrd())
        return 1;
    if (test_mission_messenger_tnmap())
        return 1;
    if (test_mission_lcs_9p())
        return 1;
    if (test_mission_maven_l1b())
        return 1;
    if (test_mission_maven_l3())
        return 1;
    if (test_mission_maven_iuvs_metadata())
        return 1;
    if (test_mission_maven_iuvs_data())
        return 1;

    printf("\nAll PDS4 UDF Sprint tests + Mission tests PASSED.\n");
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

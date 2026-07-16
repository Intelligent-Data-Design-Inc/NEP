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
#define PDS4_MAVEN_PERIAPSE_FILE "data/PDS4/mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml"
#define PDS4_NEW_HORIZONS_FILE \
    "data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx"
#define PDS4_NEW_HORIZONS_0400644769_FILE \
    "data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.lblx"
#define PDS4_NEW_HORIZONS_0284461348_FILE \
    "data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.lblx"
#define PDS4_PERSEVERANCE_FILE \
    "data/PDS4/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml"
#define PDS4_PERSEVERANCE_1737_FILE \
    "data/PDS4/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml"

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

/**
 * @internal Test MAVEN IUVS periapse metadata (nested Group_Field_Binary).
 *
 * Opens the periapse FITS-backed PDS4 label and verifies:
 * - A child group named after the FITS data file exists.
 * - The data_DENSITY table contributes 3D variables [record, outer_rep, inner_rep].
 *   - ALT: 3D NC_FLOAT with outer_rep=19, inner_rep=3.
 * - The data_TEMPERATURE table contributes scalar fields T0, T0_ALT, T0_RANDOM_UNC.
 */
static int
test_mission_maven_periapse_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    int var_ndims, var_dimids[NC_MAX_DIMS];
    size_t dim_len;

    printf("\n--- Mission: MAVEN IUVS Periapse (nested Group_Field_Binary) ---\n");

    if ((retval = nc_open(PDS4_MAVEN_PERIAPSE_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    /* The file area group is named after the FITS data file. */
    if ((retval = nc_inq_ncid(ncid,
            "mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits",
            &grp_ncid)))
        ERR(retval);
    printf("PASS: found periapse file-area group\n");

    /* Verify group has variables (scalar + 2D/3D group fields). */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars <= 0)
    {
        fprintf(stderr, "Expected nvars > 0, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d\n", nvars);

    /* ALT from data_DENSITY: 3D [record, outer_rep=19, inner_rep=3]. */
    if ((retval = nc_inq_varid(grp_ncid, "ALT", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims,
                             var_dimids, NULL)))
        ERR(retval);
    if (var_ndims != 3 || xtype != NC_FLOAT)
    {
        fprintf(stderr, "ALT: expected ndims=3 NC_FLOAT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    /* Check outer rep dim = 19. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[1], name, &dim_len)))
        ERR(retval);
    if (dim_len != 19)
    {
        fprintf(stderr, "ALT outer_rep dim: expected 19, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    /* Check inner rep dim = 3. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[2], name, &dim_len)))
        ERR(retval);
    if (dim_len != 3)
    {
        fprintf(stderr, "ALT inner_rep dim: expected 3, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: ALT is 3D NC_FLOAT [record, outer_rep=19, inner_rep=3]\n");

    /* T0 from data_TEMPERATURE: 1D NC_FLOAT scalar field. */
    if ((retval = nc_inq_varid(grp_ncid, "T0", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims,
                             NULL, NULL)))
        ERR(retval);
    if (var_ndims != 1 || xtype != NC_FLOAT)
    {
        fprintf(stderr, "T0: expected 1D NC_FLOAT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: T0 is 1D NC_FLOAT scalar field\n");

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN IUVS periapse metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test MAVEN IUVS periapse data reading (depth-2 group fields).
 *
 * Reads scalar and 3D group field data from the FITS-backed PDS4 label:
 * - T0[0]: upper boundary temperature, expected finite and > 0 K.
 * - ALT[0, 0, 0]: altitude from first record/outer/inner, expected finite and > 0.
 */
static int
test_mission_maven_periapse_data(void)
{
    int ncid, grp_ncid, varid, retval;
    float t0;
    float alt_val;
    size_t start1[1] = {0};
    size_t count1[1] = {1};
    size_t start3[3] = {0, 0, 0};
    size_t count3[3] = {1, 1, 1};

    printf("\n--- Mission: MAVEN IUVS Periapse (data reading) ---\n");

    if ((retval = nc_open(PDS4_MAVEN_PERIAPSE_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid,
            "mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits",
            &grp_ncid)))
        ERR(retval);

    /* Read T0[0] — scalar field from data_TEMPERATURE table. */
    if ((retval = nc_inq_varid(grp_ncid, "T0", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, start1, count1, &t0)))
        ERR(retval);
    if (!isfinite(t0) || t0 <= 0.0f)
    {
        fprintf(stderr, "T0[0] = %f (expected finite > 0)\n", t0);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: T0[0] = %.3f K\n", t0);

    /* Read ALT[0,0,0] — depth-2 group field from data_DENSITY table. */
    if ((retval = nc_inq_varid(grp_ncid, "ALT", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, start3, count3, &alt_val)))
        ERR(retval);
    if (!isfinite(alt_val) || alt_val <= 0.0f)
    {
        fprintf(stderr, "ALT[0,0,0] = %f (expected finite > 0)\n", alt_val);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: ALT[0,0,0] = %.3f km\n", alt_val);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: MAVEN IUVS periapse data reading tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Perseverance Mastcam-Z Array_3D_Image metadata.
 *
 * Opens the Perseverance Mastcam-Z calibrated image PDS4 label and verifies:
 * - File-area group named after the .IMG data file exists.
 * - Variable "data" is NC_SHORT with ndims=3.
 * - Dimension lengths: Band=3, Line=1200, Sample=1648.
 * - "scaling_factor" string attribute equals "5.0e-06".
 */
static int
test_mission_perseverance_mastcamz_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    int var_ndims, var_dimids[NC_MAX_DIMS];
    size_t dim_len;
    char att_buf[64];

    printf("\n--- Mission: Perseverance Mastcam-Z (Array_3D_Image metadata) ---\n");

    if ((retval = nc_open(PDS4_PERSEVERANCE_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open\n");

    /* Navigate to the file-area group named after the .IMG file. */
    if ((retval = nc_inq_ncid(ncid,
            "ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG",
            &grp_ncid)))
        ERR(retval);
    printf("PASS: found perseverance file-area group\n");

    /* Verify the group has at least one variable. */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars <= 0)
    {
        fprintf(stderr, "Expected nvars > 0, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d\n", nvars);

    /* Variable is named "data" (no <name> element in label -> reader default). */
    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims,
                             var_dimids, NULL)))
        ERR(retval);
    if (var_ndims != 3 || xtype != NC_SHORT)
    {
        fprintf(stderr, "data: expected ndims=3 NC_SHORT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: variable 'data' is NC_SHORT with ndims=3\n");

    /* Band=3 (sequence_number 1, slowest-varying in C order). */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[0], name, &dim_len)))
        ERR(retval);
    if (dim_len != 3)
    {
        fprintf(stderr, "Band dim: expected 3, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[0] (Band) = %zu\n", dim_len);

    /* Line=1200. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[1], name, &dim_len)))
        ERR(retval);
    if (dim_len != 1200)
    {
        fprintf(stderr, "Line dim: expected 1200, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[1] (Line) = %zu\n", dim_len);

    /* Sample=1648. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[2], name, &dim_len)))
        ERR(retval);
    if (dim_len != 1648)
    {
        fprintf(stderr, "Sample dim: expected 1648, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[2] (Sample) = %zu\n", dim_len);

    /* scaling_factor string attribute == "5.0e-06" (raw label value). */
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "scaling_factor", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "5.0e-06") != 0)
    {
        fprintf(stderr, "scaling_factor: expected \"5.0e-06\", got \"%s\"\n", att_buf);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: scaling_factor = \"%s\"\n", att_buf);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Perseverance Mastcam-Z metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Perseverance Mastcam-Z Array_3D_Image data reading.
 *
 * Reads pixel [0,0,0] as NC_SHORT and verifies:
 * - nc_get_vara_short() returns NC_NOERR.
 * - Value is within the valid SignedMSB2 range [-32768, 32767].
 *   A zero value is acceptable (instrument fill/invalid pixel).
 */
static int
test_mission_perseverance_mastcamz_data(void)
{
    int ncid, grp_ncid, varid, retval;
    short pixel;
    size_t start[3] = {0, 0, 0};
    size_t count[3] = {1, 1, 1};

    printf("\n--- Mission: Perseverance Mastcam-Z (Array_3D_Image data read) ---\n");

    if ((retval = nc_open(PDS4_PERSEVERANCE_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid,
            "ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG",
            &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);

    /* Read pixel [0,0,0] -- Band 0, Line 0, Sample 0. */
    if ((retval = nc_get_vara_short(grp_ncid, varid, start, count, &pixel)))
        ERR(retval);

    /* Value must be in valid SignedMSB2 range; zero is acceptable. */
    if (pixel < -32768 || pixel > 32767)
    {
        fprintf(stderr, "pixel[0,0,0] = %d (out of SignedMSB2 range)\n", (int)pixel);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: pixel[0,0,0] = %d (valid SignedMSB2)\n", (int)pixel);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Perseverance Mastcam-Z data reading tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Perseverance Mastcam-Z Sol 1737 Array_3D_Image metadata.
 *
 * Opens the second Perseverance Mastcam-Z calibrated image PDS4 label and
 * verifies the same structure as the Sol 1738 product:
 * - File-area group named after the .IMG data file exists.
 * - Variable "data" is NC_SHORT with ndims=3.
 * - Dimension lengths: Band=3, Line=1200, Sample=1648.
 * - "scaling_factor" string attribute equals "5.0e-06".
 */
static int
test_mission_perseverance_mastcamz_1737_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    char name[NC_MAX_NAME + 1];
    int var_ndims, var_dimids[NC_MAX_DIMS];
    size_t dim_len;
    char att_buf[64];

    printf("\n--- Mission: Perseverance Mastcam-Z Sol 1737 (Array_3D_Image metadata) ---\n");

    if ((retval = nc_open(PDS4_PERSEVERANCE_1737_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open\n");

    /* Navigate to the file-area group named after the .IMG file. */
    if ((retval = nc_inq_ncid(ncid,
            "ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG",
            &grp_ncid)))
        ERR(retval);
    printf("PASS: found perseverance Sol 1737 file-area group\n");

    /* Verify the group has at least one variable. */
    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    if (nvars <= 0)
    {
        fprintf(stderr, "Expected nvars > 0, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: nvars=%d\n", nvars);

    /* Variable is named "data" (no <name> element in label -> reader default). */
    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, name, &xtype, &var_ndims,
                             var_dimids, NULL)))
        ERR(retval);
    if (var_ndims != 3 || xtype != NC_SHORT)
    {
        fprintf(stderr, "data: expected ndims=3 NC_SHORT, got ndims=%d type=%d\n",
                var_ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: variable 'data' is NC_SHORT with ndims=3\n");

    /* Band=3 (sequence_number 1, slowest-varying in C order). */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[0], name, &dim_len)))
        ERR(retval);
    if (dim_len != 3)
    {
        fprintf(stderr, "Band dim: expected 3, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[0] (Band) = %zu\n", dim_len);

    /* Line=1200. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[1], name, &dim_len)))
        ERR(retval);
    if (dim_len != 1200)
    {
        fprintf(stderr, "Line dim: expected 1200, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[1] (Line) = %zu\n", dim_len);

    /* Sample=1648. */
    if ((retval = nc_inq_dim(grp_ncid, var_dimids[2], name, &dim_len)))
        ERR(retval);
    if (dim_len != 1648)
    {
        fprintf(stderr, "Sample dim: expected 1648, got %zu\n", dim_len);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: dim[2] (Sample) = %zu\n", dim_len);

    /* scaling_factor string attribute == "5.0e-06" (raw label value). */
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "scaling_factor", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "5.0e-06") != 0)
    {
        fprintf(stderr, "scaling_factor: expected \"5.0e-06\", got \"%s\"\n", att_buf);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: scaling_factor = \"%s\"\n", att_buf);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Perseverance Mastcam-Z Sol 1737 metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test Perseverance Mastcam-Z Sol 1737 Array_3D_Image data reading.
 *
 * Reads pixel [0,0,0] as NC_SHORT and verifies:
 * - nc_get_vara_short() returns NC_NOERR.
 * - Value is within the valid SignedMSB2 range [-32768, 32767].
 *   A zero value is acceptable (instrument fill/invalid pixel).
 */
static int
test_mission_perseverance_mastcamz_1737_data(void)
{
    int ncid, grp_ncid, varid, retval;
    short pixel;
    size_t start[3] = {0, 0, 0};
    size_t count[3] = {1, 1, 1};

    printf("\n--- Mission: Perseverance Mastcam-Z Sol 1737 (Array_3D_Image data read) ---\n");

    if ((retval = nc_open(PDS4_PERSEVERANCE_1737_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_ncid(ncid,
            "ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG",
            &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "data", &varid)))
        ERR(retval);

    /* Read pixel [0,0,0] -- Band 0, Line 0, Sample 0. */
    if ((retval = nc_get_vara_short(grp_ncid, varid, start, count, &pixel)))
        ERR(retval);

    /* Value must be in valid SignedMSB2 range; zero is acceptable. */
    if (pixel < -32768 || pixel > 32767)
    {
        fprintf(stderr, "pixel[0,0,0] = %d (out of SignedMSB2 range)\n", (int)pixel);
        nc_close(ncid);
        return 1;
    }
    printf("PASS: pixel[0,0,0] = %d (valid SignedMSB2)\n", (int)pixel);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: Perseverance Mastcam-Z Sol 1737 data reading tests PASSED.\n");
    return 0;
}

static int
test_mission_new_horizons(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, dimids[NC_MAX_DIMS];
    nc_type xtype;
    size_t len;
    float spectrum[4];
    short x_index;
    size_t array_start[2] = {0, 0};
    size_t array_count[2] = {1, 4};
    size_t table_start[1] = {0};
    size_t table_count[1] = {1};

    printf("\n--- Mission: New Horizons Alice ---\n");

    if ((retval = nc_open(PDS4_NEW_HORIZONS_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_ncid(ncid, "ali_0030420276_0x4b0_sci_1.fit", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "Observational Data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_FLOAT || ndims != 2)
    {
        fprintf(stderr, "Observational Data: expected 2D NC_FLOAT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)))
        ERR(retval);
    if (len != 32)
    {
        fprintf(stderr, "Observational Data Line length: expected 32, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[1], NULL, &len)))
        ERR(retval);
    if (len != 1024)
    {
        fprintf(stderr, "Observational Data Sample length: expected 1024, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_get_vara_float(grp_ncid, varid, array_start, array_count, spectrum)))
        ERR(retval);
    if (spectrum[0] != 0.0f || spectrum[1] != 0.0f ||
        spectrum[2] != 0.0f || spectrum[3] != 0.0f)
    {
        fprintf(stderr, "Unexpected Observational Data[0,0:4]: %f %f %f %f\n",
                spectrum[0], spectrum[1], spectrum[2], spectrum[3]);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "X_INDEX", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, NULL, NULL)))
        ERR(retval);
    if (xtype != NC_SHORT || ndims != 1)
    {
        fprintf(stderr, "X_INDEX: expected 1D NC_SHORT, got ndims=%d type=%d\n", ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_get_vara_short(grp_ncid, varid, table_start, table_count, &x_index)))
        ERR(retval);
    if (x_index != 1007)
    {
        fprintf(stderr, "Unexpected X_INDEX[0]: %d\n", (int)x_index);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: New Horizons Alice metadata and data reads.\n");
    return 0;
}

/**
 * @internal Test New Horizons Alice ali_0400644769_0x4b2_sci metadata.
 *
 * Opens the PDS4 label and verifies:
 * - File-area group "ali_0400644769_0x4b2_sci.fit" exists.
 * - One "record" dimension of length 102 for the Table_Binary.
 * - Three Array_2D_Spectrum variables: Observational Data, Histogram
 *   Uncertainties, Wavelength Image (all NC_FLOAT, [32, 1024], correct units).
 * - Observational Data has raw scaling_factor="1.00000000000" and
 *   value_offset="0.00000000000".
 * - One Array_1D variable: Pulse Height Distribution (PHD) (NC_INT, [64],
 *   units="COUNT").
 * - One representative Table_Binary field: SAFETY_ACTIVE (NC_UBYTE, 1D).
 */
static int
test_mission_new_horizons_0400644769_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, dimids[NC_MAX_DIMS];
    nc_type xtype;
    size_t len;
    char att_buf[128];

    printf("\n--- Mission: New Horizons Alice ali_0400644769 metadata ---\n");

    if ((retval = nc_open(PDS4_NEW_HORIZONS_0400644769_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_ncid(ncid, "ali_0400644769_0x4b2_sci.fit", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, NULL, NULL)))
        ERR(retval);
    if (ndims != 8)
    {
        fprintf(stderr, "Expected 8 group dimensions, got %d\n", ndims);
        nc_close(ncid);
        return 1;
    }
    if (nvars != 121)
    {
        fprintf(stderr, "Expected 121 group variables, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "record", dimids)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)))
        ERR(retval);
    if (len != 102)
    {
        fprintf(stderr, "Expected record length 102, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }

    /* Observational Data: Array_2D_Spectrum. */
    if ((retval = nc_inq_varid(grp_ncid, "Observational Data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_FLOAT || ndims != 2)
    {
        fprintf(stderr, "Observational Data: expected 2D NC_FLOAT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 32)
    {
        fprintf(stderr, "Observational Data Line length: expected 32, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[1], NULL, &len)) || len != 1024)
    {
        fprintf(stderr, "Observational Data Sample length: expected 1024, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "PHOTON CM**-2 S**-1") != 0)
    {
        fprintf(stderr, "Observational Data units: expected 'PHOTON CM**-2 S**-1', got '%s'\n",
                att_buf);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "scaling_factor", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "1.00000000000") != 0)
    {
        fprintf(stderr, "Observational Data scaling_factor mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "value_offset", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "0.00000000000") != 0)
    {
        fprintf(stderr, "Observational Data value_offset mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* Histogram Uncertainties: Array_2D_Spectrum. */
    if ((retval = nc_inq_varid(grp_ncid, "Histogram Uncertainties", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_FLOAT || ndims != 2)
    {
        fprintf(stderr, "Histogram Uncertainties: expected 2D NC_FLOAT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 32)
    {
        fprintf(stderr, "Histogram Uncertainties Line length: expected 32, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[1], NULL, &len)) || len != 1024)
    {
        fprintf(stderr, "Histogram Uncertainties Sample length: expected 1024, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "PHOTON CM**-2 S**-1") != 0)
    {
        fprintf(stderr, "Histogram Uncertainties units mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* Wavelength Image: Array_2D_Spectrum. */
    if ((retval = nc_inq_varid(grp_ncid, "Wavelength Image", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_FLOAT || ndims != 2)
    {
        fprintf(stderr, "Wavelength Image: expected 2D NC_FLOAT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 32)
    {
        fprintf(stderr, "Wavelength Image Line length: expected 32, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[1], NULL, &len)) || len != 1024)
    {
        fprintf(stderr, "Wavelength Image Sample length: expected 1024, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "ANGSTROM") != 0)
    {
        fprintf(stderr, "Wavelength Image units mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* Pulse Height Distribution (PHD): Array_1D. */
    if ((retval = nc_inq_varid(grp_ncid, "Pulse Height Distribution (PHD)", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_INT || ndims != 1)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD): expected 1D NC_INT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 64)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) length: expected 64, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "COUNT") != 0)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) units mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* SAFETY_ACTIVE: representative Table_Binary field. */
    if ((retval = nc_inq_varid(grp_ncid, "SAFETY_ACTIVE", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, NULL, NULL)))
        ERR(retval);
    if (xtype != NC_UBYTE || ndims != 1)
    {
        fprintf(stderr, "SAFETY_ACTIVE: expected 1D NC_UBYTE, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: New Horizons Alice ali_0400644769 metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test New Horizons Alice ali_0400644769_0x4b2_sci data reads.
 *
 * Reads:
 * - Observational Data[0, 0:4] as NC_FLOAT (verify finite values).
 * - Pulse Height Distribution (PHD)[0] as NC_INT (verify non-negative).
 * - SAFETY_ACTIVE[0] as NC_UBYTE (verify 0 or 1).
 */
static int
test_mission_new_horizons_0400644769_data(void)
{
    int ncid, grp_ncid, varid, retval;
    float spectrum[4];
    int phd;
    unsigned char safety;
    size_t array_start[2] = {0, 0};
    size_t array_count[2] = {1, 4};
    size_t start[1] = {0};
    size_t count[1] = {1};
    int i;

    printf("\n--- Mission: New Horizons Alice ali_0400644769 data reads ---\n");

    if ((retval = nc_open(PDS4_NEW_HORIZONS_0400644769_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_ncid(ncid, "ali_0400644769_0x4b2_sci.fit", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "Observational Data", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(grp_ncid, varid, array_start, array_count, spectrum)))
        ERR(retval);
    for (i = 0; i < 4; i++)
    {
        if (!isfinite(spectrum[i]))
        {
            fprintf(stderr, "Observational Data[0,%d] is not finite: %f\n", i, spectrum[i]);
            nc_close(ncid);
            return 1;
        }
    }

    if ((retval = nc_inq_varid(grp_ncid, "Pulse Height Distribution (PHD)", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_int(grp_ncid, varid, start, count, &phd)))
        ERR(retval);
    if (phd < 0)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD)[0] negative: %d\n", phd);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "SAFETY_ACTIVE", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_uchar(grp_ncid, varid, start, count, &safety)))
        ERR(retval);
    if (safety > 1)
    {
        fprintf(stderr, "SAFETY_ACTIVE[0] out of expected range: %u\n", (unsigned)safety);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: New Horizons Alice ali_0400644769 data reads PASSED.\n");
    return 0;
}

/**
 * @internal Test New Horizons Alice ali_0284461348_0x4b2_eng metadata.
 *
 * Opens the PDS4 label and verifies:
 * - File-area group "ali_0284461348_0x4b2_eng.fit" exists.
 * - One "record" dimension of length 31 for the Table_Binary.
 * - One Array_2D_Spectrum variable: Observational Data (NC_INT, [32, 1024],
 *   units="COUNT", scaling_factor="1.00000000000", value_offset="0.00000000000").
 * - One Array_1D variable: Pulse Height Distribution (PHD) Array (NC_INT, [64]).
 * - One representative Table_Binary field: SAFETY_ACTIVE (NC_UBYTE, 1D).
 */
static int
test_mission_new_horizons_0284461348_metadata(void)
{
    int ncid, grp_ncid, varid, retval;
    int ndims, nvars, dimids[NC_MAX_DIMS];
    nc_type xtype;
    size_t len;
    char att_buf[128];

    printf("\n--- Mission: New Horizons Alice ali_0284461348 metadata ---\n");

    if ((retval = nc_open(PDS4_NEW_HORIZONS_0284461348_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_ncid(ncid, "ali_0284461348_0x4b2_eng.fit", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq(grp_ncid, &ndims, &nvars, NULL, NULL)))
        ERR(retval);
    if (ndims != 4)
    {
        fprintf(stderr, "Expected 4 group dimensions, got %d\n", ndims);
        nc_close(ncid);
        return 1;
    }
    if (nvars != 119)
    {
        fprintf(stderr, "Expected 119 group variables, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_inq_dimid(grp_ncid, "record", dimids)))
        ERR(retval);
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)))
        ERR(retval);
    if (len != 31)
    {
        fprintf(stderr, "Expected record length 31, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }

    /* Observational Data: Array_2D_Spectrum. */
    if ((retval = nc_inq_varid(grp_ncid, "Observational Data", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_INT || ndims != 2)
    {
        fprintf(stderr, "Observational Data: expected 2D NC_INT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 32)
    {
        fprintf(stderr, "Observational Data Line length: expected 32, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[1], NULL, &len)) || len != 1024)
    {
        fprintf(stderr, "Observational Data Sample length: expected 1024, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "COUNT") != 0)
    {
        fprintf(stderr, "Observational Data units: expected 'COUNT', got '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "scaling_factor", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "1.00000000000") != 0)
    {
        fprintf(stderr, "Observational Data scaling_factor mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "value_offset", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "0.00000000000") != 0)
    {
        fprintf(stderr, "Observational Data value_offset mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* Pulse Height Distribution (PHD) Array: Array_1D. */
    if ((retval = nc_inq_varid(grp_ncid, "Pulse Height Distribution (PHD) Array", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, dimids, NULL)))
        ERR(retval);
    if (xtype != NC_INT || ndims != 1)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) Array: expected 1D NC_INT, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }
    if ((retval = nc_inq_dim(grp_ncid, dimids[0], NULL, &len)) || len != 64)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) Array length: expected 64, got %zu\n", len);
        nc_close(ncid);
        return 1;
    }
    memset(att_buf, 0, sizeof(att_buf));
    if ((retval = nc_get_att_text(grp_ncid, varid, "units", att_buf)))
        ERR(retval);
    if (strcmp(att_buf, "COUNT") != 0)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) Array units mismatch: '%s'\n", att_buf);
        nc_close(ncid);
        return 1;
    }

    /* SAFETY_ACTIVE: representative Table_Binary field. */
    if ((retval = nc_inq_varid(grp_ncid, "SAFETY_ACTIVE", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp_ncid, varid, NULL, &xtype, &ndims, NULL, NULL)))
        ERR(retval);
    if (xtype != NC_UBYTE || ndims != 1)
    {
        fprintf(stderr, "SAFETY_ACTIVE: expected 1D NC_UBYTE, got ndims=%d type=%d\n",
                ndims, xtype);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: New Horizons Alice ali_0284461348 metadata tests PASSED.\n");
    return 0;
}

/**
 * @internal Test New Horizons Alice ali_0284461348_0x4b2_eng data reads.
 *
 * Reads:
 * - Observational Data[0, 0:4] as NC_INT (verify non-negative).
 * - Pulse Height Distribution (PHD) Array[0] as NC_INT (verify non-negative).
 * - SAFETY_ACTIVE[0] as NC_UBYTE (verify 0 or 1).
 */
static int
test_mission_new_horizons_0284461348_data(void)
{
    int ncid, grp_ncid, varid, retval;
    int spectrum[4];
    int phd;
    unsigned char safety;
    size_t array_start[2] = {0, 0};
    size_t array_count[2] = {1, 4};
    size_t start[1] = {0};
    size_t count[1] = {1};
    int i;

    printf("\n--- Mission: New Horizons Alice ali_0284461348 data reads ---\n");

    if ((retval = nc_open(PDS4_NEW_HORIZONS_0284461348_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_ncid(ncid, "ali_0284461348_0x4b2_eng.fit", &grp_ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(grp_ncid, "Observational Data", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_int(grp_ncid, varid, array_start, array_count, spectrum)))
        ERR(retval);
    for (i = 0; i < 4; i++)
    {
        if (spectrum[i] < 0)
        {
            fprintf(stderr, "Observational Data[0,%d] negative: %d\n", i, spectrum[i]);
            nc_close(ncid);
            return 1;
        }
    }

    if ((retval = nc_inq_varid(grp_ncid, "Pulse Height Distribution (PHD) Array", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_int(grp_ncid, varid, start, count, &phd)))
        ERR(retval);
    if (phd < 0)
    {
        fprintf(stderr, "Pulse Height Distribution (PHD) Array[0] negative: %d\n", phd);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_inq_varid(grp_ncid, "SAFETY_ACTIVE", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_uchar(grp_ncid, varid, start, count, &safety)))
        ERR(retval);
    if (safety > 1)
    {
        fprintf(stderr, "SAFETY_ACTIVE[0] out of expected range: %u\n", (unsigned)safety);
        nc_close(ncid);
        return 1;
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: New Horizons Alice ali_0284461348 data reads PASSED.\n");
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

    /* Register the PDS4 UDF handler. */
    if (!NC_PDS4_initialize())
        ERR(NC_EINVAL);
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
    if (test_mission_new_horizons())
        return 1;
    if (test_mission_new_horizons_0400644769_metadata())
        return 1;
    if (test_mission_new_horizons_0400644769_data())
        return 1;
    if (test_mission_new_horizons_0284461348_metadata())
        return 1;
    if (test_mission_new_horizons_0284461348_data())
        return 1;
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
    if (test_mission_maven_periapse_metadata())
        return 1;
    if (test_mission_maven_periapse_data())
        return 1;
    if (test_mission_perseverance_mastcamz_metadata())
        return 1;
    if (test_mission_perseverance_mastcamz_data())
        return 1;
    if (test_mission_perseverance_mastcamz_1737_metadata())
        return 1;
    if (test_mission_perseverance_mastcamz_1737_data())
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

/**
 * @file
 * Test GeoTIFF CRS metadata extraction (issue #92, v1.6.0 Sprint 1).
 *
 * Validates that CRS parameters are extracted from real-world GeoTIFF files
 * and stored as NetCDF global attributes, covering projected, geographic, and
 * error-handling paths.
 *
 * @author Edward Hartnett
 * @date 2026-03-03
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include <config.h>
#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geotiffdispatch.h"

#define NASA_DATA_DIR "./"

#define ERR_CHECK(ret) do { if ((ret) != NC_NOERR) { \
    printf("FAILED at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

#ifdef HAVE_GEOTIFF

/**
 * Count the number of global attributes whose names begin with "geotiff_".
 */
static int
count_crs_attributes(int ncid)
{
    int natts;
    char att_name[NC_MAX_NAME + 1];
    int count = 0;

    if (nc_inq_natts(ncid, &natts) != NC_NOERR)
        return -1;

    for (int i = 0; i < natts; i++)
    {
        if (nc_inq_attname(ncid, NC_GLOBAL, i, att_name) != NC_NOERR)
            return -1;
        if (strncmp(att_name, "geotiff_", 8) == 0)
            count++;
    }
    return count;
}

/**
 * Test CRS extraction from projected GeoTIFF (ABBA sinusoidal).
 *
 * Expected: ModelTypeProjected, semi_major_axis=6371007.181 (sphere),
 * false_easting=0, false_northing=0, central_meridian=0.
 * geotiff_crs_name should be "Projected".
 */
static int
test_projected_crs(void)
{
    int ncid;
    int ret;
    int crs_att_count;
    char crs_name[NC_MAX_NAME + 1];
    double semi_major;

    printf("Testing projected CRS extraction (ABBA sinusoidal)...");

    ret = nc_open(NASA_DATA_DIR "ABBA_2022_C61_HNL.tif", NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* Must have at least one CRS attribute */
    crs_att_count = count_crs_attributes(ncid);
    if (crs_att_count <= 0)
    {
        printf("FAILED - no CRS attributes found (count=%d)\n", crs_att_count);
        nc_close(ncid);
        return 1;
    }

    /* CRS name must be present and non-empty */
    ret = nc_get_att_text(ncid, NC_GLOBAL, "geotiff_crs_name", crs_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - geotiff_crs_name not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (strlen(crs_name) == 0)
    {
        printf("FAILED - geotiff_crs_name is empty\n");
        nc_close(ncid);
        return 1;
    }

    /* Semi-major axis must be present and positive */
    ret = nc_get_att_double(ncid, NC_GLOBAL, "geotiff_semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("FAILED - geotiff_semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (semi_major <= 0.0)
    {
        printf("FAILED - geotiff_semi_major_axis is not positive: %f\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("ok (%d CRS attrs, crs_name='%s', semi_major=%.3f)\n",
           crs_att_count, crs_name, semi_major);
    return 0;
}

/**
 * Test CRS extraction from geographic GeoTIFF (MODIS MCDWD, Clarke 1866).
 *
 * Expected: ModelTypeGeographic, semi_major_axis=6378206.4,
 * inverse_flattening≈294.978698.
 * geotiff_crs_name should be "Geographic".
 */
static int
test_geographic_crs(void)
{
    int ncid;
    int ret;
    int crs_att_count;
    char crs_name[NC_MAX_NAME + 1];
    double semi_major;
    double inv_flat;

    printf("Testing geographic CRS extraction (MODIS MCDWD h00v02)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* Must have at least one CRS attribute */
    crs_att_count = count_crs_attributes(ncid);
    if (crs_att_count <= 0)
    {
        printf("FAILED - no CRS attributes found (count=%d)\n", crs_att_count);
        nc_close(ncid);
        return 1;
    }

    /* CRS name must be present */
    ret = nc_get_att_text(ncid, NC_GLOBAL, "geotiff_crs_name", crs_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - geotiff_crs_name not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* Semi-major axis: Clarke 1866 = 6378206.4 m */
    ret = nc_get_att_double(ncid, NC_GLOBAL, "geotiff_semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("FAILED - geotiff_semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    /* Allow 1 m tolerance */
    if (semi_major < 6378205.0 || semi_major > 6378208.0)
    {
        printf("FAILED - geotiff_semi_major_axis %.3f out of expected range "
               "[6378205, 6378208]\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    /* Inverse flattening: Clarke 1866 ≈ 294.978698 */
    ret = nc_get_att_double(ncid, NC_GLOBAL, "geotiff_inverse_flattening", &inv_flat);
    if (ret != NC_NOERR)
    {
        printf("FAILED - geotiff_inverse_flattening not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (inv_flat < 294.97 || inv_flat > 294.99)
    {
        printf("FAILED - geotiff_inverse_flattening %.6f out of expected range "
               "[294.97, 294.99]\n", inv_flat);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("ok (%d CRS attrs, crs_name='%s', semi_major=%.3f, inv_flat=%.6f)\n",
           crs_att_count, crs_name, semi_major, inv_flat);
    return 0;
}

/**
 * Test that a second geographic file (h00v03) also yields CRS attributes.
 * Both MCDWD tiles share the same CRS so this confirms consistency.
 */
static int
test_geographic_crs_second_tile(void)
{
    int ncid;
    int ret;
    int crs_att_count;

    printf("Testing geographic CRS extraction (MODIS MCDWD h00v03)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    crs_att_count = count_crs_attributes(ncid);
    if (crs_att_count <= 0)
    {
        printf("FAILED - no CRS attributes found (count=%d)\n", crs_att_count);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("ok (%d CRS attrs)\n", crs_att_count);
    return 0;
}

/**
 * Test error paths: validate_crs_completeness and extract_crs_parameters
 * return NC_EINVAL for NULL inputs.
 */
static int
test_null_input_errors(void)
{
    int ret;

    printf("Testing null-input error handling...");

    ret = validate_crs_completeness(NULL);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - validate_crs_completeness(NULL) returned %d, expected NC_EINVAL\n",
               ret);
        return 1;
    }

    ret = extract_crs_parameters(NULL, NULL);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - extract_crs_parameters(NULL, NULL) returned %d, expected NC_EINVAL\n",
               ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test that validate_crs_completeness accepts UNKNOWN CRS without error.
 */
static int
test_validate_unknown_crs(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness with UNKNOWN type...");

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_UNKNOWN;

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for UNKNOWN CRS, expected NC_NOERR\n", ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test that validate_crs_completeness accepts known CRS without ellipsoid data.
 * (Foundation implementation: permissive — do not block on missing ellipsoid.)
 */
static int
test_validate_known_crs_no_ellipsoid(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness with known CRS, no ellipsoid...");

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_GEOGRAPHIC;
    /* semi_major_axis intentionally left 0 */

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for GEOGRAPHIC CRS without ellipsoid, "
               "expected NC_NOERR (foundation is permissive)\n", ret);
        return 1;
    }

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_PROJECTED;

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for PROJECTED CRS without ellipsoid, "
               "expected NC_NOERR (foundation is permissive)\n", ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int failed = 0;

    printf("=== CRS Metadata Extraction Tests (issue #92) ===\n\n");

#ifdef HAVE_GEOTIFF
    failed += test_null_input_errors();
    failed += test_validate_unknown_crs();
    failed += test_validate_known_crs_no_ellipsoid();
    failed += test_projected_crs();
    failed += test_geographic_crs();
    failed += test_geographic_crs_second_tile();
#else
    printf("SKIP: GeoTIFF support not compiled in.\n");
#endif

    printf("\n");
    if (failed)
        printf("FAILED: %d test(s) failed.\n", failed);
    else
        printf("SUCCESS: All CRS extraction tests passed.\n");

    return failed ? 1 : 0;
}

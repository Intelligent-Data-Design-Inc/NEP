/**
 * @file
 * Test GeoTIFF CF-compliant CRS attribute mapping (issue #93, v1.6.0 Sprint 2).
 *
 * Validates that GeoTIFF files opened via the NetCDF API expose:
 *   - A scalar 'crs' variable with CF-1.8 grid_mapping_name attribute
 *   - Coordinate variables (lon/lat or x/y) with CF standard_name and units
 *   - A 'data' variable with grid_mapping and coordinates attributes
 *
 * Also retains Sprint 1 error-path tests for validate_crs_completeness and
 * extract_crs_parameters.
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
 */
static int
test_validate_known_crs_no_ellipsoid(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness with known CRS, no ellipsoid...");

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_GEOGRAPHIC;

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for GEOGRAPHIC CRS without ellipsoid, "
               "expected NC_NOERR\n", ret);
        return 1;
    }

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_PROJECTED;

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for PROJECTED CRS without ellipsoid, "
               "expected NC_NOERR\n", ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Verify CF-compliant CRS variables in an open GeoTIFF file.
 *
 * Checks:
 *   1. A variable named 'crs' exists with a 'grid_mapping_name' attribute.
 *   2. The data variable has a 'grid_mapping' attribute equal to "crs".
 *   3. If coordinate variables are expected (has_coords), lon+lat (geographic)
 *      or x+y (projected) exist with correct 'units'.
 *
 * @param ncid         Open NetCDF file ID.
 * @param is_geographic 1 for geographic CRS (lon/lat), 0 for projected (x/y).
 * @param has_coords   1 if GeoTransform tags are expected (coordinate vars created).
 * @return 0 on success, 1 on failure.
 */
static int
check_cf_crs(int ncid, int is_geographic, int has_coords)
{
    int crs_varid, data_varid, coord_varid;
    char buf[NC_MAX_NAME + 1];
    int ret;

    /* 1. 'crs' variable must exist */
    ret = nc_inq_varid(ncid, "crs", &crs_varid);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - 'crs' variable not found: %s", nc_strerror(ret));
        return 1;
    }

    /* 2. 'crs' must have grid_mapping_name attribute */
    ret = nc_get_att_text(ncid, crs_varid, "grid_mapping_name", buf);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - grid_mapping_name attr not found: %s", nc_strerror(ret));
        return 1;
    }
    if (strlen(buf) == 0)
    {
        printf("\n  FAILED - grid_mapping_name is empty");
        return 1;
    }

    /* 3. 'data' variable must have grid_mapping = "crs" */
    ret = nc_inq_varid(ncid, "data", &data_varid);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - 'data' variable not found: %s", nc_strerror(ret));
        return 1;
    }
    ret = nc_get_att_text(ncid, data_varid, "grid_mapping", buf);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - grid_mapping attr on data var not found: %s", nc_strerror(ret));
        return 1;
    }
    if (strcmp(buf, "crs") != 0)
    {
        printf("\n  FAILED - grid_mapping = '%s', expected 'crs'", buf);
        return 1;
    }

    /* 4. Coordinate variables (if expected) */
    if (has_coords)
    {
        const char *xname = is_geographic ? "lon" : "x";
        const char *yname = is_geographic ? "lat" : "y";
        const char *xunits = is_geographic ? "degrees_east" : "m";
        const char *yunits = is_geographic ? "degrees_north" : "m";

        /* x/lon variable */
        ret = nc_inq_varid(ncid, xname, &coord_varid);
        if (ret != NC_NOERR)
        {
            printf("\n  FAILED - '%s' variable not found: %s", xname, nc_strerror(ret));
            return 1;
        }
        ret = nc_get_att_text(ncid, coord_varid, "units", buf);
        if (ret != NC_NOERR)
        {
            printf("\n  FAILED - '%s' units attr not found: %s", xname, nc_strerror(ret));
            return 1;
        }
        if (strcmp(buf, xunits) != 0)
        {
            printf("\n  FAILED - '%s' units = '%s', expected '%s'", xname, buf, xunits);
            return 1;
        }

        /* y/lat variable */
        ret = nc_inq_varid(ncid, yname, &coord_varid);
        if (ret != NC_NOERR)
        {
            printf("\n  FAILED - '%s' variable not found: %s", yname, nc_strerror(ret));
            return 1;
        }
        ret = nc_get_att_text(ncid, coord_varid, "units", buf);
        if (ret != NC_NOERR)
        {
            printf("\n  FAILED - '%s' units attr not found: %s", yname, nc_strerror(ret));
            return 1;
        }
        if (strcmp(buf, yunits) != 0)
        {
            printf("\n  FAILED - '%s' units = '%s', expected '%s'", yname, buf, yunits);
            return 1;
        }
    }

    printf(" grid_mapping_name='%s'", buf); /* reuse buf from last successful read */
    return 0;
}

/**
 * Test CF CRS variables from projected GeoTIFF (ABBA sinusoidal).
 *
 * Expected: ModelTypeProjected → sinusoidal grid_mapping_name,
 * x/y coordinate variables in metres, data var has grid_mapping = "crs".
 */
static int
test_projected_crs(void)
{
    int ncid;
    int ret;
    int crs_varid;
    char grid_mapping_name[NC_MAX_NAME + 1];
    double semi_major;

    printf("Testing CF CRS variables - projected (ABBA sinusoidal)...");

    ret = nc_open(NASA_DATA_DIR "ABBA_2022_C61_HNL.tif", NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    if (check_cf_crs(ncid, 0 /* projected */, 1 /* has coords */))
    {
        printf("\n");
        nc_close(ncid);
        return 1;
    }

    /* Verify grid_mapping_name is sinusoidal for ABBA */
    nc_inq_varid(ncid, "crs", &crs_varid);
    ret = nc_get_att_text(ncid, crs_varid, "grid_mapping_name", grid_mapping_name);
    if (ret == NC_NOERR && strcmp(grid_mapping_name, "sinusoidal") != 0)
    {
        printf("\n  FAILED - grid_mapping_name='%s', expected 'sinusoidal'\n",
               grid_mapping_name);
        nc_close(ncid);
        return 1;
    }

    /* semi_major_axis on crs var: ABBA uses sphere 6371007.181 m */
    ret = nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
    if (ret == NC_NOERR && semi_major <= 0.0)
    {
        printf("\n  FAILED - semi_major_axis not positive: %f\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("\n  ok (grid_mapping_name='%s', semi_major=%.3f)\n",
           grid_mapping_name, semi_major);
    return 0;
}

/**
 * Test CF CRS variables from geographic GeoTIFF (MODIS MCDWD h00v02, Clarke 1866).
 *
 * Expected: ModelTypeGeographic → latitude_longitude grid_mapping_name,
 * lon/lat coordinate variables in degrees, data var has grid_mapping = "crs".
 */
static int
test_geographic_crs(void)
{
    int ncid;
    int ret;
    int crs_varid;
    char grid_mapping_name[NC_MAX_NAME + 1];
    double semi_major, inv_flat;

    printf("Testing CF CRS variables - geographic (MODIS MCDWD h00v02)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    if (check_cf_crs(ncid, 1 /* geographic */, 1 /* has coords */))
    {
        printf("\n");
        nc_close(ncid);
        return 1;
    }

    nc_inq_varid(ncid, "crs", &crs_varid);

    /* grid_mapping_name must be latitude_longitude */
    ret = nc_get_att_text(ncid, crs_varid, "grid_mapping_name", grid_mapping_name);
    if (ret != NC_NOERR || strcmp(grid_mapping_name, "latitude_longitude") != 0)
    {
        printf("\n  FAILED - grid_mapping_name='%s', expected 'latitude_longitude'\n",
               grid_mapping_name);
        nc_close(ncid);
        return 1;
    }

    /* semi_major_axis: Clarke 1866 = 6378206.4 m */
    ret = nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (semi_major < 6378205.0 || semi_major > 6378208.0)
    {
        printf("\n  FAILED - semi_major_axis %.3f out of range [6378205, 6378208]\n",
               semi_major);
        nc_close(ncid);
        return 1;
    }

    /* inverse_flattening: Clarke 1866 ≈ 294.978698 */
    ret = nc_get_att_double(ncid, crs_varid, "inverse_flattening", &inv_flat);
    if (ret != NC_NOERR)
    {
        printf("\n  FAILED - inverse_flattening not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (inv_flat < 294.97 || inv_flat > 294.99)
    {
        printf("\n  FAILED - inverse_flattening %.6f out of range [294.97, 294.99]\n",
               inv_flat);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("\n  ok (grid_mapping_name='%s', semi_major=%.3f, inv_flat=%.6f)\n",
           grid_mapping_name, semi_major, inv_flat);
    return 0;
}

/**
 * Test CF CRS variables from second geographic tile (MODIS MCDWD h00v03).
 */
static int
test_geographic_crs_second_tile(void)
{
    int ncid;
    int ret;

    printf("Testing CF CRS variables - geographic (MODIS MCDWD h00v03)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    if (check_cf_crs(ncid, 1 /* geographic */, 1 /* has coords */))
    {
        printf("\n");
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("\n  ok\n");
    return 0;
}

/**
 * Test that coordinate variable data is readable and in valid range.
 * Opens MCDWD h00v02 (geographic) and reads the first lon and lat values.
 */
static int
test_coord_var_data(void)
{
    int ncid, lon_varid, lat_varid;
    double lon_val, lat_val;
    size_t start = 0, count = 1;
    int ret;

    printf("Testing coordinate variable data (MCDWD h00v02)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_inq_varid(ncid, "lon", &lon_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lon' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_varid(ncid, "lat", &lat_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lat' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* Read first lon value */
    ret = nc_get_vara_double(ncid, lon_varid, &start, &count, &lon_val);
    if (ret != NC_NOERR)
    {
        printf("FAILED - reading lon[0]: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    /* h00v02 covers lon 180°E–180°W, so first pixel near -180 */
    if (lon_val < -180.1 || lon_val > 180.1)
    {
        printf("FAILED - lon[0]=%.6f out of valid range [-180, 180]\n", lon_val);
        nc_close(ncid);
        return 1;
    }

    /* Read first lat value */
    ret = nc_get_vara_double(ncid, lat_varid, &start, &count, &lat_val);
    if (ret != NC_NOERR)
    {
        printf("FAILED - reading lat[0]: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (lat_val < -90.1 || lat_val > 90.1)
    {
        printf("FAILED - lat[0]=%.6f out of valid range [-90, 90]\n", lat_val);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    ERR_CHECK(ret);

    printf("ok (lon[0]=%.6f, lat[0]=%.6f)\n", lon_val, lat_val);
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int failed = 0;

    printf("=== CF-Compliant CRS Attribute Mapping Tests (issue #93) ===\n\n");

#ifdef HAVE_GEOTIFF
    {
        char magic_tiff[4] = "II*";
        char magic_bigtiff[4] = "II+";
        int reg_ret;

        if (!GEOTIFF_INIT_OK())
        {
            printf("ERROR: Failed to initialize GeoTIFF dispatch layer\n");
            return 1;
        }
        if ((reg_ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table,
                                          magic_tiff)) != NC_NOERR)
        {
            printf("ERROR: Failed to register standard TIFF handler: %s\n",
                   nc_strerror(reg_ret));
            return 1;
        }
        if ((reg_ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table,
                                          magic_bigtiff)) != NC_NOERR)
        {
            printf("ERROR: Failed to register BigTIFF handler: %s\n",
                   nc_strerror(reg_ret));
            return 1;
        }
    }

    failed += test_null_input_errors();
    failed += test_validate_unknown_crs();
    failed += test_validate_known_crs_no_ellipsoid();
    failed += test_projected_crs();
    failed += test_geographic_crs();
    failed += test_geographic_crs_second_tile();
    failed += test_coord_var_data();
#else
    printf("SKIP: GeoTIFF support not compiled in.\n");
#endif

    printf("\n");
    if (failed)
        printf("FAILED: %d test(s) failed.\n", failed);
    else
        printf("SUCCESS: All CF CRS mapping tests passed.\n");

    return failed ? 1 : 0;
}

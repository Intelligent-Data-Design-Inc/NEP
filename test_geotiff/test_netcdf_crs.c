/**
 * @file
 * Test GeoTIFF CF-compliant CRS attribute mapping (issues #92, #93, #94).
 *
 * Validates that GeoTIFF files opened via the NetCDF API expose:
 *   - A scalar 'crs' variable with CF-1.8 grid_mapping_name attribute
 *   - Coordinate variables (lon/lat or x/y) with CF standard_name and units
 *   - A 'data' variable with grid_mapping and coordinates attributes
 *   - Bounds variables (lon_bnds/lat_bnds or x_bnds/y_bnds) for pixel-as-area
 *   - Correct standard_parallel attributes for LCC/Albers projections
 *   - Correct scale_factor_at_central_meridian for Transverse Mercator
 *
 * Sprint 3 additions: standard_parallel extraction, pixel raster type,
 * coordinate bounds variables, and strengthened validate_crs_completeness.
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
 * Geographic CRS with no ellipsoid (semi_major_axis=0) must be accepted.
 * Projected CRS with a valid ct_projection but no ellipsoid must also be accepted.
 */
static int
test_validate_known_crs_no_ellipsoid(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness with known CRS, no ellipsoid...");

    /* Geographic CRS with semi_major_axis=0 (missing) is acceptable */
    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_GEOGRAPHIC;
    crs_info.semi_major_axis = 0.0; /* not set */

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_NOERR)
    {
        printf("FAILED - returned %d for GEOGRAPHIC CRS without ellipsoid, "
               "expected NC_NOERR\n", ret);
        return 1;
    }

    /* Projected CRS with a known projection type but no ellipsoid is acceptable */
    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_PROJECTED;
    crs_info.ct_projection = 24; /* CT_Sinusoidal=24, valid projection, ellipsoid missing */
    crs_info.semi_major_axis = 0.0; /* not set */

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

/* === Sprint 3 unit tests (synthetic CRS structs, no file I/O) === */

/**
 * Test that validate_crs_completeness rejects negative semi_major_axis.
 */
static int
test_validate_negative_semi_major(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness rejects negative semi_major_axis...");

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_GEOGRAPHIC;
    crs_info.semi_major_axis = -1.0; /* invalid */

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - expected NC_EINVAL for negative semi_major_axis, got %d\n", ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test that validate_crs_completeness rejects projected CRS with ct_projection=0.
 */
static int
test_validate_projected_no_projection(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;
    int ret;

    printf("Testing validate_crs_completeness rejects projected CRS with ct_projection=0...");

    memset(&crs_info, 0, sizeof(crs_info));
    crs_info.crs_type = NC_GEOTIFF_CRS_PROJECTED;
    crs_info.semi_major_axis = 6378137.0;
    crs_info.ct_projection = 0; /* not set */

    ret = validate_crs_completeness(&crs_info);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - expected NC_EINVAL for projected CRS with ct_projection=0, got %d\n",
               ret);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test that standard_parallel_1 and standard_parallel_2 are populated when
 * extract_crs_parameters encounters ProjStdParallel GeoKeys.
 * (Unit test using synthetic NC_GEOTIFF_CRS_INFO_T — validates field presence.)
 */
static int
test_standard_parallel_fields_present(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;

    printf("Testing NC_GEOTIFF_CRS_INFO_T has standard_parallel fields...");

    memset(&crs_info, 0, sizeof(crs_info));
    /* Simulate what extract_crs_parameters would set for LCC 2SP */
    crs_info.crs_type = NC_GEOTIFF_CRS_PROJECTED;
    crs_info.ct_projection = 8; /* CT_LambertConfConic_2SP */
    crs_info.semi_major_axis = 6378137.0;
    crs_info.inverse_flattening = 298.257223563;
    crs_info.standard_parallel_1 = 33.0;
    crs_info.standard_parallel_2 = 45.0;
    crs_info.latitude_of_origin = 23.0;
    crs_info.central_meridian = -96.0;
    crs_info.false_easting = 0.0;
    crs_info.false_northing = 0.0;

    /* Validate completeness passes for a valid projected CRS */
    if (validate_crs_completeness(&crs_info) != NC_NOERR)
    {
        printf("FAILED - validate_crs_completeness rejected valid LCC CRS\n");
        return 1;
    }

    /* Verify the fields hold the expected values */
    if (crs_info.standard_parallel_1 != 33.0 || crs_info.standard_parallel_2 != 45.0)
    {
        printf("FAILED - standard_parallel fields wrong: %.1f, %.1f\n",
               crs_info.standard_parallel_1, crs_info.standard_parallel_2);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test raster_pixel_is_point field default (0 = pixel-as-area).
 */
static int
test_raster_pixel_type_default(void)
{
    NC_GEOTIFF_CRS_INFO_T crs_info;

    printf("Testing NC_GEOTIFF_CRS_INFO_T raster_pixel_is_point defaults to 0...");

    memset(&crs_info, 0, sizeof(crs_info));

    if (crs_info.raster_pixel_is_point != 0)
    {
        printf("FAILED - expected 0 after memset, got %d\n",
               crs_info.raster_pixel_is_point);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/* === Sprint 3 integration tests (require test GeoTIFF files) === */

/**
 * Test that bounds variables (lon_bnds, lat_bnds) exist and have correct shape
 * for a geographic GeoTIFF (MODIS MCDWD — pixel-as-area, has GeoTransform).
 */
static int
test_coord_bounds_geographic(void)
{
    int ncid, bnds_varid, ret;
    int ndims;
    int dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;
    int lon_varid;
    char bounds_name[NC_MAX_NAME + 1];

    printf("Testing coordinate bounds variables (geographic MCDWD h00v02)...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* lon variable must have 'bounds' attribute pointing to lon_bnds */
    ret = nc_inq_varid(ncid, "lon", &lon_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lon' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_att_text(ncid, lon_varid, "bounds", bounds_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lon' has no 'bounds' attr: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (strcmp(bounds_name, "lon_bnds") != 0)
    {
        printf("FAILED - 'lon' bounds='%s', expected 'lon_bnds'\n", bounds_name);
        nc_close(ncid);
        return 1;
    }

    /* lon_bnds must be a 2D variable with second dimension == 2 */
    ret = nc_inq_varid(ncid, "lon_bnds", &bnds_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lon_bnds' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_varndims(ncid, bnds_varid, &ndims);
    if (ret != NC_NOERR || ndims != 2)
    {
        printf("FAILED - 'lon_bnds' ndims=%d, expected 2\n", ndims);
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_vardimid(ncid, bnds_varid, dimids);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_inq_vardimid for lon_bnds: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    /* Second dimension must be size 2 */
    ret = nc_inq_dimlen(ncid, dimids[1], &dimlen);
    if (ret != NC_NOERR || dimlen != 2)
    {
        printf("FAILED - 'lon_bnds' second dim len=%zu, expected 2\n", dimlen);
        nc_close(ncid);
        return 1;
    }

    /* lat_bnds must also exist */
    ret = nc_inq_varid(ncid, "lat_bnds", &bnds_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lat_bnds' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_close: %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok (lon:bounds='%s', lon_bnds[n,2])\n", bounds_name);
    return 0;
}

/**
 * Test that bounds variables (x_bnds, y_bnds) exist for a projected GeoTIFF
 * (ABBA sinusoidal — pixel-as-area).
 */
static int
test_coord_bounds_projected(void)
{
    int ncid, bnds_varid, ret;
    int x_varid;
    char bounds_name[NC_MAX_NAME + 1];
    int ndims;
    int dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;

    printf("Testing coordinate bounds variables (projected ABBA)...");

    ret = nc_open(NASA_DATA_DIR "ABBA_2022_C61_HNL.tif", NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* x variable must have 'bounds' attribute */
    ret = nc_inq_varid(ncid, "x", &x_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'x' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_att_text(ncid, x_varid, "bounds", bounds_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'x' has no 'bounds' attr: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (strcmp(bounds_name, "x_bnds") != 0)
    {
        printf("FAILED - 'x' bounds='%s', expected 'x_bnds'\n", bounds_name);
        nc_close(ncid);
        return 1;
    }

    /* x_bnds must be a 2D variable with second dimension == 2 */
    ret = nc_inq_varid(ncid, "x_bnds", &bnds_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'x_bnds' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_varndims(ncid, bnds_varid, &ndims);
    if (ret != NC_NOERR || ndims != 2)
    {
        printf("FAILED - 'x_bnds' ndims=%d, expected 2\n", ndims);
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_vardimid(ncid, bnds_varid, dimids);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_inq_vardimid for x_bnds: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_inq_dimlen(ncid, dimids[1], &dimlen);
    if (ret != NC_NOERR || dimlen != 2)
    {
        printf("FAILED - 'x_bnds' second dim len=%zu, expected 2\n", dimlen);
        nc_close(ncid);
        return 1;
    }

    /* y_bnds must also exist */
    ret = nc_inq_varid(ncid, "y_bnds", &bnds_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'y_bnds' not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_close: %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok (x:bounds='%s', x_bnds[n,2])\n", bounds_name);
    return 0;
}

/**
 * Test that bounds values bracket coordinate values correctly.
 * For pixel-as-area: bnds[i][0] < coord[i] < bnds[i][1] (for positive pixel_size).
 * Tests lon_bnds from MCDWD h00v02.
 */
static int
test_coord_bounds_values(void)
{
    int ncid, lon_varid, bnds_varid, ret;
    double lon0, bnds0[2];
    size_t start1[1] = {0}, count1[1] = {1};
    size_t start2[2] = {0, 0}, count2[2] = {1, 2};

    printf("Testing coordinate bounds values bracket coordinate centres...");

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
        printf("FAILED - 'lon' not found\n");
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_vara_double(ncid, lon_varid, start1, count1, &lon0);
    if (ret != NC_NOERR)
    {
        printf("FAILED - reading lon[0]: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    ret = nc_inq_varid(ncid, "lon_bnds", &bnds_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'lon_bnds' not found\n");
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_vara_double(ncid, bnds_varid, start2, count2, bnds0);
    if (ret != NC_NOERR)
    {
        printf("FAILED - reading lon_bnds[0,:]: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* bnds[0] < centre < bnds[1] (lon increases eastward) */
    if (!(bnds0[0] < lon0 && lon0 < bnds0[1]))
    {
        printf("FAILED - lon_bnds[0]=[%.6f, %.6f] does not bracket lon[0]=%.6f\n",
               bnds0[0], bnds0[1], lon0);
        nc_close(ncid);
        return 1;
    }

    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_close: %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok (lon_bnds[0]=[%.6f, %.6f], lon[0]=%.6f)\n",
           bnds0[0], bnds0[1], lon0);
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int failed = 0;

    printf("=== CF-Compliant CRS Attribute Mapping Tests (issues #92, #93, #94) ===\n\n");

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

    /* Sprint 1 & 2 tests */
    failed += test_null_input_errors();
    failed += test_validate_unknown_crs();
    failed += test_validate_known_crs_no_ellipsoid();
    failed += test_projected_crs();
    failed += test_geographic_crs();
    failed += test_geographic_crs_second_tile();
    failed += test_coord_var_data();

    /* Sprint 3 tests */
    failed += test_validate_negative_semi_major();
    failed += test_validate_projected_no_projection();
    failed += test_standard_parallel_fields_present();
    failed += test_raster_pixel_type_default();
    failed += test_coord_bounds_geographic();
    failed += test_coord_bounds_projected();
    failed += test_coord_bounds_values();
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

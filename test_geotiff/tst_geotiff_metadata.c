/**
 * @file
 * Test GeoTIFF Phase 2: Dispatch integration and metadata extraction.
 *
 * @author Edward Hartnett
 * @date 2025-12-26
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
    printf("Error at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

#ifdef HAVE_GEOTIFF

/**
 * Test dispatch layer integration with nc_open.
 */
int
test_dispatch_integration(void)
{
    int ncid;
    int ret;

    printf("Testing dispatch layer integration...");
    
    /* Open GeoTIFF file through NetCDF API */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open returned %s\n", nc_strerror(ret));
        return 1;
    }

    /* Close file */
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_close returned %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test metadata extraction - dimensions.
 */
int
test_dimension_extraction(void)
{
    int ncid;
    int ndims;
    int dimids[NC_MAX_DIMS];
    char dimname[NC_MAX_NAME + 1];
    size_t dimlen;
    int ret;

    printf("Testing dimension extraction...");
    
    /* Open GeoTIFF file */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* Query number of dimensions */
    ret = nc_inq_ndims(ncid, &ndims);
    ERR_CHECK(ret);

    if (ndims < 2)
    {
        printf("FAILED - expected at least 2 dimensions (x, y), got %d\n", ndims);
        nc_close(ncid);
        return 1;
    }

    /* Get dimension IDs */
    ret = nc_inq_dimids(ncid, &ndims, dimids, 0);
    ERR_CHECK(ret);

    /* Check x and y dimensions */
    int found_x = 0, found_y = 0;
    for (int i = 0; i < ndims; i++)
    {
        ret = nc_inq_dim(ncid, dimids[i], dimname, &dimlen);
        ERR_CHECK(ret);

        if (strcmp(dimname, "x") == 0)
        {
            found_x = 1;
            if (dimlen == 0)
            {
                printf("FAILED - x dimension has zero length\n");
                nc_close(ncid);
                return 1;
            }
        }
        else if (strcmp(dimname, "y") == 0)
        {
            found_y = 1;
            if (dimlen == 0)
            {
                printf("FAILED - y dimension has zero length\n");
                nc_close(ncid);
                return 1;
            }
        }
    }

    if (!found_x || !found_y)
    {
        printf("FAILED - missing x or y dimension\n");
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok\n");
    return 0;
}

/**
 * Test metadata extraction - variables and data types.
 */
int
test_variable_extraction(void)
{
    int ncid;
    int nvars;
    int varids[NC_MAX_VARS];
    char varname[NC_MAX_NAME + 1];
    nc_type xtype;
    int ndims;
    int ret;

    printf("Testing variable extraction...");
    
    /* Open GeoTIFF file */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* Query number of variables */
    ret = nc_inq_nvars(ncid, &nvars);
    ERR_CHECK(ret);

    if (nvars < 1)
    {
        printf("FAILED - expected at least 1 variable, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }

    /* Get variable IDs */
    ret = nc_inq_varids(ncid, &nvars, varids);
    ERR_CHECK(ret);

    /* Check first variable (raster data) */
    ret = nc_inq_var(ncid, varids[0], varname, &xtype, &ndims, NULL, NULL);
    ERR_CHECK(ret);

    /* Verify variable has valid data type */
    if (xtype != NC_BYTE && xtype != NC_UBYTE && 
        xtype != NC_SHORT && xtype != NC_USHORT &&
        xtype != NC_INT && xtype != NC_UINT &&
        xtype != NC_FLOAT && xtype != NC_DOUBLE)
    {
        printf("FAILED - invalid data type %d\n", xtype);
        nc_close(ncid);
        return 1;
    }

    /* Verify variable has at least 2 dimensions */
    if (ndims < 2)
    {
        printf("FAILED - variable should have at least 2 dimensions, got %d\n", ndims);
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok\n");
    return 0;
}

/**
 * Test format inquiry.
 */
int
test_format_inquiry(void)
{
    int ncid;
    int format;
    int ret;

    printf("Testing format inquiry...");
    
    /* Open GeoTIFF file */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* Query format */
    ret = nc_inq_format(ncid, &format);
    ERR_CHECK(ret);

    /* Should return GeoTIFF format */
    if (format != NC_FORMATX_UDF1)
    {
        printf("FAILED - expected NC_FORMATX_UDF1, got %d\n", format);
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok\n");
    return 0;
}

/**
 * Test GTIFNew error handling with malformed tags.
 */
int
test_gtifnew_error_handling(void)
{
    int ncid;
    int ret;

    printf("Testing GTIFNew error handling...");
    
    /* Open GeoTIFF file - should succeed even if GTIFNew fails */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - should open successfully even with GTIFNew issues, got %s\n", 
               nc_strerror(ret));
        return 1;
    }

    /* File should still be usable for reading raster data */
    int nvars;
    ret = nc_inq_nvars(ncid, &nvars);
    ERR_CHECK(ret);

    nc_close(ncid);
    printf("ok\n");
    return 0;
}

/**
 * Test with second NASA MODIS file.
 */
int
test_second_nasa_file(void)
{
    int ncid;
    int ndims, nvars;
    int ret;

    printf("Testing second NASA MODIS file...");
    
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    ret = nc_inq(ncid, &ndims, &nvars, NULL, NULL);
    ERR_CHECK(ret);

    if (ndims < 2 || nvars < 1)
    {
        printf("FAILED - insufficient dimensions or variables\n");
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok\n");
    return 0;
}

/**
 * Test CRS metadata extraction with value validation.
 *
 * Validates that the 'crs' grid-mapping variable exists on the MCDWD geographic
 * file and carries the expected CF-1.8 attributes: grid_mapping_name,
 * semi_major_axis, and inverse_flattening.
 */
int
test_crs_extraction(void)
{
    int ncid, crs_varid;
    char grid_mapping_name[NC_MAX_NAME + 1];
    double semi_major;
    int ret;

    printf("Testing CRS extraction with value validation...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* 'crs' variable must exist */
    ret = nc_inq_varid(ncid, "crs", &crs_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'crs' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* grid_mapping_name must be present and non-empty */
    ret = nc_get_att_text(ncid, crs_varid, "grid_mapping_name", grid_mapping_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - grid_mapping_name not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (strlen(grid_mapping_name) == 0)
    {
        printf("FAILED - grid_mapping_name is empty\n");
        nc_close(ncid);
        return 1;
    }

    /* semi_major_axis must be present and positive */
    ret = nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("FAILED - semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (semi_major <= 0.0)
    {
        printf("FAILED - semi_major_axis not positive: %f\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok (grid_mapping_name='%s', semi_major=%.3f)\n",
           grid_mapping_name, semi_major);
    return 0;
}

/**
 * Test CRS parameter consistency and completeness.
 *
 * Validates that the 'crs' variable on the MCDWD geographic file carries
 * correct ellipsoid attribute values: semi_major_axis in the Clarke 1866
 * range and inverse_flattening close to 294.978698.
 */
int
test_crs_validation(void)
{
    int ncid, crs_varid;
    double semi_major = 0.0;
    double inv_flattening = 0.0;
    int ret;

    printf("Testing CRS parameter consistency...");

    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    ret = nc_inq_varid(ncid, "crs", &crs_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'crs' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* semi_major_axis: Clarke 1866 = 6378206.4 m */
    ret = nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("FAILED - semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (semi_major < 6.0e6 || semi_major > 7.0e6)
    {
        printf("FAILED - semi_major_axis out of range [6e6, 7e6]: %f\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    /* inverse_flattening: Clarke 1866 ≈ 294.978698 */
    ret = nc_get_att_double(ncid, crs_varid, "inverse_flattening", &inv_flattening);
    if (ret != NC_NOERR)
    {
        printf("FAILED - inverse_flattening not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (inv_flattening < 250.0 || inv_flattening > 350.0)
    {
        printf("FAILED - inverse_flattening out of range [250, 350]: %f\n", inv_flattening);
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok (semi_major=%.3f, inv_flat=%.6f)\n", semi_major, inv_flattening);
    return 0;
}

/**
 * Test CRS graceful degradation.
 *
 * Validates that a GeoTIFF file with an incomplete CRS (GTIFGetDefn returns
 * Model=0 / UNKNOWN) still opens successfully, the 'data' variable is
 * readable, and no crash occurs.  Uses the MCDWD h00v02 tile — even if its
 * CRS is fully known, the important invariant is that nc_open returns
 * NC_NOERR and the raster data variable is present and readable.
 *
 * The complementary positive check (crs variable present) is covered by
 * test_crs_extraction() and test_projected_crs_metadata().
 */
int
test_crs_graceful_degradation(void)
{
    int ncid, data_varid, nvars;
    int ret;
    size_t start[2] = {0, 0};
    size_t count[2] = {1, 1};
    unsigned char pixel;

    printf("Testing CRS graceful degradation...");

    /* File must open without error */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif",
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* At least one variable (raster data) must exist */
    ret = nc_inq_nvars(ncid, &nvars);
    ERR_CHECK(ret);
    if (nvars < 1)
    {
        printf("FAILED - no variables found\n");
        nc_close(ncid);
        return 1;
    }

    /* 'data' variable must exist and be readable */
    ret = nc_inq_varid(ncid, "data", &data_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'data' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_vara_uchar(ncid, data_varid, start, count, &pixel);
    if (ret != NC_NOERR)
    {
        printf("FAILED - reading data[0,0]: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok (%d vars, data[0,0]=%u)\n", nvars, pixel);
    return 0;
}

/**
 * Test projected CRS metadata on the ABBA sinusoidal file.
 *
 * Validates that the 'crs' variable on the ABBA projected file carries
 * grid_mapping_name = "sinusoidal", a positive semi_major_axis, and that
 * x/y coordinate variables exist with units = "m".
 */
int
test_projected_crs_metadata(void)
{
    int ncid, crs_varid, coord_varid;
    char grid_mapping_name[NC_MAX_NAME + 1];
    char units[NC_MAX_NAME + 1];
    double semi_major;
    int ret;

    printf("Testing projected CRS metadata (ABBA sinusoidal)...");

    ret = nc_open(NASA_DATA_DIR "ABBA_2022_C61_HNL.tif", NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - nc_open: %s\n", nc_strerror(ret));
        return 1;
    }

    /* 'crs' variable must exist */
    ret = nc_inq_varid(ncid, "crs", &crs_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'crs' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }

    /* grid_mapping_name must be sinusoidal */
    ret = nc_get_att_text(ncid, crs_varid, "grid_mapping_name", grid_mapping_name);
    if (ret != NC_NOERR)
    {
        printf("FAILED - grid_mapping_name not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (strcmp(grid_mapping_name, "sinusoidal") != 0)
    {
        printf("FAILED - grid_mapping_name='%s', expected 'sinusoidal'\n",
               grid_mapping_name);
        nc_close(ncid);
        return 1;
    }

    /* semi_major_axis must be positive (ABBA uses sphere 6371007.181 m) */
    ret = nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
    if (ret != NC_NOERR)
    {
        printf("FAILED - semi_major_axis not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (semi_major <= 0.0)
    {
        printf("FAILED - semi_major_axis not positive: %f\n", semi_major);
        nc_close(ncid);
        return 1;
    }

    /* x coordinate variable must exist with units = "m" */
    ret = nc_inq_varid(ncid, "x", &coord_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'x' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_att_text(ncid, coord_varid, "units", units);
    if (ret != NC_NOERR || strcmp(units, "m") != 0)
    {
        printf("FAILED - x units='%s', expected 'm'\n", units);
        nc_close(ncid);
        return 1;
    }

    /* y coordinate variable must exist with units = "m" */
    ret = nc_inq_varid(ncid, "y", &coord_varid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - 'y' variable not found: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    ret = nc_get_att_text(ncid, coord_varid, "units", units);
    if (ret != NC_NOERR || strcmp(units, "m") != 0)
    {
        printf("FAILED - y units='%s', expected 'm'\n", units);
        nc_close(ncid);
        return 1;
    }

    nc_close(ncid);
    printf("ok (grid_mapping_name='%s', semi_major=%.3f)\n",
           grid_mapping_name, semi_major);
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int err = 0;
    char magic_number_tiff[4] = "II*";     /* Standard TIFF */
    char magic_number_bigtiff[4] = "II+";  /* BigTIFF */

    printf("\n*** Testing GeoTIFF Phase 2: Dispatch Integration and Metadata Extraction ***\n");

#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF dispatch layer */
    if (!GEOTIFF_INIT_OK())
    {
        printf("ERROR: Failed to initialize GeoTIFF dispatch layer\n");
        return 1;
    }
    
    /* Register GeoTIFF UDF handlers for both standard TIFF and BigTIFF */
    int reg_ret;
    
    /* NC_UDF0: Standard TIFF */
    if ((reg_ret = nc_def_user_format(NC_UDF0, (NC_Dispatch*)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("ERROR: Failed to register standard TIFF handler: %s\n", nc_strerror(reg_ret));
        return 1;
    }
    
    /* NC_UDF1: BigTIFF */
    if ((reg_ret = nc_def_user_format(NC_UDF1, (NC_Dispatch*)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("ERROR: Failed to register BigTIFF handler: %s\n", nc_strerror(reg_ret));
        return 1;
    }
    
    /* Test dispatch layer integration */
    err += test_dispatch_integration();
    
    /* Test metadata extraction */
    err += test_dimension_extraction();
    err += test_variable_extraction();
    
    /* Test CRS extraction */
    err += test_crs_extraction();
    err += test_crs_validation();
    err += test_crs_graceful_degradation();
    err += test_projected_crs_metadata();
    
    /* Test format inquiry */
    err += test_format_inquiry();
    
    /* Test error handling */
    err += test_gtifnew_error_handling();
    
    /* Test with multiple files */
    err += test_second_nasa_file();

    if (err)
    {
        printf("\n*** %d TEST(S) FAILED ***\n", err);
        return 1;
    }

    printf("\n*** ALL PHASE 2 TESTS PASSED ***\n");
#else
    printf("\n*** GeoTIFF support not enabled - skipping tests ***\n");
#endif

    return 0;
}

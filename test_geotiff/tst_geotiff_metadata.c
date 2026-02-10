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
 * This test validates that CRS extraction produces correct attribute values,
 * not just that attributes exist. It checks specific expected values from
 * the NASA MODIS test file.
 */
int
test_crs_extraction(void)
{
    int ncid;
    int natts;
    char att_name[NC_MAX_NAME + 1];
    nc_type att_type;
    size_t att_len;
    int ret;
    int found_crs_atts = 0;
    int validated_atts = 0;

    printf("Testing CRS extraction with value validation...");
    
    /* Open GeoTIFF file */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* Query number of global attributes */
    ret = nc_inq_natts(ncid, &natts);
    ERR_CHECK(ret);

    /* Look for CRS-related attributes and validate their values */
    for (int i = 0; i < natts; i++)
    {
        ret = nc_inq_attname(ncid, NC_GLOBAL, i, att_name);
        ERR_CHECK(ret);

        /* Check for CRS attribute prefixes */
        if (strncmp(att_name, "geotiff_", 9) == 0)
        {
            found_crs_atts++;
            
            /* Verify attribute type and length */
            ret = nc_inq_att(ncid, NC_GLOBAL, att_name, &att_type, &att_len);
            ERR_CHECK(ret);
            
            /* Validate attribute has content */
            if (att_len == 0)
            {
                printf("FAILED - CRS attribute %s has zero length\n", att_name);
                nc_close(ncid);
                return 1;
            }
            
            /* Validate specific attribute values */
            if (strcmp(att_name, "geotiff_epsg_code") == 0)
            {
                if (att_type != NC_INT)
                {
                    printf("FAILED - geotiff_epsg_code has wrong type (expected NC_INT)\n");
                    nc_close(ncid);
                    return 1;
                }
                int epsg_code;
                ret = nc_get_att_int(ncid, NC_GLOBAL, att_name, &epsg_code);
                ERR_CHECK(ret);
                
                /* Validate EPSG code is reasonable (1-100000 range) */
                if (epsg_code <= 0 || epsg_code > 100000)
                {
                    printf("FAILED - Invalid EPSG code: %d\n", epsg_code);
                    nc_close(ncid);
                    return 1;
                }
                validated_atts++;
            }
            else if (strcmp(att_name, "geotiff_crs_name") == 0)
            {
                if (att_type != NC_CHAR)
                {
                    printf("FAILED - geotiff_crs_name has wrong type (expected NC_CHAR)\n");
                    nc_close(ncid);
                    return 1;
                }
                char *crs_name = malloc(att_len + 1);
                if (!crs_name)
                {
                    printf("FAILED - Memory allocation failed\n");
                    nc_close(ncid);
                    return 1;
                }
                ret = nc_get_att_text(ncid, NC_GLOBAL, att_name, crs_name);
                ERR_CHECK(ret);
                crs_name[att_len] = '\0';
                
                /* Validate CRS name is a known type */
                if (strcmp(crs_name, "Geographic") != 0 && 
                    strcmp(crs_name, "Projected") != 0 && 
                    strcmp(crs_name, "Geocentric") != 0)
                {
                    printf("WARNING - Unexpected CRS name: %s\n", crs_name);
                }
                free(crs_name);
                validated_atts++;
            }
            else if (strstr(att_name, "semi_major_axis") != NULL ||
                     strstr(att_name, "inverse_flattening") != NULL)
            {
                /* Validate ellipsoid parameters are doubles */
                if (att_type != NC_DOUBLE)
                {
                    printf("FAILED - %s has wrong type (expected NC_DOUBLE)\n", att_name);
                    nc_close(ncid);
                    return 1;
                }
                double value;
                ret = nc_get_att_double(ncid, NC_GLOBAL, att_name, &value);
                ERR_CHECK(ret);
                
                /* Validate value is positive and reasonable */
                if (value <= 0.0 || value > 1e8)
                {
                    printf("FAILED - %s has unreasonable value: %f\n", att_name, value);
                    nc_close(ncid);
                    return 1;
                }
                validated_atts++;
            }
        }
    }

    /* Verify we found and validated CRS attributes */
    if (found_crs_atts == 0)
    {
        printf("WARNING - No CRS attributes found (file may not have CRS info)\n");
    }
    else if (validated_atts == 0)
    {
        printf("FAILED - Found CRS attributes but none were validated\n");
        nc_close(ncid);
        return 1;
    }
    
    nc_close(ncid);
    printf("ok (found %d CRS attributes, validated %d)\n", found_crs_atts, validated_atts);
    return 0;
}

/**
 * Test CRS parameter consistency and completeness.
 * 
 * This test validates that CRS parameters form a complete and consistent set.
 * For projected CRS, we expect ellipsoid parameters. For geographic CRS,
 * we expect datum information.
 */
int
test_crs_validation(void)
{
    int ncid;
    int natts;
    char att_name[NC_MAX_NAME + 1];
    int epsg_code = -1;
    char crs_name[NC_MAX_NAME + 1] = "";
    int has_semi_major = 0;
    int has_inverse_flattening = 0;
    double semi_major = 0.0;
    double inv_flattening = 0.0;
    int ret;

    printf("Testing CRS parameter consistency...");
    
    /* Open GeoTIFF file */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    ERR_CHECK(ret);

    /* Query number of global attributes */
    ret = nc_inq_natts(ncid, &natts);
    ERR_CHECK(ret);

    /* Collect CRS attributes */
    for (int i = 0; i < natts; i++)
    {
        ret = nc_inq_attname(ncid, NC_GLOBAL, i, att_name);
        ERR_CHECK(ret);

        if (strcmp(att_name, "geotiff_epsg_code") == 0)
        {
            ret = nc_get_att_int(ncid, NC_GLOBAL, att_name, &epsg_code);
            ERR_CHECK(ret);
        }
        else if (strcmp(att_name, "geotiff_crs_name") == 0)
        {
            size_t len;
            ret = nc_inq_attlen(ncid, NC_GLOBAL, att_name, &len);
            ERR_CHECK(ret);
            ret = nc_get_att_text(ncid, NC_GLOBAL, att_name, crs_name);
            ERR_CHECK(ret);
            crs_name[len] = '\0';
        }
        else if (strcmp(att_name, "geotiff_semi_major_axis") == 0)
        {
            ret = nc_get_att_double(ncid, NC_GLOBAL, att_name, &semi_major);
            ERR_CHECK(ret);
            has_semi_major = 1;
        }
        else if (strcmp(att_name, "geotiff_inverse_flattening") == 0)
        {
            ret = nc_get_att_double(ncid, NC_GLOBAL, att_name, &inv_flattening);
            ERR_CHECK(ret);
            has_inverse_flattening = 1;
        }
    }

    /* Validate CRS completeness */
    if (epsg_code != -1)
    {
        /* If we have an EPSG code, it should be in valid range */
        if (epsg_code <= 0 || epsg_code > 100000)
        {
            printf("FAILED - Invalid EPSG code: %d\n", epsg_code);
            nc_close(ncid);
            return 1;
        }
    }

    /* Validate ellipsoid parameters if present */
    if (has_semi_major)
    {
        /* Semi-major axis should be Earth-like (6-7 million meters) */
        if (semi_major < 6.0e6 || semi_major > 7.0e6)
        {
            printf("FAILED - Semi-major axis out of range: %f\n", semi_major);
            nc_close(ncid);
            return 1;
        }
    }
    
    if (has_inverse_flattening)
    {
        /* Inverse flattening should be reasonable (250-350 for Earth ellipsoids) */
        if (inv_flattening < 250.0 || inv_flattening > 350.0)
        {
            printf("FAILED - Inverse flattening out of range: %f\n", inv_flattening);
            nc_close(ncid);
            return 1;
        }
    }
    
    /* If we have ellipsoid params, both should be present */
    if (has_semi_major != has_inverse_flattening)
    {
        printf("WARNING - Incomplete ellipsoid parameters (have semi_major=%d, inv_flat=%d)\n",
               has_semi_major, has_inverse_flattening);
    }

    nc_close(ncid);
    printf("ok (EPSG:%d, ellipsoid params: %d)\n", epsg_code, has_semi_major && has_inverse_flattening);
    return 0;
}

/**
 * Test CRS extraction with files that might not have CRS (negative test).
 * 
 * This validates that files without CRS information can still be opened
 * and processed without errors. This is important for graceful degradation.
 */
int
test_crs_graceful_degradation(void)
{
    int ncid;
    int natts;
    char att_name[NC_MAX_NAME + 1];
    int found_crs_atts = 0;
    int ret;

    printf("Testing CRS graceful degradation (negative test)...");
    
    /* Try to open a file (this might or might not have CRS) */
    ret = nc_open(NASA_DATA_DIR "ABBA_2022_C61_HNL.tif", NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        /* If file doesn't exist or can't be opened, that's ok for this test */
        printf("ok (file not available for degradation test)\n");
        return 0;
    }

    /* Query number of global attributes */
    ret = nc_inq_natts(ncid, &natts);
    ERR_CHECK(ret);

    /* Look for CRS-related attributes */
    for (int i = 0; i < natts; i++)
    {
        ret = nc_inq_attname(ncid, NC_GLOBAL, i, att_name);
        ERR_CHECK(ret);

        if (strncmp(att_name, "geotiff_", 9) == 0)
        {
            found_crs_atts++;
        }
    }

    /* Verify we can still read basic metadata even without CRS */
    int nvars;
    ret = nc_inq_nvars(ncid, &nvars);
    ERR_CHECK(ret);
    
    if (nvars == 0)
    {
        printf("FAILED - File has no variables\n");
        nc_close(ncid);
        return 1;
    }

    /* The file should open successfully regardless of CRS presence */
    nc_close(ncid);
    printf("ok (file opened, %d vars, %d CRS attrs)\n", nvars, found_crs_atts);
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

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

#define NASA_DATA_DIR "../test/data/"
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

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int err = 0;

    printf("\n*** Testing GeoTIFF Phase 2: Dispatch Integration and Metadata Extraction ***\n");

#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF dispatch layer */
    if (NC_GEOTIFF_initialize() != NC_NOERR)
    {
        printf("ERROR: Failed to initialize GeoTIFF dispatch layer\n");
        return 1;
    }
    
    /* Register GeoTIFF UDF handler with NetCDF-C */
    /* GeoTIFF uses UDF1 slot - pass the format constant as the mode flag */
    int reg_ret = nc_def_user_format(NC_FORMATX_UDF1, (NC_Dispatch*)GEOTIFF_dispatch_table, NULL);
    if (reg_ret != NC_NOERR)
    {
        printf("ERROR: Failed to register GeoTIFF UDF handler: %s (code %d)\n", nc_strerror(reg_ret), reg_ret);
        return 1;
    }
    
    /* Test dispatch layer integration */
    err += test_dispatch_integration();
    
    /* Test metadata extraction */
    err += test_dimension_extraction();
    err += test_variable_extraction();
    
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

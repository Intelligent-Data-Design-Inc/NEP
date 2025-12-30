/**
 * @file
 * Test GeoTIFF file handle and resource management.
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

#ifdef HAVE_GEOTIFF
#include <tiffio.h>
#include <geotiff/geotiff.h>
#endif

#define TEST_DATA_DIR "data/"
#define NASA_DATA_DIR "./"
#define ERR_CHECK(ret) do { if ((ret) != NC_NOERR) { \
    printf("Error at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

int total_err = 0;

#ifdef HAVE_GEOTIFF

/* Dump the metadata from a netCDF file. */
static int
dump_netcdf_file(int ncid)
{
    int ndims, nvars, ngatts, unlimdimid;
    int ret;
    
    /* Get global file information */
    if ((ret = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
    {
        printf("Error in nc_inq: %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("\n=== NetCDF File Metadata ===\n");
    printf("Number of dimensions: %d\n", ndims);
    printf("Number of variables: %d\n", nvars);
    printf("Number of global attributes: %d\n", ngatts);
    printf("Unlimited dimension ID: %d\n", unlimdimid);
    
    /* Print dimension information */
    printf("\n--- Dimensions ---\n");
    for (int i = 0; i < ndims; i++)
    {
        char dimname[NC_MAX_NAME + 1];
        size_t dimlen;
        
        if ((ret = nc_inq_dim(ncid, i, dimname, &dimlen)))
        {
            printf("Error in nc_inq_dim for dim %d: %s\n", i, nc_strerror(ret));
            return 1;
        }
        
        printf("  Dimension %d: %s = %zu%s\n", i, dimname, dimlen,
               (i == unlimdimid) ? " (UNLIMITED)" : "");
    }
    
    /* Print global attributes */
    printf("\n--- Global Attributes ---\n");
    for (int i = 0; i < ngatts; i++)
    {
        char attname[NC_MAX_NAME + 1];
        nc_type atttype;
        size_t attlen;
        
        if ((ret = nc_inq_attname(ncid, NC_GLOBAL, i, attname)))
        {
            printf("Error in nc_inq_attname for global att %d: %s\n", i, nc_strerror(ret));
            return 1;
        }
        
        if ((ret = nc_inq_att(ncid, NC_GLOBAL, attname, &atttype, &attlen)))
        {
            printf("Error in nc_inq_att for %s: %s\n", attname, nc_strerror(ret));
            return 1;
        }
        
        printf("  %s: type=%d, len=%zu\n", attname, atttype, attlen);
        
        /* Print attribute value based on type */
        if (atttype == NC_CHAR)
        {
            char *attval = malloc(attlen + 1);
            if (attval)
            {
                if ((ret = nc_get_att_text(ncid, NC_GLOBAL, attname, attval)) == NC_NOERR)
                {
                    attval[attlen] = '\0';
                    printf("    Value: \"%s\"\n", attval);
                }
                free(attval);
            }
        }
        else if (atttype == NC_INT)
        {
            int *attval = malloc(attlen * sizeof(int));
            if (attval)
            {
                if ((ret = nc_get_att_int(ncid, NC_GLOBAL, attname, attval)) == NC_NOERR)
                {
                    printf("    Value: ");
                    for (size_t j = 0; j < attlen; j++)
                        printf("%d%s", attval[j], (j < attlen - 1) ? ", " : "\n");
                }
                free(attval);
            }
        }
        else if (atttype == NC_FLOAT)
        {
            float *attval = malloc(attlen * sizeof(float));
            if (attval)
            {
                if ((ret = nc_get_att_float(ncid, NC_GLOBAL, attname, attval)) == NC_NOERR)
                {
                    printf("    Value: ");
                    for (size_t j = 0; j < attlen; j++)
                        printf("%f%s", attval[j], (j < attlen - 1) ? ", " : "\n");
                }
                free(attval);
            }
        }
        else if (atttype == NC_DOUBLE)
        {
            double *attval = malloc(attlen * sizeof(double));
            if (attval)
            {
                if ((ret = nc_get_att_double(ncid, NC_GLOBAL, attname, attval)) == NC_NOERR)
                {
                    printf("    Value: ");
                    for (size_t j = 0; j < attlen; j++)
                        printf("%f%s", attval[j], (j < attlen - 1) ? ", " : "\n");
                }
                free(attval);
            }
        }
    }
    
    /* Print variable information */
    printf("\n--- Variables ---\n");
    for (int i = 0; i < nvars; i++)
    {
        char varname[NC_MAX_NAME + 1];
        nc_type vartype;
        int varndims, vardimids[NC_MAX_VAR_DIMS], varnatts;
        
        if ((ret = nc_inq_var(ncid, i, varname, &vartype, &varndims, vardimids, &varnatts)))
        {
            printf("Error in nc_inq_var for var %d: %s\n", i, nc_strerror(ret));
            return 1;
        }
        
        printf("  Variable %d: %s\n", i, varname);
        printf("    Type: %d\n", vartype);
        printf("    Dimensions: %d [", varndims);
        for (int j = 0; j < varndims; j++)
        {
            char dimname[NC_MAX_NAME + 1];
            if ((ret = nc_inq_dimname(ncid, vardimids[j], dimname)) == NC_NOERR)
                printf("%s%s", dimname, (j < varndims - 1) ? ", " : "");
        }
        printf("]\n");
        printf("    Number of attributes: %d\n", varnatts);
        
        /* Print variable attributes */
        for (int j = 0; j < varnatts; j++)
        {
            char attname[NC_MAX_NAME + 1];
            nc_type atttype;
            size_t attlen;
            
            if ((ret = nc_inq_attname(ncid, i, j, attname)))
            {
                printf("Error in nc_inq_attname for var %d att %d: %s\n", i, j, nc_strerror(ret));
                return 1;
            }
            
            if ((ret = nc_inq_att(ncid, i, attname, &atttype, &attlen)))
            {
                printf("Error in nc_inq_att for var %d att %s: %s\n", i, attname, nc_strerror(ret));
                return 1;
            }
            
            printf("      %s: type=%d, len=%zu\n", attname, atttype, attlen);
            
            /* Print attribute value based on type */
            if (atttype == NC_CHAR)
            {
                char *attval = malloc(attlen + 1);
                if (attval)
                {
                    if ((ret = nc_get_att_text(ncid, i, attname, attval)) == NC_NOERR)
                    {
                        attval[attlen] = '\0';
                        printf("        Value: \"%s\"\n", attval);
                    }
                    free(attval);
                }
            }
            else if (atttype == NC_INT)
            {
                int *attval = malloc(attlen * sizeof(int));
                if (attval)
                {
                    if ((ret = nc_get_att_int(ncid, i, attname, attval)) == NC_NOERR)
                    {
                        printf("        Value: ");
                        for (size_t k = 0; k < attlen; k++)
                            printf("%d%s", attval[k], (k < attlen - 1) ? ", " : "\n");
                    }
                    free(attval);
                }
            }
            else if (atttype == NC_FLOAT)
            {
                float *attval = malloc(attlen * sizeof(float));
                if (attval)
                {
                    if ((ret = nc_get_att_float(ncid, i, attname, attval)) == NC_NOERR)
                    {
                        printf("        Value: ");
                        for (size_t k = 0; k < attlen; k++)
                            printf("%f%s", attval[k], (k < attlen - 1) ? ", " : "\n");
                    }
                    free(attval);
                }
            }
            else if (atttype == NC_DOUBLE)
            {
                double *attval = malloc(attlen * sizeof(double));
                if (attval)
                {
                    if ((ret = nc_get_att_double(ncid, i, attname, attval)) == NC_NOERR)
                    {
                        printf("        Value: ");
                        for (size_t k = 0; k < attlen; k++)
                            printf("%f%s", attval[k], (k < attlen - 1) ? ", " : "\n");
                    }
                    free(attval);
                }
            }
        }
    }
    
    printf("\n");
    return 0;
}

/**
 * Test successful file handle creation with real NASA GeoTIFF.
 */
int
test_successful_open_close(void)
{
    int ncid;
    int ret;

    printf("Testing successful open with NASA MODIS file...");
    
    /* Test with real NASA MODIS GeoTIFF file using nc_open() */
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - open returned %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = dump_netcdf_file(ncid)))
	return 1;
    
    /* Close the file */
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - close returned %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test error handling for invalid file path.
 */
int
test_invalid_file_path(void)
{
    int ncid;
    int ret;

    printf("Testing invalid file path...");
    
    if (!(ret = nc_open(TEST_DATA_DIR "nonexistent.tif", NC_NOWRITE, &ncid)))
    {
        printf("FAILED - should return error, got %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test error handling for non-GeoTIFF file.
 */
int
test_non_geotiff_file(void)
{
    int ncid;
    int ret;

    printf("Testing non-GeoTIFF file rejection...");
    
    /* Regular TIFF without GeoTIFF tags should be rejected */
    ret = nc_open(TEST_DATA_DIR "regular.tif", NC_NOWRITE, &ncid);
    if (ret != NC_ENOTNC)
    {
        printf("FAILED - should return NC_ENOTNC, got %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test error handling for write mode.
 */
int
test_write_mode_rejection(void)
{
    int ncid;
    int ret;

    printf("Testing write mode rejection...");
    
    ret = nc_open(TEST_DATA_DIR "le_geotiff.tif", NC_WRITE, &ncid);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - should return NC_EINVAL, got %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test error handling for NULL path.
 */
int
test_null_path(void)
{
    int ncid;
    int ret;

    printf("Testing NULL path parameter...");
    
    ret = nc_open(NULL, NC_NOWRITE, &ncid);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - should return NC_EINVAL, got %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test error handling for minimal/incomplete GeoTIFF files.
 * Synthetic test files may have GeoTIFF tags but lack required TIFF structure.
 */
int
test_minimal_geotiff_handling(void)
{
    int ncid;
    int ret;

    printf("Testing minimal GeoTIFF file handling...");
    
    /* Minimal synthetic files may fail TIFFOpen even if they have GeoTIFF tags */
    /* This tests that we handle TIFFOpen failures gracefully */
    ret = nc_open(TEST_DATA_DIR "le_geotiff.tif", NC_NOWRITE, &ncid);
    /* Should return NC_ENOTNC because TIFFOpen will fail on minimal file */
    if (ret != NC_ENOTNC)
    {
        printf("FAILED - expected NC_ENOTNC for minimal file, got %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test with second NASA MODIS GeoTIFF file.
 */
int
test_nasa_modis_file2(void)
{
    int ncid;
    int ret;

    printf("Testing NASA MODIS file 2...");
    ret = nc_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif", 
                  NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - open returned %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = dump_netcdf_file(ncid)))
	return 1;
    
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED - close returned %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");

    return 0;
}

/**
 * Test abort function.
 */
int
test_abort(void)
{
    int ret;

    printf("Testing abort function...");
    
    /* Abort should behave like close */
    ret = NC_GEOTIFF_abort(0);
    if (ret != NC_EBADID)
    {
        printf("FAILED - abort should return NC_EBADID (expected for Phase 1)\n");
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test format inquiry functions.
 */
int
test_format_inquiry(void)
{
    int format = 0;
    int mode = 0;
    int ret;

    printf("Testing format inquiry...");
    
    ret = NC_GEOTIFF_inq_format(0, &format);
    if (ret != NC_NOERR)
    {
        printf("FAILED - inq_format returned %s\n", nc_strerror(ret));
        return 1;
    }
    if (format != NC_FORMATX_NC_GEOTIFF)
    {
        printf("FAILED - wrong format value %d\n", format);
        return 1;
    }

    ret = NC_GEOTIFF_inq_format_extended(0, &format, &mode);
    if (ret != NC_NOERR)
    {
        printf("FAILED - inq_format_extended returned %s\n", nc_strerror(ret));
        return 1;
    }
    if (format != NC_FORMATX_NC_GEOTIFF || mode != NC_FORMATX_NC_GEOTIFF)
    {
        printf("FAILED - wrong format/mode values %d/%d\n", format, mode);
        return 1;
    }

    printf("ok\n");
    return 0;
}

/**
 * Test initialize and finalize functions.
 */
int
test_initialize_finalize(void)
{
    int ret;

    printf("Testing initialize/finalize...");
    
    ret = NC_GEOTIFF_initialize();
    if (ret != NC_NOERR)
    {
        printf("FAILED - initialize returned %s\n", nc_strerror(ret));
        return 1;
    }

    ret = NC_GEOTIFF_finalize();
    if (ret != NC_NOERR)
    {
        printf("FAILED - finalize returned %s\n", nc_strerror(ret));
        return 1;
    }

    printf("ok\n");
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(void)
{
    int err = 0;
    char magic_number_tiff[4] = "II*";     /* Standard TIFF */
    char magic_number_bigtiff[4] = "II+";  /* BigTIFF */

    printf("\n*** Testing GeoTIFF file handle management ***\n");

#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF dispatch layer */
    if (NC_GEOTIFF_initialize() != NC_NOERR)
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
    
    /* Test basic functionality with real NASA files */
    err += test_successful_open_close();
    err += test_nasa_modis_file2();
    
    /* Test error handling */
    err += test_invalid_file_path();
    err += test_non_geotiff_file();
    err += test_write_mode_rejection();
    err += test_null_path();
    err += test_minimal_geotiff_handling();
    
    /* Test other functions */
    err += test_abort();
    err += test_format_inquiry();
    err += test_initialize_finalize();

    if (err)
    {
        printf("\n*** %d TEST(S) FAILED ***\n", err);
        return 1;
    }

    printf("\n*** ALL TESTS PASSED ***\n");
#else
    printf("\n*** GeoTIFF support not enabled - skipping tests ***\n");
#endif

    return 0;
}

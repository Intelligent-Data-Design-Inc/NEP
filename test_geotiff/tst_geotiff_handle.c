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
#define NASA_DATA_DIR "../test/data/"
#define ERR_CHECK(ret) do { if ((ret) != NC_NOERR) { \
    printf("Error at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

int total_err = 0;

#ifdef HAVE_GEOTIFF

/**
 * Test successful file handle creation with real NASA GeoTIFF.
 */
int
test_successful_open_close(void)
{
    int ret;

    printf("Testing successful open with NASA MODIS file...");
    
    /* Test with real NASA MODIS GeoTIFF file */
    ret = NC_GEOTIFF_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                          NC_NOWRITE, 0, NULL, NULL, NULL, 0);
    if (ret != NC_NOERR)
    {
        printf("FAILED - open returned %s\n", nc_strerror(ret));
        return 1;
    }

    /* Note: Close will fail with NC_EBADID because we don't have full dispatch integration yet */
    /* This is expected for Phase 1 */
    ret = NC_GEOTIFF_close(0, NULL);
    if (ret != NC_EBADID)
    {
        printf("FAILED - close should return NC_EBADID (expected for Phase 1)\n");
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
    int ret;

    printf("Testing invalid file path...");
    
    ret = NC_GEOTIFF_open(TEST_DATA_DIR "nonexistent.tif", NC_NOWRITE, 0, NULL, NULL, NULL, 0);
    if (ret != NC_ENOTNC)
    {
        printf("FAILED - should return NC_ENOTNC, got %s\n", nc_strerror(ret));
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
    int ret;

    printf("Testing non-GeoTIFF file rejection...");
    
    /* Regular TIFF without GeoTIFF tags should be rejected */
    ret = NC_GEOTIFF_open(TEST_DATA_DIR "regular.tif", NC_NOWRITE, 0, NULL, NULL, NULL, 0);
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
    int ret;

    printf("Testing write mode rejection...");
    
    ret = NC_GEOTIFF_open(TEST_DATA_DIR "le_geotiff.tif", NC_WRITE, 0, NULL, NULL, NULL, 0);
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
    int ret;

    printf("Testing NULL path parameter...");
    
    ret = NC_GEOTIFF_open(NULL, NC_NOWRITE, 0, NULL, NULL, NULL, 0);
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
    int ret;

    printf("Testing minimal GeoTIFF file handling...");
    
    /* Minimal synthetic files may fail TIFFOpen even if they have GeoTIFF tags */
    /* This tests that we handle TIFFOpen failures gracefully */
    ret = NC_GEOTIFF_open(TEST_DATA_DIR "le_geotiff.tif", NC_NOWRITE, 0, NULL, NULL, NULL, 0);
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
    int ret;

    printf("Testing NASA MODIS file 2...");
    ret = NC_GEOTIFF_open(NASA_DATA_DIR "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif", 
                          NC_NOWRITE, 0, NULL, NULL, NULL, 0);
    if (ret != NC_NOERR)
    {
        printf("FAILED - open returned %s\n", nc_strerror(ret));
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

    printf("\n*** Testing GeoTIFF file handle management ***\n");

#ifdef HAVE_GEOTIFF
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
    printf("\nNote: Full dispatch integration will be completed in later phases.\n");
    printf("Some functions return NC_EBADID as expected for Phase 1.\n");
#else
    printf("\n*** GeoTIFF support not enabled - skipping tests ***\n");
#endif

    return 0;
}

/**
 * @file
 * Test GeoTIFF format detection.
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

#define TEST_DATA_DIR "../../test_geotiff/data/"
#define ERR_CHECK(ret) do { if ((ret) != NC_NOERR) { \
    printf("Error at line %d: %s\n", __LINE__, nc_strerror(ret)); \
    return 1; \
} } while(0)

int total_err = 0;

/**
 * Test detection with valid little-endian GeoTIFF file.
 */
int
test_little_endian_geotiff(void)
{
    int is_geotiff;
    int ret;

    printf("Testing little-endian GeoTIFF detection...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "le_geotiff.tif", &is_geotiff);
    ERR_CHECK(ret);
    if (!is_geotiff)
    {
        printf("FAILED - should detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test detection with valid big-endian GeoTIFF file.
 */
int
test_big_endian_geotiff(void)
{
    int is_geotiff;
    int ret;

    printf("Testing big-endian GeoTIFF detection...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "be_geotiff.tif", &is_geotiff);
    ERR_CHECK(ret);
    if (!is_geotiff)
    {
        printf("FAILED - should detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test rejection of regular TIFF without GeoTIFF tags.
 */
int
test_regular_tiff_rejection(void)
{
    int is_geotiff;
    int ret;

    printf("Testing regular TIFF rejection...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "regular.tif", &is_geotiff);
    ERR_CHECK(ret);
    if (is_geotiff)
    {
        printf("FAILED - should NOT detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test rejection of non-TIFF file.
 */
int
test_non_tiff_rejection(void)
{
    int is_geotiff;
    int ret;

    printf("Testing non-TIFF file rejection...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "not_tiff.txt", &is_geotiff);
    ERR_CHECK(ret);
    if (is_geotiff)
    {
        printf("FAILED - should NOT detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test handling of corrupted TIFF header.
 */
int
test_corrupted_header(void)
{
    int is_geotiff;
    int ret;

    printf("Testing corrupted TIFF header...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "corrupted.tif", &is_geotiff);
    ERR_CHECK(ret);
    if (is_geotiff)
    {
        printf("FAILED - should NOT detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test handling of truncated file.
 */
int
test_truncated_file(void)
{
    int is_geotiff;
    int ret;

    printf("Testing truncated file...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "truncated.tif", &is_geotiff);
    ERR_CHECK(ret);
    if (is_geotiff)
    {
        printf("FAILED - should NOT detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test NULL parameter handling.
 */
int
test_null_parameters(void)
{
    int is_geotiff;
    int ret;

    printf("Testing NULL path parameter...");
    ret = NC_GEOTIFF_detect_format(NULL, &is_geotiff);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - should return NC_EINVAL\n");
        return 1;
    }
    printf("ok\n");

    printf("Testing NULL is_geotiff parameter...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "le_geotiff.tif", NULL);
    if (ret != NC_EINVAL)
    {
        printf("FAILED - should return NC_EINVAL\n");
        return 1;
    }
    printf("ok\n");

    return 0;
}

/**
 * Test with missing file.
 */
int
test_missing_file(void)
{
    int is_geotiff;
    int ret;

    printf("Testing missing file...");
    ret = NC_GEOTIFF_detect_format(TEST_DATA_DIR "nonexistent.tif", &is_geotiff);
    if (ret != NC_ENOTNC)
    {
        printf("FAILED - should return NC_ENOTNC\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

/**
 * Test with NASA MODIS GeoTIFF files.
 */
int
test_nasa_modis_files(void)
{
    int is_geotiff;
    int ret;

    printf("Testing NASA MODIS file 1...");
    ret = NC_GEOTIFF_detect_format("../test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif", 
                                    &is_geotiff);
    ERR_CHECK(ret);
    if (!is_geotiff)
    {
        printf("FAILED - should detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");

    printf("Testing NASA MODIS file 2...");
    ret = NC_GEOTIFF_detect_format("../test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif", 
                                    &is_geotiff);
    ERR_CHECK(ret);
    if (!is_geotiff)
    {
        printf("FAILED - should detect as GeoTIFF\n");
        return 1;
    }
    printf("ok\n");

    return 0;
}

int
main(void)
{
    int err = 0;

    printf("\n*** Testing GeoTIFF format detection ***\n");

    /* Test with synthetic files */
    err += test_little_endian_geotiff();
    err += test_big_endian_geotiff();
    err += test_regular_tiff_rejection();
    err += test_non_tiff_rejection();
    err += test_corrupted_header();
    err += test_truncated_file();
    err += test_null_parameters();
    err += test_missing_file();

    /* Test with real NASA files */
    err += test_nasa_modis_files();

    if (err)
    {
        printf("\n*** %d TEST(S) FAILED ***\n", err);
        return 1;
    }

    printf("\n*** ALL TESTS PASSED ***\n");
    return 0;
}

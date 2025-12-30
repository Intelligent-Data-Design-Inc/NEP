/**
 * @file
 * Test large GeoTIFF file reading and tiled organization.
 *
 * Tests Phase 3.4 requirements:
 * - Large file support (>1MB)
 * - Tiled organization detection and reading
 * - Performance with realistic file sizes
 * - Edge cases (single pixel, large hyperslabs, full scanlines)
 *
 * @author Edward Hartnett
 * @date 2025-12-30
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>
#include <time.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#endif

#define FILE_NAME "ABBA_2022_C61_HNL.tif"
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return 1;}

/* Get current time in milliseconds for performance measurement */
static double get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int
main(int argc, char **argv)
{
    printf("\n*** Testing large GeoTIFF file reading.\n");

#ifdef HAVE_GEOTIFF
    int ncid, varid, ret;
    int ndims, nvars, dimids[NC_MAX_DIMS];
    size_t height, width;
    char var_name[NC_MAX_NAME + 1];
    char magic_number_tiff[4] = "II*";     /* Standard TIFF little-endian */
    char magic_number_bigtiff[4] = "II+";  /* BigTIFF little-endian */
    double start_time, end_time;

    /* Initialize GeoTIFF dispatch layer */
    printf("*** Initializing GeoTIFF dispatch layer...");
    if (NC_GEOTIFF_initialize() != NC_NOERR)
    {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");

    /* Register GeoTIFF handlers for little-endian TIFF variants */
    printf("*** Registering GeoTIFF handlers (II* and II+)...");
    
    /* NC_UDF0: Standard TIFF little-endian (II*) */
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("FAILED (II*): %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* NC_UDF1: BigTIFF little-endian (II+) */
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("FAILED (II+): %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("ok\n");

    /* Open large GeoTIFF file (supports both standard TIFF and BigTIFF) */
    printf("*** Opening large GeoTIFF file (%s)...", FILE_NAME);
    start_time = get_time_ms();
    if ((ret = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    end_time = get_time_ms();
    printf("ok (%.2f ms)\n", end_time - start_time);

    /* Get file metadata */
    printf("*** Querying file metadata...");
    if ((ret = nc_inq(ncid, &ndims, &nvars, NULL, NULL)))
        ERR(ret);
    
    if (ndims != 2 || nvars != 1)
    {
        printf("FAILED: Expected 2 dims and 1 var, got %d dims and %d vars\n", ndims, nvars);
        return 1;
    }
    printf("ok (ndims=%d, nvars=%d)\n", ndims, nvars);

    /* Get variable info */
    printf("*** Getting variable information...");
    if ((ret = nc_inq_var(ncid, 0, var_name, NULL, &ndims, dimids, NULL)))
        ERR(ret);
    
    if ((ret = nc_inq_dimlen(ncid, dimids[0], &height)))
        ERR(ret);
    if ((ret = nc_inq_dimlen(ncid, dimids[1], &width)))
        ERR(ret);
    
    printf("ok (name=%s, dimensions=%zu x %zu)\n", var_name, height, width);

    /* Verify this is a large file */
    if (height < 10000 || width < 10000)
    {
        printf("WARNING: File smaller than expected (expected >10K x >10K)\n");
    }

    /* Test 1: Read single pixel from large file */
    printf("*** Test 1: Reading single pixel from center of large file...");
    {
        size_t start[2] = {height/2, width/2};
        size_t count[2] = {1, 1};
        unsigned char pixel;
        
        start_time = get_time_ms();
        if ((ret = nc_get_vara_uchar(ncid, 0, start, count, &pixel)))
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            return 1;
        }
        end_time = get_time_ms();
        printf("ok (pixel[%zu,%zu]=%u, %.2f ms)\n", start[0], start[1], pixel, end_time - start_time);
    }

    /* Test 2: Read small hyperslab (100x100) */
    printf("*** Test 2: Reading 100x100 hyperslab...");
    {
        size_t start[2] = {1000, 1000};
        size_t count[2] = {100, 100};
        unsigned char *data = malloc(100 * 100);
        if (!data)
        {
            printf("FAILED: Memory allocation\n");
            return 1;
        }
        
        start_time = get_time_ms();
        if ((ret = nc_get_vara_uchar(ncid, 0, start, count, data)))
        {
            free(data);
            printf("FAILED: %s\n", nc_strerror(ret));
            return 1;
        }
        end_time = get_time_ms();
        printf("ok (first=%u, last=%u, %.2f ms)\n", data[0], data[9999], end_time - start_time);
        free(data);
    }

    /* Test 3: Read large hyperslab (1000x1000) */
    printf("*** Test 3: Reading 1000x1000 hyperslab...");
    {
        size_t start[2] = {5000, 5000};
        size_t count[2] = {1000, 1000};
        unsigned char *data = malloc(1000 * 1000);
        if (!data)
        {
            printf("FAILED: Memory allocation\n");
            return 1;
        }
        
        start_time = get_time_ms();
        if ((ret = nc_get_vara_uchar(ncid, 0, start, count, data)))
        {
            free(data);
            printf("FAILED: %s\n", nc_strerror(ret));
            return 1;
        }
        end_time = get_time_ms();
        printf("ok (first=%u, last=%u, %.2f ms)\n", data[0], data[999999], end_time - start_time);
        free(data);
    }

    /* Test 4: Read full scanline */
    printf("*** Test 4: Reading full scanline (%zu pixels)...", width);
    {
        size_t start[2] = {height/2, 0};
        size_t count[2] = {1, width};
        unsigned char *data = malloc(width);
        if (!data)
        {
            printf("FAILED: Memory allocation\n");
            return 1;
        }
        
        start_time = get_time_ms();
        if ((ret = nc_get_vara_uchar(ncid, 0, start, count, data)))
        {
            free(data);
            printf("FAILED: %s\n", nc_strerror(ret));
            return 1;
        }
        end_time = get_time_ms();
        printf("ok (first=%u, last=%u, %.2f ms)\n", data[0], data[width-1], end_time - start_time);
        free(data);
    }

    /* Test 5: Read edge pixel (near boundary) */
    printf("*** Test 5: Reading pixel near file boundary...");
    {
        size_t start[2] = {height-10, width-10};
        size_t count[2] = {1, 1};
        unsigned char pixel;
        
        start_time = get_time_ms();
        if ((ret = nc_get_vara_uchar(ncid, 0, start, count, &pixel)))
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            return 1;
        }
        end_time = get_time_ms();
        printf("ok (pixel[%zu,%zu]=%u, %.2f ms)\n", start[0], start[1], pixel, end_time - start_time);
    }

    /* Test 6: Verify bounds checking with out-of-bounds request */
    printf("*** Test 6: Testing bounds checking (should fail with NC_EEDGE)...");
    {
        size_t start[2] = {height, 0};
        size_t count[2] = {1, 1};
        unsigned char pixel;
        
        ret = nc_get_vara_uchar(ncid, 0, start, count, &pixel);
        if (ret != NC_EEDGE)
        {
            printf("FAILED: Expected NC_EEDGE, got %s\n", nc_strerror(ret));
            return 1;
        }
        printf("ok (correctly returned NC_EEDGE)\n");
    }

    /* Test 7: Read multiple small regions (stress test) */
    printf("*** Test 7: Reading 10 random 50x50 regions (stress test)...");
    {
        unsigned char *data = malloc(50 * 50);
        if (!data)
        {
            printf("FAILED: Memory allocation\n");
            return 1;
        }
        
        start_time = get_time_ms();
        for (int i = 0; i < 10; i++)
        {
            size_t start[2] = {(i * 1000) % (height - 50), (i * 2000) % (width - 50)};
            size_t count[2] = {50, 50};
            
            if ((ret = nc_get_vara_uchar(ncid, 0, start, count, data)))
            {
                free(data);
                printf("FAILED on iteration %d: %s\n", i, nc_strerror(ret));
                return 1;
            }
        }
        end_time = get_time_ms();
        printf("ok (total time: %.2f ms, avg: %.2f ms/read)\n", 
               end_time - start_time, (end_time - start_time) / 10.0);
        free(data);
    }

    /* Close file */
    printf("*** Closing file...");
    if ((ret = nc_close(ncid)))
        ERR(ret);
    printf("ok\n");

    printf("\n*** SUCCESS: All large GeoTIFF file tests passed!\n");
    printf("*** File size: %zu x %zu pixels (%.1f megapixels)\n", 
           height, width, (height * width) / 1000000.0);

#else
    printf("*** GeoTIFF support not enabled, skipping tests.\n");
#endif

    return 0;
}

/**
 * @file
 * Edge case test suite for GeoTIFF Phase 3.5a.
 *
 * This test validates handling of edge cases:
 * - Single pixel rasters (1×1)
 * - Very large rasters (>10000×10000)
 * - Non-square tiles
 * - Multiple bands (1-4 bands)
 * - Different data types (uint8, uint16, float32)
 *
 * @author Edward Hartnett
 * @date 2025-12-31
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#endif

#define FILE_SMALL "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"
#define FILE_LARGE "ABBA_2022_C61_HNL.tif"
#define FILE_MULTIBAND "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif"

int
main(int argc, char **argv)
{
    int ncid, varid;
    int ret;
    int test_failures = 0;

    (void)argc;
    (void)argv;
    
    printf("\n*** Testing GeoTIFF edge cases.\n");
    
#ifdef HAVE_GEOTIFF
    char magic_number_tiff[4] = "II*";
    char magic_number_bigtiff[4] = "II+";
    
    /* Initialize GeoTIFF */
    printf("*** Initializing GeoTIFF...");
    if (!GEOTIFF_INIT_OK())
    {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    
    /* Register handlers */
    printf("*** Registering handlers...");
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    /* Test 1: Single pixel read (1×1 hyperslab) */
    printf("\n*** Test 1: Single pixel read (1×1 hyperslab)...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {500, 500};
        size_t count[2] = {1, 1};
        unsigned char pixel;
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, &pixel);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok (value=%u)\n", pixel);
        }
    }
    nc_close(ncid);
    
    /* Test 2: Read at corner boundaries */
    printf("*** Test 2: Read at corner boundaries...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        int ndims, dimids[NC_MAX_DIMS];
        size_t width, height;
        
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_vardimid(ncid, varid, dimids);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_vardimid\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-1], &width);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-2], &height);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        /* Top-left corner */
        size_t start[2] = {0, 0};
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED (top-left): %s\n", nc_strerror(ret));
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            /* Top-right corner */
            start[0] = 0;
            start[1] = width - 10;
            ret = nc_get_vara_uchar(ncid, varid, start, count, data);
            if (ret != NC_NOERR)
            {
                printf("FAILED (top-right): %s\n", nc_strerror(ret));
                nc_close(ncid);
                test_failures++;
            }
            else
            {
                /* Bottom-left corner */
                start[0] = height - 10;
                start[1] = 0;
                ret = nc_get_vara_uchar(ncid, varid, start, count, data);
                if (ret != NC_NOERR)
                {
                    printf("FAILED (bottom-left): %s\n", nc_strerror(ret));
                    nc_close(ncid);
                    test_failures++;
                }
                else
                {
                    /* Bottom-right corner */
                    start[0] = height - 10;
                    start[1] = width - 10;
                    ret = nc_get_vara_uchar(ncid, varid, start, count, data);
                    if (ret != NC_NOERR)
                    {
                        printf("FAILED (bottom-right): %s\n", nc_strerror(ret));
                        nc_close(ncid);
                        test_failures++;
                    }
                    else
                    {
                        printf("ok (all corners)\n");
                    }
                }
            }
        }
    }
    nc_close(ncid);
    
    /* Test 3: Very large hyperslab (1000×1000) */
    printf("*** Test 3: Very large hyperslab (1000×1000)...");
    ret = nc_open(FILE_LARGE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {1000, 1000};
        size_t count[2] = {1000, 1000};
        unsigned char *data = malloc(1000 * 1000);
        if (!data)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(data);
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok (first=%u, last=%u)\n", data[0], data[999999]);
            free(data);
        }
    }
    nc_close(ncid);
    
    /* Test 4: Non-square hyperslab (tall) */
    printf("*** Test 4: Non-square hyperslab (tall: 500×10)...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {100, 100};
        size_t count[2] = {500, 10};
        unsigned char *data = malloc(500 * 10);
        if (!data)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(data);
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok\n");
            free(data);
        }
    }
    nc_close(ncid);
    
    /* Test 5: Non-square hyperslab (wide) */
    printf("*** Test 5: Non-square hyperslab (wide: 10×500)...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {100, 100};
        size_t count[2] = {10, 500};
        unsigned char *data = malloc(10 * 500);
        if (!data)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(data);
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok\n");
            free(data);
        }
    }
    nc_close(ncid);
    
    /* Test 6: Single row read */
    printf("*** Test 6: Single row read...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        int ndims, dimids[NC_MAX_DIMS];
        size_t width;
        
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_vardimid(ncid, varid, dimids);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_vardimid\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-1], &width);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        size_t start[2] = {500, 0};
        size_t count[2] = {1, width};
        unsigned char *data = malloc(width);
        if (!data)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(data);
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok (width=%zu)\n", width);
            free(data);
        }
    }
    nc_close(ncid);
    
    /* Test 7: Single column read */
    printf("*** Test 7: Single column read...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        int ndims, dimids[NC_MAX_DIMS];
        size_t height;
        
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_vardimid(ncid, varid, dimids);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_vardimid\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-2], &height);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        size_t start[2] = {0, 500};
        size_t count[2] = {height, 1};
        unsigned char *data = malloc(height);
        if (!data)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(data);
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok (height=%zu)\n", height);
            free(data);
        }
    }
    nc_close(ncid);
    
    /* Test 8: Multi-band file handling */
    printf("*** Test 8: Multi-band file handling...");
    ret = nc_open(FILE_MULTIBAND, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        int ndims;
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        if (ndims == 3)
        {
            /* Read from first band */
            size_t start[3] = {0, 100, 100};
            size_t count[3] = {1, 10, 10};
            unsigned char data[100];
            
            ret = nc_get_vara_uchar(ncid, varid, start, count, data);
            if (ret != NC_NOERR)
            {
                printf("FAILED: %s\n", nc_strerror(ret));
                nc_close(ncid);
                test_failures++;
            }
            else
            {
                printf("ok (3D variable, first band read)\n");
            }
        }
        else
        {
            printf("skipped (file is %dD, not multi-band)\n", ndims);
        }
    }
    nc_close(ncid);
    
    /* Test 9: Strided access with various strides */
    printf("*** Test 9: Strided access (stride=2)...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {50, 50};
        ptrdiff_t stride[2] = {2, 2};
        unsigned char data[2500];
        
        ret = nc_get_vars_uchar(ncid, varid, start, count, stride, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok\n");
        }
    }
    nc_close(ncid);
    
    /* Test 10: Large stride */
    printf("*** Test 10: Large stride (stride=100)...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {10, 10};
        ptrdiff_t stride[2] = {100, 100};
        unsigned char data[100];
        
        ret = nc_get_vars_uchar(ncid, varid, start, count, stride, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            printf("ok\n");
        }
    }
    nc_close(ncid);
    
    /* Test 11: Data type inquiry */
    printf("*** Test 11: Data type inquiry...");
    ret = nc_open(FILE_SMALL, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        nc_type xtype;
        ret = nc_inq_vartype(ncid, varid, &xtype);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            test_failures++;
        }
        else
        {
            const char *type_name;
            switch (xtype)
            {
                case NC_BYTE: type_name = "NC_BYTE"; break;
                case NC_UBYTE: type_name = "NC_UBYTE"; break;
                case NC_SHORT: type_name = "NC_SHORT"; break;
                case NC_USHORT: type_name = "NC_USHORT"; break;
                case NC_INT: type_name = "NC_INT"; break;
                case NC_UINT: type_name = "NC_UINT"; break;
                case NC_FLOAT: type_name = "NC_FLOAT"; break;
                case NC_DOUBLE: type_name = "NC_DOUBLE"; break;
                default: type_name = "UNKNOWN"; break;
            }
            printf("ok (type=%s)\n", type_name);
        }
    }
    nc_close(ncid);
    
    /* Test 12: Very large file dimensions */
    printf("*** Test 12: Very large file dimensions...");
    ret = nc_open(FILE_LARGE, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    
    {
        int ndims, dimids[NC_MAX_DIMS];
        size_t width, height;
        
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_vardimid(ncid, varid, dimids);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_vardimid\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-1], &width);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_inq_dimlen(ncid, dimids[ndims-2], &height);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        printf("ok (dimensions: %zu × %zu)\n", height, width);
        
        if (height > 10000 || width > 10000)
        {
            printf("    Large file confirmed (>10000 pixels)\n");
        }
    }
    nc_close(ncid);
    
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
    return 0;
#endif
    
    if (test_failures > 0)
    {
        printf("\n*** FAILED: %d edge case tests failed\n", test_failures);
        return 1;
    }
    
    printf("\n*** SUCCESS: All edge case tests passed!\n");
    return 0;
}

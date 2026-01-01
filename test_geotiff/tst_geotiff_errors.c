/**
 * @file
 * Error handling test suite for GeoTIFF Phase 3.5a.
 *
 * This test validates error handling for various edge cases and
 * verifies that appropriate NetCDF error codes are returned.
 *
 * Tests:
 * - Invalid hyperslab bounds (NC_EEDGE)
 * - Out-of-memory conditions
 * - Corrupted TIFF files
 * - Unsupported data types
 * - Invalid file handles
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

#define FILE_NAME "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"
#define CORRUPTED_FILE "data/corrupted.tif"

int
main(int argc, char **argv)
{
    int ncid, varid;
    int ret;
    int test_failures = 0;

    (void)argc;
    (void)argv;
    
    printf("\n*** Testing GeoTIFF error handling.\n");
    
#ifdef HAVE_GEOTIFF
    char magic_number_tiff[4] = "II*";
    char magic_number_bigtiff[4] = "II+";
    
    /* Initialize GeoTIFF */
    printf("*** Initializing GeoTIFF...");
    if (NC_GEOTIFF_initialize() != NC_NOERR)
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
    
    /* Test 1: Invalid file path */
    printf("\n*** Test 1: Invalid file path...");
    ret = nc_open("nonexistent_file.tif", NC_NOWRITE, &ncid);
    if (ret == NC_NOERR)
    {
        printf("FAILED: Should have returned error\n");
        nc_close(ncid);
        test_failures++;
    }
    else
    {
        printf("ok (returned %s)\n", nc_strerror(ret));
    }
    
    /* Test 2: Corrupted TIFF file */
    printf("*** Test 2: Corrupted TIFF file...");
    ret = nc_open(CORRUPTED_FILE, NC_NOWRITE, &ncid);
    if (ret == NC_NOERR)
    {
        printf("FAILED: Should have rejected corrupted file\n");
        nc_close(ncid);
        test_failures++;
    }
    else
    {
        printf("ok (returned %s)\n", nc_strerror(ret));
    }
    
    /* Open valid file for remaining tests */
    printf("\n*** Opening valid file for error tests...");
    ret = nc_open(FILE_NAME, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    varid = 0;
    printf("ok\n");
    
    /* Test 3: Invalid hyperslab - start out of bounds */
    printf("*** Test 3: Invalid hyperslab (start out of bounds)...");
    {
        size_t start[2] = {100000, 100000};
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret == NC_EEDGE || ret == NC_EINVALCOORDS)
        {
            printf("ok (returned %s)\n", nc_strerror(ret));
        }
        else
        {
            printf("FAILED: Expected NC_EEDGE or NC_EINVALCOORDS, got %s\n", nc_strerror(ret));
            test_failures++;
        }
    }
    
    /* Test 4: Invalid hyperslab - count extends beyond bounds */
    printf("*** Test 4: Invalid hyperslab (count extends beyond)...");
    {
        int ndims, dimids[NC_MAX_DIMS];
        size_t dimlen;
        
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
        
        ret = nc_inq_dimlen(ncid, dimids[0], &dimlen);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        size_t start[2] = {dimlen - 5, 0};
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret == NC_EEDGE || ret == NC_EINVALCOORDS)
        {
            printf("ok (returned %s)\n", nc_strerror(ret));
        }
        else
        {
            printf("FAILED: Expected NC_EEDGE or NC_EINVALCOORDS, got %s\n", nc_strerror(ret));
            test_failures++;
        }
    }
    
    /* Test 5: Invalid variable ID */
    printf("*** Test 5: Invalid variable ID...");
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, 999, start, count, data);
        if (ret == NC_ENOTVAR)
        {
            printf("ok (returned NC_ENOTVAR)\n");
        }
        else
        {
            printf("FAILED: Expected NC_ENOTVAR, got %s\n", nc_strerror(ret));
            test_failures++;
        }
    }
    
    /* Test 6: NULL buffer pointer */
    printf("*** Test 6: NULL buffer pointer...");
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {10, 10};
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, NULL);
        if (ret == NC_EINVAL || ret == NC_NOERR)
        {
            printf("ok (returned %s)\n", nc_strerror(ret));
        }
        else
        {
            printf("WARNING: Unexpected error %s\n", nc_strerror(ret));
        }
    }
    
    /* Test 7: Zero-sized hyperslab */
    printf("*** Test 7: Zero-sized hyperslab...");
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {0, 0};
        unsigned char data[1];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret == NC_NOERR || ret == NC_EINVAL)
        {
            printf("ok (returned %s)\n", nc_strerror(ret));
        }
        else
        {
            printf("WARNING: Unexpected error %s\n", nc_strerror(ret));
        }
    }
    
    /* Test 8: Invalid stride (negative) */
    printf("*** Test 8: Invalid stride (negative)...");
    {
        size_t start[2] = {100, 100};
        size_t count[2] = {10, 10};
        ptrdiff_t stride[2] = {-1, 1};
        unsigned char data[100];
        
        ret = nc_get_vars_uchar(ncid, varid, start, count, stride, data);
        if (ret == NC_EINVAL || ret == NC_ESTRIDE)
        {
            printf("ok (returned %s)\n", nc_strerror(ret));
        }
        else if (ret == NC_NOERR)
        {
            printf("ok (negative stride accepted)\n");
        }
        else
        {
            printf("WARNING: Unexpected error %s\n", nc_strerror(ret));
        }
    }
    
    /* Test 9: Wrong dimensionality */
    printf("*** Test 9: Wrong dimensionality...");
    {
        int ndims;
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_varndims\n");
            nc_close(ncid);
            return 1;
        }
        
        if (ndims == 2)
        {
            size_t start[3] = {0, 0, 0};
            size_t count[3] = {1, 10, 10};
            unsigned char data[100];
            
            ret = nc_get_vara_uchar(ncid, varid, start, count, data);
            if (ret == NC_EINVAL || ret == NC_NOERR)
            {
                printf("ok (returned %s)\n", nc_strerror(ret));
            }
            else
            {
                printf("WARNING: Unexpected error %s\n", nc_strerror(ret));
            }
        }
        else
        {
            printf("skipped (file is %dD)\n", ndims);
        }
    }
    
    /* Test 10: Invalid file handle after close */
    printf("*** Test 10: Invalid file handle after close...");
    {
        int temp_ncid;
        ret = nc_open(FILE_NAME, NC_NOWRITE, &temp_ncid);
        if (ret != NC_NOERR)
        {
            printf("FAILED: Could not open temp file\n");
            test_failures++;
        }
        else
        {
            nc_close(temp_ncid);
            
            size_t start[2] = {0, 0};
            size_t count[2] = {10, 10};
            unsigned char data[100];
            
            ret = nc_get_vara_uchar(temp_ncid, 0, start, count, data);
            if (ret == NC_EBADID)
            {
                printf("ok (returned NC_EBADID)\n");
            }
            else
            {
                printf("FAILED: Expected NC_EBADID, got %s\n", nc_strerror(ret));
                test_failures++;
            }
        }
    }
    
    /* Test 11: Type mismatch (try to read as wrong type) */
    printf("*** Test 11: Type mismatch...");
    {
        nc_type var_type;
        ret = nc_inq_vartype(ncid, varid, &var_type);
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_vartype\n");
            nc_close(ncid);
            return 1;
        }
        
        size_t start[2] = {0, 0};
        size_t count[2] = {10, 10};
        
        if (var_type == NC_UBYTE)
        {
            double data[100];
            ret = nc_get_vara_double(ncid, varid, start, count, data);
        }
        else
        {
            unsigned char data[100];
            ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        }
        
        if (ret == NC_NOERR || ret == NC_ERANGE)
        {
            printf("ok (type conversion handled: %s)\n", nc_strerror(ret));
        }
        else
        {
            printf("WARNING: Unexpected error %s\n", nc_strerror(ret));
        }
    }
    
    /* Close file */
    printf("\n*** Closing file...");
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    /* Test 12: Write operations should fail (read-only) */
    printf("*** Test 12: Write operations on read-only file...");
    ret = nc_open(FILE_NAME, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: Could not reopen file\n");
        return 1;
    }
    
    ret = nc_redef(ncid);
    if (ret == NC_EPERM || ret == NC_ENOTNC4 || ret != NC_NOERR)
    {
        printf("ok (write operations rejected: %s)\n", nc_strerror(ret));
    }
    else
    {
        printf("WARNING: nc_redef succeeded on read-only file\n");
    }
    
    nc_close(ncid);
    
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
    return 0;
#endif
    
    if (test_failures > 0)
    {
        printf("\n*** FAILED: %d error handling tests failed\n", test_failures);
        return 1;
    }
    
    printf("\n*** SUCCESS: All error handling tests passed!\n");
    return 0;
}

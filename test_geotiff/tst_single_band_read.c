/**
 * @file
 * Test GeoTIFF Phase 3.2: Single-band raster data reading.
 *
 * This test verifies nc_get_vara functionality for reading
 * single-band (2D) GeoTIFF raster data.
 *
 * @author Edward Hartnett
 * @date 2025-12-30
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#endif

#define FILE_NAME "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"

int
main(int argc, char **argv)
{
    int ncid, varid;
    int ret;
    int nvars;
    char magic_number_tiff[4] = "II*";     /* Standard TIFF */
    char magic_number_bigtiff[4] = "II+";  /* BigTIFF */

    (void)argc;
    (void)argv;
    
    printf("\n*** Testing single-band raster reading.\n");
    
#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF dispatch layer */
    printf("*** Initializing GeoTIFF...");
    if (NC_GEOTIFF_initialize() != NC_NOERR)
    {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    
    /* Register GeoTIFF UDF handlers for both standard TIFF and BigTIFF */
    printf("*** Registering handlers (II* and II+)...");
    
    /* NC_UDF0: Standard TIFF */
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("FAILED (II*): %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* NC_UDF1: BigTIFF */
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("FAILED (II+): %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("ok\n");
    
    /* Test 1: Open file */
    printf("*** Test 1: Opening GeoTIFF file...");
    ret = nc_open(FILE_NAME, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    /* Test 2: Get variable ID */
    printf("*** Test 2: Getting variable ID...");
    ret = nc_inq_nvars(ncid, &nvars);
    if (ret != NC_NOERR || nvars < 1)
    {
        printf("FAILED: nvars=%d, %s\n", nvars, nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    varid = 0; /* First variable */
    printf("ok (found %d variables)\n", nvars);
    
    /* Test 3: Read single pixel */
    printf("*** Test 3: Reading single pixel...");
    {
        size_t start[2] = {0, 0};
        size_t count[2] = {1, 1};
        unsigned char pixel;
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, &pixel);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            return 1;
        }
        printf("ok (value=%u)\n", pixel);
    }
    
    /* Test 4: Read small hyperslab (10x10) */
    printf("*** Test 4: Reading 10x10 hyperslab...");
    {
        size_t start[2] = {100, 100};
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            nc_close(ncid);
            return 1;
        }
        printf("ok (first value=%u, last value=%u)\n", data[0], data[99]);
    }
    
    /* Test 5: Read larger hyperslab (100x100) */
    printf("*** Test 5: Reading 100x100 hyperslab...");
    {
        size_t start[2] = {500, 500};
        size_t count[2] = {100, 100};
        unsigned char *data = malloc(100 * 100);
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
            return 1;
        }
        printf("ok (first value=%u, last value=%u)\n", data[0], data[9999]);
        free(data);
    }
    
    /* Test 6: Test bounds checking - should fail with NC_EEDGE */
    printf("*** Test 6: Testing bounds checking...");
    {
        size_t start[2] = {10000, 10000}; /* Out of bounds */
        size_t count[2] = {10, 10};
        unsigned char data[100];
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, data);
        if (ret == NC_EEDGE)
        {
            printf("ok (correctly returned NC_EEDGE)\n");
        }
        else
        {
            printf("FAILED: expected NC_EEDGE, got %s\n", nc_strerror(ret));
            nc_close(ncid);
            return 1;
        }
    }
    
    /* Test 7: Read full scanline */
    printf("*** Test 7: Reading full scanline...");
    {
        int ndims;
        int dimids[NC_MAX_DIMS];
        size_t dimlen;
        
        /* Get dimension info */
        ret = nc_inq_varndims(ncid, varid, &ndims);
        if (ret != NC_NOERR || ndims != 2)
        {
            printf("FAILED: ndims=%d\n", ndims);
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
        
        ret = nc_inq_dimlen(ncid, dimids[1], &dimlen); /* x dimension */
        if (ret != NC_NOERR)
        {
            printf("FAILED: nc_inq_dimlen\n");
            nc_close(ncid);
            return 1;
        }
        
        size_t start[2] = {0, 0};
        size_t count[2] = {1, dimlen};
        unsigned char *scanline = malloc(dimlen);
        if (!scanline)
        {
            printf("FAILED: malloc\n");
            nc_close(ncid);
            return 1;
        }
        
        ret = nc_get_vara_uchar(ncid, varid, start, count, scanline);
        if (ret != NC_NOERR)
        {
            printf("FAILED: %s\n", nc_strerror(ret));
            free(scanline);
            nc_close(ncid);
            return 1;
        }
        printf("ok (width=%zu, first=%u, last=%u)\n", dimlen, scanline[0], scanline[dimlen-1]);
        free(scanline);
    }
    
    /* Close file */
    printf("*** Closing file...");
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
    return 0;
#endif
    
    printf("\n*** SUCCESS: All single-band reading tests passed!\n");
    return 0;
}

/**
 * @file
 * Test to compare GeoTIFF values between NetCDF and libgeotiff retrieval.
 *
 * This test opens a sample GeoTIFF file, reads the first 10 data values
 * using the NetCDF API, then reads the same values using libgeotiff/libtiff
 * directly, and verifies they match.
 *
 * @author Edward Hartnett
 * @date 2026-01-01
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#include <tiffio.h>
#endif

#define FILE_NAME "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"
#define NUM_VALUES 10

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("\n*** Testing simple GeoTIFF value comparison.\n");
    
#ifdef HAVE_GEOTIFF
    int ncid, varid;
    int ret;
    int nvars;
    char magic_number_tiff[4] = "II*";
    char magic_number_bigtiff[4] = "II+";
    unsigned char netcdf_values[NUM_VALUES];
    unsigned char libtiff_values[NUM_VALUES];
    int i;
    
    printf("*** Initializing GeoTIFF...");
    if (!GEOTIFF_INIT_OK())
    {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    
    printf("*** Registering handlers...");
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("FAILED (II*): %s\n", nc_strerror(ret));
        return 1;
    }
    
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("FAILED (II+): %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    printf("*** Step 1: Reading first %d values via NetCDF API...\n", NUM_VALUES);
    
    ret = nc_open(FILE_NAME, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED to open file: %s\n", nc_strerror(ret));
        return 1;
    }
    
    ret = nc_inq_nvars(ncid, &nvars);
    if (ret != NC_NOERR || nvars < 1)
    {
        printf("FAILED: nvars=%d, %s\n", nvars, nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    
    varid = 0;
    
    size_t start[2] = {0, 0};
    size_t count[2] = {1, NUM_VALUES};
    
    ret = nc_get_vara_uchar(ncid, varid, start, count, netcdf_values);
    if (ret != NC_NOERR)
    {
        printf("FAILED to read via NetCDF: %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    
    printf("NetCDF values: ");
    for (i = 0; i < NUM_VALUES; i++)
    {
        printf("%u ", netcdf_values[i]);
    }
    printf("\n");
    
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED to close NetCDF file: %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("*** Step 2: Reading same %d values via libgeotiff...\n", NUM_VALUES);
    
    TIFF *tif = TIFFOpen(FILE_NAME, "r");
    if (!tif)
    {
        printf("FAILED to open TIFF file\n");
        return 1;
    }
    
    uint32_t width, height;
    uint16_t samples_per_pixel, bits_per_sample;
    
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    
    int is_tiled = TIFFIsTiled(tif);
    
    printf("TIFF dimensions: %u x %u, samples=%u, bits=%u, %s\n", 
           width, height, samples_per_pixel, bits_per_sample,
           is_tiled ? "tiled" : "scanline");
    
    if (bits_per_sample != 8)
    {
        printf("WARNING: Expected 8 bits per sample, got %u\n", bits_per_sample);
    }
    
    unsigned char *buffer = NULL;
    tsize_t buffer_size;
    
    if (is_tiled)
    {
        buffer_size = TIFFTileSize(tif);
        buffer = (unsigned char *)_TIFFmalloc(buffer_size);
        if (!buffer)
        {
            printf("FAILED to allocate tile buffer\n");
            TIFFClose(tif);
            return 1;
        }
        
        if (TIFFReadTile(tif, buffer, 0, 0, 0, 0) < 0)
        {
            printf("FAILED to read tile\n");
            _TIFFfree(buffer);
            TIFFClose(tif);
            return 1;
        }
    }
    else
    {
        buffer_size = TIFFScanlineSize(tif);
        buffer = (unsigned char *)_TIFFmalloc(buffer_size);
        if (!buffer)
        {
            printf("FAILED to allocate scanline buffer\n");
            TIFFClose(tif);
            return 1;
        }
        
        if (TIFFReadScanline(tif, buffer, 0, 0) < 0)
        {
            printf("FAILED to read scanline\n");
            _TIFFfree(buffer);
            TIFFClose(tif);
            return 1;
        }
    }
    
    for (i = 0; i < NUM_VALUES; i++)
    {
        libtiff_values[i] = buffer[i];
    }
    
    printf("libgeotiff values: ");
    for (i = 0; i < NUM_VALUES; i++)
    {
        printf("%u ", libtiff_values[i]);
    }
    printf("\n");
    
    _TIFFfree(buffer);
    TIFFClose(tif);
    
    printf("*** Step 3: Comparing values...\n");
    
    int all_match = 1;
    for (i = 0; i < NUM_VALUES; i++)
    {
        if (netcdf_values[i] != libtiff_values[i])
        {
            printf("MISMATCH at index %d: NetCDF=%u, libgeotiff=%u\n",
                   i, netcdf_values[i], libtiff_values[i]);
            all_match = 0;
        }
    }
    
    if (!all_match)
    {
        printf("FAILED: Values do not match!\n");
        return 1;
    }
    
    printf("SUCCESS: All %d values match!\n", NUM_VALUES);
    
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
    return 0;
#endif
    
    printf("\n*** SUCCESS: Simple GeoTIFF comparison test passed!\n");
    return 0;
}

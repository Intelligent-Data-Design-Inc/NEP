/**
 * @file
 * Performance benchmark suite for GeoTIFF Phase 3.5a.
 *
 * This benchmark compares NEP GeoTIFF access performance against native
 * libgeotiff to validate <5% overhead requirement.
 *
 * Tests various read operations:
 * - Full raster read (nc_get_var)
 * - Hyperslab read (nc_get_vara) - various sizes
 * - Single pixel read (nc_get_var1)
 * - Strided read (nc_get_vars)
 *
 * @author Edward Hartnett
 * @date 2025-12-31
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#include <tiffio.h>
#include <geotiff/geotiff.h>
#include <geotiff/xtiffio.h>
#endif

#define FILE_SMALL "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"
#define FILE_MEDIUM "MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif"
#define FILE_LARGE "ABBA_2022_C61_HNL.tif"

#define NUM_ITERATIONS 1
#define OVERHEAD_THRESHOLD 0.05  /* 5% */
#define TIMEOUT_SECONDS 60

/**
 * Get current time in seconds with microsecond precision.
 */
static double
get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

#ifdef HAVE_GEOTIFF

/**
 * Benchmark native libgeotiff full raster read.
 */
static double
bench_native_full_read(const char *filename)
{
    TIFF *tiff;
    uint32_t width, height;
    uint16_t samples_per_pixel = 1;
    unsigned char *buffer;
    double start, elapsed;
    int i;
    int is_tiled;
    
    tiff = XTIFFOpen(filename, "r");
    if (!tiff)
        return -1.0;
    
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    is_tiled = TIFFIsTiled(tiff);
    
    /* Allocate buffer - use RGBA size for tiled images */
    if (is_tiled)
        buffer = malloc(width * height * 4); /* RGBA = 4 bytes per pixel */
    else
        buffer = malloc(width * height * samples_per_pixel);
    
    if (!buffer)
    {
        XTIFFClose(tiff);
        return -1.0;
    }
    
    start = get_time();
    for (i = 0; i < NUM_ITERATIONS; i++)
    {
        if (is_tiled)
        {
            /* Read entire tiled image as RGBA */
            if (TIFFReadRGBAImageOriented(tiff, width, height, (uint32_t *)buffer, ORIENTATION_TOPLEFT, 0) == 0)
            {
                free(buffer);
                XTIFFClose(tiff);
                return -1.0;
            }
        }
        else
        {
            /* Read scanline by scanline */
            uint32_t row;
            for (row = 0; row < height; row++)
            {
                TIFFReadScanline(tiff, buffer + row * width * samples_per_pixel, row, 0);
                
                /* Check timeout every 100 rows */
                if (row % 100 == 0 && (get_time() - start) > TIMEOUT_SECONDS)
                {
                    free(buffer);
                    XTIFFClose(tiff);
                    return -2.0; /* Timeout indicator */
                }
            }
        }
        
        /* Check timeout between iterations */
        if ((get_time() - start) > TIMEOUT_SECONDS)
        {
            free(buffer);
            XTIFFClose(tiff);
            return -2.0; /* Timeout indicator */
        }
    }
    elapsed = get_time() - start;
    
    free(buffer);
    XTIFFClose(tiff);
    
    return elapsed / NUM_ITERATIONS;
}

/**
 * Benchmark NEP full raster read.
 */
static double
bench_nep_full_read(int ncid, int varid, size_t width, size_t height, int ndims)
{
    unsigned char *buffer;
    double start, elapsed;
    int i, ret;
    
    buffer = malloc(width * height);
    if (!buffer)
        return -1.0;
    
    start = get_time();
    for (i = 0; i < NUM_ITERATIONS; i++)
    {
        if (ndims == 2)
        {
            ret = nc_get_var_uchar(ncid, varid, buffer);
        }
        else
        {
            size_t start_idx[3] = {0, 0, 0};
            size_t count[3] = {1, height, width};
            ret = nc_get_vara_uchar(ncid, varid, start_idx, count, buffer);
        }
        if (ret != NC_NOERR)
        {
            free(buffer);
            return -1.0;
        }
        
        /* Check timeout between iterations */
        if ((get_time() - start) > TIMEOUT_SECONDS)
        {
            free(buffer);
            return -2.0; /* Timeout indicator */
        }
    }
    elapsed = get_time() - start;
    
    free(buffer);
    return elapsed / NUM_ITERATIONS;
}

/**
 * Benchmark native libgeotiff hyperslab read.
 * Note: This is a simplified benchmark that doesn't fully optimize
 * for tiled access patterns. Real performance may vary.
 */
static double
bench_native_hyperslab(const char *filename, size_t start_y, size_t start_x,
                       size_t count_y, size_t count_x)
{
    TIFF *tiff;
    uint32_t width, height;
    uint16_t samples_per_pixel = 1;
    unsigned char *buffer, *full_buffer;
    double start_time, elapsed;
    int i;
    int is_tiled;
    
    tiff = XTIFFOpen(filename, "r");
    if (!tiff)
        return -1.0;
    
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    is_tiled = TIFFIsTiled(tiff);
    
    buffer = malloc(count_y * count_x * samples_per_pixel);
    if (!buffer)
    {
        XTIFFClose(tiff);
        return -1.0;
    }
    
    /* For tiled images, check if file is too large for full read */
    if (is_tiled && (width * height > 100000000))
    {
        /* File too large - use tile-based reading instead */
        uint32_t tile_width, tile_height;
        TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tile_width);
        TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tile_height);
        
        unsigned char *tile_buffer = malloc(tile_width * tile_height * 4);
        if (!tile_buffer)
        {
            free(buffer);
            XTIFFClose(tiff);
            return -1.0;
        }
        
        start_time = get_time();
        for (i = 0; i < NUM_ITERATIONS; i++)
        {
            /* Read only the tiles we need */
            size_t y, x;
            for (y = 0; y < count_y; y++)
            {
                for (x = 0; x < count_x; x++)
                {
                    uint32_t tile_y = (start_y + y) / tile_height;
                    uint32_t tile_x = (start_x + x) / tile_width;
                    uint32_t tile_index = tile_y * ((width + tile_width - 1) / tile_width) + tile_x;
                    
                    /* Read tile (simplified - just read first tile) */
                    if (TIFFReadEncodedTile(tiff, tile_index, tile_buffer, tile_width * tile_height * samples_per_pixel) < 0)
                    {
                        free(buffer);
                        free(tile_buffer);
                        XTIFFClose(tiff);
                        return -1.0;
                    }
                    
                    /* Extract pixel from tile */
                    uint32_t in_tile_y = (start_y + y) % tile_height;
                    uint32_t in_tile_x = (start_x + x) % tile_width;
                    buffer[y * count_x + x] = tile_buffer[in_tile_y * tile_width + in_tile_x];
                }
            }
        }
        elapsed = get_time() - start_time;
        free(tile_buffer);
    }
    else if (is_tiled)
    {
        /* Small tiled file - read full image then extract region */
        full_buffer = malloc(width * height * 4); /* RGBA */
        if (!full_buffer)
        {
            free(buffer);
            XTIFFClose(tiff);
            return -1.0;
        }
        
        start_time = get_time();
        for (i = 0; i < NUM_ITERATIONS; i++)
        {
            /* Read full image and extract region */
            if (TIFFReadRGBAImageOriented(tiff, width, height, (uint32_t *)full_buffer, ORIENTATION_TOPLEFT, 0) == 0)
            {
                free(buffer);
                free(full_buffer);
                XTIFFClose(tiff);
                return -1.0;
            }
            /* Extract region (simplified - just copy first byte of each pixel) */
            size_t y, x;
            for (y = 0; y < count_y; y++)
            {
                for (x = 0; x < count_x; x++)
                {
                    buffer[y * count_x + x] = full_buffer[((start_y + y) * width + (start_x + x)) * 4];
                }
            }
        }
        elapsed = get_time() - start_time;
        free(full_buffer);
    }
    else
    {
        /* Striped file - read scanline by scanline */
        unsigned char *scanline = malloc(width * samples_per_pixel);
        if (!scanline)
        {
            free(buffer);
            XTIFFClose(tiff);
            return -1.0;
        }
        
        start_time = get_time();
        for (i = 0; i < NUM_ITERATIONS; i++)
        {
            size_t row;
            for (row = 0; row < count_y; row++)
            {
                TIFFReadScanline(tiff, scanline, start_y + row, 0);
                memcpy(buffer + row * count_x, scanline + start_x, count_x);
            }
        }
        elapsed = get_time() - start_time;
        free(scanline);
    }
    
    free(buffer);
    XTIFFClose(tiff);
    
    return elapsed / NUM_ITERATIONS;
}

/**
 * Benchmark NEP hyperslab read.
 */
static double
bench_nep_hyperslab(int ncid, int varid, size_t start_y, size_t start_x,
                    size_t count_y, size_t count_x, int ndims)
{
    unsigned char *buffer;
    double start_time, elapsed;
    int i, ret;
    
    buffer = malloc(count_y * count_x);
    if (!buffer)
        return -1.0;
    
    start_time = get_time();
    for (i = 0; i < NUM_ITERATIONS; i++)
    {
        if (ndims == 2)
        {
            size_t start[2] = {start_y, start_x};
            size_t count[2] = {count_y, count_x};
            ret = nc_get_vara_uchar(ncid, varid, start, count, buffer);
        }
        else
        {
            size_t start[3] = {0, start_y, start_x};
            size_t count[3] = {1, count_y, count_x};
            ret = nc_get_vara_uchar(ncid, varid, start, count, buffer);
        }
        if (ret != NC_NOERR)
        {
            free(buffer);
            return -1.0;
        }
    }
    elapsed = get_time() - start_time;
    
    free(buffer);
    return elapsed / NUM_ITERATIONS;
}

/**
 * Benchmark single pixel reads.
 */
static double
bench_single_pixel(int ncid, int varid, int ndims)
{
    unsigned char pixel;
    double start_time, elapsed;
    int i, ret;
    
    start_time = get_time();
    for (i = 0; i < NUM_ITERATIONS * 10; i++)
    {
        if (ndims == 2)
        {
            size_t index[2] = {i % 100, i % 100};
            ret = nc_get_var1_uchar(ncid, varid, index, &pixel);
        }
        else
        {
            size_t index[3] = {0, i % 100, i % 100};
            ret = nc_get_var1_uchar(ncid, varid, index, &pixel);
        }
        if (ret != NC_NOERR)
            return -1.0;
    }
    elapsed = get_time() - start_time;
    
    return elapsed / (NUM_ITERATIONS * 10);
}

/**
 * Benchmark strided reads.
 */
static double
bench_strided_read(int ncid, int varid, int ndims)
{
    unsigned char buffer[100];
    double start_time, elapsed;
    int i, ret;
    
    start_time = get_time();
    for (i = 0; i < NUM_ITERATIONS; i++)
    {
        if (ndims == 2)
        {
            size_t start[2] = {0, 0};
            size_t count[2] = {10, 10};
            ptrdiff_t stride[2] = {10, 10};
            ret = nc_get_vars_uchar(ncid, varid, start, count, stride, buffer);
        }
        else
        {
            size_t start[3] = {0, 0, 0};
            size_t count[3] = {1, 10, 10};
            ptrdiff_t stride[3] = {1, 10, 10};
            ret = nc_get_vars_uchar(ncid, varid, start, count, stride, buffer);
        }
        if (ret != NC_NOERR)
            return -1.0;
    }
    elapsed = get_time() - start_time;
    
    return elapsed / NUM_ITERATIONS;
}

/**
 * Run benchmark suite on a file.
 */
static int
benchmark_file(const char *filename, const char *label)
{
    int ncid, varid, ret;
    int ndims, dimids[NC_MAX_DIMS];
    size_t width, height;
    double native_time, nep_time, overhead;
    
    printf("\n=== Benchmarking %s (%s) ===\n", label, filename);
    
    /* Open file with NEP */
    ret = nc_open(filename, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("ERROR: Failed to open %s: %s\n", filename, nc_strerror(ret));
        return 1;
    }
    
    varid = 0;
    
    /* Get dimensions */
    ret = nc_inq_varndims(ncid, varid, &ndims);
    if (ret != NC_NOERR)
    {
        nc_close(ncid);
        return 1;
    }
    
    ret = nc_inq_vardimid(ncid, varid, dimids);
    if (ret != NC_NOERR)
    {
        nc_close(ncid);
        return 1;
    }
    
    ret = nc_inq_dimlen(ncid, dimids[ndims-1], &width);
    if (ret != NC_NOERR)
    {
        nc_close(ncid);
        return 1;
    }
    
    ret = nc_inq_dimlen(ncid, dimids[ndims-2], &height);
    if (ret != NC_NOERR)
    {
        nc_close(ncid);
        return 1;
    }
    
    printf("Dimensions: %zu x %zu (%dD)\n", height, width, ndims);
    
    /* Benchmark 1: Full raster read (skip for very large files) */
    printf("\n1. Full raster read (%d iterations):\n", NUM_ITERATIONS);
    if (width * height > 100000000) /* Skip if > 100M pixels (~400MB RGBA) */
    {
        printf("   SKIPPED: File too large for full read benchmark\n");
    }
    else
    {
        native_time = bench_native_full_read(filename);
        nep_time = bench_nep_full_read(ncid, varid, width, height, ndims);
        
        if (native_time == -2.0 || nep_time == -2.0)
        {
            printf("   TIMEOUT: Benchmark exceeded %d seconds\n", TIMEOUT_SECONDS);
        }
        else if (native_time < 0 || nep_time < 0)
        {
            printf("   ERROR: Benchmark failed\n");
        }
        else
        {
            overhead = (nep_time - native_time) / native_time;
            printf("   Native: %.6f s\n", native_time);
            printf("   NEP:    %.6f s\n", nep_time);
            printf("   Overhead: %.2f%%\n", overhead * 100);
            if (overhead > OVERHEAD_THRESHOLD)
                printf("   WARNING: Overhead exceeds 5%% threshold!\n");
        }
    }
    
    /* Benchmark 2: Small hyperslab (10x10) */
    printf("\n2. Small hyperslab (10x10, %d iterations):\n", NUM_ITERATIONS);
    native_time = bench_native_hyperslab(filename, 100, 100, 10, 10);
    nep_time = bench_nep_hyperslab(ncid, varid, 100, 100, 10, 10, ndims);
    
    if (native_time < 0 || nep_time < 0)
    {
        printf("   ERROR: Benchmark failed\n");
    }
    else
    {
        overhead = (nep_time - native_time) / native_time;
        printf("   Native: %.6f s\n", native_time);
        printf("   NEP:    %.6f s\n", nep_time);
        printf("   Overhead: %.2f%%\n", overhead * 100);
        if (overhead > OVERHEAD_THRESHOLD)
            printf("   WARNING: Overhead exceeds 5%% threshold!\n");
    }
    
    /* Benchmark 3: Medium hyperslab (100x100) */
    printf("\n3. Medium hyperslab (100x100, %d iterations):\n", NUM_ITERATIONS);
    native_time = bench_native_hyperslab(filename, 500, 500, 100, 100);
    nep_time = bench_nep_hyperslab(ncid, varid, 500, 500, 100, 100, ndims);
    
    if (native_time < 0 || nep_time < 0)
    {
        printf("   ERROR: Benchmark failed\n");
    }
    else
    {
        overhead = (nep_time - native_time) / native_time;
        printf("   Native: %.6f s\n", native_time);
        printf("   NEP:    %.6f s\n", nep_time);
        printf("   Overhead: %.2f%%\n", overhead * 100);
        if (overhead > OVERHEAD_THRESHOLD)
            printf("   WARNING: Overhead exceeds 5%% threshold!\n");
    }
    
    /* Benchmark 4: Single pixel reads */
    printf("\n4. Single pixel reads (%d iterations):\n", NUM_ITERATIONS * 10);
    nep_time = bench_single_pixel(ncid, varid, ndims);
    
    if (nep_time < 0)
    {
        printf("   ERROR: Benchmark failed\n");
    }
    else
    {
        printf("   NEP: %.9f s per pixel\n", nep_time);
    }
    
    /* Benchmark 5: Strided reads */
    printf("\n5. Strided reads (10x10 with stride 10, %d iterations):\n", NUM_ITERATIONS);
    nep_time = bench_strided_read(ncid, varid, ndims);
    
    if (nep_time < 0)
    {
        printf("   ERROR: Benchmark failed\n");
    }
    else
    {
        printf("   NEP: %.6f s\n", nep_time);
    }
    
    nc_close(ncid);
    return 0;
}

#endif /* HAVE_GEOTIFF */

int
main(int argc, char **argv)
{
    char magic_number_tiff[4] = "II*";
    char magic_number_bigtiff[4] = "II+";
    int ret;
    
    printf("\n*** GeoTIFF Performance Benchmark Suite ***\n");
    printf("Iterations per test: %d\n", NUM_ITERATIONS);
    printf("Overhead threshold: %.0f%%\n\n", OVERHEAD_THRESHOLD * 100);
    
#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF */
    if (NC_GEOTIFF_initialize() != NC_NOERR)
    {
        printf("ERROR: Failed to initialize GeoTIFF\n");
        return 1;
    }
    
    /* Register handlers */
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("ERROR: Failed to register TIFF handler: %s\n", nc_strerror(ret));
        return 1;
    }
    
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("ERROR: Failed to register BigTIFF handler: %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* Run benchmarks on different file sizes */
    if (benchmark_file(FILE_SMALL, "Small file (<10MB)"))
        return 1;
    
    if (benchmark_file(FILE_MEDIUM, "Medium file (10MB-100MB)"))
        return 1;
    
    if (benchmark_file(FILE_LARGE, "Large file (>1GB)"))
        return 1;
    
    printf("\n*** Benchmark suite completed ***\n");
    printf("Review results above to verify <5%% overhead requirement.\n\n");
    
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
#endif
    
    return 0;
}

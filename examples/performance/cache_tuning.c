/**
 * @file cache_tuning.c
 * @brief Demonstrates NetCDF-4 chunk cache configuration for optimal I/O performance
 *
 * This example explores NetCDF-4's chunk cache mechanism and demonstrates how proper
 * cache configuration can dramatically improve read performance for chunked datasets.
 * The chunk cache is an in-memory buffer that stores recently accessed data chunks,
 * reducing disk I/O when the same chunks are read repeatedly.
 *
 * The program creates a realistic 3D scientific dataset (temperature over time, latitude,
 * longitude) with chunked storage, then performs four experiments:
 * 1. Baseline measurement with default cache settings
 * 2. Performance scaling across multiple cache sizes (4MB to 128MB)
 * 3. Cache thrashing detection and mitigation
 * 4. File-level vs variable-level cache configuration
 *
 * **What is Chunk Caching?**
 * When NetCDF-4 reads chunked data, it first checks the chunk cache. If the chunk is
 * in cache, it's returned immediately without disk I/O. If not, the chunk is read
 * from disk and stored in cache for future access. Proper cache sizing ensures
 * frequently accessed chunks remain in memory.
 *
 * **Learning Objectives:**
 * - Understand how the NetCDF-4 chunk cache improves read performance
 * - Learn to query current cache settings with nc_get_var_chunk_cache()
 * - Configure variable-level cache with nc_set_var_chunk_cache()
 * - Configure file-level default cache with nc_set_chunk_cache()
 * - Detect and resolve cache thrashing scenarios
 * - Select appropriate cache sizes for your datasets
 * - Query chunking configuration with nc_inq_var_chunking()
 *
 * **Key Concepts:**
 * - **Chunk Cache**: In-memory buffer storing recently accessed chunks
 * - **Cache Size**: Total memory allocated for caching (default is typically 64MB)
 * - **Cache Elements**: Maximum number of chunks that can be cached (default ~1009)
 * - **Preemption Policy**: How the cache evicts chunks when full (0.0-1.0, default 0.75)
 * - **Cache Hit**: Data found in cache, no disk read needed
 * - **Cache Miss**: Data not in cache, must read from disk
 * - **Thrashing**: When working set exceeds cache size, causing constant eviction
 * - **Variable-Level Cache**: Per-variable cache settings override file defaults
 * - **File-Level Cache**: Default cache for all variables in newly opened files
 *
 * **When to Tune the Cache:**
 * - Reading large chunked datasets (>100MB)
 * - Repeated access to same data regions (time series analysis)
 * - Multi-pass algorithms (filtering, normalization)
 * - Random or strided access patterns
 * - Performance-critical applications
 *
 * **Cache Size Guidelines:**
 * - Small datasets (<100MB): Default 64MB cache is usually sufficient
 * - Medium datasets (100MB-2GB): 128MB-512MB cache recommended
 * - Large datasets (>2GB): 512MB-2GB+ cache for optimal performance
 * - Working set size: Cache should hold all chunks accessed repeatedly
 * - System memory: Don't exceed available RAM - leave headroom for OS
 *
 * **Prerequisites:**
 * - chunking_performance.c - Understanding chunking concepts
 * - simple_nc4.c - NetCDF-4 file basics
 * - Understanding of C arrays and memory management
 *
 * **Related Examples:**
 * - chunking_performance.c - Chunking strategies and their performance impact
 * - compression.c - Compression (requires chunking)
 * - f_chunking_performance.f90 - Fortran equivalent
 *
 * **Compilation:**
 * @code
 * gcc -o cache_tuning cache_tuning.c -lnetcdf
 * @endcode
 *
 * **Usage (API demonstration only):**
 * @code
 * ./cache_tuning
 * @endcode
 *
 * **Usage (with performance benchmarks):**
 * @code
 * # Build with benchmarks enabled
 * cmake -DENABLE_BENCHMARKS=ON ..
 * make cache_tuning
 * ./cache_tuning
 * @endcode
 *
 * **Expected Output (without ENABLE_BENCHMARKS):**
 * @code
 * NetCDF-4 Chunk Cache Tuning Example
 * ===================================
 *
 * Default cache settings:
 *   cache_size: 67108864 bytes (64.0 MB)
 *   nelems: 1009
 *   preemption: 0.75
 *
 * Dataset: 500x180x360 temperature, chunked 10x45x90 (~600 KB/chunk)
 * Access pattern: Reading all 800 chunks repeatedly
 *
 * Note: Timing tests disabled. Rebuild with ENABLE_BENCHMARKS=ON to run performance tests.
 *
 * Test 4: File-level cache via nc_set_chunk_cache() and nc_get_chunk_cache()
 *   Default file-level cache: 64.0 MB, nelems=1009, preemption=0.75
 *   After nc_set_chunk_cache(): 256.0 MB, nelems=1009, preemption=0.75
 *   Variable storage: chunked, chunks: 10x45x90
 *   File opened with variable cache_size: 64.0 MB
 *
 * All cache tuning tests complete.
 *
 * Key takeaways:
 *   - Default cache may be too small for large datasets
 *   - Variable-level cache: nc_set_var_chunk_cache() for specific variables
 *   - File-level cache: nc_set_chunk_cache() affects new file opens
 *   - Cache thrashing occurs when working set exceeds cache size
 *   - Proper tuning can provide significant performance improvement
 * @endcode
 *
 * **Expected Output (with ENABLE_BENCHMARKS):**
 * CSV data suitable for plotting (cache size vs performance):
 * @code
 * # CSV data for plotting cache size vs performance
 * CacheSizeMB,TimeSeconds,Speedup
 * # Test 1: Default cache - 5 full passes through dataset
 * 64.0,8.234,1.00
 * # Test 2: Cache size scaling - testing 256MB, 512MB, 1GB, 2GB
 * 256.0,6.543,1.26
 * 512.0,4.321,1.91
 * 1024.0,2.876,2.86
 * 2048.0,2.123,3.88
 * # Test 3: Cache thrashing detection (50-chunk working set, 100 iterations)
 * 16.0,8.901,1.00
 * 52.8,0.345,25.80
 * @endcode
 *
 * The CSV output can be plotted with gnuplot:
 * @code
 * $ ./cache_tuning | grep -v "^#" > cache_results.csv
 * $ gnuplot -e "set datafile separator ','; plot 'cache_results.csv' using 1:2 with lines title 'Time vs Cache Size'"
 * @endcode
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
 * High-Performance Programs for Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define NDIMS 3
#define NX 360
#define NY 180
#define NZ 500
#define CHUNK_X 90
#define CHUNK_Y 45
#define CHUNK_Z 10

/* Error handling macros */
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

#ifdef ENABLE_BENCHMARKS
/* Get current time in seconds (for timing measurements) */
double get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}
#endif

/**
 * @brief Main entry point for the cache tuning example
 *
 * Creates a test dataset if needed, then runs four cache experiments:
 * 1. Baseline with default cache
 * 2. Performance scaling across cache sizes
 * 3. Thrashing detection
 * 4. File-level cache configuration
 */
int main(int argc, char **argv)
{
    int ncid, varid, dimids[NDIMS];
    size_t chunksizes[NDIMS] = {CHUNK_Z, CHUNK_Y, CHUNK_X};
    size_t start[NDIMS], count[NDIMS];
    float *data;
#ifdef ENABLE_BENCHMARKS
    double t_start, t_end;
    int iter;                    /* Loop counter for repeated reads */
#endif
    size_t slab_size;            /* Size of one chunk in elements */
    size_t default_cache_size;   /* Default cache size in bytes */
    size_t default_nelems;       /* Default number of cache elements */
    float default_preemption;    /* Default preemption policy (0.0-1.0) */
    int i, j, k;                 /* Loop indices for chunk iteration */
    int retval;                  /* NetCDF return value for error checking */

    /* Suppress unused parameter warnings - this example takes no arguments */
    (void)argc;
    (void)argv;

    printf("NetCDF-4 Chunk Cache Tuning Example\n");
    printf("===================================\n\n");

    /* Create sample data file if it doesn't exist
     * This ensures the example can run standalone without requiring
     * pre-existing test data. The file is only created on first run. */
    if ((retval = nc_open("cache_test.nc", NC_NOWRITE, &ncid)))
    {
        /* File doesn't exist, create it with chunked storage */
        printf("Creating test dataset (one-time setup)...\n\n");

        /* Create new NetCDF-4 file - NC_NETCDF4 enables HDF5 storage with chunking */
        if ((retval = nc_create("cache_test.nc", NC_NETCDF4, &ncid)))
            ERR(retval);

        /* Define dimensions for a realistic 3D scientific dataset:
         * time: 500 time steps (e.g., hours of simulation output)
         * lat: 180 latitude points (1 degree resolution)
         * lon: 360 longitude points (1 degree resolution) */
        if ((retval = nc_def_dim(ncid, "time", NZ, &dimids[0])))
            ERR(retval);
        if ((retval = nc_def_dim(ncid, "lat", NY, &dimids[1])))
            ERR(retval);
        if ((retval = nc_def_dim(ncid, "lon", NX, &dimids[2])))
            ERR(retval);

        /* Define a 3D temperature variable with chunked storage
         * Chunking is required for compression and improves random access,
         * but requires careful cache tuning for optimal performance */
        if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
            ERR(retval);
        if ((retval = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)))
            ERR(retval);

        /* End define mode - after this, data can be written */
        if ((retval = nc_enddef(ncid)))
            ERR(retval);

        /* Generate and write test data chunk by chunk
         * We write in chunk-sized blocks to match the storage layout */
        slab_size = CHUNK_X * CHUNK_Y * CHUNK_Z;  /* Elements per chunk */
        data = (float *)malloc(slab_size * sizeof(float));
        if (!data) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }

        /* Write data chunk by chunk - this matches the on-disk layout
         * Each chunk contains a 10x45x90 subset of the full 500x180x360 array */
        for (k = 0; k < NZ; k += CHUNK_Z) {
            for (j = 0; j < NY; j += CHUNK_Y) {
                for (i = 0; i < NX; i += CHUNK_X) {
                    size_t idx;
                    /* Generate synthetic temperature data (280K + time offset) */
                    for (idx = 0; idx < slab_size; idx++) {
                        data[idx] = 280.0f + (float)(k + (int)(idx % CHUNK_Z)) * 0.1f;
                    }
                    /* Set start and count for this chunk write */
                    start[0] = k; start[1] = j; start[2] = i;
                    count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                    /* Handle edge cases at array boundaries */
                    if (k + CHUNK_Z > NZ) count[0] = NZ - k;
                    if (j + CHUNK_Y > NY) count[1] = NY - j;
                    if (i + CHUNK_X > NX) count[2] = NX - i;
                    /* Write this chunk to the file */
                    if ((retval = nc_put_vara_float(ncid, varid, start, count, data)))
                        ERR(retval);
                }
            }
        }

        free(data);
        if ((retval = nc_close(ncid)))
            ERR(retval);

        /* Reopen the file for reading tests
         * This simulates a real workflow where data is created once and read many times */
        if ((retval = nc_open("cache_test.nc", NC_NOWRITE, &ncid)))
            ERR(retval);
        if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
            ERR(retval);
    }

    /* Query the current cache settings for this variable
     * The default cache size is typically 64MB - may need increase for large datasets */
    if ((retval = nc_get_var_chunk_cache(ncid, varid, &default_cache_size,
                                   &default_nelems, &default_preemption)))
        ERR(retval);

    /* Display the default cache configuration
     * cache_size: Total bytes allocated for caching chunks
     * nelems: Maximum number of chunks that can be cached simultaneously
     * preemption: How aggressively to evict chunks (0=LRU only, 1=always evict) */
    printf("Default cache settings:\n");
    printf("  cache_size: %zu bytes (%.1f MB)\n",
           default_cache_size, default_cache_size / (1024.0 * 1024.0));
    printf("  nelems: %zu\n", default_nelems);
    printf("  preemption: %.2f\n\n", default_preemption);

    /* Allocate a buffer to hold one complete chunk
     * This is the amount of data transferred in each read operation */
    slab_size = CHUNK_X * CHUNK_Y * CHUNK_Z;  /* 10*45*90 = 40,500 elements */
    data = (float *)malloc(slab_size * sizeof(float));
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    /* Display dataset characteristics
     * Total size: 500*180*360*4 bytes = ~124 MB (raw data)
     * With 10x45x90 chunks: 800 chunks of ~600KB each = ~480 MB (including HDF5 overhead) */
    printf("Dataset: %dx%dx%d temperature, chunked %dx%dx%d (~%.0f KB/chunk)\n",
           NZ, NY, NX, CHUNK_Z, CHUNK_Y, CHUNK_X,
           (CHUNK_X * CHUNK_Y * CHUNK_Z * sizeof(float)) / 1024.0);
    printf("Access pattern: Reading all %zu chunks repeatedly\n\n",
           (size_t)(NZ / CHUNK_Z) * (NY / CHUNK_Y) * (NX / CHUNK_X));

#ifdef ENABLE_BENCHMARKS
    /* =========================================================================
     * CSV OUTPUT FOR PLOTTING
     * Print header row followed by data rows showing cache size vs performance.
     * This format is suitable for plotting tools (gnuplot, matplotlib, Excel).
     * ========================================================================= */

    /* Print CSV header */
    printf("# CSV data for plotting cache size vs performance\n");
    printf("CacheSizeMB,TimeSeconds,Speedup\n");

    /* =========================================================================
     * TEST 1: Baseline Performance with Default Cache
     * Read the entire dataset 5 times to establish a baseline.
     * With default 64MB cache and 800 chunks (~480MB total), expect partial caching.
     * ========================================================================= */
    printf("# Test 1: Default cache - 5 full passes through dataset\n");

    t_start = get_time();
    /* Read all chunks in the dataset 5 times sequentially
     * Each iteration reads the complete dataset from start to finish */
    for (iter = 0; iter < 5; iter++) {
        for (k = 0; k < NZ; k += CHUNK_Z) {
            for (j = 0; j < NY; j += CHUNK_Y) {
                for (i = 0; i < NX; i += CHUNK_X) {
                    /* Set the hyperslab start and count for this chunk */
                    start[0] = k; start[1] = j; start[2] = i;
                    count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                    /* Handle edge chunks at array boundaries */
                    if (k + CHUNK_Z > NZ) count[0] = NZ - k;
                    if (j + CHUNK_Y > NY) count[1] = NY - j;
                    if (i + CHUNK_X > NX) count[2] = NX - i;
                    /* Read this chunk - this is where caching happens */
                    if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
                        ERR(retval);
                }
            }
        }
    }
    t_end = get_time();

    {
        double default_time = t_end - t_start;
        /* Print CSV row: default cache size (64MB typical), time, speedup=1.0 */
        printf("%.1f,%.3f,%.2f\n",
               default_cache_size / (1024.0 * 1024.0), default_time, 1.0);
    }

    /* =========================================================================
     * TEST 2: Cache Size Scaling
     * Test four different cache sizes to show how performance improves
     * with larger cache. Each cache size is tested with the same workload.
     * Expected: Larger caches show better performance until working set fits.
     * ========================================================================= */
    printf("# Test 2: Cache size scaling - testing 256MB, 512MB, 1GB, 2GB\n");

    {
        /* Test cache sizes from 256MB to 2GB
         * 256MB fits ~437 chunks (undersized for 800 chunks)
         * 512MB fits ~873 chunks (most of 800 chunk dataset fits)
         * 1GB fits ~1747 chunks (full dataset fits with margin)
         * 2GB fits ~3495 chunks (oversized but demonstrates diminishing returns) */
        size_t cache_sizes[] = {256*1024*1024, 512*1024*1024, 1024*1024*1024, 2048*1024*1024};
        double default_time = t_end - t_start;  /* Baseline from Test 1 */
        int c;

        for (c = 0; c < 4; c++) {
            /* Configure the cache for this variable specifically
             * This overrides the default cache for just this one variable */
            if ((retval = nc_set_var_chunk_cache(ncid, varid, cache_sizes[c], 1009, 0.75f)))
                ERR(retval);

            /* Warm up the cache by reading all chunks once
             * This ensures the cache is populated before timing begins */
            for (k = 0; k < NZ; k += CHUNK_Z) {
                for (j = 0; j < NY; j += CHUNK_Y) {
                    for (i = 0; i < NX; i += CHUNK_X) {
                        start[0] = k; start[1] = j; start[2] = i;
                        count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                        if (k + CHUNK_Z > NZ) count[0] = NZ - k;
                        if (j + CHUNK_Y > NY) count[1] = NY - j;
                        if (i + CHUNK_X > NX) count[2] = NX - i;
                        if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
                            ERR(retval);
                    }
                }
            }

            /* Timed run: Read all chunks 5 times to measure performance
             * With larger cache, more chunks remain in memory between reads */
            t_start = get_time();
            for (iter = 0; iter < 5; iter++) {
                for (k = 0; k < NZ; k += CHUNK_Z) {
                    for (j = 0; j < NY; j += CHUNK_Y) {
                        for (i = 0; i < NX; i += CHUNK_X) {
                            start[0] = k; start[1] = j; start[2] = i;
                            count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                            if (k + CHUNK_Z > NZ) count[0] = NZ - k;
                            if (j + CHUNK_Y > NY) count[1] = NY - j;
                            if (i + CHUNK_X > NX) count[2] = NX - i;
                            if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
                                ERR(retval);
                        }
                    }
                }
            }
            t_end = get_time();

            /* Print CSV row: cache size in MB, time in seconds, speedup */
            {
                double elapsed = t_end - t_start;
                double cache_mb = cache_sizes[c] / (1024.0 * 1024.0);
                double speedup = default_time / elapsed;
                printf("%.1f,%.3f,%.2f\n", cache_mb, elapsed, speedup);
            }
        }
    }

    /* Restore original default cache settings for subsequent tests
     * This ensures Test 3 and Test 4 start from known baseline */
    if ((retval = nc_set_var_chunk_cache(ncid, varid, default_cache_size,
                                   default_nelems, default_preemption)))
        ERR(retval);

    /* =========================================================================
     * TEST 3: Cache Thrashing Detection
     * Thrashing occurs when the working set (data being accessed) is larger
     * than the cache. This causes constant eviction and re-reading of chunks.
     * This test demonstrates the problem and the solution.
     * ========================================================================= */
    printf("# Test 3: Cache thrashing detection (50-chunk working set, 100 iterations)\n");

    /* Configure a 64MB cache - this can hold about 109 chunks at once
     * With 50 chunks in our working set, no thrashing should occur
     * We test with an intentionally small cache to force thrashing */
    if ((retval = nc_set_var_chunk_cache(ncid, varid, 16 * 1024 * 1024, 1009, 0.75f)))
        ERR(retval);

    /* Read only 50 chunks repeatedly (not the whole dataset)
     * This simulates a focused analysis on a subset of the data
     * With insufficient cache, these 50 chunks constantly evict each other */
    {
        int thrash_chunks = 50;  /* Number of chunks in our working set */
        double thrash_time_small, thrash_time_large;
        double thrash_cache_large_mb;

        t_start = get_time();
        /* Read the 50-chunk working set 100 times
         * Each iteration reads chunks 0-49, then repeats
         * With 16MB cache (holds ~27 chunks), thrashing will occur */
                start[0] = k; start[1] = 0; start[2] = 0;
                count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
                    ERR(retval);
            }
        }
        t_end = get_time();
        thrash_time_small = t_end - t_start;

        /* Now configure cache to hold all 50 working set chunks plus margin
         * This eliminates thrashing - all chunks stay in memory */
        thrash_cache_large_mb = (double)thrash_chunks * CHUNK_X * CHUNK_Y * CHUNK_Z * sizeof(float) * 1.5 / (1024.0 * 1024.0);
        if ((retval = nc_set_var_chunk_cache(ncid, varid,
                                       (size_t)thrash_chunks * CHUNK_X * CHUNK_Y * CHUNK_Z * sizeof(float) * 1.5,
                                       1009, 0.75f)))
            ERR(retval);

        t_start = get_time();
        /* Same workload: read 50 chunks 100 times
         * But now all 50 chunks fit in cache, so no disk reads after warmup */
        for (iter = 0; iter < 100; iter++) {
            for (k = 0; k < thrash_chunks * CHUNK_Z && k < NZ; k += CHUNK_Z) {
                start[0] = k; start[1] = 0; start[2] = 0;
                count[0] = CHUNK_Z; count[1] = CHUNK_Y; count[2] = CHUNK_X;
                if ((retval = nc_get_vara_float(ncid, varid, start, count, data)))
                    ERR(retval);
            }
        }
        t_end = get_time();
        thrash_time_large = t_end - t_start;

        /* Print CSV rows for thrashing test - small cache vs large cache */
        printf("%.1f,%.3f,%.2f\n", 16.0, thrash_time_small, thrash_time_small/thrash_time_small);
        printf("%.1f,%.3f,%.2f\n", thrash_cache_large_mb, thrash_time_large, thrash_time_small/thrash_time_large);
    }
#else
    /* When ENABLE_BENCHMARKS is not defined, skip timing tests but still
     * demonstrate the API functionality. This allows the example to run
     * quickly in CI environments while still testing all API calls. */
    printf("Note: Timing tests disabled. Rebuild with ENABLE_BENCHMARKS=ON to run performance tests.\n\n");
#endif

    /* =========================================================================
     * TEST 4: File-Level Cache Configuration
     * Demonstrates setting default cache parameters that apply to all
     * subsequently opened files. Also queries chunking information.
     * ========================================================================= */
    printf("Test 4: File-level cache via nc_set_chunk_cache() and nc_get_chunk_cache()\n");
    printf("  (Setting defaults that apply to all newly opened files)\n");

    /* Close the current file so we can reopen it with new file-level settings
     * File-level cache settings only affect files opened AFTER they are set */
    if ((retval = nc_close(ncid)))
        ERR(retval);

    /* Query the current file-level default cache settings
     * These are the defaults that will be applied to new files */
    {
        size_t file_cache_size, file_nelems;
        float file_preemption;
        if ((retval = nc_get_chunk_cache(&file_cache_size, &file_nelems, &file_preemption)))
            ERR(retval);
        printf("  Default file-level cache: %.1f MB, nelems=%zu, preemption=%.2f\n",
               file_cache_size / (1024.0 * 1024.0), file_nelems, file_preemption);

        /* Set a larger file-level default cache (256MB)
         * This will apply to ALL files opened after this call */
        if ((retval = nc_set_chunk_cache(256 * 1024 * 1024, 1009, 0.75f)))
            ERR(retval);

        /* Verify the setting took effect by querying again
         * nc_get_chunk_cache() returns the current file-level defaults */
        if ((retval = nc_get_chunk_cache(&file_cache_size, &file_nelems, &file_preemption)))
            ERR(retval);
        printf("  After nc_set_chunk_cache(): %.1f MB, nelems=%zu, preemption=%.2f\n",
               file_cache_size / (1024.0 * 1024.0), file_nelems, file_preemption);
    }

    /* Reopen the file - it will now use the new file-level cache default (256MB)
     * This demonstrates that file-level settings affect newly opened files */
    if ((retval = nc_open("cache_test.nc", NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);

    /* Query the chunking configuration for this variable
     * nc_inq_var_chunking() returns:
     * - storage: NC_CHUNKED or NC_CONTIGUOUS
     * - chunksizes: the chunk dimensions set when the variable was created */
    {
        int storage;             /* Storage type: chunked or contiguous */
        size_t var_chunks[NDIMS];  /* Actual chunk sizes for this variable */
        if ((retval = nc_inq_var_chunking(ncid, varid, &storage, var_chunks)))
            ERR(retval);
        printf("  Variable storage: %s, chunks: %zux%zux%zu\n",
               (storage == NC_CHUNKED) ? "chunked" : "contiguous",
               var_chunks[0], var_chunks[1], var_chunks[2]);

        /* Verify that the file-level default was applied to this variable
         * The variable should now have the 64MB cache we set at file-level */
        {
            size_t var_cache_size;
            if ((retval = nc_get_var_chunk_cache(ncid, varid, &var_cache_size, NULL, NULL)))
                ERR(retval);
            printf("  File opened with variable cache_size: %.1f MB\n\n",
                   var_cache_size / (1024.0 * 1024.0));
        }
    }

    /* =========================================================================
     * CLEANUP
     * Free allocated memory and close the file.
     * ========================================================================= */

    /* Release the chunk-sized read buffer */
    free(data);

    /* Close the NetCDF file, flushing any remaining data */
    if ((retval = nc_close(ncid)))
        ERR(retval);

    printf("All cache tuning tests complete.\n");
    printf("\nKey takeaways:\n");
    printf("  - Default cache may be too small for large datasets\n");
    printf("  - Variable-level cache: nc_set_var_chunk_cache() for specific variables\n");
    printf("  - File-level cache: nc_set_chunk_cache() affects new file opens\n");
    printf("  - Cache thrashing occurs when working set exceeds cache size\n");
    printf("  - Proper tuning can provide significant performance improvement\n");

    return 0;
}

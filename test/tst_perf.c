/**
 * @file
 * Test performance.
 *
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "ncsqueeze.h"
#include "ncsqueeze_test.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <netcdf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> /* Extra high precision time info. */
#include <sys/stat.h> /* To get file sizes. */

#define FILE_NAME "tst_perf.nc"
#define TEST "tst_perf"
#define STR_LEN 255
#define MAX_LEN 1024
#define X_NAME "X"
#define Y_NAME "Y"
#define NDIM2 2
#define NDIM3 3
#define VAR_NAME "Pike"
#define VAR_NAME_2 "Spock"
#define TIME_IN_STARFLEET "time_in_starfleet"
#define KLIGONS_PUNCHED "Klingons_punched"
#define PHASER_FIRED "phaser_fired"

/* This must be an even number. Set to 18 to test all 9 zlib
 * levels. Set to 2 to speed CI testing. */
#define NCOMPRESSION 3
#define MAX_COMPRESSION_STR 6

#define NX_BIG 5000
#define NY_BIG 100
#define NUM_REC 100

#define MIN_ZSTD 0
#define MAX_ZSTD 9
#define MIN_ZLIB 1
#define MIN_LZ4 1

#define UNCOMPRESSED 0
#define COMPRESS_ZSTD 1
#define COMPRESS_ZLIB 2
#define COMPRESS_LZ4 3
#define COMPRESS_BZIP2 4
#define COMPRESS_JPEG 5
#define COMPRESS_LZF 6

/* Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0;

/* Prototype from tst_utils.c. */
int nc4_timeval_subtract(struct timeval *result, struct timeval *x,
                         struct timeval *y);

int test_compression(int f)
{
    struct stat st;
    float *data_out;
    size_t x;
    int level = MIN_ZSTD;
    char compression[MAX_COMPRESSION_STR + 1];
    /* float a = 5.0; */
    char file_name[STR_LEN + 1];
    float *data_in;
    int ncid;
    int dimid[NDIM3];
    int varid;
    size_t start[NDIM3] = {0, 0, 0};
    size_t count[NDIM3] = {1, NX_BIG, NY_BIG};
    size_t chunksizes[NDIM3] = {1, NX_BIG, NY_BIG};
    struct timeval start_time, end_time, diff_time;
    int meta_write_us;
    int ret;

    /* Allocate memory for one record. */
    if (!(data_out = malloc(NX_BIG * NY_BIG * sizeof(float)))) ERR;

    if (!(data_in = malloc(NX_BIG * NY_BIG * sizeof(float)))) ERR;

    if (!f)
	strcpy(compression, "none");
    if (f == COMPRESS_ZSTD)
	strcpy(compression, "zstd");
    if (f == COMPRESS_ZLIB)
	strcpy(compression, "zlib");
    if (f == COMPRESS_LZ4)
	strcpy(compression, "lz4");
    if (f == COMPRESS_BZIP2)
	strcpy(compression, "bzip2");
    if (f == COMPRESS_JPEG)
	strcpy(compression, "jpeg");
    if (f == COMPRESS_LZF)
	strcpy(compression, "lzf");

    sprintf(file_name, "%s_%s.nc", TEST, compression);

    /* Create file. */
    if (gettimeofday(&start_time, NULL)) ERR;
    if (nc_create(file_name, NC_CLOBBER|NC_NETCDF4, &ncid)) ERR;
    if (nc_def_dim(ncid, PHASER_FIRED, NC_UNLIMITED, &dimid[0])) ERR;
    if (nc_def_dim(ncid, TIME_IN_STARFLEET, NX_BIG, &dimid[1])) ERR;
    if (nc_def_dim(ncid, KLIGONS_PUNCHED, NY_BIG, &dimid[2])) ERR;
    if (nc_def_var(ncid, VAR_NAME_2, NC_FLOAT, NDIM3, dimid, &varid)) ERR;
    if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)) ERR;
    
    /* The parameter f denotes the desired compression. */
    if (f)
    {
	if (f == COMPRESS_ZSTD)
	{
	    if ((ret = nc_def_var_zstandard(ncid, varid, level)))
	    {
		printf("ret %d\n", ret);
		ERR;
	    }
	}
	else if (f == COMPRESS_ZLIB)
	{
	    level = MIN_ZLIB;
	    if (nc_def_var_deflate(ncid, varid, 0, 1, level)) ERR;
	}
#if BUILD_LZ4    
	else if (f == COMPRESS_LZ4)
	{
	    level = MIN_LZ4;
	    if (nc_def_var_lz4(ncid, varid, level)) ERR;
	}
#endif    
#if BUILD_BZIP2    
	else if (f == COMPRESS_BZIP2)
	{
	    level = 3;
	    if (nc_def_var_bzip2(ncid, varid, level)) ERR;
	}
#endif    
#if BUILD_JPEG    
	else if (f == COMPRESS_JPEG)
	{
	    if (nc_def_var_jpeg(ncid, varid, 50, NX_BIG * sizeof(float), NY_BIG, 0)) ERR;
	}
#endif    
#if BUILD_LZF    
	else if (f == COMPRESS_LZF)
	{
	    if (nc_def_var_lzf(ncid, varid)) ERR;
	}
#endif
    }

    /* Write the data records. */
    for (start[0] = 0; start[0] < NUM_REC; start[0]++)
    {
	/* Create a new record to write. */
	for (x = 0; x < NX_BIG * NY_BIG; x++)
	    data_out[x] = 1014.0 - ((start[0] + 1.0) * 10) + (rand() % 20 - 10) + 9.0/(x + 1);
	if (nc_put_vara_float(ncid, varid, start, count, data_out)) ERR;
    }

    /* Close the file. */
    if (nc_close(ncid)) ERR;
    if (gettimeofday(&end_time, NULL)) ERR;
    if (nc4_timeval_subtract(&diff_time, &end_time, &start_time)) ERR;
    meta_write_us = (int)diff_time.tv_sec * MILLION + (int)diff_time.tv_usec;
    stat(file_name, &st);
    printf("%s, %.2f, %.2f\n", (f ? compression : "none"), (float)meta_write_us/MILLION,
	   (float)st.st_size/MILLION);
	    
    /* Check file. */
    {
	int ncid;
	int varid = 0;
	/* int level_in, deflate; */
		
	if (nc_open(file_name, NC_NOWRITE, &ncid)) ERR;
	/* if (nc_inq_var_deflate(ncid, varid, &deflate, &level_in)) ERR; */
	/* if (f) */
	/* { */
	/*     if (f < NCOMPRESSION / 2 + 1) */
	/*     { */
	/* 	if (!deflate) ERR; */
	/*     } */
	/*     else */
	/*     { */
	/*     } */
	/* } */
	/* else */
	/* { */
	/*     if (deflate) ERR; */
	/* } */
	for (start[0] = 0; start[0] < NUM_REC; start[0]++)
	{
	    if (nc_get_vara_float(ncid, varid, start, count, data_in)) ERR;
	    for (x = 0; x < NX_BIG * NY_BIG; x++)
		if (data_in[x] != data_out[x])
		{
		    printf("data_out[%ld] %g data_in[%ld] %g\n", x, data_out[x], x, data_in[x]);
		    ERR;
		}
	}
	if (nc_close(ncid)) ERR;
    }

    free(data_in);
    free(data_out);
    return 0;
}

int
main()
{
    printf("\n*** Checking Performance of filters.\n");
    printf("*** Checking zlib performance on large float data set...");
    printf("\ncompression, write time (s), file size (MB)\n");

    /* Initialize random numbers. */
    srand(time(NULL));
    
    test_compression(UNCOMPRESSED);
    test_compression(COMPRESS_ZSTD);
    test_compression(COMPRESS_ZLIB);
#if BUILD_LZ4    
    test_compression(COMPRESS_LZ4);
#endif    
#if BUILD_BZIP2    
    test_compression(COMPRESS_BZIP2);
#endif    
/* #if BUILD_JPEG    
    test_compression(COMPRESS_JPEG);
#endif    
#if BUILD_LZF    
    test_compression(COMPRESS_LZF);
#endif    
 */    
    SUMMARIZE_ERR;
    FINAL_RESULTS;
}

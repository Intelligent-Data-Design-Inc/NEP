/**
 * @file
 * Header file for data from the NetCDF Community Codec Repository library.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _NCCOMPRESS_H
#define _NCCOMPRESS_H

#include <stdio.h>
#include <netcdf.h>
#include <netcdf_filter.h>

/** The filter ID for BZIP2 compression. */
#define BZIP2_ID 307

/** The filter ID for LZ4 compression. */
#define LZ4_ID 32004

/** The filter ID for JPEG compression. */
#define JPEG_ID 32019

/** The filter ID for LZF compression. */
#define LZF_ID 32000

/** Number of elements in JPEG parameter array. */
#define NCC_JPEG_NELEM 4

#if defined(__cplusplus)
extern "C" {
#endif

    /* Library prototypes... */
    int nc_def_var_bzip2(int ncid, int varid, int level);
    int nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp);
    int nc_def_var_lz4(int ncid, int varid, int level);
    int nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp);
    int nc_def_var_jpeg(int ncid, int varid, int quality_factor, int nx,
			int ny, int rgb);
    int nc_inq_var_jpeg(int ncid, int varid, int *jpegp, int *quality_factorp, int *nxp,
			int *nyp, int *rgbp);
    int nc_def_var_lzf(int ncid, int varid);
    int nc_inq_var_lzf(int ncid, int varid, int *lzfp);
    
#if defined(__cplusplus)
}
#endif

#endif /* _NCCOMPRESS_H */

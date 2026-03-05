/**
 * @file nep.h
 * @brief NetCDF Extension Pack (NEP) - Public API
 *
 * Defines compression filter IDs and API, and the centralized mapping of
 * file formats to NetCDF User-Defined Format (UDF) slot numbers.
 *
 * @section udf_slots UDF Slot Allocation
 *
 * NetCDF-C 4.10.0+ provides 10 UDF slots (UDF0-UDF9):
 * - UDF0: GeoTIFF BigTIFF (magic: "II+")
 * - UDF1: GeoTIFF standard TIFF (magic: "II*")
 * - UDF2: NASA CDF format (magic: 0xCDF30001)
 * - UDF3: GRIB2 format (reserved)
 * - UDF4-UDF9: Reserved for future use
 *
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NEP_H
#define NEP_H

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

/**
 * @defgroup nep_udf_slots NEP UDF Slot Allocation
 * @{
 */

/** GeoTIFF BigTIFF format uses UDF0 slot */
#define NEP_UDF_GEOTIFF_BIGTIFF NC_UDF0

/** GeoTIFF standard TIFF format uses UDF1 slot */
#define NEP_UDF_GEOTIFF_STANDARD NC_UDF1

/** NASA CDF format uses UDF2 slot */
#define NEP_UDF_CDF NC_UDF2

/** GRIB2 format uses UDF3 slot (reserved) */
#define NEP_UDF_GRIB2 NC_UDF3

/** @} */

/**
 * @defgroup nep_magic_numbers Format Magic Numbers
 * @{
 */

/** GeoTIFF standard TIFF magic number: "II*" */
#define NEP_MAGIC_GEOTIFF_STANDARD "II*"

/** GeoTIFF BigTIFF magic number: "II+" */
#define NEP_MAGIC_GEOTIFF_BIGTIFF "II+"

/** NASA CDF magic number: 0xCDF30001 */
#define NEP_MAGIC_CDF "\xCD\xF3\x00\x01"

/** @} */

/**
 * @defgroup nep_format_names Format Display Names
 * @{
 */

/** GeoTIFF format display name */
#define NEP_FORMAT_NAME_GEOTIFF "GeoTIFF"

/** NASA CDF format display name */
#define NEP_FORMAT_NAME_CDF "NASA CDF"

/** GRIB2 format display name */
#define NEP_FORMAT_NAME_GRIB2 "GRIB2"

/** @} */

#endif /* NEP_H */

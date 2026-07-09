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
 * - UDF2: GRIB2 meteorological data (magic: "GRIB")
 * - UDF3: FITS astronomical data (magic: "SIMPLE")
 * - UDF4: NASA CDF format (magic: 0xCDF30001)
 * - UDF5: PDS4 planetary data system (magic: "<?xml")
 * - UDF6-UDF9: Reserved for future use
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

/** The filter ID for LZF compression. */
#define LZF_ID 32000

#if defined(__cplusplus)
extern "C" {
#endif

    /* Library prototypes... */
    int nc_def_var_bzip2(int ncid, int varid, int level);
    int nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp);
    int nc_def_var_lz4(int ncid, int varid, int level);
    int nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp);
    int nc_def_var_lzf(int ncid, int varid);
    int nc_inq_var_lzf(int ncid, int varid, int *lzfp);
    
#if defined(__cplusplus)
}
#endif

/**
 * @defgroup nep_udf_slots NEP UDF Slot Allocation
 * @{
 *
 * NetCDF-C exposes ten User-Defined Format (UDF) slots (UDF0–UDF9). Each NEP
 * format handler occupies a permanently assigned slot so that multiple handlers
 * can be enabled simultaneously without conflict. Slot assignments are stable
 * across NEP releases; UDF6–UDF9 are reserved for future formats.
 *
 * | Slot  | Macro                      | Format                   | Magic       |
 * |-------|----------------------------|--------------------------|-------------|
 * | UDF0  | NEP_UDF_GEOTIFF_BIGTIFF    | GeoTIFF BigTIFF          | `II+`       |
 * | UDF1  | NEP_UDF_GEOTIFF_STANDARD   | GeoTIFF standard TIFF    | `II*`       |
 * | UDF2  | NEP_UDF_GRIB2              | GRIB2 meteorological     | `GRIB`      |
 * | UDF3  | NEP_UDF_FITS               | FITS astronomical        | `SIMPLE`    |
 * | UDF4  | NEP_UDF_CDF                | NASA CDF space physics   | `0xCDF30001`|
 * | UDF5  | NEP_UDF_PDS4               | NASA/ESA PDS4 planetary  | `<?xml`     |
 * | UDF6–9| —                          | Reserved                 | —           |
 *
 * Call `NC_GEOTIFF_initialize()`, `NC_GRIB2_initialize()`, `NC_FITS_initialize()`,
 * `NC_CDF_initialize()`, or `NC_PDS4_initialize()` to register the corresponding
 * handler before calling `nc_open()`. With `.ncrc` autoload (NetCDF-C main branch)
 * no explicit call is needed.
 */

/** GeoTIFF BigTIFF format uses UDF0 slot */
#define NEP_UDF_GEOTIFF_BIGTIFF NC_UDF0

/** GeoTIFF standard TIFF format uses UDF1 slot */
#define NEP_UDF_GEOTIFF_STANDARD NC_UDF1

/** GRIB2 meteorological format uses UDF2 slot */
#define NEP_UDF_GRIB2 NC_UDF2

/** NASA CDF format uses UDF4 slot */
#define NEP_UDF_CDF NC_UDF4

/** FITS astronomical data format uses UDF3 slot */
#define NEP_UDF_FITS NC_UDF3

/** PDS4 planetary data system format uses UDF5 slot */
#define NEP_UDF_PDS4 NC_UDF5

/** @} */

/**
 * @defgroup nep_magic_numbers Format Magic Numbers
 * @{
 *
 * Each NEP format handler is identified by a short byte sequence (magic number)
 * at the start of the file. NetCDF-C compares the first bytes of any file opened
 * with `nc_open()` against each registered magic string to select the correct
 * UDF handler. All magic strings are null-terminated C strings except
 * `NEP_MAGIC_CDF`, which contains embedded null bytes and must be matched by
 * length.
 *
 * | Macro                       | Value         | Format                 | Notes                              |
 * |-----------------------------|---------------|------------------------|-------------------------------------|
 * | NEP_MAGIC_GEOTIFF_STANDARD  | `"II*"`        | GeoTIFF standard TIFF  | Little-endian TIFF byte-order mark |
 * | NEP_MAGIC_GEOTIFF_BIGTIFF   | `"II+"`        | GeoTIFF BigTIFF        | Little-endian BigTIFF marker       |
 * | NEP_MAGIC_GRIB2             | `"GRIB"`       | GRIB2                  | All GRIB editions share this magic |
 * | NEP_MAGIC_FITS              | `"SIMPLE"`     | FITS                   | First 6 bytes of every FITS file   |
 * | NEP_MAGIC_CDF               | `\xCD\xF3\x00\x01` | NASA CDF          | 4-byte binary signature            |
 * | NEP_MAGIC_PDS4              | `"<?xml"`      | PDS4                   | XML declaration; namespace checked |
 */

/** GeoTIFF standard TIFF magic number: "II*" */
#define NEP_MAGIC_GEOTIFF_STANDARD "II*"

/** GeoTIFF BigTIFF magic number: "II+" */
#define NEP_MAGIC_GEOTIFF_BIGTIFF "II+"

/** NASA CDF magic number: 0xCDF30001 */
#define NEP_MAGIC_CDF "\xCD\xF3\x00\x01"

/** GRIB2 magic number: "GRIB" */
#define NEP_MAGIC_GRIB2 "GRIB"

/** FITS magic number: "SIMPLE" (first 6 bytes of a FITS file) */
#define NEP_MAGIC_FITS "SIMPLE"

/** PDS4 magic number: XML label files begin with "<?xml" */
#define NEP_MAGIC_PDS4 "<?xml"

/** @} */

/**
 * @defgroup nep_format_names Format Display Names
 * @{
 *
 * Human-readable display name strings for each NEP format handler. These are
 * used in log messages, error output, and `.ncrc` UDF registration entries to
 * identify the active handler. Use these macros rather than hard-coded strings
 * to ensure consistent naming across the library.
 *
 * | Macro                    | Value         | Format                  |
 * |--------------------------|---------------|-------------------------|
 * | NEP_FORMAT_NAME_GEOTIFF  | `"GeoTIFF"`   | GeoTIFF / BigTIFF       |
 * | NEP_FORMAT_NAME_GRIB2    | `"GRIB2"`     | GRIB2 meteorological    |
 * | NEP_FORMAT_NAME_FITS     | `"FITS"`      | FITS astronomical       |
 * | NEP_FORMAT_NAME_CDF      | `"NASA CDF"`  | NASA CDF space physics  |
 * | NEP_FORMAT_NAME_PDS4     | `"PDS4"`      | NASA/ESA PDS4 planetary |
 */

/** GeoTIFF format display name */
#define NEP_FORMAT_NAME_GEOTIFF "GeoTIFF"

/** NASA CDF format display name */
#define NEP_FORMAT_NAME_CDF "NASA CDF"

/** GRIB2 format display name */
#define NEP_FORMAT_NAME_GRIB2 "GRIB2"

/** FITS format display name */
#define NEP_FORMAT_NAME_FITS "FITS"

/** PDS4 format display name */
#define NEP_FORMAT_NAME_PDS4 "PDS4"

/** @} */

#endif /* NEP_H */

/**
 * @file NEP.h
 * @brief NetCDF Extension Pack (NEP) - Master UDF Slot Allocation
 * 
 * This header defines the centralized mapping of file formats to NetCDF 
 * User-Defined Format (UDF) slot numbers. This ensures consistent slot 
 * allocation across all NEP format handlers and prevents conflicts between 
 * different format implementations.
 * 
 * @section udf_slots UDF Slot Allocation Strategy
 * 
 * NetCDF-C provides user-defined format slots for custom format handlers.
 * NetCDF-C 4.10.0+ provides 10 slots (UDF0-UDF9):
 * 
 * - **UDF0**: GeoTIFF BigTIFF (little-endian, magic: "II+")
 * - **UDF1**: GeoTIFF standard TIFF (little-endian, magic: "II*")
 * - **UDF2**: NASA CDF format (magic: 0xCDF30001)
 * - **UDF3**: GRIB2 format (reserved for future use)
 * - **UDF4-UDF9**: Reserved for future format extensions
 * 
 * @section magic_numbers Magic Number Detection
 * 
 * Each format uses a magic number for automatic format detection:
 * - GeoTIFF standard: "II*\0" (0x49 0x49 0x2A 0x00)
 * - GeoTIFF BigTIFF: "II+\0" (0x49 0x49 0x2B 0x00)
 * - NASA CDF: "\xCD\xF3\x00\x01" (0xCD 0xF3 0x00 0x01)
 * 
 * @section conditional_compilation Conditional Compilation
 * 
 * NEP supports both old and new versions of NetCDF-C:
 * 
 * NetCDF-C 4.10.0+ automatically loads UDF plugins via RC file configuration.
 * Initialization functions are called by NetCDF-C; no manual nc_def_user_format()
 * calls are needed.
 * 
 * Example RC file (.ncrc):
 * @code
 * NETCDF.UDF2.LIBRARY=/path/to/libnep.so
 * NETCDF.UDF2.INIT=NC_CDF_initialize
 * NETCDF.UDF2.MAGIC=\xCD\xF3\x00\x01
 * @endcode
 * 
 * @section adding_formats Adding New Format Handlers
 * 
 * To add a new format handler to NEP:
 * 
 * 1. Choose an available UDF slot (UDF4-UDF9)
 * 2. Define slot constant in this file (e.g., NEP_UDF_MYFORMAT)
 * 3. Define magic number constant if applicable
 * 4. Create dispatch header (e.g., myformatdispatch.h)
 * 5. Implement dispatch table and initialization function
 * 6. Update this documentation
 * 
 * @author Edward Hartnett
 * @date 2026-02-02
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#ifndef NEP_H
#define NEP_H

#include "netcdf.h"

/**
 * @defgroup nep_udf_slots NEP UDF Slot Allocation
 * 
 * Constants defining which NetCDF-C UDF slots are used by each format handler.
 * These constants should be used instead of hardcoded NC_UDF* values to ensure
 * consistent slot allocation across the codebase.
 * 
 * @{
 */

/** GeoTIFF standard TIFF format (little-endian) uses UDF1 slot */
#define NEP_UDF_GEOTIFF_STANDARD NC_UDF1

/** GeoTIFF BigTIFF format (little-endian) uses UDF0 slot */
#define NEP_UDF_GEOTIFF_BIGTIFF NC_UDF0

/** NASA CDF format uses UDF2 slot */
#define NEP_UDF_CDF NC_UDF2

/** GRIB2 format uses UDF3 slot (reserved for future implementation) */
#define NEP_UDF_GRIB2 NC_UDF3

#ifdef NC_UDF4
/** Reserved for future format - UDF4 slot */
#define NEP_UDF_RESERVED_4 NC_UDF4
#endif

#ifdef NC_UDF5
/** Reserved for future format - UDF5 slot */
#define NEP_UDF_RESERVED_5 NC_UDF5
#endif

#ifdef NC_UDF6
/** Reserved for future format - UDF6 slot */
#define NEP_UDF_RESERVED_6 NC_UDF6
#endif

#ifdef NC_UDF7
/** Reserved for future format - UDF7 slot */
#define NEP_UDF_RESERVED_7 NC_UDF7
#endif

#ifdef NC_UDF8
/** Reserved for future format - UDF8 slot */
#define NEP_UDF_RESERVED_8 NC_UDF8
#endif

#ifdef NC_UDF9
/** Reserved for future format - UDF9 slot */
#define NEP_UDF_RESERVED_9 NC_UDF9
#endif

/** @} */ /* end of nep_udf_slots group */

/**
 * @defgroup nep_magic_numbers Format Magic Numbers
 * 
 * Magic number constants for automatic format detection. These are used
 * when registering UDF handlers with nc_def_user_format().
 * 
 * @{
 */

/** GeoTIFF standard TIFF magic number (little-endian): "II*" */
#define NEP_MAGIC_GEOTIFF_STANDARD "II*"

/** GeoTIFF BigTIFF magic number (little-endian): "II+" */
#define NEP_MAGIC_GEOTIFF_BIGTIFF "II+"

/** NASA CDF magic number: 0xCDF30001 (CDF version 3) */
#define NEP_MAGIC_CDF "\xCD\xF3\x00\x01"

/** @} */ /* end of nep_magic_numbers group */

/**
 * @defgroup nep_format_names Format Display Names
 * 
 * Human-readable format names for logging and error messages.
 * 
 * @{
 */

/** GeoTIFF format display name */
#define NEP_FORMAT_NAME_GEOTIFF "GeoTIFF"

/** NASA CDF format display name */
#define NEP_FORMAT_NAME_CDF "NASA CDF"

/** GRIB2 format display name */
#define NEP_FORMAT_NAME_GRIB2 "GRIB2"

/** @} */ /* end of nep_format_names group */

#endif /* NEP_H */

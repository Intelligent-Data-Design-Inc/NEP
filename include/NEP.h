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
 * Newer versions (4.9.3+) provide 10 slots (UDF0-UDF9), while older versions
 * only provide 2 slots (UDF0-UDF1). NEP adapts to the available slots:
 * 
 * **With newer NetCDF-C (NC_UDF2 defined):**
 * - **UDF0**: GeoTIFF BigTIFF (little-endian, magic: "II+")
 * - **UDF1**: GeoTIFF standard TIFF (little-endian, magic: "II*")
 * - **UDF2**: NASA CDF format (magic: 0xCDF30001)
 * - **UDF3**: GRIB2 format (reserved for future use)
 * - **UDF4-UDF9**: Reserved for future format extensions
 * 
 * **With older NetCDF-C (only UDF0-UDF1 available):**
 * - **UDF0**: NASA CDF format OR GeoTIFF BigTIFF (conflict!)
 * - **UDF1**: GRIB2 format OR GeoTIFF standard TIFF (conflict!)
 * 
 * Note: With older NetCDF-C, only one format can be used at a time due to
 * slot conflicts. Applications must choose which format to enable.
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
 * - **New NetCDF-C** (with NC_HAS_UDF_SELF_LOAD):
 *   - UDF plugins loaded automatically via RC file configuration
 *   - Initialization functions called by NetCDF-C
 *   - No manual nc_def_user_format() calls needed
 * 
 * - **Old NetCDF-C** (without NC_HAS_UDF_SELF_LOAD):
 *   - Applications must call initialization functions explicitly
 *   - Initialization functions call nc_def_user_format() to register
 *   - Manual registration required at startup
 * 
 * Use HAVE_NETCDF_UDF_SELF_REGISTRATION to conditionally compile:
 * @code
 * #ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
 *   // Old NetCDF-C: manually register dispatch table
 *   nc_def_user_format(NEP_UDF_GEOTIFF_STANDARD, &dispatch, magic);
 * #endif
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

/* Extended UDF slots (UDF2-UDF9) are only available in newer NetCDF-C versions.
 * For older versions, we fall back to using available slots. */
#ifdef NC_UDF2
/** NASA CDF format uses UDF2 slot */
#define NEP_UDF_CDF NC_UDF2
#else
/** NASA CDF format uses UDF0 slot (fallback for older NetCDF-C) */
#define NEP_UDF_CDF NC_UDF0
#endif

#ifdef NC_UDF3
/** GRIB2 format uses UDF3 slot (reserved for future implementation) */
#define NEP_UDF_GRIB2 NC_UDF3
#else
/** GRIB2 format uses UDF1 slot (fallback for older NetCDF-C) */
#define NEP_UDF_GRIB2 NC_UDF1
#endif

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

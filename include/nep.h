/**
 * @file nep.h
 * @brief NetCDF Expansion Pack (NEP) - Public API
 *
 * Defines NEP logging and the centralized mapping of file formats to
 * NetCDF User-Defined Format (UDF) slot numbers.
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
 * - UDF6: DICOM medical imaging data (magic: "DICM")
 * - UDF7: Legacy PDB protein structure data (magic: "HEADER")
 * - UDF8: PDBx/mmCIF protein structure data (magic: "data_")
 * - UDF9: NEXTCDF-4 HDF5 backend
 *
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NEP_H
#define NEP_H

#include <stdio.h>
#include <stdint.h>
#include <netcdf.h>
#include <netcdf_filter.h>

/** NEXTCDF-4 16-bit IEEE 754 half-precision floating point type.
 *  Untyped I/O uses raw uint16_t; typed float/double I/O converts
 *  through HDF5. */
#define NC_FLOAT16     17
/** NEXTCDF-4 16-bit bfloat16 floating point type (HDF5 2.1.1+).
 *  Untyped I/O uses raw uint16_t; typed float/double I/O converts
 *  through HDF5. */
#define NC_BFLOAT16    18
/** NEXTCDF-4 8-bit E4M3 floating point type (HDF5 2.1.1+).
 *  Untyped I/O (raw uint8_t) only. */
#define NC_FLOAT8_E4M3 19
/** NEXTCDF-4 8-bit E5M2 floating point type (HDF5 2.1.1+).
 *  Untyped I/O (raw uint8_t) only. */
#define NC_FLOAT8_E5M2 20
/** NEXTCDF-4 6-bit E2M3 floating point type (HDF5 2.1.1+).
 *  Untyped I/O (raw uint8_t) only. */
#define NC_FLOAT6_E2M3 21
/** NEXTCDF-4 6-bit E3M2 floating point type (HDF5 2.1.1+).
 *  Untyped I/O (raw uint8_t) only. */
#define NC_FLOAT6_E3M2 22
/** NEXTCDF-4 4-bit E2M1 floating point type (HDF5 2.1.1+).
 *  Untyped I/O (raw uint8_t) only. */
#define NC_FLOAT4_E2M1 23
/** NEXTCDF-4 single-precision complex number type. */
#define NC_COMPLEX     24
/** NEXTCDF-4 double-precision complex number type. */
#define NC_DOUBLECOMPLEX 25
/** NEXTCDF-4 8-bit bitfield type. */
#define NC_BITFIELD8   26
/** NEXTCDF-4 16-bit bitfield type. */
#define NC_BITFIELD16  27
/** NEXTCDF-4 32-bit bitfield type. */
#define NC_BITFIELD32  28
/** NEXTCDF-4 64-bit bitfield type. */
#define NC_BITFIELD64  29
/** NEXTCDF-4 object reference type. */
#define NC_REF_OBJECT  30
/** NEXTCDF-4 dataset region reference type. */
#define NC_REF_REGION  31

/** @def NEP_HAVE_NATIVE_BFLOAT16
 *  1 when nc_bfloat16_t is the compiler's `__bf16` type, 0 when it is
 *  `uint16_t`. */
/** @typedef nc_bfloat16_t
 *  One bfloat16 value in memory: 2 bytes holding the bfloat16 bit
 *  pattern either way. With `__bf16`, C casts convert numerically, so
 *  code that holds raw bits in `uint16_t` passes them by casting the
 *  pointer (`(nc_bfloat16_t *)(void *)bits`), not by assigning values. */
#if defined(__BFLT16_MAX__)
#define NEP_HAVE_NATIVE_BFLOAT16 1
typedef __bf16 nc_bfloat16_t;
#else
#define NEP_HAVE_NATIVE_BFLOAT16 0
typedef uint16_t nc_bfloat16_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

    /* Library prototypes... */
    /** Set the NEP logging level (0=errors, higher=more verbose, -1=off). */
    int nep_set_log_level(int new_level);

    /**
     * @brief Read one value from a variable as bfloat16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param indexp Element coordinates (NULL reads element 0).
     * @param ip Returned bfloat16 value.
     * @return NC_NOERR on success; error as in nc_get_vara_bfloat16().
     * @see nc_get_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_var1_bfloat16(int ncid, int varid, const size_t *indexp, nc_bfloat16_t *ip);

    /**
     * @brief Write one value to a variable as bfloat16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param indexp Element coordinates (NULL writes element 0).
     * @param op The bfloat16 value to write.
     * @return NC_NOERR on success; error as in nc_put_vara_bfloat16().
     * @see nc_put_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_var1_bfloat16(int ncid, int varid, const size_t *indexp, const nc_bfloat16_t *op);

    /**
     * @brief Read a whole variable as bfloat16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param ip Returned bfloat16 values, one per element.
     * @return NC_NOERR on success; error as in nc_get_vara_bfloat16().
     * @see nc_get_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_var_bfloat16(int ncid, int varid, nc_bfloat16_t *ip);

    /**
     * @brief Write a whole variable from bfloat16 values.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param op The bfloat16 values to write, one per element.
     * @return NC_NOERR on success; error as in nc_put_vara_bfloat16().
     * @see nc_put_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_var_bfloat16(int ncid, int varid, const nc_bfloat16_t *op);

    /**
     * @brief Read a strided hyperslab of a variable as bfloat16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param stridep Stride in each dimension.
     * @param ip Returned bfloat16 values.
     * @return NC_NOERR on success; error as in nc_get_vara_bfloat16().
     * @see nc_get_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_vars_bfloat16(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, nc_bfloat16_t *ip);

    /**
     * @brief Write a strided hyperslab of a variable from bfloat16 values.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param stridep Stride in each dimension.
     * @param op The bfloat16 values to write.
     * @return NC_NOERR on success; error as in nc_put_vara_bfloat16().
     * @see nc_put_vara_bfloat16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_vars_bfloat16(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const nc_bfloat16_t *op);

    /**
     * @brief Read a hyperslab of a variable as bfloat16.
     *
     * Works on any file the netCDF API can open. An NC_BFLOAT16
     * variable returns its raw bit patterns with no conversion. An
     * NC_FLOAT or NC_DOUBLE variable is read with the corresponding
     * typed call and every value is converted to bfloat16 by NEP,
     * rounding to nearest with ties to even (a single rounding from
     * double); the bfloat16-to-float/double direction is exact. An
     * NC_CHAR variable returns NC_ECHAR; every other variable type
     * (including NC_FLOAT16) returns NC_EBADTYPE, and nothing is read.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID (NC_BFLOAT16, NC_FLOAT or NC_DOUBLE).
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param ip Returned bfloat16 values.
     * @return NC_NOERR on success; NC_ERANGE if a finite value
     * overflowed to ±inf (all values are still converted); NC_EBADTYPE,
     * NC_ECHAR, NC_EINVAL, NC_ENOMEM or other errors from the
     * underlying netCDF call.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_vara_bfloat16(int ncid, int varid, const size_t *startp, const size_t *countp, nc_bfloat16_t *ip);

    /**
     * @brief Write a hyperslab of a variable from bfloat16 values.
     *
     * Works on any file the netCDF API can open. An NC_BFLOAT16
     * variable stores the raw bit patterns with no conversion. An
     * NC_FLOAT or NC_DOUBLE variable is written with the corresponding
     * typed call after every value is widened exactly to float or
     * double by NEP; puts never overflow. An NC_CHAR variable returns
     * NC_ECHAR; every other variable type (including NC_FLOAT16)
     * returns NC_EBADTYPE, and nothing is written.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID (NC_BFLOAT16, NC_FLOAT or NC_DOUBLE).
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param op The bfloat16 values to write.
     * @return NC_NOERR on success; NC_EBADTYPE, NC_ECHAR, NC_EINVAL,
     * NC_ENOMEM or other errors from the underlying netCDF call.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_vara_bfloat16(int ncid, int varid, const size_t *startp, const size_t *countp, const nc_bfloat16_t *op);

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
 * across NEP releases.
 *
 * | Slot  | Macro                      | Format                   | Magic       |
 * |-------|----------------------------|--------------------------|-------------|
 * | UDF0  | NEP_UDF_GEOTIFF_BIGTIFF    | GeoTIFF BigTIFF          | `II+`       |
 * | UDF1  | NEP_UDF_GEOTIFF_STANDARD   | GeoTIFF standard TIFF    | `II*`       |
 * | UDF2  | NEP_UDF_GRIB2              | GRIB2 meteorological     | `GRIB`      |
 * | UDF3  | NEP_UDF_FITS               | FITS astronomical        | `SIMPLE`    |
 * | UDF4  | NEP_UDF_CDF                | NASA CDF space physics   | `0xCDF30001`|
 * | UDF5  | NEP_UDF_PDS4               | NASA/ESA PDS4 planetary  | `<?xml`     |
 * | UDF6  | NEP_UDF_DICOM              | DICOM medical imaging    | `DICM`      |
 * | UDF7  | NEP_UDF_PDB                | Legacy PDB protein data  | `HEADER`    |
 * | UDF8  | NEP_UDF_MMCIF              | PDBx/mmCIF protein data   | `data_`     |
 * | UDF9  | NEP_UDF_NEXTCDF4           | NEXTCDF-4 HDF5 backend   | explicit    |
 *
 * Call `NC_GEOTIFF_initialize()`, `NC_GRIB2_initialize()`, `NC_FITS_initialize()`,
 * `NC_CDF_initialize()`, `NC_PDS4_initialize()`, `NC_DICOM_initialize()`,
 * `NC_PDB_initialize()`, or `NC_MMCIF_initialize()` to register the corresponding
 * handler before calling
 * `nc_open()`. With `.ncrc` autoload (NetCDF-C main branch) no explicit call
 * is needed.
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

/** DICOM medical imaging data format uses UDF6 slot */
#define NEP_UDF_DICOM NC_UDF6

/** Legacy PDB protein structure data format uses UDF7 slot */
#define NEP_UDF_PDB NC_UDF7

/** PDBx/mmCIF protein structure data format uses UDF8 slot */
#define NEP_UDF_MMCIF NC_UDF8

/** NEXTCDF-4 HDF5 backend uses UDF9 slot */
#define NEP_UDF_NEXTCDF4 NC_UDF9

/** Explicit create/open mode for the NEXTCDF-4 backend */
#define NC_NEXTCDF4 NEP_UDF_NEXTCDF4

/** Request NetCDF-4-model compatibility from the NEXTCDF-4 backend */
#define NC_NETCDF4_MODEL 0x04000000

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
 * | NEP_MAGIC_DICOM             | `"DICM"`       | DICOM                  | 4-byte prefix at byte offset 128   |
 * | NEP_MAGIC_PDB               | `"HEADER"`     | Legacy PDB             | First record name of the file      |
 * | NEP_MAGIC_MMCIF             | `"data_"`      | PDBx/mmCIF             | First token of every CIF data block|
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

/** DICOM magic number: "DICM" prefix at byte offset 128 */
#define NEP_MAGIC_DICOM "DICM"

/** Legacy PDB magic number: "HEADER" record name at the start of the file */
#define NEP_MAGIC_PDB "HEADER"

/** PDBx/mmCIF magic number: "data_" token at the start of the file */
#define NEP_MAGIC_MMCIF "data_"

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
 * | NEP_FORMAT_NAME_DICOM    | `"DICOM"`     | DICOM medical imaging   |
 * | NEP_FORMAT_NAME_PDB      | `"PDB"`       | Legacy PDB protein data |
 * | NEP_FORMAT_NAME_MMCIF    | `"mmCIF"`     | PDBx/mmCIF protein data |
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

/** DICOM format display name */
#define NEP_FORMAT_NAME_DICOM "DICOM"

/** Legacy PDB format display name */
#define NEP_FORMAT_NAME_PDB "PDB"

/** PDBx/mmCIF format display name */
#define NEP_FORMAT_NAME_MMCIF "mmCIF"

/** @} */

#endif /* NEP_H */

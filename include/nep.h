/**
 * @file nep.h
 * @brief NetCDF Expansion Pack (NEP) - Public API
 *
 * Defines NEP logging and the centralized mapping of file formats to
 * NetCDF User-Defined Format (UDF) slot numbers.
 *
 * @section udf_slots UDF Slot Allocation
 *
 * Current netcdf-c `main` provides 64 UDF slots (UDF0-UDF63). NEP
 * occupies UDF0-UDF9:
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
 *  Untyped I/O uses raw uint16_t; typed float/double I/O uses NEP
 *  round-to-nearest-even, with single rounding from double. */
#define NC_FLOAT16     17
/** NEXTCDF-4 16-bit bfloat16 floating point type (HDF5 2.1.1+).
 *  Untyped I/O uses raw uint16_t; typed float/double I/O uses NEP
 *  round-to-nearest-even, with single rounding from double. */
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
/** NEXTCDF-4 HDF5 standard reference type (`H5T_STD_REF`). Elements
 *  are opaque 64-byte nc_std_ref_t tokens covering object, dataset
 *  region, and attribute references; see nc_ref_object() and friends.
 *  Native mode only. */
#define NC_STD_REF     30

/** @def NC_STD_REF_SIZE
 *  Bytes in one opaque NEXTCDF-4 reference token. Matches HDF5
 *  `H5R_REF_BUF_SIZE` (the size of `H5R_ref_t`). */
#define NC_STD_REF_SIZE 64

/** @typedef nc_std_ref_t
 *  One opaque NEXTCDF-4 reference token; layout-compatible with HDF5
 *  `H5R_ref_t` so tokens move to and from the HDF5 reference API
 *  without conversion. Never interpret the bytes. */
typedef struct nc_std_ref_s {
    union {
        unsigned char bytes[NC_STD_REF_SIZE];
        int64_t align;
    } u;
} nc_std_ref_t;

/** Object reference token kind, reported by nc_ref_get_type(). */
#define NC_STD_REF_OBJECT 1
/** Dataset region reference token kind, reported by nc_ref_get_type(). */
#define NC_STD_REF_REGION 2
/** Attribute reference token kind, reported by nc_ref_get_type(). */
#define NC_STD_REF_ATTR   3

/** Reference target kind for nc_ref_object(), nc_ref_attr() and the
 *  nc_deref_*() functions: a NetCDF variable (HDF5 dataset). */
#define NC_REF_OBJ_VAR  1
/** Reference target kind: a NetCDF group (HDF5 group). */
#define NC_REF_OBJ_GRP  2
/** Reference target kind: a NetCDF user-defined type (HDF5 committed
 *  datatype). */
#define NC_REF_OBJ_TYPE 3
/** Resolved object kind reported by nc_ref_resolve() and
 *  nc_ref_open_remote() when the stored path carried a `#attname`
 *  suffix: an attribute on the resolved owner. (Distinct from
 *  NC_STD_REF_ATTR, which shares its numeric value with
 *  NC_REF_OBJ_TYPE.) */
#define NC_REF_OBJ_ATT  4

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

/** @def NEP_HAVE_NATIVE_FLOAT16
 *  1 when nc_float16_t is the compiler's `_Float16` type, 0 when it is
 *  `uint16_t`. */
/** @typedef nc_float16_t
 *  One IEEE 754 binary16 value in memory: 2 bytes holding the float16
 *  bit pattern either way. With `_Float16`, C casts convert
 *  numerically, so code that holds raw bits in `uint16_t` passes them
 *  by casting the pointer (`(nc_float16_t *)(void *)bits`), not by
 *  assigning values. */
#if defined(__FLT16_MAX__)
#define NEP_HAVE_NATIVE_FLOAT16 1
typedef _Float16 nc_float16_t;
#else
#define NEP_HAVE_NATIVE_FLOAT16 0
typedef uint16_t nc_float16_t;
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

    /**
     * @brief Read one value from a variable as IEEE 754 float16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param indexp Element coordinates (NULL reads element 0).
     * @param ip Returned float16 value.
     * @return NC_NOERR on success; error as in nc_get_vara_float16().
     * @see nc_get_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_var1_float16(int ncid, int varid, const size_t *indexp, nc_float16_t *ip);

    /**
     * @brief Write one value to a variable as IEEE 754 float16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param indexp Element coordinates (NULL writes element 0).
     * @param op The float16 value to write.
     * @return NC_NOERR on success; error as in nc_put_vara_float16().
     * @see nc_put_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_var1_float16(int ncid, int varid, const size_t *indexp, const nc_float16_t *op);

    /**
     * @brief Read a whole variable as IEEE 754 float16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param ip Returned float16 values, one per element.
     * @return NC_NOERR on success; error as in nc_get_vara_float16().
     * @see nc_get_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_var_float16(int ncid, int varid, nc_float16_t *ip);

    /**
     * @brief Write a whole variable from IEEE 754 float16 values.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param op The float16 values to write, one per element.
     * @return NC_NOERR on success; error as in nc_put_vara_float16().
     * @see nc_put_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_var_float16(int ncid, int varid, const nc_float16_t *op);

    /**
     * @brief Read a strided hyperslab of a variable as IEEE 754 float16.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param stridep Stride in each dimension.
     * @param ip Returned float16 values.
     * @return NC_NOERR on success; error as in nc_get_vara_float16().
     * @see nc_get_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_vars_float16(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, nc_float16_t *ip);

    /**
     * @brief Write a strided hyperslab of a variable from IEEE 754 float16 values.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID.
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param stridep Stride in each dimension.
     * @param op The float16 values to write.
     * @return NC_NOERR on success; error as in nc_put_vara_float16().
     * @see nc_put_vara_float16()
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_vars_float16(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const nc_float16_t *op);

    /**
     * @brief Read a hyperslab of a variable as IEEE 754 float16.
     *
     * Works on any file the netCDF API can open. An NC_FLOAT16
     * variable returns its raw bit patterns with no conversion. An
     * NC_FLOAT or NC_DOUBLE variable is read with the corresponding
     * typed call and every value is converted to float16 by NEP,
     * rounding to nearest with ties to even (a single rounding from
     * double); the float16-to-float/double direction is exact. An
     * NC_CHAR variable returns NC_ECHAR; every other variable type
     * (including NC_BFLOAT16) returns NC_EBADTYPE, and nothing is read.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID (NC_FLOAT16, NC_FLOAT or NC_DOUBLE).
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param ip Returned float16 values.
     * @return NC_NOERR on success; NC_ERANGE if a finite value
     * overflowed to ±inf (all values are still converted); NC_EBADTYPE,
     * NC_ECHAR, NC_EINVAL, NC_ENOMEM or other errors from the
     * underlying netCDF call.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_get_vara_float16(int ncid, int varid, const size_t *startp, const size_t *countp, nc_float16_t *ip);

    /**
     * @brief Write a hyperslab of a variable from IEEE 754 float16 values.
     *
     * Works on any file the netCDF API can open. An NC_FLOAT16
     * variable stores the raw bit patterns with no conversion. An
     * NC_FLOAT or NC_DOUBLE variable is written with the corresponding
     * typed call after every value is widened exactly to float or
     * double by NEP; puts never overflow. An NC_CHAR variable returns
     * NC_ECHAR; every other variable type (including NC_BFLOAT16)
     * returns NC_EBADTYPE, and nothing is written.
     *
     * @param ncid NetCDF file ID.
     * @param varid Variable ID (NC_FLOAT16, NC_FLOAT or NC_DOUBLE).
     * @param startp Start index in each dimension.
     * @param countp Edge count in each dimension.
     * @param op The float16 values to write.
     * @return NC_NOERR on success; NC_EBADTYPE, NC_ECHAR, NC_EINVAL,
     * NC_ENOMEM or other errors from the underlying netCDF call.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_put_vara_float16(int ncid, int varid, const size_t *startp, const size_t *countp, const nc_float16_t *op);

    /**
     * @brief Create an HDF5 object reference to a variable, group, or
     * user-defined type in a NEXTCDF-4 file.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_object(int ncid, int obj_type, int objid, nc_std_ref_t *ref);

    /**
     * @brief Resolve an object reference token back to NetCDF ids.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_deref_object(int ncid, const nc_std_ref_t *ref, int *obj_typep,
                        int *grpidp, int *idp);

    /**
     * @brief Create a dataset region reference to a strided hyperslab
     * of a variable in a NEXTCDF-4 file.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_region(int ncid, int varid, const size_t *startp,
                      const size_t *countp, const ptrdiff_t *stridep,
                      nc_std_ref_t *ref);

    /**
     * @brief Resolve a dataset region reference token back to its
     * variable and selection.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_deref_region(int ncid, const nc_std_ref_t *ref, int *grpidp,
                        int *varidp, int *ndimsp, size_t *startp,
                        size_t *countp, ptrdiff_t *stridep);

    /**
     * @brief Create an HDF5 attribute reference to an attribute on a
     * variable, group, or committed datatype in a NEXTCDF-4 file.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_attr(int ncid, int obj_type, int objid,
                    const char *att_name, nc_std_ref_t *ref);

    /**
     * @brief Resolve an attribute reference token back to its owner
     * and attribute name.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_deref_attr(int ncid, const nc_std_ref_t *ref, int *obj_typep,
                      int *grpidp, int *idp, char *att_name);

    /**
     * @brief Create an object reference pointing into another file.
     *
     * The token encodes @p filename; storing it in an `NC_STD_REF`
     * variable or attribute of the file @p ncid produces a
     * cross-file reference. @p ncid is validated like nc_ref_object()
     * but is otherwise unused.
     *
     * @p filename must be an absolute filesystem path: a stored
     * relative name would be reinterpreted against whatever working
     * directory a later reader happens to have. The target file must
     * also be writable by the application — HDF5 may reopen persisted
     * external references read-write, so a read-only target can be
     * referenced but never dereferenced.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_object_ext(int ncid, const char *filename, const char *path,
                          nc_std_ref_t *ref);

    /**
     * @brief Create a dataset region reference pointing into another
     * file.
     *
     * Like nc_ref_region(), but the target is the variable named by
     * @p path in @p filename. @p filename must be an absolute
     * filesystem path and the target file must be writable by the
     * application, as for nc_ref_object_ext().
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_region_ext(int ncid, const char *filename, const char *path,
                          const size_t *startp, const size_t *countp,
                          const ptrdiff_t *stridep, nc_std_ref_t *ref);

    /**
     * @brief Create an attribute reference pointing into another file.
     *
     * Like nc_ref_attr(), but the target is the attribute @p att_name
     * on the object named by @p path in @p filename. @p path may name
     * any attribute-bearing object kind. @p filename must be an
     * absolute filesystem path and the target file must be writable by
     * the application, as for nc_ref_object_ext().
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_attr_ext(int ncid, const char *filename, const char *path,
                        const char *att_name, nc_std_ref_t *ref);

    /**
     * @brief Open the external file a reference token points into.
     *
     * The token records an absolute filename for its target file; this
     * function opens that file as a new NEXTCDF-4 session and returns
     * its NetCDF file ID. Pass the returned ncid to nc_deref_object(),
     * nc_deref_region(), or nc_deref_attr() to resolve the token; the
     * IDs those calls return belong to the target session and remain
     * valid until it is closed.
     *
     * The caller owns the returned session: close it with nc_close().
     * Each call returns an independent session, even for tokens into
     * the same file. Token lifetime is independent — release @p ref
     * with nc_ref_free() when done; freeing the token does not close
     * the target session, and closing the target session does not
     * affect the token.
     *
     * The target file must be writable by the application: HDF5 may
     * reopen persisted external references read-write, so the session
     * is opened with NC_WRITE even when only reads are intended.
     * Dereferencing never intentionally modifies target contents.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_open_target(int ncid, const nc_std_ref_t *ref,
                           int *target_ncidp);

    /**
     * @brief Report the kind of a reference token.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_get_type(const nc_std_ref_t *ref, int *ref_typep);

    /**
     * @brief Copy the file name stored in a reference token.
     *
     * Returns the name of the file the token points into, which
     * differs from the containing file only for external references.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_get_file_name(const nc_std_ref_t *ref, char *name, size_t size);

    /**
     * @brief Copy the object path stored in a reference token.
     *
     * For an attribute reference this is the path of the object that
     * carries the attribute; use nc_deref_attr() for the attribute
     * name itself.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_get_obj_name(const nc_std_ref_t *ref, char *name, size_t size);

    /**
     * @brief Release the resources held by a reference token.
     *
     * An HDF5 reference token holds a reference to the file it was
     * created in; until it is released the file cannot fully close.
     * Call this on every token obtained from nc_ref_object(),
     * nc_ref_region(), nc_ref_attr(), or the `nc_ref_*_ext()`
     * variants when the token is no longer needed. Tokens read from
     * a dataset or attribute may also be passed here. Releasing an
     * uninitialized or already-released token is safe.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_free(nc_std_ref_t *ref);

    /**
     * @brief Open a remote reference target and resolve its object path.
     *
     * Remote references are a stored convention, not an HDF5 token:
     * the application keeps a URL and an object path as strings (for
     * example in parallel NC_STRING variables or attributes), and this
     * function resolves one pair. The URL is opened with
     * `nc_open(url, NC_NOWRITE, ...)` — no NC_NEXTCDF4 flag — so any
     * remote-capable dispatch in the installed NetCDF-C may answer:
     * DAP2, DAP4, or NCZarr file/byte-range stores. (s3:// URLs
     * additionally require NetCDF-C built with NC_HAS_S3.) The
     * returned session is an ordinary upstream NetCDF file ID, not a
     * NEXTCDF-4 session.
     *
     * @p path follows the nc_ref_resolve() grammar: a leading `/`,
     * `/`-separated group components, a final component naming a
     * variable, group, or committed datatype, and an optional
     * `#attname` suffix selecting an attribute of that object.
     *
     * On success the caller owns @p target_ncidp and must close it
     * with nc_close(); each call returns an independent session. The
     * IDs reported through @p kindp/@p objidp belong to that session
     * and die with it. If the open succeeds but @p path fails to
     * resolve, the session is closed, @p target_ncidp is set to -1 and
     * the resolution error is returned. Remote access is read-only.
     *
     * Requires NEP built with `NEP_ENABLE_REMOTE_REFS` (see
     * NEP_HAS_REMOTE_REFS in `nep_meta.h`).
     *
     * @param ncid NetCDF file ID (NEXTCDF-4) of the file holding the
     * reference; provides context only.
     * @param url URL the installed NetCDF-C can open remotely.
     * @param path Object path inside the remote dataset (see
     * nc_ref_resolve()).
     * @param target_ncidp Receives the remote session's NetCDF file
     * ID; set to -1 on failure.
     * @param kindp Receives `NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP`,
     * `NC_REF_OBJ_TYPE`, or `NC_REF_OBJ_ATT` when @p path carried a
     * `#attname` suffix.
     * @param objidp Receives the resolved ID: a varid, group ncid,
     * typeid, or — for `NC_REF_OBJ_ATT` — the attribute owner's
     * varid, group ncid or typeid.
     * @param att_name Buffer receiving the attribute name when @p path
     * carried a `#attname` suffix; may be NULL otherwise.
     * @param att_name_size Size of @p att_name in bytes.
     * @return `NC_NOERR` on success; `NC_ENOTBUILT` when built without
     * `NEP_ENABLE_REMOTE_REFS`; `NC_ENOTNC4` for a non-NEXTCDF-4
     * @p ncid; `NC_EINVAL` for bad arguments or malformed @p path;
     * otherwise the `nc_open`/inquiry error for an unreachable URL or
     * missing object.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_open_remote(int ncid, const char *url, const char *path,
                           int *target_ncidp, int *kindp, int *objidp,
                           char *att_name, size_t att_name_size);

    /**
     * @brief Resolve an object path on an already-open remote session.
     *
     * Maps a stored object path to NetCDF IDs on @p ncid, which is
     * typically a session returned by nc_ref_open_remote() but may be
     * any open NetCDF file ID. Use it to resolve several stored paths
     * against one remote session without paying a remote open per
     * reference.
     *
     * @p path grammar: a leading `/`, `/`-separated group components,
     * and a final component naming a variable, group or committed
     * datatype. An optional `#attname` suffix selects an attribute of
     * the named object. `/` alone resolves to the root group.
     * Resolution precedence for the final component is variable, then
     * group, then committed type.
     *
     * When @p path carries no `#` suffix, @p kindp receives
     * `NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP` or `NC_REF_OBJ_TYPE` and
     * @p objidp the varid, group ncid or typeid. With a `#attname`
     * suffix, @p kindp receives `NC_REF_OBJ_ATT`, @p att_name the
     * attribute name and @p objidp the owner's varid, group ncid or
     * typeid. Attribute existence is verified with nc_inq_attid() for
     * variable and group owners; attributes on committed-datatype
     * owners are not verifiable through the NetCDF API and the name is
     * returned unchecked.
     *
     * @param ncid Open NetCDF file ID to resolve against.
     * @param path Object path (see above).
     * @param kindp Receives the resolved kind (see above).
     * @param objidp Receives the resolved ID (see above).
     * @param att_name Buffer receiving the attribute name, or NULL
     * when @p path carries no `#` suffix.
     * @param att_name_size Size of @p att_name in bytes.
     * @return `NC_NOERR` on success; `NC_ENOTBUILT` when built without
     * `NEP_ENABLE_REMOTE_REFS`; `NC_EINVAL` for bad arguments or
     * malformed @p path; `NC_ENOTVAR` when the final component names
     * no variable, group or type; `NC_ENOTATT` for a missing
     * attribute; other inquiry errors propagate.
     *
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_ref_resolve(int ncid, const char *path, int *kindp,
                       int *objidp, char *att_name, size_t att_name_size);

    /**
     * @brief Start a single-writer/multiple-reader session on a native NEXTCDF-4 file.
     *
     * SWMR (Single Writer / Multiple Reader) is an HDF5 concurrency mode:
     * one process holds a file open for writing while any number of other
     * processes hold the same file open read-only and watch the data grow,
     * without file locks or re-opening. The writer calls nc_swmr_flush()
     * (or nc_sync()) to publish what it has written; each reader calls
     * nc_swmr_refresh() to make the newly published records visible. NEP
     * exposes this through libncnextcdf4 on NEXTCDF-4 (HDF5) files only.
     * This function is the writer-side entry point; readers never call
     * it — they open with NC_SWMR_READ instead.
     *
     * Link libncnextcdf4 and enable NEP_HAS_NEXTCDF4. A file or subgroup ncid
     * addresses the whole file. Pending definitions are committed before HDF5
     * SWMR activation; calling again on an active writer is a no-op. Only serial
     * native files with superblock v3 and a SWMR-capable driver are supported.
     * Variables must have fixed-size atomic types (including NEP small floats,
     * complex and bitfield types), not NC_STRING, NC_STD_REF or user types.
     * Declared user types, even unused ones, are excluded. All schema and
     * attributes freeze at activation; in-bounds writes and unlimited growth
     * remain allowed. This is session state, not a persistent mode marker.
     *
     * Alternatively request NC_SWMR_WRITE at create/open: creates defer start
     * until enddef (including automatic enddef on write); writable opens start
     * after metadata loading. Closing an inactive requested file never starts
     * SWMR. Failed activation may leave ordinary definitions committed; it
     * does not roll them back and leaves the session inactive and closable.
     *
     * @param ncid Open file or group identifier.
     * @return NC_NOERR on success; NC_EBADID for an invalid identifier;
     * NC_ENOTNC4 for another backend; NC_EPERM for read-only access;
     * NC_EINVAL for compatibility/MPI sessions, old superblocks or unsupported
     * drivers; NC_ENOTBUILT for excluded schema; NC_EHDFERR for HDF5 failures;
     * or an error propagated while committing pending metadata.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_start_swmr_write(int ncid);

    /**
     * @brief Publish all datasets and dimension scales of an active SWMR writer.
     *
     * This is the writer's half of the SWMR handshake. In a SWMR session
     * data written by the writer is not automatically visible to readers:
     * it sits in the writer's HDF5 caches until the writer publishes it.
     * nc_swmr_flush() flushes every variable dataset and every
     * dimension-scale dataset, then flushes the file, which is what makes
     * the appended records readable by SWMR readers after their next
     * nc_swmr_refresh(). nc_sync() on an active writer performs the same
     * publication, so applications may use either call as their publish
     * boundary.
     *
     * Link libncnextcdf4. Publication is reader visibility, not an
     * atomic multi-variable timestep, rollback or power-loss durability
     * guarantee. HDF5 may also flush opportunistically before this call.
     * Readers must open with NC_SWMR_READ and call nc_swmr_refresh() to
     * observe published data. Does not enter define mode or activate an
     * inactive session.
     *
     * @param ncid Open file or group identifier; the entire file is flushed.
     * @return NC_NOERR on success; NC_EBADID for an invalid identifier;
     * NC_ENOTNC4 for another backend; NC_EPERM for read-only access;
     * NC_EINVAL for an inactive writer; NC_EHDFERR for a flush failure.
     * Partial publication on error is not rolled back.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_swmr_flush(int ncid);

    /**
     * @brief Refresh a NEXTCDF-4 SWMR reader session so newly published
     * data becomes visible.
     *
     * This is the reader's half of the SWMR handshake. A SWMR reader
     * holds the file open with HDF5 SWMR read access; its datasets and
     * dimension extents are snapshots that do not change on their own.
     * After the writer publishes new records with nc_swmr_flush() or
     * nc_sync(), the reader calls nc_swmr_refresh() once and then reads
     * the new data with the ordinary nc_inq_dimlen()/nc_get_vara() calls.
     * A typical reader loop is: wait for the application's own publish
     * signal, call nc_swmr_refresh(), call nc_inq_dimlen() for the
     * current record count, then read the new records. Any number of
     * readers may do this independently, and readers may open before or
     * after the writer starts SWMR writing and may close in either order.
     *
     * Link libncnextcdf4. Requires a file opened read-only with
     * NC_NEXTCDF4|NC_SWMR_READ; a file or subgroup ncid addresses the
     * whole file. Every open variable dataset and dimension-scale
     * dataset is refreshed through H5Drefresh, then the cached length
     * of each dimension is reloaded from its scale dataset's current
     * extent. Calling when nothing new was published is a cheap no-op,
     * so it is safe to call on every iteration of a polling loop.
     *
     * Refresh is not a re-open: the SWMR writer contract freezes the
     * schema and attributes at activation, so variables, dimensions,
     * attributes or types added afterwards are never expected and are
     * not discovered — reopen the file to see such changes. Reads
     * mid-append are well defined at the dataset level, but whether a
     * multi-variable logical record is complete is an application
     * concern; a common protocol is for the writer to update a scalar
     * marker variable last, after the record data, and for readers to
     * check the marker after refreshing. Calling this on an ordinary
     * read-only, writable or writer session returns NC_EINVAL.
     *
     * @param ncid Open file or group identifier of a SWMR reader session.
     * @return NC_NOERR on success; NC_EBADID for an invalid identifier;
     * NC_ENOTNC4 for another backend; NC_EINVAL when the session was not
     * opened with NC_SWMR_READ; NC_EHDFERR on an HDF5 refresh failure.
     * @author Edward Hartnett, Intelligent Data Design, Inc.
     */
    int nc_swmr_refresh(int ncid);

#if defined(__cplusplus)
}
#endif

/**
 * @defgroup nep_udf_slots NEP UDF Slot Allocation
 * @{
 *
 * Current netcdf-c `main` exposes 64 User-Defined Format (UDF) slots
 * (UDF0–UDF63). Each NEP format handler occupies a permanently assigned slot
 * so that multiple handlers can be enabled simultaneously without conflict.
 * Slot assignments are stable across NEP releases.
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

/** Request a native serial NEXTCDF-4 SWMR writer session at create/open.
 *
 * SWMR (Single Writer / Multiple Reader) lets one process write a file
 * while other processes read it concurrently. Passing this flag in the
 * cmode of nc_create() or the mode of nc_open(NC_WRITE) asks the backend
 * to start HDF5 SWMR writing automatically: creates defer activation
 * until enddef commits the schema, writable opens activate after
 * metadata load. Equivalent to calling nc_start_swmr_write() after an
 * ordinary open. Requires a native NEXTCDF-4 file on a superblock-v3,
 * SWMR-capable driver; rejected on MPI sessions and compatibility
 * modes. Once active, the schema and attributes are frozen and the
 * writer publishes data with nc_swmr_flush() or nc_sync(). This is a
 * NEP-only flag, not an upstream NetCDF mode. */
#define NC_SWMR_WRITE 0x08000000

/** Request a native serial NEXTCDF-4 SWMR reader session at open.
 *
 * Read-only flag combined with NC_NEXTCDF4 in the mode of nc_open().
 * The file is opened with HDF5 SWMR read access so it can coexist with
 * a live writer and observe records the writer publishes. Any file the
 * HDF5 layer accepts for SWMR reading may be opened: a live writer's
 * file, a quiescent file after the writer closed, or a file never
 * written in SWMR mode — provided its superblock is v3, which includes
 * NC_NETCDF4_MODEL files. After opening, the reader polls
 * nc_swmr_refresh() and reads with the ordinary inquiry/data calls;
 * multiple independent readers are supported. Rejected at nc_create,
 * and at open when combined with NC_WRITE, NC_SWMR_WRITE,
 * NC_CLASSIC_MODEL or MPI parameters. This is a NEP-only flag, not an
 * upstream NetCDF mode. See nc_swmr_refresh(). */
#define NC_SWMR_READ 0x10000000

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

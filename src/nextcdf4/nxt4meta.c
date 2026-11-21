/**
 * @file nxt4meta.c
 * @brief NEXTCDF-4 metadata model: dimensions, variables,
 * attributes, type mapping, and on-disk discovery.
 *
 * This file contains the two halves of the metadata lifecycle (see
 * @ref nextcdf4_architecture): the def/inq dispatch entries that
 * build and query the in-memory `NC_*_INFO_T` tree during define
 * mode, and the loader (`NEXTCDF4_load_metadata` and its helpers)
 * that rebuilds the same tree from HDF5 contents at open.

 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <hdf5_hl.h>
#include "nclist.h"
#include "nxt4internal.h"

/**
 * @brief Attribute names that are internal to NEXTCDF-4/HDF5.
 *
 * NEXTCDF-4 (and the NetCDF-4/HDF5 convention generally) stores
 * bookkeeping as ordinary HDF5 attributes: marker attributes like
 * `_Nextcdf4Backend`, dimension-scale attributes like `CLASS` and
 * `NAME`, and netcdf-c's own conventions like `_Netcdf4Coordinates`
 * and `_FillValue`. They must never appear in `nc_inq_att*` results,
 * so is_reserved_att() filters them during metadata load and
 * `nc_put_att`/`nc_rename_att` reject them as user names.
 */
static const char *reserved_atts[] = {
    NEXTCDF4_BACKEND_ATT,
    NEXTCDF4_MODEL_ATT,
    NEXTCDF4_DIMID_ATT,
    NEXTCDF4_VARID_ATT,
    NEXTCDF4_VARDIMIDS_ATT,
    "CLASS",
    "NAME",
    "REFERENCE_LIST",
    "DIMENSION_LIST",
    "_Netcdf4Coordinates",
    "_FillValue",
    "_Format",
    "_nc3_strict",
    "_NCProperties",
    "_SuperblockVersion",
    "_IsNetcdf4",
    "_ARRAY_DIMENSIONS",
    "_Codecs",
    NULL
};

/**
 * @brief Test whether an attribute name is reserved for internal use.
 *
 * See #reserved_atts. Used to hide NEXTCDF-4 bookkeeping attributes
 * from users and to reject reserved names in `nc_put_att`.
 *
 * @param name Name.
 * @return Nonzero when @p name is a reserved attribute name.
 */
static int
is_reserved_att(const char *name)
{
    size_t i;
    for (i = 0; reserved_atts[i]; i++)
        if (!strcmp(reserved_atts[i], name))
            return 1;
    return 0;
}

/**
 * @brief Free a dynamically-allocated string list used by the loader.
 *
 * The `H5Literate` callbacks (find_dim_cb, find_var_cb) build
 * `NULL`-terminated-capacity arrays of `strdup`'d names; this frees
 * the elements and the array.
 *
 * @param names String array.
 * @param n Element count.
 */
static void
free_name_list(char **names, size_t n)
{
    size_t i;
    if (!names)
        return;
    for (i = 0; i < n; i++)
        free(names[i]);
    free(names);
}

/**
 * @brief Check that a file is writable and currently in define mode.
 *
 * Common guard for every metadata-changing dispatch entry
 * (`nc_def_*`, `nc_rename_*`, `nc_put_att`, `nc_del_att`). netCDF
 * semantics require define mode for metadata changes; files opened
 * read-only reject them outright.
 *
 * @param file NEXTCDF-4 file state.
 * @return `NC_NOERR`, `NC_EPERM` (read-only file), or
 * `NC_ENOTINDEFINE`.
 */
int
NEXTCDF4_check_write_define(NEXTCDF4_FILE_INFO_T *file)
{
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode)
        return NC_ENOTINDEFINE;
    return NC_NOERR;
}

/**
 * @brief Map an atomic nc_type to its on-disk HDF5 datatype.
 *
 * Every netCDF atomic type is stored as a copy of an HDF5 predefined
 * type — little-endian for numerics so files are
 * architecture-independent. The mapping covers the standard types
 * (`NC_BYTE`…`NC_DOUBLE`, `NC_CHAR`, `NC_STRING`) plus NEXTCDF-4's
 * extended set: `NC_FLOAT16`, `NC_BFLOAT16`, the HDF5 2.x small
 * floats (FP8/FP6/FP4), complex, bitfields, and the reference type
 * `NC_STD_REF`. The caller owns the returned
 * `hid_t` and must `H5Tclose` it. The inverse mapping (disk →
 * nc_type) is map_nc_type().
 *
 * @param xtype NetCDF atomic type.
 * @param typep Destination for an HDF5 datatype identifier.
 * @return `NC_NOERR` or `NC_EBADTYPE`.
 */
int
NEXTCDF4_map_hdf_type(nc_type xtype, hid_t *typep)
{
    if (!typep)
        return NC_EINVAL;
    switch (xtype) {
    case NC_BYTE:
        *typep = H5Tcopy(H5T_STD_I8LE);
        return NC_NOERR;
    case NC_UBYTE:
        *typep = H5Tcopy(H5T_STD_U8LE);
        return NC_NOERR;
    case NC_CHAR:
        *typep = H5Tcopy(H5T_C_S1);
        if (*typep >= 0) {
            H5Tset_size(*typep, 1);
            H5Tset_strpad(*typep, H5T_STR_NULLPAD);
        }
        return NC_NOERR;
    case NC_SHORT:
        *typep = H5Tcopy(H5T_STD_I16LE);
        return NC_NOERR;
    case NC_USHORT:
        *typep = H5Tcopy(H5T_STD_U16LE);
        return NC_NOERR;
    case NC_INT:
        *typep = H5Tcopy(H5T_STD_I32LE);
        return NC_NOERR;
    case NC_UINT:
        *typep = H5Tcopy(H5T_STD_U32LE);
        return NC_NOERR;
    case NC_INT64:
        *typep = H5Tcopy(H5T_STD_I64LE);
        return NC_NOERR;
    case NC_UINT64:
        *typep = H5Tcopy(H5T_STD_U64LE);
        return NC_NOERR;
    case NC_FLOAT:
        *typep = H5Tcopy(H5T_IEEE_F32LE);
        return NC_NOERR;
    case NC_DOUBLE:
        *typep = H5Tcopy(H5T_IEEE_F64LE);
        return NC_NOERR;
    case NC_FLOAT16:
        *typep = H5Tcopy(H5T_IEEE_F16LE);
        return NC_NOERR;
    case NC_BFLOAT16:
        *typep = H5Tcopy(H5T_FLOAT_BFLOAT16LE);
        return NC_NOERR;
    case NC_FLOAT8_E4M3:
        *typep = H5Tcopy(H5T_FLOAT_F8E4M3);
        return NC_NOERR;
    case NC_FLOAT8_E5M2:
        *typep = H5Tcopy(H5T_FLOAT_F8E5M2);
        return NC_NOERR;
    case NC_FLOAT6_E2M3:
        *typep = H5Tcopy(H5T_FLOAT_F6E2M3);
        return NC_NOERR;
    case NC_FLOAT6_E3M2:
        *typep = H5Tcopy(H5T_FLOAT_F6E3M2);
        return NC_NOERR;
    case NC_FLOAT4_E2M1:
        *typep = H5Tcopy(H5T_FLOAT_F4E2M1);
        return NC_NOERR;
    case NC_COMPLEX:
        *typep = H5Tcopy(H5T_COMPLEX_IEEE_F32LE);
        return NC_NOERR;
    case NC_DOUBLECOMPLEX:
        *typep = H5Tcopy(H5T_COMPLEX_IEEE_F64LE);
        return NC_NOERR;
    case NC_BITFIELD8:
        *typep = H5Tcopy(H5T_STD_B8LE);
        return NC_NOERR;
    case NC_BITFIELD16:
        *typep = H5Tcopy(H5T_STD_B16LE);
        return NC_NOERR;
    case NC_BITFIELD32:
        *typep = H5Tcopy(H5T_STD_B32LE);
        return NC_NOERR;
    case NC_BITFIELD64:
        *typep = H5Tcopy(H5T_STD_B64LE);
        return NC_NOERR;
    case NC_STD_REF:
        *typep = H5Tcopy(H5T_STD_REF);
        return NC_NOERR;
    case NC_STRING:
        *typep = H5Tcopy(H5T_C_S1);
        if (*typep >= 0) {
            H5Tset_size(*typep, H5T_VARIABLE);
            H5Tset_strpad(*typep, H5T_STR_NULLTERM);
        }
        return NC_NOERR;
    default:
        return NC_EBADTYPE;
    }
}

/**
 * @brief Return the in-memory size, in bytes, of an atomic nc_type.
 *
 * This is the size of one element in a user buffer: `NC_STRING` is
 * `sizeof(char *)`, references are their opaque token sizes (8 and
 * 12 bytes), the sub-byte small floats round up to 1 byte.
 *
 * @param xtype NetCDF atomic type.
 * @param sizep Destination for the size in bytes.
 * @return `NC_NOERR` or `NC_EBADTYPE`.
 */
int
NEXTCDF4_type_size(nc_type xtype, size_t *sizep)
{
    size_t size;
    switch (xtype) {
    case NC_BYTE:
    case NC_UBYTE:
    case NC_CHAR:
        size = 1;
        break;
    case NC_SHORT:
    case NC_USHORT:
        size = 2;
        break;
    case NC_INT:
    case NC_UINT:
    case NC_FLOAT:
        size = 4;
        break;
    case NC_INT64:
    case NC_UINT64:
    case NC_DOUBLE:
        size = 8;
        break;
    case NC_FLOAT16:
    case NC_BFLOAT16:
        size = 2;
        break;
    case NC_FLOAT8_E4M3:
    case NC_FLOAT8_E5M2:
    case NC_FLOAT6_E2M3:
    case NC_FLOAT6_E3M2:
    case NC_FLOAT4_E2M1:
        size = 1;
        break;
    case NC_COMPLEX:
        size = 2 * sizeof(float);
        break;
    case NC_DOUBLECOMPLEX:
        size = 2 * sizeof(double);
        break;
    case NC_BITFIELD8:
        size = 1;
        break;
    case NC_BITFIELD16:
        size = 2;
        break;
    case NC_BITFIELD32:
        size = 4;
        break;
    case NC_BITFIELD64:
        size = 8;
        break;
    case NC_STD_REF:
        size = sizeof(H5R_ref_t);
        break;
    case NC_STRING:
        size = sizeof(char *);
        break;
    default:
        return NC_EBADTYPE;
    }
    if (sizep)
        *sizep = size;
    return NC_NOERR;
}

/**
 * @brief Return the short name for an atomic nc_type.
 *
 * These names ("float16", "bfloat16", ...) label the in-memory
 * `NC_TYPE_INFO_T` records set_var_type() builds for atomic-typed
 * variables.
 *
 * @param xtype NetCDF atomic type.
 * @return Static name string, or `"unknown"`.
 */
const char *
NEXTCDF4_type_name(nc_type xtype)
{
    switch (xtype) {
    case NC_BYTE:   return "byte";
    case NC_UBYTE:  return "ubyte";
    case NC_CHAR:   return "char";
    case NC_SHORT:  return "short";
    case NC_USHORT: return "ushort";
    case NC_INT:    return "int";
    case NC_UINT:   return "uint";
    case NC_INT64:  return "int64";
    case NC_UINT64: return "uint64";
    case NC_FLOAT:      return "float";
    case NC_DOUBLE:     return "double";
    case NC_FLOAT16:    return "float16";
    case NC_BFLOAT16:   return "bfloat16";
    case NC_FLOAT8_E4M3: return "float8_e4m3";
    case NC_FLOAT8_E5M2: return "float8_e5m2";
    case NC_FLOAT6_E2M3: return "float6_e2m3";
    case NC_FLOAT6_E3M2: return "float6_e3m2";
    case NC_FLOAT4_E2M1: return "float4_e2m1";
    case NC_COMPLEX:    return "complex";
    case NC_DOUBLECOMPLEX: return "doublecomplex";
    case NC_BITFIELD8:  return "bitfield8";
    case NC_BITFIELD16: return "bitfield16";
    case NC_BITFIELD32: return "bitfield32";
    case NC_BITFIELD64: return "bitfield64";
    case NC_STD_REF:    return "std_ref";
    case NC_STRING:     return "string";
    default:            return "unknown";
    }
}

/**
 * @brief Check whether an atomic type is allowed for the file's
 * compatibility mode.
 *
 * Called at `nc_def_var`/`nc_put_att` time. `NC_CLASSIC_MODEL` files
 * accept only the six classic types; `NC_NETCDF4_MODEL` adds the
 * enhanced types (unsigned ints, int64, string) but not the
 * NEXTCDF-4-only types (small floats, complex, bitfields,
 * references), keeping the file readable by upstream netcdf-c. See
 * the mode table in @ref nextcdf4_architecture.
 *
 * @param file NEXTCDF-4 file state.
 * @param xtype Candidate NetCDF type.
 * @return `NC_NOERR` or `NC_EBADTYPE`/`NC_ENOTNC4`.
 */
int
NEXTCDF4_check_atomic_type(NEXTCDF4_FILE_INFO_T *file, nc_type xtype)
{
    int is_ok;

    switch (xtype) {
    case NC_BYTE:
    case NC_CHAR:
    case NC_SHORT:
    case NC_INT:
    case NC_FLOAT:
    case NC_DOUBLE:
        is_ok = 1;
        break;
    case NC_UBYTE:
    case NC_USHORT:
    case NC_UINT:
    case NC_INT64:
    case NC_UINT64:
        is_ok = !file->netcdf4_model && !(file->mode & NC_CLASSIC_MODEL);
        break;
    case NC_STRING:
        return (!file->netcdf4_model && !(file->mode & NC_CLASSIC_MODEL))
            ? NC_NOERR : NC_EBADTYPE;
    case NC_FLOAT16:
    case NC_BFLOAT16:
    case NC_FLOAT8_E4M3:
    case NC_FLOAT8_E5M2:
    case NC_FLOAT6_E2M3:
    case NC_FLOAT6_E3M2:
    case NC_FLOAT4_E2M1:
    case NC_COMPLEX:
    case NC_DOUBLECOMPLEX:
    case NC_BITFIELD8:
    case NC_BITFIELD16:
    case NC_BITFIELD32:
    case NC_BITFIELD64:
    case NC_STD_REF:
        return (!file->netcdf4_model && !(file->mode & NC_CLASSIC_MODEL))
            ? NC_NOERR : NC_ENOTNC4;
    default:
        return NC_EBADTYPE;
    }
    return is_ok ? NC_NOERR : NC_ENOTNC4;
}

/**
 * @brief Build the `NC_TYPE_INFO_T` record for an atomic-typed
 * variable.
 *
 * netcdf-c expects every variable to point at a type record; atomic
 * types get a small synthetic one whose `hdr.id` is the `nc_type`
 * itself. The `nc_type_class` mapping groups the extended types onto
 * classic classes (small floats → `NC_FLOAT`, bitfields → `NC_INT`,
 * references → `NC_OPAQUE`) so shared `NC4_*` inquiry helpers keep
 * working.
 *
 * @param var Variable.
 * @param xtype NetCDF atomic type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
set_var_type(NC_VAR_INFO_T *var, nc_type xtype)
{
    NC_TYPE_INFO_T *type;
    const char *name = NEXTCDF4_type_name(xtype);
    size_t size;

    if (xtype == NC_STRING) {
        size = sizeof(char *);
    } else if (NEXTCDF4_type_size(xtype, &size)) {
        return NC_EBADTYPE;
    }
    if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
        return NC_ENOMEM;
    if (!(type->hdr.name = strdup(name))) {
        free(type);
        return NC_ENOMEM;
    }
    type->hdr.sort = NCTYP;
    type->hdr.id = (size_t)xtype;
    type->rc = 1;
    if (xtype == NC_FLOAT || xtype == NC_DOUBLE ||
        xtype == NC_FLOAT16 || xtype == NC_BFLOAT16 ||
        xtype == NC_FLOAT8_E4M3 || xtype == NC_FLOAT8_E5M2 ||
        xtype == NC_FLOAT6_E2M3 || xtype == NC_FLOAT6_E3M2 ||
        xtype == NC_FLOAT4_E2M1)
        type->nc_type_class = NC_FLOAT;
    else if (xtype == NC_CHAR)
        type->nc_type_class = NC_STRING;
    else if (xtype == NC_STRING)
        type->nc_type_class = NC_STRING;
    else if (xtype == NC_COMPLEX || xtype == NC_DOUBLECOMPLEX) {
        type->nc_type_class = NC_COMPOUND;
        type->u.c.field = nclistnew();
    } else if (xtype >= NC_BITFIELD8 && xtype <= NC_BITFIELD64)
        type->nc_type_class = NC_INT;
    else if (xtype == NC_STD_REF)
        type->nc_type_class = NC_OPAQUE;
    else
        type->nc_type_class = NC_INT;
    type->endianness = NC_ENDIAN_NATIVE;
    type->size = size;
    var->type_info = type;
    var->endianness = type->endianness;
    return NC_NOERR;
}

/**
 * @brief Write a null-terminated scalar string HDF5 attribute.
 *
 * Helper for the hidden marker attributes (`CLASS`, `NAME`) written
 * on dimension scales.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param value Data buffer.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
static int
write_string_att(hid_t loc, const char *name, const char *value)
{
    hid_t space = -1;
    hid_t type = -1;
    hid_t attr = -1;
    int ret = NC_EHDFERR;

    if ((space = H5Screate(H5S_SCALAR)) < 0 ||
        (type = H5Tcopy(H5T_C_S1)) < 0 ||
        H5Tset_size(type, strlen(value) + 1) < 0 ||
        H5Tset_strpad(type, H5T_STR_NULLTERM) < 0 ||
        (attr = H5Acreate2(loc, name, type, space, H5P_DEFAULT,
                          H5P_DEFAULT)) < 0 ||
        H5Awrite(attr, type, value) < 0)
        goto done;
    ret = NC_NOERR;

done:
    if (attr >= 0)
        H5Aclose(attr);
    if (type >= 0)
        H5Tclose(type);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Write a one-element 32-bit integer HDF5 attribute.
 *
 * Helper for the hidden id attributes (`_Netcdf4Dimid`,
 * `_Netcdf4Varid`).
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param value Data buffer.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
static int
write_int_att(hid_t loc, const char *name, int value)
{
    hid_t space = -1;
    hid_t attr = -1;
    int ret = NC_EHDFERR;

    if ((space = H5Screate(H5S_SCALAR)) < 0 ||
        (attr = H5Acreate2(loc, name, H5T_STD_I32LE, space, H5P_DEFAULT,
                          H5P_DEFAULT)) < 0 ||
        H5Awrite(attr, H5T_NATIVE_INT, &value) < 0)
        goto done;
    ret = NC_NOERR;

done:
    if (attr >= 0)
        H5Aclose(attr);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Write an array of 32-bit integers as an HDF5 attribute.
 *
 * Helper for `_Netcdf4Coordinates`, which records a variable's dimids
 * so open can recover them without walking dimension scales. A
 * zero-length attribute uses an `H5S_NULL` dataspace.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param values Array of int values to write.
 * @param len Number of values.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
static int
write_int_array_att(hid_t loc, const char *name, const int *values, size_t len)
{
    hid_t space = -1;
    hid_t attr = -1;
    hsize_t hlen = len;
    int ret = NC_EHDFERR;

    if (len == 0) {
        if ((space = H5Screate(H5S_NULL)) < 0)
            goto done;
    } else {
        if ((space = H5Screate_simple(1, &hlen, NULL)) < 0)
            goto done;
    }
    if ((attr = H5Acreate2(loc, name, H5T_STD_I32LE, space, H5P_DEFAULT,
                          H5P_DEFAULT)) < 0)
        goto done;
    if (len > 0 && H5Awrite(attr, H5T_NATIVE_INT, values) < 0)
        goto done;
    ret = NC_NOERR;

done:
    if (attr >= 0)
        H5Aclose(attr);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Map an on-disk HDF5 datatype back to a NetCDF atomic type.
 *
 * Inverse of NEXTCDF4_map_hdf_type(), used at open. HDF5 class +
 * size + sign resolves the standard types; small floats, references
 * and complex need `H5Tequal` against specific predefined types.
 * Datatypes that don't map (e.g. committed user types, which are
 * loaded by load_one_type instead) yield `NC_EBADTYPE`.
 *
 * @param htype HDF5 datatype to map.
 * @param xtypep Destination for the NetCDF atomic type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
map_nc_type(hid_t htype, nc_type *xtypep)
{
    H5T_class_t cls;
    size_t size;
    H5T_sign_t sign;

    cls = H5Tget_class(htype);
    size = H5Tget_size(htype);
    sign = H5Tget_sign(htype);

    if (cls == H5T_INTEGER) {
        switch (size) {
        case 1:
            *xtypep = (sign == H5T_SGN_NONE) ? NC_UBYTE : NC_BYTE;
            return NC_NOERR;
        case 2:
            *xtypep = (sign == H5T_SGN_NONE) ? NC_USHORT : NC_SHORT;
            return NC_NOERR;
        case 4:
            *xtypep = (sign == H5T_SGN_NONE) ? NC_UINT : NC_INT;
            return NC_NOERR;
        case 8:
            *xtypep = (sign == H5T_SGN_NONE) ? NC_UINT64 : NC_INT64;
            return NC_NOERR;
        }
    } else if (cls == H5T_FLOAT) {
        if (size == 4) {
            *xtypep = NC_FLOAT;
            return NC_NOERR;
        } else if (size == 8) {
            *xtypep = NC_DOUBLE;
            return NC_NOERR;
        } else if (size == 2) {
            if (H5Tequal(htype, H5T_IEEE_F16LE) > 0 ||
                H5Tequal(htype, H5T_IEEE_F16BE) > 0)
                *xtypep = NC_FLOAT16;
            else if (H5Tequal(htype, H5T_FLOAT_BFLOAT16LE) > 0 ||
                     H5Tequal(htype, H5T_FLOAT_BFLOAT16BE) > 0)
                *xtypep = NC_BFLOAT16;
            else
                return NC_EBADTYPE;
            return NC_NOERR;
        } else if (size == 1) {
            if (H5Tequal(htype, H5T_FLOAT_F8E4M3) > 0)
                *xtypep = NC_FLOAT8_E4M3;
            else if (H5Tequal(htype, H5T_FLOAT_F8E5M2) > 0)
                *xtypep = NC_FLOAT8_E5M2;
            else if (H5Tequal(htype, H5T_FLOAT_F6E2M3) > 0)
                *xtypep = NC_FLOAT6_E2M3;
            else if (H5Tequal(htype, H5T_FLOAT_F6E3M2) > 0)
                *xtypep = NC_FLOAT6_E3M2;
            else if (H5Tequal(htype, H5T_FLOAT_F4E2M1) > 0)
                *xtypep = NC_FLOAT4_E2M1;
            else
                return NC_EBADTYPE;
            return NC_NOERR;
        }
    } else if (cls == H5T_STRING && size == 1) {
        *xtypep = NC_CHAR;
        return NC_NOERR;
    } else if (cls == H5T_COMPLEX) {
        if (size == 2 * sizeof(float)) {
            *xtypep = NC_COMPLEX;
            return NC_NOERR;
        } else if (size == 2 * sizeof(double)) {
            *xtypep = NC_DOUBLECOMPLEX;
            return NC_NOERR;
        }
    } else if (cls == H5T_BITFIELD) {
        switch (size) {
        case 1:
            *xtypep = NC_BITFIELD8;
            return NC_NOERR;
        case 2:
            *xtypep = NC_BITFIELD16;
            return NC_NOERR;
        case 4:
            *xtypep = NC_BITFIELD32;
            return NC_NOERR;
        case 8:
            *xtypep = NC_BITFIELD64;
            return NC_NOERR;
        }
    } else if (cls == H5T_REFERENCE) {
        if (H5Tequal(htype, H5T_STD_REF) > 0) {
            *xtypep = NC_STD_REF;
            return NC_NOERR;
        }
    }
    return NC_EBADTYPE;
}

/**
 * @brief Resolve where an attribute lives: on a variable's dataset or
 * on the root group.
 *
 * netCDF attributes are either per-variable or global (`NC_GLOBAL`,
 * which NEXTCDF-4 stores on the root group). Returns both the
 * in-memory `NCindex` and the HDF5 `hid_t` the attribute is written
 * to.
 *
 * @param grp Group.
 * @param var Variable, or NULL for a global attribute.
 * @param listp Destination for the attribute index.
 * @param locp Destination for the HDF5 location identifier.
 * @return `NC_NOERR` or `NC_EBADID` when the variable's HDF5 dataset
 * is not open.
 */
static int
get_att_context(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var, NCindex **listp,
                hid_t *locp)
{
    if (var) {
        NEXTCDF4_VAR_INFO_T *vinfo = var->format_var_info;
        if (vinfo && vinfo->hdf_dataset >= 0)
            *locp = vinfo->hdf_dataset;
        else
            return NC_EBADID;
        *listp = var->att;
    } else {
        NEXTCDF4_FILE_INFO_T *file = grp->nc4_info->format_file_info;
        *locp = file->rootid;
        *listp = grp->att;
    }
    return NC_NOERR;
}

/**
 * @brief Find an existing attribute by name in an attribute index.
 * @param list Attribute index to search.
 * @param name Name.
 * @return Pointer to the attribute, or NULL if not found.
 */
static NC_ATT_INFO_T *
find_att(NCindex *list, const char *name)
{
    NC_OBJ *obj;
    if (!list)
        return NULL;
    obj = ncindexlookup(list, name);
    if (!obj)
        return NULL;
    return (NC_ATT_INFO_T *)obj;
}

/**
 * @brief Allocate and add a new attribute to the in-memory model.
 *
 * Deep-copies the attribute value: numeric data by `memcpy`,
 * `NC_STRING` values element-by-element (NULL elements become empty
 * strings), so the attribute record owns its data outright.
 *
 * @param list Attribute index to add to.
 * @param container Object that owns the attribute (group or var `hdr`).
 * @param name Name.
 * @param xtype NetCDF atomic type.
 * @param len Number of elements.
 * @param data Data buffer.
 * @param attp Destination for the new attribute.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
add_att(NCindex *list, NC_OBJ *container, const char *name,
        nc_type xtype, size_t len, const void *data,
        NC_ATT_INFO_T **attp)
{
    NC_ATT_INFO_T *att = NULL;
    size_t size;
    void *copy = NULL;
    int ret;
    size_t i;

    if ((ret = nc4_att_list_add(list, name, &att)))
        return ret;
    if (len > 0) {
        if (xtype == NC_STRING) {
            const char **src = (const char **)data;
            char **cp = malloc(len * sizeof(char *));
            if (!cp) {
                nc4_att_list_del(list, att);
                return NC_ENOMEM;
            }
            for (i = 0; i < len; i++) {
                if (src && src[i])
                    cp[i] = strdup(src[i]);
                else
                    cp[i] = strdup("");
                if (!cp[i]) {
                    size_t j;
                    for (j = 0; j < i; j++)
                        free(cp[j]);
                    free(cp);
                    nc4_att_list_del(list, att);
                    return NC_ENOMEM;
                }
            }
            copy = cp;
            size = sizeof(char *);
        } else {
            if ((ret = NEXTCDF4_type_size(xtype, &size)))
                return ret;
            if (!(copy = malloc(len * size))) {
                nc4_att_list_del(list, att);
                return NC_ENOMEM;
            }
            memcpy(copy, data, len * size);
        }
    }
    att->nc_typeid = xtype;
    att->len = len;
    att->data = copy;
    att->container = container;
    if (attp)
        *attp = att;
    return NC_NOERR;
}

/**
 * @brief Read one HDF5 attribute into the in-memory model.
 *
 * Loader helper: opens the attribute, maps its datatype through
 * map_nc_type(), reads the values with a matching memory type, and
 * registers the result via add_att(). Attributes whose datatype does
 * not map to an atomic nc_type fail — the caller skips them so an
 * unrecognised attribute cannot sink a file open.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param list Attribute index to add to.
 * @param container Object that owns the attribute.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
read_hdf5_att(hid_t loc, const char *name, NCindex *list, NC_OBJ *container)
{
    hid_t attr = -1;
    hid_t ftype = -1;
    hid_t fspace = -1;
    hid_t ntype = -1;
    nc_type xtype = NC_NAT;
    int close_ntype = 0;
    hssize_t npoints;
    size_t size;
    size_t len;
    void *data = NULL;
    int ret = NC_EHDFERR;

    if ((attr = H5Aopen(loc, name, H5P_DEFAULT)) < 0)
        return NC_EHDFERR;
    if ((ftype = H5Aget_type(attr)) < 0)
        goto done;
    if ((fspace = H5Aget_space(attr)) < 0)
        goto done;
    if ((npoints = H5Sget_simple_extent_npoints(fspace)) < 0)
        goto done;
    if (map_nc_type(ftype, &xtype))
        goto done;
    if (NEXTCDF4_type_size(xtype, &size))
        goto done;
    len = (size_t)npoints;
    if (len > 0) {
        if (len > SIZE_MAX / size) { ret = NC_EINVAL; goto done; }
        if (!(data = malloc(len * size)))
            goto done;
        if (xtype == NC_CHAR) {
            if ((ntype = H5Tcopy(H5T_C_S1)) < 0)
                goto done;
            close_ntype = 1;
            H5Tset_size(ntype, 1);
            H5Tset_strpad(ntype, H5T_STR_NULLPAD);
        } else {
            ntype = NEXTCDF4_memory_type(xtype, &close_ntype);
        }
        if (ntype < 0)
            goto done;
        if (H5Aread(attr, ntype, data) < 0)
            goto done;
    }
    if ((ret = add_att(list, container, name, xtype, len, data, NULL)))
        goto done;
    ret = NC_NOERR;

done:
    free(data);
    if (close_ntype && ntype >= 0)
        H5Tclose(ntype);
    if (fspace >= 0)
        H5Sclose(fspace);
    if (ftype >= 0)
        H5Tclose(ftype);
    if (attr >= 0)
        H5Aclose(attr);
    return ret;
}

/**
 * @brief Define a dimension — the `nc_def_dim()` dispatch entry.
 *
 * NEXTCDF-4 stores each dimension as an HDF5 **dimension scale**: a
 * 1-D `int8` dataset marked `CLASS = "DIMENSION_SCALE"` with hidden
 * `NAME` and `_Netcdf4Dimid` attributes. An unlimited dimension
 * (`len == 0`) becomes a chunked dataset with `H5S_UNLIMITED` extent.
 * Classic-model files get at most one unlimited dimension.
 *
 * @param ncid NetCDF file identifier.
 * @param name Dimension name.
 * @param len Dimension length, or 0 for unlimited.
 * @param idp Destination for the assigned dimension id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_dim(int ncid, const char *name, size_t len, int *idp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_DIM_INFO_T *dim = NULL;
    NEXTCDF4_DIM_INFO_T *dinfo = NULL;
    NEXTCDF4_GRP_INFO_T *ginfo;
    hid_t space = -1;
    hid_t dcpl = -1;
    hid_t dtype = -1;
    hid_t hdf_grp = -1;
    hsize_t cur = (len == 0) ? 0 : len;
    hsize_t max = (len == 0) ? H5S_UNLIMITED : 0;
    hsize_t chunk = 1;
    int assignedid;
    int ret;

    /* Validate, allocate the in-memory dim record, then create the
     * dimension-scale dataset (chunked+unlimited for len==0) and write
     * its CLASS/NAME/_Netcdf4Dimid attributes. */
    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_check_dup_name(grp, (char *)name)))
        return ret;

    /* Classic model: at most one unlimited dimension. */
    if ((file->mode & NC_CLASSIC_MODEL) && len == 0) {
        size_t i;
        for (i = 0; i < ncindexsize(grp->dim); i++) {
            NC_DIM_INFO_T *d = (NC_DIM_INFO_T *)ncindexith(grp->dim, i);
            if (d && d->unlimited)
                return NC_EINVAL;
        }
    }

    ginfo = grp->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;

    ELOG((3, "%s: name=%s ncid=%d", __func__, name, ncid));
    if (!(dinfo = calloc(1, sizeof(*dinfo))))
        return NC_ENOMEM;

    assignedid = h5->next_dimid;
    if ((ret = nc4_dim_list_add(grp, name, len, assignedid, &dim)))
        goto fail;
    dim->unlimited = (len == 0);
    dim->format_dim_info = dinfo;
    h5->next_dimid = assignedid + 1;

    /* Create the HDF5 dimension scale dataset. */
    if ((dtype = H5Tcopy(H5T_STD_I8LE)) < 0)
        goto fail;
    if (len == 0) {
        if ((space = H5Screate_simple(1, &cur, &max)) < 0)
            goto fail;
        if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            goto fail;
        if (H5Pset_chunk(dcpl, 1, &chunk) < 0)
            goto fail;
    } else {
        if ((space = H5Screate_simple(1, &cur, NULL)) < 0)
            goto fail;
    }
    dinfo->hdf_dataset = H5Dcreate2(hdf_grp, name, dtype, space,
                                    H5P_DEFAULT, (dcpl >= 0) ? dcpl : H5P_DEFAULT,
                                    H5P_DEFAULT);
    if (dinfo->hdf_dataset < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }

    if ((ret = write_string_att(dinfo->hdf_dataset, "CLASS",
                                NEXTCDF4_DIMCLASS)) ||
        (ret = write_string_att(dinfo->hdf_dataset, "NAME", name)) ||
        (ret = write_int_att(dinfo->hdf_dataset, NEXTCDF4_DIMID_ATT,
                             assignedid)))
        goto fail;

    if (idp)
        *idp = assignedid;
    H5Tclose(dtype);
    H5Sclose(space);
    if (dcpl >= 0)
        H5Pclose(dcpl);
    return NC_NOERR;

fail:
    H5E_BEGIN_TRY {
        if (dinfo && dinfo->hdf_dataset >= 0)
            H5Dclose(dinfo->hdf_dataset);
    } H5E_END_TRY;
    if (dim)
        nc4_dim_list_del(grp, dim);
    free(dinfo);
    if (space >= 0)
        H5Sclose(space);
    if (dcpl >= 0)
        H5Pclose(dcpl);
    if (dtype >= 0)
        H5Tclose(dtype);
    return ret;
}

/**
 * @brief Inquire about a dimension — the `nc_inq_dim()` dispatch
 * entry.
 *
 * Answers from the in-memory tree; for an unlimited dimension `lenp`
 * is the *current* length.
 *
 * @param ncid NetCDF file identifier.
 * @param dimid Dimension id.
 * @param name Destination for the dimension name.
 * @param lenp Destination for the dimension length.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_dim(int ncid, int dimid, char *name, size_t *lenp)
{
    NC_FILE_INFO_T *h5;
    NC_DIM_INFO_T *dim = NULL;
    NC_GRP_INFO_T *dim_grp = NULL;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_dim(h5->root_grp, dimid, &dim, &dim_grp)))
        return ret;
    if (name)
        strncpy(name, dim->hdr.name, NC_MAX_NAME);
    if (lenp)
        *lenp = dim->len;
    return NC_NOERR;
}

/**
 * @brief Define a variable — the `nc_def_var()` dispatch entry.
 *
 * Builds the `NC_VAR_INFO_T` record and creates the HDF5 dataset
 * immediately: scalar variables get a scalar dataspace, dimensioned
 * variables a simple dataspace (unlimited dims force chunked layout).
 * The hidden `_Netcdf4Varid` and `_Netcdf4Coordinates` attributes are
 * written on the dataset, and each dimension's scale is attached with
 * `H5DSattach_scale`. A 1-D variable named after its only dimension
 * is a **coordinate variable** — it replaces the dimension's scale
 * dataset so the variable's data *is* the scale.
 *
 * `var->storage` starts at -1 ("not specified") so that layout is
 * decided here and a later def-var setter cannot silently change it
 * (issue #382).
 *
 * @param ncid NetCDF file identifier.
 * @param name Variable name.
 * @param xtype NetCDF atomic type.
 * @param ndims Number of dimensions.
 * @param dimidsp Array of dimension ids.
 * @param varidp Destination for the assigned variable id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var(int ncid, const char *name, nc_type xtype, int ndims,
                 const int *dimidsp, int *varidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;

    ELOG((3, "%s: name=%s ncid=%d", __func__, name, ncid));
    NC_VAR_INFO_T *var = NULL;
    NEXTCDF4_VAR_INFO_T *vinfo = NULL;
    NEXTCDF4_GRP_INFO_T *ginfo;
    hid_t hdf_type = -1;
    hid_t space = -1;
    hid_t dcpl = -1;
    hid_t hdf_grp = -1;
    hsize_t dims[NC_MAX_VAR_DIMS];
    hsize_t maxdims[NC_MAX_VAR_DIMS];
    hsize_t chunks[NC_MAX_VAR_DIMS];
    int has_unlimited = 0;
    int coordinate = 0;
    int i;
    int ret;

    /* Validate name/type/mode, build the NC_VAR_INFO_T, then create
     * the HDF5 dataset (coordinate variables replace their
     * dimension's scale dataset), write the hidden id attributes, and
     * attach the dimension scales. */
    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NEXTCDF4_check_atomic_type(file, xtype)))
        return ret;
    if (ndims < 0 || ndims > NC_MAX_VAR_DIMS)
        return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_check_dup_name(grp, (char *)name)))
        return ret;

    ginfo = grp->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;

    if (!(vinfo = calloc(1, sizeof(*vinfo))))
        return NC_ENOMEM;

    if ((ret = nc4_var_list_add(grp, name, ndims, &var)))
        goto fail;
    if ((ret = nc4_var_set_ndims(var, ndims)))
        goto fail;
    if ((ret = set_var_type(var, xtype)))
        goto fail;
    /* The filter list exists from birth, as upstream does in
     * NC4_HDF5_def_var, so NC4_hdf5_addfilter can snapshot it. */
    if (!(var->filters = (void *)nclistnew())) {
        ret = NC_ENOMEM;
        goto fail;
    }
    var->format_var_info = vinfo;

    for (i = 0; i < ndims; i++) {
        NC_DIM_INFO_T *dim = NULL;
        if ((ret = nc4_find_dim(grp, dimidsp[i], &dim, NULL)))
            goto fail;
        var->dimids[i] = dimidsp[i];
        var->dim[i] = dim;
        dims[i] = dim->len;
        if (dim->unlimited) {
            maxdims[i] = H5S_UNLIMITED;
            has_unlimited = 1;
            chunks[i] = 1;
        } else {
            maxdims[i] = dim->len;
            chunks[i] = (dim->len > 0) ? dim->len : 1;
        }
    }

    /* Classic model unlimited ordering. */
    if (file->mode & NC_CLASSIC_MODEL) {
        int n_unlim = 0;
        for (i = 0; i < ndims; i++)
            if (var->dim[i]->unlimited)
                n_unlim++;
        if (n_unlim > 1)
            return NC_EINVAL;
        if (n_unlim == 1 && ndims > 0 && !var->dim[0]->unlimited)
            return NC_EINVAL;
    }

    /* -1 marks "storage not specified": the dataset is created
     * chunked when it has an unlimited dimension and contiguous
     * otherwise, and a later def-var setter must not change that
     * layout (#382). */
    var->storage = has_unlimited ? NC_CHUNKED : -1;

    coordinate = ndims == 1 && !strcmp(name, var->dim[0]->hdr.name);
    if ((ret = NEXTCDF4_map_hdf_type(xtype, &hdf_type)))
        goto fail;

    if (ndims == 0) {
        if ((space = H5Screate(H5S_SCALAR)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    } else {
        if ((space = H5Screate_simple(ndims, dims, maxdims)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if (has_unlimited) {
            if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0) {
                ret = NC_EHDFERR;
                goto fail;
            }
            if (H5Pset_chunk(dcpl, ndims, chunks) < 0) {
                ret = NC_EHDFERR;
                goto fail;
            }
        }
    }

    if (coordinate) {
        NEXTCDF4_DIM_INFO_T *dinfo = var->dim[0]->format_dim_info;
        if (!dinfo || dinfo->hdf_dataset < 0) { ret = NC_EFILEMETA; goto fail; }
        H5Dclose(dinfo->hdf_dataset);
        dinfo->hdf_dataset = -1;
        if (H5Ldelete(hdf_grp, name, H5P_DEFAULT) < 0)
            { ret = NC_EHDFERR; goto fail; }
        dinfo->hdf_dataset = H5Dcreate2(hdf_grp, name, hdf_type, space,
                                        H5P_DEFAULT, dcpl >= 0 ? dcpl : H5P_DEFAULT,
                                        H5P_DEFAULT);
        if (dinfo->hdf_dataset < 0 || H5DSset_scale(dinfo->hdf_dataset, name) < 0 ||
            (ret = write_int_att(dinfo->hdf_dataset, NEXTCDF4_DIMID_ATT,
                                 var->dimids[0])))
            { if (!ret) ret = NC_EHDFERR; goto fail; }
        vinfo->hdf_dataset = H5Dopen2(hdf_grp, name, H5P_DEFAULT);
    } else {
        vinfo->hdf_dataset = H5Dcreate2(hdf_grp, name, hdf_type, space,
                                        H5P_DEFAULT, dcpl >= 0 ? dcpl : H5P_DEFAULT,
                                        H5P_DEFAULT);
    }
    if (vinfo->hdf_dataset < 0) { ret = NC_EHDFERR; goto fail; }

    if ((ret = write_int_att(vinfo->hdf_dataset, NEXTCDF4_VARID_ATT,
                             var->hdr.id)) ||
        (ret = write_int_array_att(vinfo->hdf_dataset,
                                   NEXTCDF4_VARDIMIDS_ATT,
                                   (const int *)var->dimids, ndims)))
        goto fail;
    if (!coordinate) {
        for (i = 0; i < ndims; i++) {
            NEXTCDF4_DIM_INFO_T *dinfo = var->dim[i]->format_dim_info;
            if (!dinfo || dinfo->hdf_dataset < 0 ||
                H5DSattach_scale(vinfo->hdf_dataset, dinfo->hdf_dataset,
                                 (unsigned)i) < 0) {
                ret = NC_EHDFERR;
                goto fail;
            }
        }
    }

    if (varidp)
        *varidp = var->hdr.id;
    H5Tclose(hdf_type);
    H5Sclose(space);
    if (dcpl >= 0)
        H5Pclose(dcpl);
    return NC_NOERR;

fail:
    H5E_BEGIN_TRY {
        if (vinfo && vinfo->hdf_dataset >= 0)
            H5Dclose(vinfo->hdf_dataset);
    } H5E_END_TRY;
    if (var)
        nc4_var_list_del(grp, var);
    free(vinfo);
    if (hdf_type >= 0)
        H5Tclose(hdf_type);
    if (space >= 0)
        H5Sclose(space);
    if (dcpl >= 0)
        H5Pclose(dcpl);
    return ret;
}

/**
 * @brief Create or re-create the HDF5 dataset for a variable.
 *
 * Called by `NEXTCDF4_def_var` and by every def-var setter
 * (`def_var_chunking`, `deflate`, `fletcher32`, `filter`, `fill`,
 * `endian`): because HDF5 dataset creation properties are immutable,
 * changing chunking/compression/fill/endian requires deleting the
 * dataset link and recreating it with a new creation property list.
 * The hidden id attributes and dimension-scale attachments are
 * rewritten each time. The variable's filter pipeline comes from
 * `var->filters` via `nx4_apply_filters()`.
 *
 * @param var Variable.
 * @param h5 NetCDF-4 file state.
 * @param file NEXTCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_create_var_dataset(NC_VAR_INFO_T *var, NC_FILE_INFO_T *h5,
                       NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp)
{
    NEXTCDF4_VAR_INFO_T *vinfo = (NEXTCDF4_VAR_INFO_T *)var->format_var_info;
    NEXTCDF4_GRP_INFO_T *ginfo = (NEXTCDF4_GRP_INFO_T *)grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    hid_t hdf_type = -1;
    hid_t space = -1;
    hid_t dcpl = -1;
    hsize_t dims[NC_MAX_VAR_DIMS];
    hsize_t maxdims[NC_MAX_VAR_DIMS];
    hsize_t chunks[NC_MAX_VAR_DIMS];
    int has_unlimited = 0;
    int coordinate = 0;
    int i, ret;

    coordinate = (var->ndims == 1 &&
                  !strcmp(var->hdr.name, var->dim[0]->hdr.name));

    /* Recreation is delete-and-rebuild: drop the old link and dataset,
     * rebuild dataspace and creation properties from the current
     * settings, recreate, rewrite the id attributes and scale
     * attachments. */
    H5E_BEGIN_TRY {
        if (vinfo && vinfo->hdf_dataset >= 0) {
            H5Dclose(vinfo->hdf_dataset);
            vinfo->hdf_dataset = -1;
        }
        if (coordinate) {
            NEXTCDF4_DIM_INFO_T *dinfo =
                (NEXTCDF4_DIM_INFO_T *)var->dim[0]->format_dim_info;
            if (dinfo && dinfo->hdf_dataset >= 0) {
                H5Dclose(dinfo->hdf_dataset);
                dinfo->hdf_dataset = -1;
            }
        }
        H5Ldelete(hdf_grp, var->hdr.name, H5P_DEFAULT);
    } H5E_END_TRY;

    if ((ret = NEXTCDF4_map_hdf_type(var->type_info->hdr.id, &hdf_type)))
        return ret;

    for (i = 0; i < (int)var->ndims; i++) {
        dims[i] = var->dim[i]->len;
        if (var->dim[i]->unlimited) {
            maxdims[i] = H5S_UNLIMITED;
            has_unlimited = 1;
        } else {
            maxdims[i] = var->dim[i]->len;
        }
    }

    if (var->endianness == NC_ENDIAN_BIG)
        H5Tset_order(hdf_type, H5T_ORDER_BE);
    else if (var->endianness == NC_ENDIAN_LITTLE)
        H5Tset_order(hdf_type, H5T_ORDER_LE);

    if (var->ndims == 0) {
        if ((space = H5Screate(H5S_SCALAR)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    } else {
        if ((space = H5Screate_simple(var->ndims, dims, maxdims)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    }

    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }

    if (var->storage == NC_CHUNKED ||
        (var->storage < 0 && (has_unlimited ||
                              nclistlength((NClist *)var->filters) > 0))) {
        for (i = 0; i < (int)var->ndims; i++) {
            if (var->chunksizes && var->chunksizes[i])
                chunks[i] = var->chunksizes[i];
            else if (var->dim[i]->unlimited)
                chunks[i] = 1;
            else
                chunks[i] = (var->dim[i]->len > 0) ? var->dim[i]->len : 1;
        }
        if (H5Pset_chunk(dcpl, var->ndims, chunks) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if (var->storage < 0)
            var->storage = NC_CHUNKED;
    }

    if (var->ndims == 0) {
        H5Pclose(dcpl);
        dcpl = H5P_DEFAULT;
    } else {
        if (!var->no_fill && var->fill_value) {
            hid_t fill_type = H5Tget_native_type(hdf_type, H5T_DIR_DEFAULT);
            if (fill_type < 0 ||
                H5Pset_fill_value(dcpl, fill_type, var->fill_value) < 0) {
                if (fill_type >= 0)
                    H5Tclose(fill_type);
                ret = NC_EHDFERR;
                goto fail;
            }
            H5Tclose(fill_type);
        } else if (var->no_fill) {
            H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER);
        }

        if ((ret = nx4_apply_filters(dcpl, var)))
            goto fail;
    }

    if (coordinate) {
        NEXTCDF4_DIM_INFO_T *dinfo =
            (NEXTCDF4_DIM_INFO_T *)var->dim[0]->format_dim_info;
        if (!dinfo) {
            ret = NC_EFILEMETA;
            goto fail;
        }
        dinfo->hdf_dataset = H5Dcreate2(hdf_grp, var->hdr.name, hdf_type, space,
                                        H5P_DEFAULT, dcpl, H5P_DEFAULT);
        if (dinfo->hdf_dataset < 0 ||
            H5DSset_scale(dinfo->hdf_dataset, var->hdr.name) < 0 ||
            (ret = write_int_att(dinfo->hdf_dataset, NEXTCDF4_DIMID_ATT,
                                 var->dimids[0])))
            { if (!ret) ret = NC_EHDFERR; goto fail; }
        vinfo->hdf_dataset = H5Dopen2(hdf_grp, var->hdr.name, H5P_DEFAULT);
        if (vinfo->hdf_dataset < 0)
            { ret = NC_EHDFERR; goto fail; }
    } else {
        vinfo->hdf_dataset = H5Dcreate2(hdf_grp, var->hdr.name, hdf_type, space,
                                        H5P_DEFAULT, dcpl, H5P_DEFAULT);
        if (vinfo->hdf_dataset < 0)
            { ret = NC_EHDFERR; goto fail; }
    }

    if (dcpl >= 0 && dcpl != H5P_DEFAULT)
        H5Pclose(dcpl);
    dcpl = -1;

    if ((ret = write_int_att(vinfo->hdf_dataset, NEXTCDF4_VARID_ATT,
                             var->hdr.id)) ||
        (ret = write_int_array_att(vinfo->hdf_dataset,
                                   NEXTCDF4_VARDIMIDS_ATT,
                                   (const int *)var->dimids, var->ndims)))
        goto fail;

    if (!coordinate) {
        for (i = 0; i < (int)var->ndims; i++) {
            NEXTCDF4_DIM_INFO_T *dinfo =
                (NEXTCDF4_DIM_INFO_T *)var->dim[i]->format_dim_info;
            if (!dinfo || dinfo->hdf_dataset < 0 ||
                H5DSattach_scale(vinfo->hdf_dataset, dinfo->hdf_dataset,
                                 (unsigned)i) < 0) {
                ret = NC_EHDFERR;
                goto fail;
            }
        }
    }

    H5Tclose(hdf_type);
    H5Sclose(space);
    return NC_NOERR;

fail:
    H5E_BEGIN_TRY { if (vinfo && vinfo->hdf_dataset >= 0) H5Dclose(vinfo->hdf_dataset); } H5E_END_TRY;
    if (vinfo)
        vinfo->hdf_dataset = -1;
    H5E_BEGIN_TRY { H5Ldelete(hdf_grp, var->hdr.name, H5P_DEFAULT); } H5E_END_TRY;
    if (hdf_type >= 0)
        H5Tclose(hdf_type);
    if (space >= 0)
        H5Sclose(space);
    if (dcpl >= 0 && dcpl != H5P_DEFAULT)
        H5Pclose(dcpl);
    return ret;
}

/**
 * @brief Common prologue for the def-var setter dispatch functions.
 *
 * Resolves the variable and file state and checks define-mode write
 * access, so each setter only validates its own arguments.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable identifier.
 * @param h5 Destination for the NetCDF-4 file state.
 * @param file Destination for the NEXTCDF-4 file state.
 * @param grp Destination for the group.
 * @param var Destination for the variable.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_find_var_for_write(int ncid, int varid, NC_FILE_INFO_T **h5,
                       NEXTCDF4_FILE_INFO_T **file, NC_GRP_INFO_T **grp,
                       NC_VAR_INFO_T **var)
{
    int ret;
    if ((ret = nc4_find_grp_h5_var(ncid, varid, h5, grp, var)))
        return ret;
    if ((ret = NEXTCDF4_get_file(ncid, NULL, file)))
        return ret;
    return NEXTCDF4_check_write_define(*file);
}

/**
 * @brief Set storage and chunk sizes — the `nc_def_var_chunking()`
 * dispatch entry.
 *
 * Records the layout and per-dimension chunk sizes, then recreates
 * the dataset via create_var_dataset() since layout is fixed at
 * dataset creation in HDF5.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable identifier.
 * @param storage Storage class (`NC_CHUNKED` or `NC_CONTIGUOUS`;
 * `NC_COMPACT` is rejected).
 * @param chunksizesp Chunk size for each dimension, or NULL.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_chunking(int ncid, int varid, int storage,
                          const size_t *chunksizesp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int i, ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_EINVAL;

    if (storage != NC_CHUNKED && storage != NC_CONTIGUOUS)
        return NC_EINVAL;
    if (var->ndims == 0 && storage == NC_CHUNKED)
        return NC_EINVAL;
    /* A filtered variable cannot be contiguous. */
    if (storage != NC_CHUNKED &&
        nclistlength((NClist *)var->filters) > 0)
        return NC_EINVAL;

    var->storage = storage;
    if (storage == NC_CHUNKED) {
        if (!chunksizesp)
            return NC_EINVAL;
        if (!var->chunksizes) {
            if (!(var->chunksizes = calloc(var->ndims, sizeof(size_t))))
                return NC_ENOMEM;
        }
        for (i = 0; i < (int)var->ndims; i++)
            var->chunksizes[i] = chunksizesp[i];
    }

    return nx4_create_var_dataset(var, h5, file, grp);
}

/**
 * @brief Set fill behavior — the `nc_def_var_fill()` dispatch entry.
 *
 * Stores the no-fill flag and a private copy of the fill value on the
 * variable, then recreates the dataset: fill value and fill time are
 * HDF5 creation properties. (Known defect #377: a bfloat16 fill value
 * is not recovered on reopen.)
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param no_fill Nonzero to suppress fill values.
 * @param fill_value Fill value, or NULL for the type default.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    size_t size;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;

    var->no_fill = no_fill ? 1 : 0;
    if (var->fill_value) {
        free(var->fill_value);
        var->fill_value = NULL;
    }
    if (!no_fill && fill_value) {
        if ((ret = NEXTCDF4_type_size(var->type_info->hdr.id, &size)))
            return ret;
        if (!(var->fill_value = malloc(size)))
            return NC_ENOMEM;
        memcpy(var->fill_value, fill_value, size);
        var->fill_val_changed = 1;
    }

    return nx4_create_var_dataset(var, h5, file, grp);
}

/**
 * @brief Set the on-disk byte order — the `nc_def_var_endian()`
 * dispatch entry.
 *
 * Overrides the datatype's byte order in create_var_dataset();
 * `NC_ENDIAN_NATIVE` leaves the little-endian default produced by
 * NEXTCDF4_map_hdf_type().
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param endianness `NC_ENDIAN_NATIVE`, `NC_ENDIAN_LITTLE` or `NC_ENDIAN_BIG`.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_endian(int ncid, int varid, int endianness)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (endianness != NC_ENDIAN_NATIVE && endianness != NC_ENDIAN_LITTLE &&
        endianness != NC_ENDIAN_BIG)
        return NC_EINVAL;

    var->endianness = endianness;
    return nx4_create_var_dataset(var, h5, file, grp);
}

/**
 * @brief Set quantization — the `nc_def_var_quantize()` dispatch
 * entry.
 *
 * Quantization records a mode and precision on the variable; it is
 * applied at write time by nx4_convert_type(), so no dataset
 * recreation is needed here.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param quantize_mode `NC_NOQUANTIZE`, `NC_QUANTIZE_BITGROOM`, `NC_QUANTIZE_GRANULARBR` or `NC_QUANTIZE_BITROUND`.
 * @param nsd Number of significant digits.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_quantize(int ncid, int varid, int quantize_mode, int nsd)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (quantize_mode != NC_NOQUANTIZE &&
        quantize_mode != NC_QUANTIZE_BITGROOM &&
        quantize_mode != NC_QUANTIZE_GRANULARBR &&
        quantize_mode != NC_QUANTIZE_BITROUND)
        return NC_EINVAL;
    if (quantize_mode != NC_NOQUANTIZE && nsd < 1)
        return NC_EINVAL;

    var->quantize_mode = quantize_mode;
    var->nsd = nsd;
    return NC_NOERR;
}

/**
 * @brief Inquire about all supported properties of a variable — the
 * `nc_inq_var_all()` dispatch entry.
 *
 * The kitchen-sink inquiry used internally by netcdf-c; every output
 * pointer is optional. The `idp`/`nparamsp`/`params` filter outputs
 * are never populated — upstream's shared `NC4_inq_var_all` doesn't
 * populate them either; filter inquiry goes through
 * `nc_inq_var_filter_ids()`/`nc_inq_var_filter_info()`.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name Destination for the variable name.
 * @param xtypep Destination for the variable type.
 * @param ndimsp Destination for the rank.
 * @param dimidsp Destination for the dimension ids.
 * @param nattsp Destination for the number of attributes.
 * @param shufflep Destination for the shuffle flag.
 * @param deflatep Destination for the deflate flag.
 * @param deflate_levelp Destination for the deflate level.
 * @param fletcher32p Destination for the Fletcher32 flag.
 * @param contiguousp Destination for the contiguous-storage flag.
 * @param chunksizesp Destination for the chunk sizes.
 * @param no_fillp Destination for the no-fill flag.
 * @param fill_valuep Destination for the fill value.
 * @param endiannessp Destination for the endianness.
 * @param idp Destination for the filter ids.
 * @param nparamsp Destination for the filter parameter count.
 * @param params Destination for the filter parameters.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                     int *ndimsp, int *dimidsp, int *nattsp,
                     int *shufflep, int *deflatep, int *deflate_levelp,
                     int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                     int *no_fillp, void *fill_valuep, int *endiannessp,
                     unsigned int *idp, size_t *nparamsp, unsigned int *params)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    struct NC_HDF5_Filter *fspec = NULL;
    size_t size;
    int i, ret;

    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return ret;

    if (name)
        strncpy(name, var->hdr.name, NC_MAX_NAME);
    if (xtypep)
        *xtypep = var->type_info->hdr.id;
    if (ndimsp)
        *ndimsp = (int)var->ndims;
    if (dimidsp)
        for (i = 0; i < (int)var->ndims; i++)
            dimidsp[i] = var->dimids[i];
    if (nattsp)
        *nattsp = ncindexsize(var->att);
    if (shufflep)
        *shufflep = (!NC4_hdf5_filter_lookup(var, H5Z_FILTER_SHUFFLE, NULL));
    if (fletcher32p)
        *fletcher32p = (!NC4_hdf5_filter_lookup(var, H5Z_FILTER_FLETCHER32, NULL));
    if (deflatep || deflate_levelp) {
        int has_deflate = !NC4_hdf5_filter_lookup(var, H5Z_FILTER_DEFLATE,
                                                  &fspec);
        if (deflatep)
            *deflatep = has_deflate;
        if (deflate_levelp)
            *deflate_levelp = (has_deflate && fspec->nparams > 0) ?
                              (int)fspec->params[0] : 0;
    }
    if (contiguousp)
        *contiguousp = (var->storage < 0) ? NC_CONTIGUOUS : var->storage;
    if (chunksizesp && var->storage == NC_CHUNKED) {
        for (i = 0; i < (int)var->ndims; i++)
            chunksizesp[i] = (var->chunksizes && var->chunksizes[i]) ?
                             var->chunksizes[i] : (size_t)((var->dim[i]->unlimited) ? 1 : var->dim[i]->len);
    }
    if (no_fillp)
        *no_fillp = (int)var->no_fill;
    if (fill_valuep && !var->no_fill && var->fill_value) {
        if ((ret = NEXTCDF4_type_size(var->type_info->hdr.id, &size)))
            return ret;
        memcpy(fill_valuep, var->fill_value, size);
    }
    if (endiannessp)
        *endiannessp = var->endianness;

    (void)idp; (void)nparamsp; (void)params; (void)h5;
    return NC_NOERR;
}

/**
 * @brief Inquire about a variable's name, type, dimensions, and
 * attributes — the `nc_inq_var()` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name Destination for the variable name.
 * @param xtypep Destination for the variable type.
 * @param ndimsp Destination for the rank.
 * @param dimidsp Destination for the dimension ids.
 * @param nattsp Destination for the number of attributes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_var(int ncid, int varid, char *name, nc_type *xtypep,
                 int *ndimsp, int *dimidsp, int *nattsp)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int ret;
    int i;

    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return ret;
    if (name)
        strncpy(name, var->hdr.name, NC_MAX_NAME);
    if (xtypep)
        *xtypep = var->type_info->hdr.id;
    if (ndimsp)
        *ndimsp = (int)var->ndims;
    if (dimidsp) {
        for (i = 0; i < (int)var->ndims; i++)
            dimidsp[i] = var->dimids[i];
    }
    if (nattsp)
        *nattsp = ncindexsize(var->att);
    return NC_NOERR;
}

/**
 * @brief Create or replace an attribute — the `nc_put_att_*` dispatch
 * entry.
 *
 * Converts the value from the caller's memory type to the attribute's
 * storage type through nx4_convert_type() (overflow returns
 * `NC_ERANGE` after the write), replaces any same-named attribute in
 * both the in-memory index and HDF5, and persists immediately —
 * attributes are written to the file in define mode, not deferred to
 * `_enddef`. Reserved (internal) names are rejected.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param datatype NetCDF atomic type.
 * @param len Number of elements.
 * @param value Attribute data.
 * @param memtype In-memory type for the attribute value.
 * @return `NC_NOERR` on success, `NC_ERANGE` when the value was
 * written but overflowed during conversion, or a NetCDF error code.
 */
int
NEXTCDF4_put_att(int ncid, int varid, const char *name, nc_type datatype,
                 size_t len, const void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NCindex *list;
    hid_t loc;
    NC_ATT_INFO_T *att;
    size_t size;
    void *data = NULL;
    hid_t hdf_type = -1;
    hid_t space = -1;
    hid_t attr = -1, mem_type = -1;
    int close_mem = 0;
    hsize_t hlen = len;
    int range_error = 0;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NEXTCDF4_check_atomic_type(file, datatype)))
        return ret;
    if ((ret = NC_check_name(name)))
        return ret;
    if (is_reserved_att(name))
        return NC_EBADNAME;
    if (varid == NC_GLOBAL) {
        grp = h5->root_grp;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
    }

    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;

    if (memtype == NC_NAT) memtype = datatype;
    if ((ret = nx4_convert_type(NULL, NULL, memtype, datatype, 0, &range_error,
                                NULL, 0, NC_NOQUANTIZE, 0)) && datatype != memtype)
        return ret;
    if (len && !value) return NC_EINVAL;
    if ((ret = NEXTCDF4_type_size(datatype, &size))) return ret;
    if (len > SIZE_MAX / size) return NC_EINVAL;
    if (len) {
        if (!(data = malloc(len * size))) return NC_ENOMEM;
        if (datatype == memtype) memcpy(data, value, len * size);
        else {
            ret = nx4_convert_type(value, data, memtype, datatype, len,
                                    &range_error, NULL, 0, NC_NOQUANTIZE, 0);
            if (ret) { free(data); return ret; }
        }
    }
    /* Replace an existing attribute of the same name. */
    if ((att = find_att(list, name))) {
        nc4_att_list_del(list, att);
        H5E_BEGIN_TRY {
            H5Adelete(loc, name);
        } H5E_END_TRY;
    }

    if ((ret = add_att(list, var ? &var->hdr : &grp->hdr, name, datatype,
                       len, data, &att)))
        goto fail;
    if (data) {
        free(data);
        data = NULL;
    }

    /* Persist to HDF5 immediately. */
    if ((ret = NEXTCDF4_map_hdf_type(datatype, &hdf_type)))
        goto fail;
    if (len == 0) {
        if ((space = H5Screate(H5S_NULL)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    } else {
        if ((space = H5Screate_simple(1, &hlen, NULL)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    }
    {
        if (datatype == NC_CHAR) {
            mem_type = hdf_type;
        } else if (datatype == NC_STRING) {
            mem_type = H5Tcopy(H5T_C_S1);
            if (mem_type >= 0) {
                H5Tset_size(mem_type, H5T_VARIABLE);
                H5Tset_strpad(mem_type, H5T_STR_NULLTERM);
            }
            close_mem = 1;
        } else {
            mem_type = NEXTCDF4_memory_type(datatype, &close_mem);
            if (mem_type < 0) { ret = NC_EBADTYPE; goto fail; }
        }
        if ((attr = H5Acreate2(loc, name, hdf_type, space, H5P_DEFAULT,
                               H5P_DEFAULT)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if (len > 0 && H5Awrite(attr, mem_type, att->data) < 0)
            { ret = NC_EHDFERR; goto fail; }
        if (close_mem) {
            H5Tclose(mem_type);
            close_mem = 0;
        }
    }

    H5Aclose(attr);
    H5Tclose(hdf_type);
    H5Sclose(space);
    return range_error ? NC_ERANGE : NC_NOERR;

fail:
    if (close_mem && mem_type >= 0)
        H5Tclose(mem_type);
    if (data)
        free(data);
    if (attr >= 0)
        H5Aclose(attr);
    if (hdf_type >= 0)
        H5Tclose(hdf_type);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Inquire about an attribute — the `nc_inq_att()` dispatch
 * entry.
 *
 * Stock `NC4_inq_att` resolves `att->nc_typeid` through
 * nc4_find_type(), which knows only stock atomic typeids and the
 * file's user-defined types — NEP atomic types (float16, bitfield,
 * `NC_STD_REF`, ...) all fail there. This version returns the
 * stored typeid directly, so `nc_inq_att()` and the generic
 * `nc_get_att()` work for NEP types.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param xtypep Destination for the attribute's type, or NULL.
 * @param lenp Destination for the attribute's length, or NULL.
 * @return `NC_NOERR` on success, `NC_ENOTATT` when the attribute
 * does not exist, or a NetCDF error code.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
NEXTCDF4_inq_att(int ncid, int varid, const char *name, nc_type *xtypep,
                 size_t *lenp)
{
    NCindex *list;
    hid_t loc;
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NC_ATT_INFO_T *att;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if (varid == NC_GLOBAL) {
        grp = h5->root_grp;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
    }
    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;
    if (!(att = find_att(list, name)))
        return NC_ENOTATT;
    if (xtypep)
        *xtypep = att->nc_typeid;
    if (lenp)
        *lenp = att->len;
    return NC_NOERR;
}

/**
 * @brief Read an attribute's value — the `nc_get_att_*` dispatch
 * entry.
 *
 * Reads from the in-memory record (all attributes were loaded at
 * open), converting through nx4_convert_type() when the caller asks
 * for a different type. `NC_STRING` attributes return `strdup`'d
 * strings the caller owns.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param value Destination buffer.
 * @param memtype Requested in-memory type.
 * @return `NC_NOERR` on success, `NC_ERANGE` when data was delivered
 * but overflowed, or a NetCDF error code.
 */
int
NEXTCDF4_get_att(int ncid, int varid, const char *name, void *value,
                 nc_type memtype)
{
    NCindex *list;
    hid_t loc;
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NC_ATT_INFO_T *att;
    int range_error;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if (varid == NC_GLOBAL) {
        grp = h5->root_grp;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
    }
    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;

    if (!(att = find_att(list, name)))
        return NC_ENOTATT;

    if (memtype == NC_NAT) memtype = att->nc_typeid;
    if (att->nc_typeid != memtype &&
        (ret = nx4_convert_type(NULL, NULL, att->nc_typeid, memtype, 0,
                                &range_error, NULL, 0, NC_NOQUANTIZE, 0))) return ret;
    if (att->nc_typeid == memtype && att->len > 0) {
        if (memtype == NC_STRING) {
            char **src = (char **)att->data;
            char **dst = (char **)value;
            size_t i;
            for (i = 0; i < att->len; i++) {
                dst[i] = strdup(src[i]);
                if (!dst[i])
                    return NC_ENOMEM;
            }
            return NC_NOERR;
        } else {
            size_t size;
            if ((ret = NEXTCDF4_type_size(memtype, &size)))
                return ret;
            memcpy(value, att->data, att->len * size);
            return NC_NOERR;
        }
    }

    if (att->len == 0)
        return NC_NOERR;

    ret = nx4_convert_type(att->data, value, att->nc_typeid, memtype,
                            att->len, &range_error, NULL, 0, NC_NOQUANTIZE, 0);
    return ret ? ret : range_error ? NC_ERANGE : NC_NOERR;
}

/**
 * @brief Rename an attribute — the `nc_rename_att()` dispatch entry.
 *
 * HDF5 attributes cannot be renamed in place, so the old attribute is
 * deleted and the value rewritten under the new name via
 * NEXTCDF4_put_att(); the in-memory index is rebuilt by
 * delete-and-re-add for the same reason.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Current attribute name.
 * @param newname New attribute name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_rename_att(int ncid, int varid, const char *name, const char *newname)
{
    NCindex *list;
    hid_t loc;
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NC_ATT_INFO_T *att;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NC_check_name(newname)))
        return ret;
    if (is_reserved_att(newname))
        return NC_EBADNAME;
    if (varid == NC_GLOBAL) {
        grp = h5->root_grp;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
    }
    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;
    if (!(att = find_att(list, name)))
        return NC_ENOTATT;
    if (find_att(list, newname))
        return NC_ENAMEINUSE;

    /* Persist the new name to HDF5. */
    H5E_BEGIN_TRY {
        H5Adelete(loc, name);
    } H5E_END_TRY;
    /* Re-add with the new name so nc4_att_list_add handles indexing. */
    if ((ret = add_att(list, att->container, newname, att->nc_typeid,
                       att->len, att->data, NULL))) {
        add_att(list, att->container, name, att->nc_typeid, att->len,
                att->data, NULL);
        return ret;
    }
    if ((ret = NEXTCDF4_put_att(ncid, varid, newname, att->nc_typeid,
                                att->len, att->data, att->nc_typeid))) {
        nc4_att_list_del(list, att);
        nc4_att_free(att);
        return ret;
    }
    nc4_att_list_del(list, att);
    nc4_att_free(att);

    if (H5Fflush(file->hdfid, H5F_SCOPE_LOCAL) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * Find a 1D coordinate variable that uses a dimension.
 * @param grp Group.
 * @param dim Dimension.
 * @return Pointer to the coordinate variable, or NULL if none.
 */
static NC_VAR_INFO_T *
find_coord_var(NC_GRP_INFO_T *grp, NC_DIM_INFO_T *dim)
{
    size_t i;
    if (!grp->vars)
        return NULL;
    for (i = 0; i < (size_t)ncindexsize(grp->vars); i++) {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        if (var && var->ndims == 1 && var->dimids[0] == dim->hdr.id &&
            var->hdr.name && !strcmp(var->hdr.name, dim->hdr.name))
            return var;
    }
    return NULL;
}

/**
 * Determine if a variable is a 1D coordinate variable.
 * @param var Variable.
 * @return Pointer to the dimension, or NULL if none.
 */
static NC_DIM_INFO_T *
coord_var_dim(NC_VAR_INFO_T *var)
{
    if (var->ndims == 1 && var->dim[0] &&
        var->hdr.name && var->dim[0]->hdr.name &&
        !strcmp(var->hdr.name, var->dim[0]->hdr.name))
        return var->dim[0];
    return NULL;
}

/**
 * @brief Rename a dimension and update its HDF5 link.
 * @param ncid NetCDF file identifier.
 * @param dimid Dimension id.
 * @param name New dimension name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_rename_dim(int ncid, int dimid, const char *name)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_DIM_INFO_T *dim = NULL;
    NEXTCDF4_DIM_INFO_T *dinfo = NULL;
    NEXTCDF4_GRP_INFO_T *ginfo;
    hid_t hdf_grp = -1;
    char *oldname;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NC_check_name(name)))
        return ret;

    if ((ret = nc4_find_dim(grp, dimid, &dim, NULL)))
        return ret;
    if (!dim)
        return NC_EBADDIM;
    if (!strcmp(dim->hdr.name, name))
        return NC_NOERR;

    if ((ret = nc4_check_dup_name(dim->container, (char *)name)))
        return ret;

    dinfo = (NEXTCDF4_DIM_INFO_T *)dim->format_dim_info;
    ginfo = dim->container->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    oldname = dim->hdr.name;

    if (dinfo && dinfo->hdf_dataset >= 0) {
        if (H5Lmove(hdf_grp, oldname, hdf_grp, name, H5P_DEFAULT,
                    H5P_DEFAULT) < 0)
            return NC_EHDFERR;
        if (H5DSset_scale(dinfo->hdf_dataset, name) < 0)
            return NC_EHDFERR;
    }

    free(dim->hdr.name);
    if (!(dim->hdr.name = strdup(name)))
        return NC_ENOMEM;
    if (!ncindexrebuild(dim->container->dim))
        return NC_EHDFERR;

    /* Do not rename the in-memory coordinate variable here; netcdf-c
     * semantics keep the variable name unchanged until the caller
     * explicitly renames it with nc_rename_var. The HDF5 link, however,
     * has already moved with the dimension-scale dataset. */

    if (H5Fflush(file->hdfid, H5F_SCOPE_LOCAL) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * @brief Rename a variable and update its HDF5 link.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name New variable name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_rename_var(int ncid, int varid, const char *name)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NEXTCDF4_VAR_INFO_T *vinfo = NULL;
    NEXTCDF4_GRP_INFO_T *ginfo;
    NC_DIM_INFO_T *dim;
    NEXTCDF4_DIM_INFO_T *dinfo;
    hid_t hdf_grp = -1;
    char *oldname;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if ((ret = NC_check_name(name)))
        return ret;

    if (varid < 0 || varid >= ncindexsize(grp->vars))
        return NC_ENOTVAR;
    var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid);
    if (!var)
        return NC_ENOTVAR;
    if (!strcmp(var->hdr.name, name))
        return NC_NOERR;

    if ((ret = nc4_check_dup_name(var->container, (char *)name)))
        return ret;

    vinfo = (NEXTCDF4_VAR_INFO_T *)var->format_var_info;
    ginfo = var->container->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    oldname = var->hdr.name;

    if (vinfo && vinfo->hdf_dataset >= 0) {
        char cur_path[1024];
        ssize_t n;
        const char *cur_name = oldname;

        n = H5Iget_name(vinfo->hdf_dataset, cur_path, sizeof(cur_path));
        if (n > 0 && n < (ssize_t)sizeof(cur_path)) {
            char *p = strrchr(cur_path, '/');
            if (p)
                cur_name = p + 1;
        }

        if (H5Lexists(hdf_grp, name, H5P_DEFAULT) > 0) {
            /* The target link already exists. If it is the same object
             * as the variable's dataset (e.g. the dimension was already
             * renamed), no link move is needed. Otherwise delete the
             * existing object and move the variable into its name. */
            if (strcmp(cur_name, name) != 0) {
                H5E_BEGIN_TRY {
                    H5Ldelete(hdf_grp, name, H5P_DEFAULT);
                } H5E_END_TRY;
                if (H5Lmove(hdf_grp, cur_name, hdf_grp, name, H5P_DEFAULT,
                            H5P_DEFAULT) < 0)
                    return NC_EHDFERR;
            }
        } else if (H5Lexists(hdf_grp, cur_name, H5P_DEFAULT) > 0) {
            if (H5Lmove(hdf_grp, cur_name, hdf_grp, name, H5P_DEFAULT,
                        H5P_DEFAULT) < 0)
                return NC_EHDFERR;
        }
        /* If cur_name has no link the dataset was already moved. */
    }

    free(var->hdr.name);
    if (!(var->hdr.name = strdup(name)))
        return NC_ENOMEM;
    if (!ncindexrebuild(var->container->vars))
        return NC_EHDFERR;

    dim = coord_var_dim(var);
    if (dim) {
        dinfo = (NEXTCDF4_DIM_INFO_T *)dim->format_dim_info;
        if (dinfo && dinfo->hdf_dataset >= 0) {
            if (dinfo->hdf_dataset != vinfo->hdf_dataset) {
                if (H5Dclose(dinfo->hdf_dataset) < 0)
                    return NC_EHDFERR;
                dinfo->hdf_dataset = H5Dopen2(hdf_grp, name, H5P_DEFAULT);
                if (dinfo->hdf_dataset < 0)
                    return NC_EHDFERR;
            }
            if (H5DSset_scale(dinfo->hdf_dataset, name) < 0)
                return NC_EHDFERR;
        }
        free(dim->hdr.name);
        if (!(dim->hdr.name = strdup(name)))
            return NC_ENOMEM;
        if (!ncindexrebuild(var->container->dim))
            return NC_EHDFERR;
    }

    if (H5Fflush(file->hdfid, H5F_SCOPE_LOCAL) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * @brief Delete an attribute from a group or variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_del_att(int ncid, int varid, const char *name)
{
    NCindex *list;
    hid_t loc;
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var = NULL;
    NC_ATT_INFO_T *att;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(file)))
        return ret;
    if (varid == NC_GLOBAL) {
        grp = h5->root_grp;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
    }
    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;
    if (!(att = find_att(list, name)))
        return NC_ENOTATT;

    H5E_BEGIN_TRY {
        H5Adelete(loc, name);
    } H5E_END_TRY;
    nc4_att_list_del(list, att);
    nc4_att_free(att);
    return NC_NOERR;
}

/**
 * @brief Accumulator used while iterating a group's links looking for
 * dimension scales.
 *
 * `H5Literate` visits links in name order, but dimids must be
 * recovered in id order — find_dim_cb() collects name/dimid pairs
 * here and load_dimensions() replays them sorted.
 */
typedef struct {
    char **names;
    int *dimids;
    int count;
    int capacity;
} dim_list_t;

/**
 * @brief `H5Literate` callback collecting dimension-scale datasets.
 *
 * A link is a dimension scale when its dataset carries
 * `CLASS = "DIMENSION_SCALE"` and a `_Netcdf4Dimid` attribute; those
 * two hidden attributes are how the loader recovers both the name and
 * the dimid. Anything else is skipped.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param info Link info (unused).
 * @param op_data dim_list_t collection context.
 * @return 0 to continue iteration, negative on error.
 */
static herr_t
find_dim_cb(hid_t loc, const char *name, const H5L_info_t *info, void *op_data)
{
    dim_list_t *dl = op_data;
    hid_t dset = -1;
    hid_t att = -1;
    char cls[64];
    int dimid = -1;
    (void)info;

    H5E_BEGIN_TRY {
        dset = H5Dopen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (dset < 0)
        return 0;

    H5E_BEGIN_TRY {
        att = H5Aopen(dset, "CLASS", H5P_DEFAULT);
    } H5E_END_TRY;
    if (att >= 0) {
        hid_t type = H5Aget_type(att);
        if (type >= 0) {
            H5Aread(att, type, cls);
            H5Tclose(type);
        }
        H5Aclose(att);
    }
    H5Dclose(dset);

    if (att < 0 || strcmp(cls, NEXTCDF4_DIMCLASS))
        return 0;

    H5E_BEGIN_TRY {
        dset = H5Dopen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (dset >= 0) {
        H5E_BEGIN_TRY {
            att = H5Aopen(dset, NEXTCDF4_DIMID_ATT, H5P_DEFAULT);
        } H5E_END_TRY;
        if (att >= 0) {
            H5Aread(att, H5T_NATIVE_INT, &dimid);
            H5Aclose(att);
        }
        H5Dclose(dset);
    }

    if (dimid < 0)
        return 0;

    if (dl->count >= dl->capacity) {
        dl->capacity = dl->capacity ? dl->capacity * 2 : 4;
        dl->names = realloc(dl->names, dl->capacity * sizeof(char *));
        dl->dimids = realloc(dl->dimids, dl->capacity * sizeof(int));
    }
    dl->names[dl->count] = strdup(name);
    dl->dimids[dl->count] = dimid;
    dl->count++;
    return 0;
}

/**
 * @brief Load one dimension from its scale dataset into the in-memory
 * tree.
 *
 * Recovers current length and unlimited-ness (`H5S_UNLIMITED` max
 * extent) from the dataset's dataspace, keeps the dataset `hid_t`
 * open in `NEXTCDF4_DIM_INFO_T` for later scale attachment.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group.
 * @param name Name.
 * @param dimid Dimension identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_one_dim(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp,
             const char *name, int dimid)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    hid_t dset = -1;
    hid_t space = -1;
    hid_t att = -1;
    hsize_t dims[1];
    hsize_t maxdims[1];
    NC_DIM_INFO_T *dim = NULL;
    NEXTCDF4_DIM_INFO_T *dinfo = NULL;
    size_t len;
    int unlimited;
    int ret = NC_NOERR;

    if (!(dinfo = calloc(1, sizeof(*dinfo))))
        return NC_ENOMEM;

    if ((dset = H5Dopen2(hdf_grp, name, H5P_DEFAULT)) < 0) {
        free(dinfo);
        return NC_EHDFERR;
    }
    if ((space = H5Dget_space(dset)) < 0) {
        ret = NC_EHDFERR;
        goto done;
    }
    if (H5Sget_simple_extent_dims(space, dims, maxdims) < 0) {
        ret = NC_EHDFERR;
        goto done;
    }
    len = (size_t)dims[0];
    unlimited = (maxdims[0] == H5S_UNLIMITED);

    if ((ret = nc4_dim_list_add(grp, name, len, dimid, &dim))) {
        free(dinfo);
        goto done;
    }
    dim->unlimited = unlimited;
    dim->format_dim_info = dinfo;
    dinfo->hdf_dataset = dset;
    dset = -1; /* ownership transferred to dinfo */

done:
    if (dset >= 0)
        H5Dclose(dset);
    if (space >= 0)
        H5Sclose(space);
    if (att >= 0)
        H5Aclose(att);
    return ret;
}

/**
 * @brief Load all of a group's dimensions in stable dimid order.
 *
 * HDF5 iterates links by name, but netCDF dimids are positional — so
 * the callback pass collects them and this replays them 0..maxid.
 * Missing ids (a sparse id space) fail as `NC_EFILEMETA`.
 *
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_dimensions(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
                NC_GRP_INFO_T *grp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    dim_list_t dl = {0};
    int maxid = -1;
    int loaded = 0;
    int id;
    int i;
    int ret;
    (void)h5;

    if (H5Literate(hdf_grp, H5_INDEX_NAME, H5_ITER_INC, NULL,
                   find_dim_cb, &dl) < 0)
        return NC_EHDFERR;

    for (i = 0; i < dl.count; i++)
        if (dl.dimids[i] > maxid)
            maxid = dl.dimids[i];

    for (id = 0; id <= maxid; id++) {
        for (i = 0; i < dl.count; i++) {
            if (dl.dimids[i] == id) {
                if ((ret = load_one_dim(file, grp,
                                        dl.names[i], dl.dimids[i]))) {
                    free_name_list(dl.names, dl.count);
                    free(dl.dimids);
                    return ret;
                }
                loaded++;
                break;
            }
        }
    }

    if (loaded != dl.count) {
        free_name_list(dl.names, dl.count);
        free(dl.dimids);
        return NC_EFILEMETA;
    }

    free_name_list(dl.names, dl.count);
    free(dl.dimids);
    return NC_NOERR;
}

/**
 * @brief Accumulator used while iterating a group's links looking for
 * variable datasets — the varid counterpart of #dim_list_t.
 */
typedef struct {
    char **names;
    int *varids;
    int count;
    int capacity;
} var_list_t;

/**
 * @brief `H5Literate` callback collecting candidate variable
 * datasets.
 *
 * Datasets carrying `_Netcdf4Varid` are variables unconditionally.
 * Without it (files written by upstream netcdf-c or older NEXTCDF-4),
 * heuristics apply: dimension scales count only if they are
 * coordinate variables (carry `_Netcdf4Coordinates`), and plain
 * datasets count only if they have attached scales or the
 * coordinates attribute — this is what lets the loader read
 * unmarked netCDF-4/HDF5 files.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param info Link info (unused).
 * @param op_data var_list_t collection context.
 * @return 0 to continue iteration, negative on error.
 */
static herr_t
find_var_cb(hid_t loc, const char *name, const H5L_info_t *info, void *op_data)
{
    var_list_t *vl = op_data;
    hid_t dset = -1;
    hid_t att = -1;
    int varid = -1;
    int is_scale;
    (void)info;

    H5E_BEGIN_TRY {
        dset = H5Dopen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (dset < 0)
        return 0;

    /* NEXTCDF-4 files store a variable id attribute. */
    H5E_BEGIN_TRY {
        att = H5Aopen(dset, NEXTCDF4_VARID_ATT, H5P_DEFAULT);
    } H5E_END_TRY;
    if (att >= 0) {
        H5Aread(att, H5T_NATIVE_INT, &varid);
        H5Aclose(att);
        att = -1;
    }

    /* If no varid is present, accept datasets that look like NetCDF-4
     * variables: coordinate variables (scales with _Netcdf4Coordinates) or
     * non-scale datasets with attached dimension scales or _Netcdf4Coordinates. */
    if (varid < 0) {
        hid_t space = -1;
        int ndims = 0;

        is_scale = H5DSis_scale(dset);
        if (is_scale > 0) {
            H5E_BEGIN_TRY {
                att = H5Aopen(dset, NEXTCDF4_VARDIMIDS_ATT, H5P_DEFAULT);
            } H5E_END_TRY;
            if (att < 0) {
                H5Dclose(dset);
                return 0;
            }
            H5Aclose(att);
            att = -1;
        } else if (is_scale < 0) {
            H5Dclose(dset);
            return -1;
        } else {
            int has_vardimids = 0;
            int has_scales = 0;

            H5E_BEGIN_TRY {
                att = H5Aopen(dset, NEXTCDF4_VARDIMIDS_ATT, H5P_DEFAULT);
            } H5E_END_TRY;
            if (att >= 0) {
                H5Aclose(att);
                att = -1;
                has_vardimids = 1;
            }
            if ((space = H5Dget_space(dset)) >= 0) {
                ndims = H5Sget_simple_extent_ndims(space);
                H5Sclose(space);
            }
            if (ndims > 0) {
                has_scales = H5DSget_num_scales(dset, 0);
                if (has_scales < 0)
                    has_scales = 0;
            }
            if (!has_vardimids && ndims > 0 && !has_scales) {
                H5Dclose(dset);
                return 0;
            }
        }
        varid = -1;
    }

    H5Dclose(dset);

    if (vl->count >= vl->capacity) {
        vl->capacity = vl->capacity ? vl->capacity * 2 : 4;
        vl->names = realloc(vl->names, vl->capacity * sizeof(char *));
        vl->varids = realloc(vl->varids, vl->capacity * sizeof(int));
    }
    vl->names[vl->count] = strdup(name);
    vl->varids[vl->count] = varid;
    vl->count++;
    return 0;
}

/**
 * @brief Find a dimension by name in a group or any ancestor.
 *
 * Dimension scope in netCDF flows downward — a variable may use any
 * ancestor's dimensions, never a sibling's.
 *
 * @param grp Group.
 * @param name Name.
 * @return Pointer to the dimension, or NULL if not found.
 */
static NC_DIM_INFO_T *
find_dim_by_name(NC_GRP_INFO_T *grp, const char *name)
{
    NC_GRP_INFO_T *g;
    size_t i;
    for (g = grp; g; g = g->parent) {
        if (g->dim) {
            for (i = 0; i < ncindexsize(g->dim); i++) {
                NC_DIM_INFO_T *dim = (NC_DIM_INFO_T *)ncindexith(g->dim, i);
                if (dim && !strcmp(dim->hdr.name, name))
                    return dim;
            }
        }
    }
    return NULL;
}

/**
 * @brief Context for scale_resolve_visitor(): group to search,
 * result dimid slot, and a done flag.
 */
typedef struct {
    NC_GRP_INFO_T *grp;
    int *dimid;
    int found;
} resolve_scale_t;

/**
 * @brief `H5DSiterate_scales` visitor resolving one attached scale to
 * a dimid.
 *
 * Prefers the scale's `_Netcdf4Dimid` attribute; falls back to
 * matching the scale's name against the group's dimensions for files
 * written without the attribute. Only the first attached scale per
 * dimension is used.
 *
 * @param did Dimension scale dataset being visited (unused).
 * @param dim Dimension index being visited (unused).
 * @param dsid Variable dataset the scale is attached to.
 * @param op_data resolve_scale_t search context.
 * @return 0 always (iteration continues or stops via @p found).
 */
static int
scale_resolve_visitor(hid_t did, unsigned int dim, hid_t dsid, void *op_data)
{
    resolve_scale_t *r = op_data;
    NC_DIM_INFO_T *diminfo = NULL;
    int dimid = -1;
    char name[NC_MAX_NAME + 1];
    hid_t att = -1;
    (void)did;
    (void)dim;

    if (r->found)
        return 0;

    H5E_BEGIN_TRY {
        att = H5Aopen(dsid, NEXTCDF4_DIMID_ATT, H5P_DEFAULT);
    } H5E_END_TRY;
    if (att >= 0) {
        H5Aread(att, H5T_NATIVE_INT, &dimid);
        H5Aclose(att);
    }
    if (dimid >= 0)
        nc4_find_dim(r->grp, dimid, &diminfo, NULL);
    if (!diminfo) {
        name[0] = '\0';
        H5DSget_scale_name(dsid, name, sizeof(name));
        if (name[0])
            diminfo = find_dim_by_name(r->grp, name);
    }
    if (diminfo) {
        *r->dimid = diminfo->hdr.id;
        r->found = 1;
    }
    return 0;
}

/**
 * @brief Resolve a variable's dimids from its attached dimension
 * scales.
 *
 * Fallback for variables lacking the `_Netcdf4Coordinates` attribute:
 * each dimension must have at least one scale attached, resolved by
 * scale_resolve_visitor().
 *
 * @param grp Group.
 * @param dset Variable dataset.
 * @param ndims Number of dimensions.
 * @param dimids Destination for the resolved dimension IDs.
 * @return `NC_NOERR` on success, `NC_EFILEMETA`/`NC_EBADDIM` when the
 * scales cannot be resolved.
 */
static int
resolve_var_dimids(NC_GRP_INFO_T *grp, hid_t dset, int ndims, int *dimids)
{
    int d;
    int ret = NC_NOERR;

    for (d = 0; d < ndims; d++) {
        int num_scales;
        resolve_scale_t r = {grp, &dimids[d], 0};

        dimids[d] = -1;
        num_scales = H5DSget_num_scales(dset, (unsigned)d);
        if (num_scales < 0)
            num_scales = 0;
        if (num_scales == 0) {
            ret = NC_EFILEMETA;
            break;
        }
        if (H5DSiterate_scales(dset, (unsigned)d, NULL, scale_resolve_visitor, &r) < 0) {
            ret = NC_EHDFERR;
            break;
        }
        if (!r.found) {
            ret = NC_EBADDIM;
            break;
        }
    }
    return ret;
}

/**
 * @brief Load one variable dataset into the in-memory tree.
 *
 * Rebuilds the full `NC_VAR_INFO_T`: type via map_nc_type(), rank and
 * dims from the dataspace, dimids from `_Netcdf4Coordinates` (or
 * attached scales), then — from the dataset creation property list —
 * chunking/storage, fill behavior and value, the filter pipeline
 * (deflate/shuffle/fletcher32), and endianness from the datatype's
 * byte order. `meta_read`/`atts_read`/`created` are set so netcdf-c
 * treats the variable as fully loaded, not new.
 *
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @param name Name.
 * @param varid Variable identifier (currently unused; ids come from
 * load order).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_one_var(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
             NC_GRP_INFO_T *grp, const char *name, int varid)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    hid_t dset = -1;
    hid_t ftype = -1;
    hid_t space = -1;
    hid_t att = -1;
    hid_t aspace = -1;
    NC_VAR_INFO_T *var = NULL;
    NEXTCDF4_VAR_INFO_T *vinfo = NULL;
    hsize_t dims[NC_MAX_VAR_DIMS];
    hsize_t maxdims[NC_MAX_VAR_DIMS];
    nc_type xtype;
    int ndims;
    int dimids[NC_MAX_VAR_DIMS];
    hsize_t npoints;
    int i;
    int ret;
    (void)h5;

    if (!(vinfo = calloc(1, sizeof(*vinfo))))
        return NC_ENOMEM;

    if ((dset = H5Dopen2(hdf_grp, name, H5P_DEFAULT)) < 0) {
        free(vinfo);
        return NC_EHDFERR;
    }

    if ((ftype = H5Dget_type(dset)) < 0 ||
        (space = H5Dget_space(dset)) < 0) {
        ret = NC_EHDFERR;
        goto done;
    }
    if (map_nc_type(ftype, &xtype)) {
        ret = NC_EBADTYPE;
        goto done;
    }
    if ((ndims = H5Sget_simple_extent_ndims(space)) < 0) {
        ret = NC_EHDFERR;
        goto done;
    }
    if (ndims > 0) {
        if (H5Sget_simple_extent_dims(space, dims, maxdims) < 0) {
            ret = NC_EHDFERR;
            goto done;
        }
        H5E_BEGIN_TRY {
            att = H5Aopen(dset, NEXTCDF4_VARDIMIDS_ATT, H5P_DEFAULT);
        } H5E_END_TRY;
        if (att >= 0) {
            if ((aspace = H5Aget_space(att)) < 0) {
                ret = NC_EHDFERR;
                goto done;
            }
            if ((npoints = H5Sget_simple_extent_npoints(aspace)) < 0 ||
                npoints != (hsize_t)ndims) {
                ret = NC_EFILEMETA;
                goto done;
            }
            if (H5Aread(att, H5T_NATIVE_INT, dimids) < 0) {
                ret = NC_EHDFERR;
                goto done;
            }
            H5Aclose(att);
            att = -1;
            H5Sclose(aspace);
            aspace = -1;
        } else if ((ret = resolve_var_dimids(grp, dset, ndims, dimids))) {
            /* Reached when there is no _Netcdf4Coordinates attribute:
             * resolve the attached HDF5 dimension scales instead. */
            goto done;
        }
    }

    if ((ret = nc4_var_list_add(grp, name, ndims, &var))) {
        free(vinfo);
        goto done;
    }
    if ((ret = nc4_var_set_ndims(var, ndims))) {
        free(vinfo);
        goto done;
    }
    if ((ret = set_var_type(var, xtype))) {
        free(vinfo);
        goto done;
    }
    /* The filter list exists from birth, as upstream does in
     * hdf5open.c, so NC4_hdf5_addfilter can snapshot it. */
    if (!(var->filters = (void *)nclistnew())) {
        free(vinfo);
        ret = NC_ENOMEM;
        goto done;
    }
    var->format_var_info = vinfo;
    (void)varid;
    vinfo->hdf_dataset = dset;
    dset = -1; /* ownership transferred */

    for (i = 0; i < ndims; i++) {
        NC_DIM_INFO_T *dim = NULL;
        if ((ret = nc4_find_dim(grp, dimids[i], &dim, NULL))) {
            free(vinfo);
            goto done;
        }
        var->dimids[i] = dimids[i];
        var->dim[i] = dim;
    }
    /* Mark the variable as fully loaded and committed so netcdf-c does
     * not try to re-read its metadata or treat it as a new variable. */
    var->meta_read = 1;
    var->atts_read = 1;
    var->created = 1;

    /* Read the dataset creation properties. */
    {
        hid_t dcpl;
        int j;
        H5T_order_t order;
        H5D_fill_time_t fill_time;

        if ((dcpl = H5Dget_create_plist(vinfo->hdf_dataset)) >= 0) {
            hsize_t chunks[NC_MAX_VAR_DIMS];
            int chunk_ndims = H5Pget_chunk(dcpl, ndims, chunks);
            if (chunk_ndims > 0) {
                var->storage = NC_CHUNKED;
                if (!var->chunksizes)
                    var->chunksizes = calloc(ndims, sizeof(size_t));
                if (var->chunksizes)
                    for (j = 0; j < chunk_ndims; j++)
                        var->chunksizes[j] = (size_t)chunks[j];
            } else if (ndims > 0) {
                var->storage = NC_CONTIGUOUS;
            }

            if (H5Pget_fill_time(dcpl, &fill_time) >= 0 &&
                fill_time == H5D_FILL_TIME_NEVER)
                var->no_fill = 1;

            if (!var->no_fill) {
                size_t fsize;
                if (var->fill_value) {
                    free(var->fill_value);
                    var->fill_value = NULL;
                }
                if (!NEXTCDF4_type_size(xtype, &fsize) &&
                    (var->fill_value = malloc(fsize))) {
                    hid_t mem_type = H5Tget_native_type(ftype, H5T_DIR_DEFAULT);
                    int rv;
                    if (mem_type < 0) {
                        free(var->fill_value);
                        var->fill_value = NULL;
                    } else {
                        rv = H5Pget_fill_value(dcpl, mem_type, var->fill_value);
                        if (rv < 0) {
                            free(var->fill_value);
                            var->fill_value = NULL;
                        }
                        H5Tclose(mem_type);
                    }
                }
            }

            if ((ret = nx4_load_filters(dcpl, var, vinfo)))
                goto done;
            H5Pclose(dcpl);
        }

        order = H5Tget_order(ftype);
        if (order == H5T_ORDER_BE)
            var->endianness = NC_ENDIAN_BIG;
        else if (order == H5T_ORDER_LE)
            var->endianness = NC_ENDIAN_LITTLE;
        else
            var->endianness = NC_ENDIAN_NATIVE;
    }

done:
    if (dset >= 0)
        H5Dclose(dset);
    if (ftype >= 0)
        H5Tclose(ftype);
    if (space >= 0)
        H5Sclose(space);
    if (att >= 0)
        H5Aclose(att);
    if (aspace >= 0)
        H5Sclose(aspace);
    return ret;
}

/**
 * @brief Context for load_att_cb(): the attribute index to fill, the
 * owning object, and the HDF5 location to iterate.
 */
typedef struct {
    NCindex *list;
    NC_OBJ *container;
    hid_t loc;
} att_iter_data_t;

/**
 * @brief `H5Aiterate2` callback loading a single user attribute.
 *
 * Reserved (internal) attributes and attributes whose datatype
 * doesn't map to an atomic nc_type are skipped — neither should make
 * an otherwise-valid file unopenable.
 *
 * @param loc HDF5 location identifier.
 * @param attr_name HDF5 attribute name.
 * @param ainfo Attribute info (unused).
 * @param op_data att_iter_data_t iteration context.
 * @return 0 always; failures are swallowed as skips.
 */
static herr_t
load_att_cb(hid_t loc, const char *attr_name, const H5A_info_t *ainfo,
            void *op_data)
{
    att_iter_data_t *ad = op_data;
    (void)ainfo;
    if (is_reserved_att(attr_name))
        return 0;
    if (read_hdf5_att(loc, attr_name, ad->list, ad->container))
        return 0; /* skip attributes that cannot be mapped */
    return 0;
}

/**
 * @brief Load all of a group's variables and their attributes.
 *
 * Collects candidates with find_var_cb(), then loads in stored-varid
 * order when `_Netcdf4Varid` attributes exist (preserving the ids
 * assigned at create), else in HDF5 iteration order. Each variable's
 * user attributes are iterated with `H5Aiterate2`.
 *
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_variables(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
               NC_GRP_INFO_T *grp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    var_list_t vl = {0};
    att_iter_data_t ad;
    int loaded = 0;
    int i;
    int ret;
    (void)h5;

    int maxid = -1;
    int id;

    if (H5Literate(hdf_grp, H5_INDEX_NAME, H5_ITER_INC, NULL,
                   find_var_cb, &vl) < 0)
        return NC_EHDFERR;

    for (i = 0; i < vl.count; i++)
        if (vl.varids[i] > maxid)
            maxid = vl.varids[i];

    if (maxid < 0) {
        /* No stored varids: load in HDF5 iteration order. */
        for (i = 0; i < vl.count; i++) {
            if ((ret = load_one_var(file, h5, grp, vl.names[i], vl.varids[i]))) {
                free_name_list(vl.names, vl.count);
                free(vl.varids);
                return ret;
            }
            ad.loc = ((NEXTCDF4_VAR_INFO_T *)
                      ((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))
                      ->format_var_info)->hdf_dataset;
            ad.list = ((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))->att;
            ad.container = &((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))->hdr;
            H5Aiterate2(ad.loc, H5_INDEX_NAME, H5_ITER_INC, NULL,
                        load_att_cb, &ad);
            loaded++;
        }
    } else {
        /* Stored varids present: load in id order so nc4_var_list_add
         * assigns the same ids. */
        for (id = 0; id <= maxid; id++) {
            for (i = 0; i < vl.count; i++) {
                if (vl.varids[i] == id) {
                    if ((ret = load_one_var(file, h5, grp, vl.names[i], vl.varids[i]))) {
                        free_name_list(vl.names, vl.count);
                        free(vl.varids);
                        return ret;
                    }
                    ad.loc = ((NEXTCDF4_VAR_INFO_T *)
                              ((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))
                              ->format_var_info)->hdf_dataset;
                    ad.list = ((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))->att;
                    ad.container = &((NC_VAR_INFO_T *)ncindexith(grp->vars, loaded))->hdr;
                    H5Aiterate2(ad.loc, H5_INDEX_NAME, H5_ITER_INC, NULL,
                                load_att_cb, &ad);
                    loaded++;
                    break;
                }
            }
        }
    }

    if (loaded != vl.count) {
        free_name_list(vl.names, vl.count);
        free(vl.varids);
        return NC_EFILEMETA;
    }

    free_name_list(vl.names, vl.count);
    free(vl.varids);
    return NC_NOERR;
}

/**
 * @brief Load a group's own (non-global) attributes.
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_group_attributes(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
                      NC_GRP_INFO_T *grp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    att_iter_data_t ad;
    (void)file;
    (void)h5;
    ad.loc = hdf_grp;
    ad.list = grp->att;
    ad.container = &grp->hdr;
    if (H5Aiterate2(hdf_grp, H5_INDEX_NAME, H5_ITER_INC, NULL,
                    load_att_cb, &ad) < 0)
        return NC_EHDFERR;
    grp->atts_read = 1;
    return NC_NOERR;
}

/**
 * @brief Context carried to load_children_cb(): file state, parent
 * group record, and parent HDF5 handle.
 */
typedef struct {
    NEXTCDF4_FILE_INFO_T *file;
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *parent;
    hid_t parent_hdf;
} load_children_data_t;

static int load_children(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
                         hid_t parent_hdf, NC_GRP_INFO_T *parent);
static int load_one_type(NC_GRP_INFO_T *grp, hid_t htype, const char *name);

/**
 * @brief Load one committed HDF5 datatype into the in-memory type
 * list.
 *
 * Committed datatypes are the on-disk form of user-defined types.
 * The HDF5 class sets the `nc_type_class`; enum and vlen base types
 * are recovered from the datatype's super-type. The `hid_t` is kept
 * open in `NEXTCDF4_TYPE_INFO_T`.
 *
 * @param grp Group.
 * @param htype HDF5 committed datatype (ownership transferred on
 * success).
 * @param name Name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_one_type(NC_GRP_INFO_T *grp, hid_t htype, const char *name)
{
    NC_TYPE_INFO_T *type = NULL;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    H5T_class_t cls;
    size_t size = 0;
    hid_t super = -1;
    nc_type xtype;
    int ret;

    cls = H5Tget_class(htype);
    size = H5Tget_size(htype);

    if (cls == H5T_STRING && H5Tis_variable_str(htype))
        size = sizeof(char *);

    if ((ret = nc4_type_list_add(grp, size, name, &type))) {
        H5Tclose(htype);
        return ret;
    }

    if (!(tinfo = calloc(1, sizeof(*tinfo)))) {
        H5Tclose(htype);
        return NC_ENOMEM;
    }
    tinfo->hdf_type = htype;
    type->format_type_info = tinfo;

    switch (cls) {
    case H5T_COMPOUND:
        type->nc_type_class = NC_COMPOUND;
        type->u.c.field = nclistnew();
        break;
    case H5T_ENUM:
        type->nc_type_class = NC_ENUM;
        type->u.e.enum_member = nclistnew();
        if ((super = H5Tget_super(htype)) < 0)
            return NC_EHDFERR;
        if (map_nc_type(super, &xtype) == NC_NOERR)
            type->u.e.base_nc_typeid = xtype;
        H5Tclose(super);
        break;
    case H5T_OPAQUE:
        type->nc_type_class = NC_OPAQUE;
        break;
    case H5T_VLEN:
        type->nc_type_class = NC_VLEN;
        if ((super = H5Tget_super(htype)) >= 0) {
            if (map_nc_type(super, &xtype) == NC_NOERR)
                type->u.v.base_nc_typeid = xtype;
            H5Tclose(super);
        }
        break;
    case H5T_STRING:
        type->nc_type_class = NC_STRING;
        break;
    default:
        break;
    }

    return NC_NOERR;
}

/**
 * @brief Load one group's full metadata: dimensions, variables,
 * attributes, then children recursively.
 *
 * Order matters — dimensions must exist before variables resolve
 * their dimids, and children load last. An unmarked root group with
 * nothing loadable is rejected (`NC_ENOTNC`): a plain HDF5 file with
 * no netCDF structure is not a netCDF file.
 *
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @param hdf_grp HDF5 group identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_group_metadata(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
                    NC_GRP_INFO_T *grp, hid_t hdf_grp)
{
    int ret;
    (void)hdf_grp;

    if ((ret = load_dimensions(file, h5, grp)))
        return ret;
    if ((ret = load_variables(file, h5, grp)))
        return ret;
    if ((ret = load_group_attributes(file, h5, grp)))
        return ret;
    if ((ret = load_children(file, h5, hdf_grp, grp)))
        return ret;
    /* Unmarked non-NetCDF-4 HDF5 files must have at least dimensions or
     * variables that follow the NetCDF-4 dimension-scale convention. */
    if (grp == h5->root_grp && !file->backend_marked &&
        ncindexsize(grp->dim) == 0 && ncindexsize(grp->vars) == 0)
        return NC_ENOTNC;
    return NC_NOERR;
}

/**
 * @brief Open one child HDF5 group and load it recursively.
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param parent Parent group.
 * @param parent_hdf Parent HDF5 group identifier.
 * @param name Name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_one_group(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
               NC_GRP_INFO_T *parent, hid_t parent_hdf, const char *name)
{
    NC_GRP_INFO_T *grp = NULL;
    NEXTCDF4_GRP_INFO_T *ginfo = NULL;
    hid_t child_hdf = -1;
    int ret;

    if ((child_hdf = H5Gopen2(parent_hdf, name, H5P_DEFAULT)) < 0)
        return NC_EHDFERR;

    if ((ret = nc4_grp_list_add(h5, parent, (char *)name, &grp)))
        goto fail;

    if (!(ginfo = calloc(1, sizeof(*ginfo)))) {
        ret = NC_ENOMEM;
        goto fail;
    }
    ginfo->hdf_group = child_hdf;
    grp->format_grp_info = ginfo;

    if ((ret = load_group_metadata(file, h5, grp, child_hdf)))
        goto fail;

    return NC_NOERR;

fail:
    if (ginfo)
        ginfo->hdf_group = -1;
    if (child_hdf >= 0)
        H5Gclose(child_hdf);
    return ret;
}

/**
 * @brief `H5Literate` callback dispatching each child link to the
 * right loader.
 *
 * Links that open as groups become subgroups (load_one_group); links
 * that open as committed datatypes become user-defined types
 * (load_one_type); datasets were already handled by the variable
 * pass. Anything else is skipped.
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param info Link info (unused).
 * @param op_data load_children_data_t iteration context.
 * @return 0 to continue iteration, -1 on load failure.
 */
static herr_t
load_children_cb(hid_t loc, const char *name, const H5L_info_t *info, void *op_data)
{
    load_children_data_t *ld = op_data;
    hid_t child_g = -1;
    hid_t child_t = -1;
    (void)info;

    H5E_BEGIN_TRY {
        child_g = H5Gopen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (child_g >= 0) {
        if (load_one_group(ld->file, ld->h5, ld->parent, loc, name)) {
            H5Gclose(child_g);
            return -1;
        }
        return 0;
    }

    H5E_BEGIN_TRY {
        child_t = H5Topen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (child_t >= 0) {
        if (load_one_type(ld->parent, child_t, name)) {
            H5Tclose(child_t);
            return -1;
        }
        return 0;
    }

    return 0;
}

/**
 * @brief Iterate a group's links loading child groups and committed
 * types.
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param parent_hdf Parent HDF5 group identifier.
 * @param parent Parent group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_children(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
              hid_t parent_hdf, NC_GRP_INFO_T *parent)
{
    load_children_data_t ld = {file, h5, parent, parent_hdf};

    if (H5Literate(parent_hdf, H5_INDEX_NAME, H5_ITER_INC, NULL,
                   load_children_cb, &ld) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * @brief Load all metadata for an open NEXTCDF-4 file — the entry
 * point NEXTCDF4_open() calls after checking the marker attributes.
 *
 * Recursively walks the HDF5 file starting at the root group and
 * rebuilds the complete `NC_*_INFO_T` tree (groups, dimensions,
 * variables, attributes, committed types) that the dispatch entries
 * then operate on. See @ref nextcdf4_architecture.
 *
 * @param file Open NEXTCDF-4 state.
 * @param h5 Common NetCDF-4 file metadata.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_load_metadata(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5)
{
    ELOG((2, "%s", __func__));

    NEXTCDF4_GRP_INFO_T *ginfo;
    hid_t hdf_grp;

    ginfo = h5->root_grp ? h5->root_grp->format_grp_info : NULL;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;

    return load_group_metadata(file, h5, h5->root_grp, hdf_grp);
}

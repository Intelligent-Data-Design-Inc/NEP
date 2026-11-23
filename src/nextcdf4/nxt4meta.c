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
static int nx4_set_att_order(hid_t ocpl);
static int nx4_dcpl_fill(hid_t dcpl, NC_FILE_INFO_T *h5,
                         NC_VAR_INFO_T *var, hid_t hdf_type);
static int nx4_rewrite_atts(hid_t loc, NCindex *list, NC_GRP_INFO_T *grp);
static int nx4_reattach_dim(NC_GRP_INFO_T *grp, NC_DIM_INFO_T *dim,
                            hid_t old_dset, hid_t new_dset,
                            NC_VAR_INFO_T *skip);

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
 * read-only reject them outright. A metadata call in data mode
 * silently re-enters define mode, as upstream `NC4_redef` does;
 * classic-model files keep the strict `NC_ENOTINDEFINE` rule.
 *
 * @param h5 NetCDF-4 file state (may be NULL).
 * @param file NEXTCDF-4 file state.
 * @return `NC_NOERR`, `NC_EPERM` (read-only file), or
 * `NC_ENOTINDEFINE`.
 */
int
NEXTCDF4_check_write_define(NC_FILE_INFO_T *h5, NEXTCDF4_FILE_INFO_T *file)
{
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode) {
        /* Metadata calls in data mode silently re-enter define mode,
         * as upstream does in NC4_redef; classic-model files forbid
         * the implicit switch. */
        if (file->mode & NC_CLASSIC_MODEL)
            return NC_ENOTINDEFINE;
        file->define_mode = 1;
        if (h5)
            h5->flags |= NC_INDEF;
    }
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
        is_ok = !(file->mode & NC_CLASSIC_MODEL);
        break;
    case NC_STRING:
        return (file->mode & NC_CLASSIC_MODEL) ? NC_EBADTYPE : NC_NOERR;
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
            ? NC_NOERR
            : NC_ENOTNC4;
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
 * @param var Variable, or NULL for a group (global) attribute.
 * @param listp Destination for the attribute index.
 * @param locp Destination for the HDF5 location identifier — a
 * negative value when the variable's dataset has not been created
 * yet (user-defined types defer it to `_enddef`), in which case the
 * caller must skip HDF5 writes and mark the attributes pending.
 * @return `NC_NOERR` on success.
 */
static int
get_att_context(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var, NCindex **listp,
                hid_t *locp)
{
    if (var) {
        NEXTCDF4_VAR_INFO_T *vinfo = var->format_var_info;
        *locp = (vinfo) ? vinfo->hdf_dataset : -1;
        *listp = var->att;
    } else {
        NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
        NEXTCDF4_FILE_INFO_T *file = grp->nc4_info->format_file_info;
        *locp = (ginfo && ginfo->hdf_group >= 0) ? ginfo->hdf_group
                                               : file->rootid;
        *listp = grp->att;
    }
    return NC_NOERR;
}

/**
 * @brief Delete an attribute from an index and free it.
 *
 * `nc4_att_list_del()` deletes by `att->hdr.id`, but ids drift out of
 * sync with list positions after earlier deletes: mid-list removal
 * does not rehash the name map or renumber the survivors, so a later
 * delete can drop the wrong attribute and `ncindexlookup()` returns
 * stale positions. Find the real slot, delete it, renumber, rebuild
 * the name map, then free the record.
 *
 * @param list Attribute index.
 * @param att Attribute to delete.
 * @return `NC_NOERR` on success, `NC_ENOTATT` when absent.
 */
static int
nx4_att_list_del(NCindex *list, NC_ATT_INFO_T *att)
{
    size_t i;

    if (!list || !att)
        return NC_ENOTATT;
    for (i = 0; i < ncindexsize(list); i++)
        if (ncindexith(list, i) == (NC_OBJ *)att)
            break;
    if (i == ncindexsize(list))
        return NC_ENOTATT;
    ncindexidel(list, i);
    for (i = 0; i < ncindexsize(list); i++) {
        NC_ATT_INFO_T *a = (NC_ATT_INFO_T *)ncindexith(list, i);
        if (a)
            a->hdr.id = (int)i;
    }
    ncindexrebuild(list);
    if (att->nc_typeid > NC_MAX_ATOMIC_TYPE &&
        att->nc_typeid < NC_FIRSTUSERTYPEID) {
        /* nc4_att_free() reclaims through NC_reclaim_data(), which
         * cannot resolve the NEP extended atomic ids — free the flat
         * payload here so its error path drops the record only. */
        free(att->data);
        att->data = NULL;
        att->len = 0;
        att->nc_typeid = NC_BYTE;
    }
    nc4_att_free(att);
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
 * @brief Report whether a type is (transitively) variable-sized.
 *
 * Dynamic types — vlens and compounds containing them — carry
 * pointers inside each element, so values must be deep-copied and
 * reclaimed rather than `memcpy`'d and `free`'d.
 *
 * @param grp Group used to resolve user-defined types.
 * @param xtype NetCDF atomic or user-defined type.
 * @param vsp Destination for the variable-sized flag.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_type_varsized(NC_GRP_INFO_T *grp, nc_type xtype, int *vsp)
{
    NC_TYPE_INFO_T *t = NULL;

    *vsp = 0;
    if (xtype == NC_STRING) {
        *vsp = 1;
        return NC_NOERR;
    }
    if (xtype < NC_FIRSTUSERTYPEID)
        return NC_NOERR;
    if (nx4_find_type(grp, xtype, &t) || !t)
        return NC_EBADTYPE;
    *vsp = t->varsized;
    return NC_NOERR;
}

/**
 * @brief Allocate and add a new attribute to the in-memory model.
 *
 * Deep-copies the attribute value: numeric data by `memcpy`,
 * `NC_STRING` values element-by-element (NULL elements become empty
 * strings), and variable-sized user types (vlens and compounds
 * containing them) recursively through netcdf-c's shared
 * `NC_copy_data()` — so the attribute record owns its data outright
 * and the caller may mutate or free the input immediately.
 *
 * @param list Attribute index to add to.
 * @param container Object that owns the attribute (group or var `hdr`).
 * @param grp Group used to resolve user-defined type sizes.
 * @param name Name.
 * @param xtype NetCDF atomic or user-defined type.
 * @param len Number of elements.
 * @param data Data buffer.
 * @param attp Destination for the new attribute.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
add_att(NCindex *list, NC_OBJ *container, NC_GRP_INFO_T *grp,
        const char *name, nc_type xtype, size_t len, const void *data,
        NC_ATT_INFO_T **attp)
{
    NC_ATT_INFO_T *att = NULL;
    size_t size;
    void *copy = NULL;
    int varsized = 0;
    int ret;
    size_t i;

    if ((ret = nc4_att_list_add(list, name, &att)))
        return ret;
    if ((ret = nx4_type_varsized(grp, xtype, &varsized))) {
        nc4_att_list_del(list, att);
        return ret;
    }
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
            if ((ret = nx4_type_size(grp, xtype, &size))) {
                nc4_att_list_del(list, att);
                return ret;
            }
            if (len > SIZE_MAX / size) {
                nc4_att_list_del(list, att);
                return NC_EINVAL;
            }
            if (!(copy = malloc(len * size))) {
                nc4_att_list_del(list, att);
                return NC_ENOMEM;
            }
            if (varsized) {
                /* Dynamic payloads are deep-copied element by
                 * element; the top-level allocation stays owned here
                 * either way. On failure the partially copied value
                 * is reclaimed before the record is dropped. */
                if ((ret = NC_copy_data(grp->nc4_info->controller,
                                        xtype, data, len, copy))) {
                    NC_reclaim_data(grp->nc4_info->controller, xtype,
                                    copy, len);
                    free(copy);
                    nc4_att_list_del(list, att);
                    return ret;
                }
            } else {
                memcpy(copy, data, len * size);
            }
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
 * @param grp Group used to resolve committed user-defined datatypes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
read_hdf5_att(hid_t loc, const char *name, NCindex *list, NC_OBJ *container,
              NC_GRP_INFO_T *grp)
{
    hid_t attr = -1;
    hid_t ftype = -1;
    hid_t fspace = -1;
    hid_t ntype = -1;
    nc_type xtype = NC_NAT;
    int close_ntype = 1;
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
    if (map_nc_type(ftype, &xtype)) {
        /* Not an atomic type: a committed user-defined datatype
         * resolves to its registered type record by object
         * identity. */
        NC_TYPE_INFO_T *utype = NULL;
        if (nx4_hdf_type_match(grp, ftype, &utype) || !utype)
            goto done;
        xtype = (nc_type)utype->hdr.id;
    }
    if (nx4_type_size(grp, xtype, &size))
        goto done;
    len = (size_t)npoints;
    if (len > 0) {
        if (len > SIZE_MAX / size) { ret = NC_EINVAL; goto done; }
        if (!(data = malloc(len * size)))
            goto done;
        if (xtype == NC_CHAR) {
            if ((ntype = H5Tcopy(H5T_C_S1)) < 0)
                goto done;
            H5Tset_size(ntype, 1);
            H5Tset_strpad(ntype, H5T_STR_NULLPAD);
        } else if (xtype >= NC_FIRSTUSERTYPEID && H5Tcommitted(ftype) <= 0) {
            /* Anonymous compound datatype: a foreign (upstream)
             * file — its serialized-compound layout decodes only
             * through the packed native datatype. */
            NC_TYPE_INFO_T *ut = NULL;
            if (!nx4_find_type(grp, xtype, &ut) && ut &&
                ut->nc_type_class == NC_COMPOUND) {
                if ((ntype = H5Tget_native_type(ftype,
                                                H5T_DIR_DEFAULT)) < 0)
                    goto done;
            } else if (nx4_memory_type(grp, xtype, &ntype)) {
                goto done;
            }
        } else if (nx4_memory_type(grp, xtype, &ntype)) {
            goto done;
        }
        if (H5Aread(attr, ntype, data) < 0)
            goto done;
    }
    if ((ret = add_att(list, container, grp, name, xtype, len, data, NULL)))
        goto done;
    ret = NC_NOERR;

done:
    if (data) {
        /* Variable-sized types (vlens and compounds containing
         * them): add_att() deep-copied the value, so the
         * HDF5-allocated payloads in the read buffer are reclaimed
         * before the top-level buffer is freed. */
        int vs = 0;
        if (!nx4_type_varsized(grp, xtype, &vs) && vs)
            NC_reclaim_data(grp->nc4_info->controller, xtype, data, len);
        free(data);
    }
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
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_check_dup_name(grp, (char *)name)))
        return ret;
    if (ncindexlookup(grp->dim, (char *)name))
        return NC_ENAMEINUSE;

    /* Classic model: at most one unlimited dimension. */
    if ((file->mode & NC_CLASSIC_MODEL) && len == 0) {
        size_t i;
        for (i = 0; i < ncindexsize(grp->dim); i++) {
            NC_DIM_INFO_T *d = (NC_DIM_INFO_T *)ncindexith(grp->dim, i);
            if (d && d->unlimited)
                return NC_EUNLIMIT;
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
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto fail;
    if (nx4_set_att_order(dcpl))
        goto fail;
    if (len == 0) {
        if ((space = H5Screate_simple(1, &cur, &max)) < 0)
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
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (xtype >= NC_FIRSTUSERTYPEID) {
        /* User-defined types are an enhanced-model feature; both
         * native and NC_NETCDF4_MODEL files accept them (the model
         * mode only restricts which *atomic* types may appear). */
        if (file->mode & NC_CLASSIC_MODEL)
            return NC_ENOTNC4;
    } else if ((ret = NEXTCDF4_check_atomic_type(file, xtype)))
        return ret;
    if (ndims < 0 || ndims > NC_MAX_VAR_DIMS)
        return NC_EINVAL;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_check_dup_name(grp, (char *)name)))
        return ret;
    /* For non-scalar vars, dim IDs must be provided. */
    if (ndims && !dimidsp)
        return NC_EINVAL;

    ginfo = grp->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;

    if (!(vinfo = calloc(1, sizeof(*vinfo))))
        return NC_ENOMEM;

    if ((ret = nc4_var_list_add(grp, name, ndims, &var)))
        goto fail;
    if (xtype >= NC_FIRSTUSERTYPEID) {
        /* The variable shares the group's type record — the
         * reference count balances the group's own ownership with
         * each variable using it (nc4_var_free decrements). */
        NC_TYPE_INFO_T *utype = NULL;
        if (nx4_find_type(grp, xtype, &utype) || !utype) {
            ret = NC_EBADTYPE;
            goto fail;
        }
        var->type_info = utype;
        utype->rc++;
    } else if ((ret = set_var_type(var, xtype)))
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
            { ret = NC_EINVAL; goto fail; }
        if (n_unlim == 1 && ndims > 0 && !var->dim[0]->unlimited)
            { ret = NC_EINVAL; goto fail; }
    }

    /* -1 marks "storage not specified": the dataset is created
     * chunked when it has an unlimited dimension and contiguous
     * otherwise, and a later def-var setter must not change that
     * layout (#382). */
    var->storage = has_unlimited ? NC_CHUNKED : -1;

    if (xtype >= NC_FIRSTUSERTYPEID) {
        /* Committed datatypes exist only once `_enddef` commits the
         * group's user types, so a user-typed variable's dataset is
         * deferred to nx4_create_var_dataset() then — including the
         * coordinate-variable scale swap, dimension-scale attachment
         * and hidden id attributes. Inquiries and def-var setters
         * work on the in-memory record meanwhile. */
        vinfo->hdf_dataset = -1;
        if (varidp)
            *varidp = var->hdr.id;
        return NC_NOERR;
    }

    coordinate = ndims == 1 && !strcmp(name, var->dim[0]->hdr.name) &&
                 var->dim[0]->container == grp;
    if ((ret = NEXTCDF4_map_hdf_type(xtype, &hdf_type)))
        goto fail;

    if (ndims == 0) {
        if ((space = H5Screate(H5S_SCALAR)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0 ||
            nx4_set_att_order(dcpl)) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if ((ret = nx4_dcpl_fill(dcpl, h5, var, hdf_type)))
            goto fail;
    } else {
        if ((space = H5Screate_simple(ndims, dims, maxdims)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0 ||
            nx4_set_att_order(dcpl)) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if (has_unlimited && H5Pset_chunk(dcpl, ndims, chunks) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if ((ret = nx4_dcpl_fill(dcpl, h5, var, hdf_type)))
            goto fail;
    }

    if (coordinate) {
        NEXTCDF4_DIM_INFO_T *dinfo = var->dim[0]->format_dim_info;
        if (!dinfo || dinfo->hdf_dataset < 0) { ret = NC_EFILEMETA; goto fail; }
        /* Variables defined before this coordinate variable attached
         * the placeholder scale; detach them before it is deleted so
         * their DIMENSION_LIST entries do not point at a dead object,
         * then reattach them to the variable's new scale below. */
        if ((ret = nx4_reattach_dim(grp, var->dim[0], dinfo->hdf_dataset,
                                  -1, var)))
            goto fail;
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
        if ((ret = nx4_reattach_dim(grp, var->dim[0], -1,
                                  dinfo->hdf_dataset, var)))
            goto fail;
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
    if (var) {
        /* var_free() does not know about the HDF5 filter list —
         * release it before the record is deleted. */
        NC4_hdf5_filter_freelist(var);
        nc4_var_list_del(grp, var);
    }
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
 * @brief Enable HDF5 attribute creation-order tracking on an object
 * creation property list.
 *
 * Upstream netcdf-4 tracks attribute creation order on every group
 * and dataset so `nc_inq_attname()` returns names in creation order
 * after reopen; NEXTCDF-4 does the same and iterates with
 * `H5_INDEX_CRT_ORDER` at load.
 *
 * @param ocpl Object creation property list.
 * @return `NC_NOERR` on success, `NC_EHDFERR` on failure.
 */
static int
nx4_set_att_order(hid_t ocpl)
{
    return (H5Pset_attr_creation_order(ocpl, H5P_CRT_ORDER_TRACKED |
                                      H5P_CRT_ORDER_INDEXED) < 0)
           ? NC_EHDFERR : NC_NOERR;
}

/**
 * @brief Write the variable's fill setting to a dataset creation
 * property list.
 *
 * Mirrors upstream: `no_fill` disables fill entirely; otherwise the
 * user's fill value or the type default is written, so unwritten
 * elements read back `NC_FILL_*` instead of HDF5's zero default.
 *
 * @param dcpl Dataset creation property list.
 * @param h5 NetCDF-4 file state.
 * @param var Variable.
 * @param hdf_type Dataset HDF5 datatype.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_dcpl_fill(hid_t dcpl, NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var,
              hid_t hdf_type)
{
    void *fillp = NULL;
    hid_t fill_type = -1;
    int ret = NC_NOERR;

    if (var->no_fill) {
        if (H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER) < 0)
            return NC_EHDFERR;
        return NC_NOERR;
    }
    /* A fill message on an H5T_STD_REF dataset makes the dataset
     * unreadable to H5Dopen2 — reference datasets get no fill. */
    if (var->type_info->hdr.id == NC_STD_REF)
        return NC_NOERR;
    if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID &&
        var->type_info->varsized) {
        /* Variable-sized types do have an HDF5 fill representation —
         * the zeroed descriptor (empty vlens, null embedded heaps).
         * Without an explicit fill message H5Dread fails outright on
         * unwritten elements of a vlen dataset instead of returning
         * empty vlens. var->fill_value is a deep copy owned by the
         * variable, so shallow-copying its descriptor into the fill
         * message is safe: H5Pset_fill_value serializes it at create
         * time. */
        if (!(fillp = calloc(1, var->type_info->size)))
            return NC_ENOMEM;
        if (var->fill_value)
            memcpy(fillp, var->fill_value, var->type_info->size);
        if (H5Pset_fill_value(dcpl, hdf_type, fillp) < 0) {
            free(fillp);
            return NC_EHDFERR;
        }
        free(fillp);
        return NC_NOERR;
    }
    if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID) {
        /* User-defined types have no netCDF default fill, matching
         * upstream: only an explicit `nc_def_var_fill`/`_FillValue`
         * value is stored in the DCPL, in the variable's own type.
         * Unwritten elements otherwise read back whatever HDF5's
         * default fill produces. */
        if (!var->fill_value)
            return NC_NOERR;
        if (!(fillp = malloc(var->type_info->size)))
            return NC_ENOMEM;
        memcpy(fillp, var->fill_value, var->type_info->size);
    } else if (var->type_info->hdr.id > NC_STRING) {
        /* NEP-only atomic types have no entry in `h5->alltypes`, so
         * upstream's nc4_get_fill_value() reports NC_EBADTYPID.
         * Default to an all-zero fill (positive zero for the small
         * floats, empty bitfields) or the explicit `_FillValue`. */
        size_t size;
        if ((ret = NEXTCDF4_type_size(var->type_info->hdr.id, &size)))
            return ret;
        if (!(fillp = calloc(1, size)))
            return NC_ENOMEM;
        if (var->fill_value)
            memcpy(fillp, var->fill_value, size);
    } else if ((ret = nc4_get_fill_value(h5, var, &fillp)))
        return ret;
    if (fillp) {
        /* NEP-only atomic types: `fillp` holds raw file-format bytes
         * and `H5Tget_native_type()` widens them to the nearest native
         * type (bfloat16 -> a 4-byte native float), which reads past
         * the buffer (#377) — pass the dataset's own datatype. The
         * same applies to user-defined types: the fill value is
         * already in the file layout. */
        if (var->type_info->hdr.id > NC_STRING)
            fill_type = H5Tcopy(hdf_type);
        else
            fill_type = H5Tget_native_type(hdf_type, H5T_DIR_DEFAULT);
        if (fill_type < 0 ||
            H5Pset_fill_value(dcpl, fill_type, fillp) < 0)
            ret = NC_EHDFERR;
        if (fill_type >= 0)
            H5Tclose(fill_type);
        if (var->type_info->hdr.id == NC_STRING)
            free(*(char **)fillp);
        free(fillp);
    }
    return ret;
}

/**
 * @brief Create or re-create the HDF5 dataset for a variable.
 *
 * Called by `NEXTCDF4_def_var` and from `NEXTCDF4__enddef` for every
 * variable a def-var setter marked `dcpl_dirty`: because HDF5 dataset
 * creation properties are immutable, changing
 * chunking/compression/fill/endian requires deleting the dataset link
 * and recreating it with a new creation property list. Recreation
 * detaches the dimension-scale references that point at the old
 * object, rewrites the hidden id attributes and the variable's scale
 * attachments, re-attaches a coordinate variable's dependents to the
 * new scale, and restores the user attributes the delete dropped.
 * The variable's filter pipeline comes from `var->filters` via
 * `nx4_apply_filters()`.
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
    hid_t old_dset = -1;
    hsize_t dims[NC_MAX_VAR_DIMS];
    hsize_t maxdims[NC_MAX_VAR_DIMS];
    hsize_t chunks[NC_MAX_VAR_DIMS];
    int has_unlimited = 0;
    int coordinate = 0;
    int i, ret;

    coordinate = (var->ndims == 1 &&
                  !strcmp(var->hdr.name, var->dim[0]->hdr.name) &&
                  var->dim[0]->container == grp);

    /* Recreation is delete-and-rebuild. Before the old dataset is
     * deleted, remove the dimension-scale references that point at
     * it: a non-coordinate variable detaches from each of its dims'
     * scales (removing the scale's dangling REFERENCE_LIST entry), and
     * a coordinate variable — whose dataset IS the scale — detaches
     * every dependent variable so its DIMENSION_LIST entry does not
     * outlive the object. First creation of a coordinate variable
     * hits the same path: dependents then detach from the placeholder
     * scale def_dim created. */
    if (coordinate) {
        NEXTCDF4_DIM_INFO_T *dinfo =
            (NEXTCDF4_DIM_INFO_T *)var->dim[0]->format_dim_info;
        if (dinfo)
            old_dset = dinfo->hdf_dataset;
    } else if (vinfo) {
        old_dset = vinfo->hdf_dataset;
    }
    if (old_dset >= 0) {
        if (coordinate) {
            if ((ret = nx4_reattach_dim(grp, var->dim[0], old_dset,
                                      -1, var)))
                return ret;
        } else {
            for (i = 0; i < (int)var->ndims; i++) {
                NEXTCDF4_DIM_INFO_T *dinfo = (NEXTCDF4_DIM_INFO_T *)
                    var->dim[i]->format_dim_info;
                if (dinfo && dinfo->hdf_dataset >= 0)
                    H5E_BEGIN_TRY {
                        H5DSdetach_scale(old_dset, dinfo->hdf_dataset,
                                         (unsigned)i);
                    } H5E_END_TRY;
            }
        }
    }

    /* Drop the old link and dataset, then rebuild dataspace and
     * creation properties from the current settings, recreate, and
     * rewrite the id attributes and scale attachments. */
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

    /* Atomic variables map their type directly; user-defined types
     * copy the committed datatype so the dataset stays linked to the
     * named type. */
    if ((ret = nx4_file_type(grp, var->type_info->hdr.id, &hdf_type)))
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

    if (var->type_info->hdr.id < NC_FIRSTUSERTYPEID) {
        if (var->endianness == NC_ENDIAN_BIG)
            H5Tset_order(hdf_type, H5T_ORDER_BE);
        else if (var->endianness == NC_ENDIAN_LITTLE)
            H5Tset_order(hdf_type, H5T_ORDER_LE);
    }

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

    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0 ||
        nx4_set_att_order(dcpl)) {
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

    if ((ret = nx4_dcpl_fill(dcpl, h5, var, hdf_type)))
        goto fail;

    if (var->ndims > 0 &&
        (ret = nx4_apply_filters(dcpl, var)))
        goto fail;

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
    } else {
        NEXTCDF4_DIM_INFO_T *dinfo =
            (NEXTCDF4_DIM_INFO_T *)var->dim[0]->format_dim_info;
        /* Reattach every dependent variable to the new scale object —
         * on recreation their DIMENSION_LIST entries pointed at the
         * deleted one; on first creation they pointed at the
         * placeholder scale. */
        if (dinfo && dinfo->hdf_dataset >= 0 &&
            (ret = nx4_reattach_dim(grp, var->dim[0], -1,
                                  dinfo->hdf_dataset, var)))
            goto fail;
    }

    /* Recreation dropped every attribute on the dataset — the hidden
     * id attributes were rewritten above; restore the user's
     * attributes in index order. On first creation the list is
     * empty. */
    if ((ret = nx4_rewrite_atts(vinfo->hdf_dataset, var->att, grp)))
        goto fail;

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
    /* Unlike the metadata-definition calls, the def-var setters do
     * not implicitly re-enter define mode — upstream leaves them
     * illegal outside it. */
    if ((*file)->no_write)
        return NC_EPERM;
    if (!(*file)->define_mode)
        return NC_ENOTINDEFINE;
    /* Dataset creation properties are immutable once the dataset
     * exists: any def-var setter after `_enddef` committed it is too
     * late, matching upstream's `var->created` checks. */
    if ((*var)->created)
        return NC_ELATEDEF;
    return NC_NOERR;
}

/**
 * @brief Set storage and chunk sizes — the `nc_def_var_chunking()`
 * dispatch entry.
 *
 * Records the layout and per-dimension chunk sizes and marks the
 * variable `dcpl_dirty`; layout is fixed at dataset creation in
 * HDF5, so `_enddef` rebuilds the dataset once with the new
 * creation properties.
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

    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
}

/**
 * @brief Set fill behavior — the `nc_def_var_fill()` dispatch entry.
 *
 * Stores the no-fill flag and a private copy of the fill value on the
 * variable and marks it `dcpl_dirty`: fill value and fill time are
 * HDF5 creation properties, so `_enddef` rebuilds the dataset once
 * with them.
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
        /* A variable-sized fill value owns nested payloads — reclaim
         * them before dropping the top-level buffer. */
        if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID &&
            var->type_info->varsized)
            NC_reclaim_data(h5->controller,
                            (nc_type)var->type_info->hdr.id,
                            var->fill_value, 1);
        free(var->fill_value);
        var->fill_value = NULL;
    }
    if (!no_fill && fill_value) {
        size = var->type_info->size;
        if (!size)
            return NC_EBADTYPE;
        if (!(var->fill_value = malloc(size)))
            return NC_ENOMEM;
        if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID &&
            var->type_info->varsized) {
            /* Deep-copy the vlen descriptor and its payload: the
             * caller's buffer stays caller-owned. */
            if ((ret = NC_copy_data(h5->controller,
                                    (nc_type)var->type_info->hdr.id,
                                    fill_value, 1, var->fill_value))) {
                NC_reclaim_data(h5->controller,
                                (nc_type)var->type_info->hdr.id,
                                var->fill_value, 1);
                free(var->fill_value);
                var->fill_value = NULL;
                return ret;
            }
        } else {
            memcpy(var->fill_value, fill_value, size);
        }
        var->fill_val_changed = 1;
    }

    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
}

/**
 * @brief Set the on-disk byte order — the `nc_def_var_endian()`
 * dispatch entry.
 *
 * Records the byte order on the variable and marks it `dcpl_dirty`;
 * the `_enddef` rebuild applies it to the dataset's datatype.
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
    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
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
    if (fill_valuep && !var->no_fill) {
        if (var->fill_value) {
            memcpy(fill_valuep, var->fill_value, var->type_info->size);
        } else if (var->type_info->hdr.id > NC_STRING) {
            /* NEP-only atomics and user-defined types have no
             * upstream default; report the all-zero fill used at
             * dataset creation. */
            memset(fill_valuep, 0, var->type_info->size);
        } else if ((ret = nc4_get_default_fill_value(var->type_info,
                                                     fill_valuep)))
            return ret;
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
 * @brief Write one in-memory attribute to an HDF5 object.
 *
 * The attribute name/type/length/data come from @p att; the HDF5
 * datatype maps from `att->nc_typeid`.
 *
 * @param loc HDF5 object (dataset or group) to write on.
 * @param att Attribute to write.
 * @param grp Group used to resolve user-defined datatypes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_write_att(hid_t loc, const NC_ATT_INFO_T *att, NC_GRP_INFO_T *grp)
{
    hid_t hdf_type = -1, space = -1, aid = -1, mem_type = -1;
    hsize_t hlen = att->len;
    int close_mem = 0;
    int ret;

    if ((ret = nx4_file_type(grp, (nc_type)att->nc_typeid, &hdf_type)))
        return ret;
    if (att->len == 0) {
        if ((space = H5Screate(H5S_NULL)) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
    } else if ((space = H5Screate_simple(1, &hlen, NULL)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if (att->nc_typeid == NC_CHAR) {
        mem_type = hdf_type;
    } else if (att->nc_typeid == NC_STRING) {
        mem_type = H5Tcopy(H5T_C_S1);
        if (mem_type >= 0) {
            H5Tset_size(mem_type, H5T_VARIABLE);
            H5Tset_strpad(mem_type, H5T_STR_NULLTERM);
        }
        close_mem = 1;
    } else {
        /* nx4_memory_type() returns an owned handle for both atomic
         * and user-defined types. */
        if ((ret = nx4_memory_type(grp, (nc_type)att->nc_typeid,
                                   &mem_type)))
            goto fail;
        close_mem = 1;
    }
    if ((aid = H5Acreate2(loc, att->hdr.name, hdf_type, space,
                          H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if (att->len > 0 && H5Awrite(aid, mem_type, att->data) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    ret = NC_NOERR;

fail:
    if (close_mem && mem_type >= 0)
        H5Tclose(mem_type);
    if (aid >= 0)
        H5Aclose(aid);
    if (hdf_type >= 0)
        H5Tclose(hdf_type);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Rewrite every attribute on an HDF5 object in index order.
 *
 * netcdf-c defers attribute writes to `_enddef`, so the on-disk
 * creation order always matches the metadata index order. NEXTCDF-4
 * writes eagerly: a renamed or replaced attribute would land last in
 * HDF5 creation order and reopen in the wrong position. Deleting the
 * indexed attributes and rewriting them in index order keeps the two
 * consistent. Internal (`_Netcdf4*`, `CLASS`, ...) attributes are not
 * in the index and are untouched.
 *
 * @param loc HDF5 object (dataset or group).
 * @param list Attribute index.
 * @param grp Group used to resolve user-defined datatypes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_rewrite_atts(hid_t loc, NCindex *list, NC_GRP_INFO_T *grp)
{
    size_t i;
    int ret;

    for (i = 0; i < ncindexsize(list); i++) {
        NC_ATT_INFO_T *a = (NC_ATT_INFO_T *)ncindexith(list, i);
        if (a && a->hdr.name)
            H5E_BEGIN_TRY {
                H5Adelete(loc, a->hdr.name);
            } H5E_END_TRY;
    }
    for (i = 0; i < ncindexsize(list); i++) {
        NC_ATT_INFO_T *a = (NC_ATT_INFO_T *)ncindexith(list, i);
        if (a && (ret = nx4_write_att(loc, a, grp)))
            return ret;
    }
    return NC_NOERR;
}

/**
 * @brief Materialize attributes deferred while their datatype or
 * dataset was pending — called at `_enddef`.
 *
 * Attributes whose datatype was an uncommitted user type, or that
 * live on a user-typed variable whose dataset did not exist yet,
 * are marked `atts_pending` on the owning group/variable. Once
 * NEXTCDF4_write_types() and the deferred dataset creations have
 * run, this rewrites the pending objects' attribute lists in index
 * order, preserving definition order on disk.
 *
 * @param grp Group to walk (recursively).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_flush_pending_atts(NC_GRP_INFO_T *grp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    NEXTCDF4_FILE_INFO_T *file = grp->nc4_info->format_file_info;
    size_t i;
    int ret;

    if (ginfo && ginfo->atts_pending) {
        hid_t loc = ginfo->hdf_group >= 0 ? ginfo->hdf_group : file->rootid;
        if ((ret = nx4_rewrite_atts(loc, grp->att, grp)))
            return ret;
        ginfo->atts_pending = 0;
    }

    for (i = 0; i < ncindexsize(grp->vars); i++) {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        NEXTCDF4_VAR_INFO_T *vinfo;
        if (!var || !(vinfo = var->format_var_info))
            continue;
        if (vinfo->atts_pending) {
            if (vinfo->hdf_dataset < 0)
                return NC_EFILEMETA;
            if ((ret = nx4_rewrite_atts(vinfo->hdf_dataset, var->att,
                                        grp)))
                return ret;
            vinfo->atts_pending = 0;
        }
    }

    for (i = 0; i < ncindexsize(grp->children); i++) {
        NC_GRP_INFO_T *child =
            (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        if (child && (ret = nx4_flush_pending_atts(child)))
            return ret;
    }
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
    NEXTCDF4_VAR_INFO_T *vinfo = NULL;
    NC_TYPE_INFO_T *utype = NULL;
    NCindex *list;
    hid_t loc;
    NC_ATT_INFO_T *att;
    size_t size;
    void *data = NULL;
    int pending = 0;
    int range_error = 0;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (is_reserved_att(name) && strcmp(name, NC_FillValue))
        return NC_EBADNAME;
    if (varid == NC_GLOBAL) {
        /* A subgroup ncid's global attributes live on that group, so
         * locally defined types resolve in the right scope. */
        if ((ret = nc4_find_nc4_grp(ncid, &grp)))
            return ret;
        var = NULL;
    } else {
        if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
            return ret;
        if (!var)
            return NC_ENOTVAR;
        vinfo = var->format_var_info;
    }

    if (datatype >= NC_FIRSTUSERTYPEID) {
        /* User-defined attribute types are an enhanced-model feature
         * and must be visible from the owning group's scope. */
        if (file->mode & NC_CLASSIC_MODEL)
            return NC_ENOTNC4;
        if (nx4_find_type(grp, datatype, &utype) || !utype)
            return NC_EBADTYPE;
    } else {
        /* Upstream hdf5attr.c semantics: classic-model files accept
         * only the six classic attribute types. */
        if (file->mode & NC_CLASSIC_MODEL && datatype > NC_DOUBLE)
            return NC_ESTRICTNC3;
        if ((ret = NEXTCDF4_check_atomic_type(file, datatype)))
            return ret;
    }

    if (var && !strcmp(name, NC_FillValue)) {
        /* Upstream routes variable _FillValue attributes through the
         * fill machinery: exactly one value of the variable's type,
         * and no changes after data have been written. */
        if (len != 1)
            return NC_EINVAL;
        if (var->written_to)
            return NC_ELATEFILL;
        if (datatype != var->type_info->hdr.id)
            return NC_EBADTYPE;
    }

    if ((ret = get_att_context(grp, var, &list, &loc)))
        return ret;

    if (memtype == NC_NAT) memtype = datatype;
    if (utype) {
        /* User-defined types have no numeric conversion: the buffer
         * must carry the attribute's own type. */
        if (memtype != datatype)
            return NC_EBADTYPE;
    } else if ((ret = nx4_convert_type(NULL, NULL, memtype, datatype, 0,
                                       &range_error, NULL, 0, NC_NOQUANTIZE,
                                       0, 0)) && datatype != memtype)
        return ret;
    if (len && !value) return NC_EINVAL;
    if ((ret = nx4_type_size(grp, datatype, &size))) return ret;
    if (len > SIZE_MAX / size) return NC_EINVAL;
    if (len) {
        if (!(data = malloc(len * size))) return NC_ENOMEM;
        if (datatype == memtype) memcpy(data, value, len * size);
        else {
            ret = nx4_convert_type(value, data, memtype, datatype, len,
                                    &range_error, NULL, 0, NC_NOQUANTIZE, 0, 0);
            if (ret) { free(data); return ret; }
        }
    }
    if (var && !strcmp(name, NC_FillValue)) {
        /* Mirror the value into var->fill_value; a pre-commit variable
         * is marked dirty so the `_enddef` rebuild carries it in the
         * DCPL fill message too. Once the dataset is committed the
         * DCPL is immutable — the attribute (which is written
         * normally, below) and var->fill_value carry the value, the
         * same user-visible behavior upstream has. */
        if (var->fill_value) {
            if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID &&
                var->type_info->varsized)
                NC_reclaim_data(h5->controller,
                                (nc_type)var->type_info->hdr.id,
                                var->fill_value, 1);
            free(var->fill_value);
            var->fill_value = NULL;
        }
        if (datatype == NC_STRING)
            var->fill_value = strdup(*(char **)data);
        else if (!(var->fill_value = malloc(size)))
            { free(data); return NC_ENOMEM; }
        if (var->fill_value && datatype != NC_STRING) {
            if (utype && utype->varsized) {
                /* Dynamic fill values are deep-copied — the caller's
                 * vlen payload stays caller-owned. */
                if ((ret = NC_copy_data(h5->controller, datatype,
                                        data, 1, var->fill_value))) {
                    NC_reclaim_data(h5->controller, datatype,
                                    var->fill_value, 1);
                    free(var->fill_value);
                    var->fill_value = NULL;
                    free(data);
                    return ret;
                }
            } else {
                memcpy(var->fill_value, data, size);
            }
        }
        if (!var->fill_value)
            { free(data); return NC_ENOMEM; }
        var->fill_val_changed = 1;
        if (!var->created)
            ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    }

    /* The HDF5 write is deferred when the attribute's datatype is
     * still uncommitted (compound/enum members may still be added)
     * or when the owning variable's dataset does not exist yet. The
     * in-memory record keeps definition order; `_enddef`
     * materializes the list. */
    pending = (vinfo && vinfo->hdf_dataset < 0) ||
              (utype && !utype->committed) ||
              loc < 0;

    /* Replace an existing attribute of the same name. Upstream keeps
     * the attribute's index position, so the record is deleted and a
     * new one reinserted at the same slot rather than appended. */
    if ((att = find_att(list, name))) {
        size_t pos;
        for (pos = 0; pos < ncindexsize(list); pos++)
            if (ncindexith(list, pos) == (NC_OBJ *)att)
                break;
        nx4_att_list_del(list, att);
        if ((ret = add_att(list, var ? &var->hdr : &grp->hdr, grp, name,
                           datatype, len, data, &att)))
            goto fail;
        ncindexidel(list, ncindexsize(list) - 1);
        nclistinsert(list->list, pos, (NC_OBJ *)att);
        for (pos = 0; pos < ncindexsize(list); pos++) {
            NC_ATT_INFO_T *a = (NC_ATT_INFO_T *)ncindexith(list, pos);
            if (a)
                a->hdr.id = (int)pos;
        }
        ncindexrebuild(list);
        if (pending) {
            if (vinfo) vinfo->atts_pending = 1;
            else ((NEXTCDF4_GRP_INFO_T *)grp->format_grp_info)->atts_pending = 1;
        } else if ((ret = nx4_rewrite_atts(loc, list, grp)))
            goto fail;
    } else {
        if ((ret = add_att(list, var ? &var->hdr : &grp->hdr, grp, name,
                           datatype, len, data, &att)))
            goto fail;
        if (pending) {
            if (vinfo) vinfo->atts_pending = 1;
            else ((NEXTCDF4_GRP_INFO_T *)grp->format_grp_info)->atts_pending = 1;
        } else if ((ret = nx4_write_att(loc, att, grp)))
            goto fail;
    }
    if (data) {
        free(data);
        data = NULL;
    }

    return range_error ? NC_ERANGE : NC_NOERR;

fail:
    if (data)
        free(data);
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
        if ((ret = nc4_find_nc4_grp(ncid, &grp)))
            return ret;
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
        if ((ret = nc4_find_nc4_grp(ncid, &grp)))
            return ret;
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
    if (att->nc_typeid >= NC_FIRSTUSERTYPEID) {
        /* User-defined types have no numeric conversion: the buffer
         * must carry the attribute's own type. */
        if (memtype != att->nc_typeid)
            return NC_EBADTYPE;
    } else if (att->nc_typeid != memtype &&
               (ret = nx4_convert_type(NULL, NULL, att->nc_typeid, memtype, 0,
                                       &range_error, NULL, 0, NC_NOQUANTIZE,
                                       0, 0)))
        return ret;
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
            NC_TYPE_INFO_T *at = NULL;
            size_t size;
            if ((ret = nx4_type_size(grp, memtype, &size)))
                return ret;
            /* Variable-sized types (vlens and compounds containing
             * them) are deep-copied: the caller receives owned
             * payloads it can reclaim with nc_reclaim_data() without
             * disturbing the cached attribute. */
            if (memtype >= NC_FIRSTUSERTYPEID &&
                !nx4_find_type(grp, memtype, &at) && at && at->varsized)
                return NC_copy_data(h5->controller, memtype, att->data,
                                    att->len, value);
            memcpy(value, att->data, att->len * size);
            return NC_NOERR;
        }
    }

    if (att->len == 0)
        return NC_NOERR;

    ret = nx4_convert_type(att->data, value, att->nc_typeid, memtype,
                            att->len, &range_error, NULL, 0, NC_NOQUANTIZE, 0, 0);
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
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!newname) return NC_EINVAL;
    if ((ret = NC_check_name(newname)))
        return ret;
    if (is_reserved_att(newname))
        return NC_EBADNAME;
    if (varid == NC_GLOBAL) {
        if ((ret = nc4_find_nc4_grp(ncid, &grp)))
            return ret;
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

    /* Upstream semantics: the rename keeps the attribute's index
     * position — only the name changes in place, then the object's
     * attributes are rewritten in index order so the on-disk
     * creation order matches. A pending object (deferred dataset or
     * uncommitted datatype) just stays pending. */
    free(att->hdr.name);
    if (!(att->hdr.name = strdup(newname)))
        return NC_ENOMEM;
    if (!ncindexrebuild(list))
        return NC_EINTERNAL;

    if (loc >= 0) {
        NC_TYPE_INFO_T *utype = NULL;
        nx4_find_type(grp, att->nc_typeid, &utype);
        if (utype && !utype->committed) {
            if (var)
                ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->atts_pending = 1;
            else
                ((NEXTCDF4_GRP_INFO_T *)grp->format_grp_info)->atts_pending = 1;
        } else {
            H5E_BEGIN_TRY {
                H5Adelete(loc, name);
            } H5E_END_TRY;
            if ((ret = nx4_rewrite_atts(loc, list, grp)))
                return ret;
        }
    }

    if (H5Fflush(file->hdfid, H5F_SCOPE_LOCAL) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * @brief Create a standalone dimension-scale dataset for a dimension.
 *
 * Used when a coordinate-variable relationship ends and the
 * dimension no longer shares its dataset with a variable. The scale
 * carries `CLASS`, `NAME` and `_Netcdf4Dimid` attributes like the
 * scales `def_dim` creates.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group containing the dimension.
 * @param dim Dimension.
 * @param name Link and scale name.
 * @param dsetp Receives the new dataset identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_dimscale_create(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp,
                    NC_DIM_INFO_T *dim, const char *name, hid_t *dsetp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    hid_t dtype = -1, space = -1, dcpl = -1, dset = -1;
    hsize_t cur = dim->len;
    hsize_t max = H5S_UNLIMITED, chunk = 1;
    int ret = NC_NOERR;

    if ((dtype = H5Tcopy(H5T_STD_I8LE)) < 0)
        return NC_EHDFERR;
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0 ||
        nx4_set_att_order(dcpl))
        { ret = NC_EHDFERR; goto done; }
    if (dim->unlimited) {
        if ((space = H5Screate_simple(1, &cur, &max)) < 0 ||
            H5Pset_chunk(dcpl, 1, &chunk) < 0)
            { ret = NC_EHDFERR; goto done; }
    } else if ((space = H5Screate_simple(1, &cur, NULL)) < 0)
        { ret = NC_EHDFERR; goto done; }
    if ((dset = H5Dcreate2(hdf_grp, name, dtype, space, H5P_DEFAULT,
                           dcpl, H5P_DEFAULT)) < 0)
        { ret = NC_EHDFERR; goto done; }
    if ((ret = write_string_att(dset, "CLASS", NEXTCDF4_DIMCLASS)) ||
        (ret = write_string_att(dset, "NAME", name)) ||
        (ret = write_int_att(dset, NEXTCDF4_DIMID_ATT, (int)dim->hdr.id)))
        goto done;
    *dsetp = dset;
    dset = -1;
done:
    if (dset >= 0) H5Dclose(dset);
    if (dtype >= 0) H5Tclose(dtype);
    if (space >= 0) H5Sclose(space);
    if (dcpl >= 0) H5Pclose(dcpl);
    return ret;
}

/**
 * @brief Reattach every variable on a dimension from one scale
 * dataset to another.
 *
 * Mirrors upstream's rec_detach_scales(): all variables in @p grp
 * (recursively) that attach @p old_dset as the scale for @p dim are
 * detached and re-attached to @p new_dset. @p skip is exempted (the
 * coordinate variable being broken/reformed never self-attaches).
 *
 * @param grp Group to walk.
 * @param dim Dimension whose scale changed.
 * @param old_dset Previous scale dataset.
 * @param new_dset New scale dataset.
 * @param skip Variable to exempt, or NULL.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_reattach_dim(NC_GRP_INFO_T *grp, NC_DIM_INFO_T *dim, hid_t old_dset,
                 hid_t new_dset, NC_VAR_INFO_T *skip)
{
    size_t i;
    int d, ret;

    if (grp->vars)
        for (i = 0; i < ncindexsize(grp->vars); i++) {
            NC_VAR_INFO_T *v = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
            NEXTCDF4_VAR_INFO_T *vi;
            if (!v || v == skip)
                continue;
            vi = v->format_var_info;
            if (!vi || vi->hdf_dataset < 0)
                continue;
            for (d = 0; d < v->ndims; d++) {
                if (v->dim[d] != dim)
                    continue;
                if (old_dset >= 0)
                    H5E_BEGIN_TRY {
                        /* A former coordinate variable was never
                         * attached to its own dataset; detaching what
                         * is not attached is harmless. */
                        H5DSdetach_scale(vi->hdf_dataset, old_dset,
                                         (unsigned)d);
                    } H5E_END_TRY;
                if (new_dset >= 0 &&
                    H5DSattach_scale(vi->hdf_dataset, new_dset,
                                     (unsigned)d) < 0)
                    return NC_EHDFERR;
            }
        }
    if (grp->children)
        for (i = 0; i < ncindexsize(grp->children); i++) {
            NC_GRP_INFO_T *child =
                (NC_GRP_INFO_T *)ncindexith(grp->children, i);
            if (child && (ret = nx4_reattach_dim(child, dim, old_dset,
                                               new_dset, skip)))
                return ret;
        }
    return NC_NOERR;
}

/**
 * @brief Break a coordinate-variable relationship.
 *
 * Upstream nc4_break_coord_var() semantics: the shared dataset stays
 * with the variable under its link name, the dimension gets a
 * standalone scale dataset under @p scalename, all other variables
 * are reattached to it, and the variable's dataset loses its
 * `_Netcdf4Dimid` marker so the loader does not recover a phantom
 * dimension.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group containing the dimension.
 * @param dim Dimension leaving the relationship.
 * @param cvar Former coordinate variable.
 * @param scalename Name for the dimension's new scale dataset.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_break_coord_var(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp,
                    NC_DIM_INFO_T *dim, NC_VAR_INFO_T *cvar,
                    const char *scalename)
{
    NEXTCDF4_DIM_INFO_T *dinfo = dim->format_dim_info;
    hid_t old_dset = dinfo->hdf_dataset;
    hid_t new_dset = -1;
    int ret;

    if ((ret = nx4_dimscale_create(file, grp, dim, scalename, &new_dset)))
        return ret;
    if ((ret = nx4_reattach_dim(grp, dim, old_dset, new_dset, cvar))) {
        H5Dclose(new_dset);
        return ret;
    }
    H5E_BEGIN_TRY {
        H5Adelete(old_dset, NEXTCDF4_DIMID_ATT);
    } H5E_END_TRY;
    if (dinfo->hdf_dataset >= 0)
        H5Dclose(dinfo->hdf_dataset);
    dinfo->hdf_dataset = new_dset;
    return NC_NOERR;
}

/**
 * @brief Reform a coordinate-variable relationship.
 *
 * Upstream nc4_reform_coord_var() semantics: @p var's dataset becomes
 * the dimension's scale — it gains `CLASS`/`NAME`/`_Netcdf4Dimid`
 * markers, every other variable on the dimension is reattached to it,
 * and the dimension's previous standalone scale (if any) is unlinked.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group containing the dimension.
 * @param dim Dimension regaining a coordinate variable.
 * @param var Variable whose dataset becomes the scale.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
nx4_reform_coord_var(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp,
                     NC_DIM_INFO_T *dim, NC_VAR_INFO_T *var)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    NEXTCDF4_DIM_INFO_T *dinfo = dim->format_dim_info;
    NEXTCDF4_VAR_INFO_T *vinfo = var->format_var_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    hid_t old_dset = dinfo ? dinfo->hdf_dataset : -1;
    int shared_old = 0;
    int ret;

    if (!dinfo || !vinfo || vinfo->hdf_dataset < 0)
        return NC_EFILEMETA;

    /* Detach any scales on the reforming variable itself (coordinate
     * variables are never attached to a scale). */
    {
        int d;
        for (d = 0; d < var->ndims; d++) {
            if (var->dim[d] == dim && old_dset >= 0 &&
                old_dset != vinfo->hdf_dataset)
                H5DSdetach_scale(vinfo->hdf_dataset, old_dset,
                                 (unsigned)d);
        }
    }

    /* Make the variable's dataset a dimension scale again. */
    if (H5DSset_scale(vinfo->hdf_dataset, var->hdr.name) < 0)
        return NC_EHDFERR;
    if ((ret = write_int_att(vinfo->hdf_dataset, NEXTCDF4_DIMID_ATT,
                             (int)dim->hdr.id)))
        return ret;

    /* Reattach other variables from the old scale to this dataset. */
    if ((ret = nx4_reattach_dim(grp, dim, old_dset, vinfo->hdf_dataset,
                                var)))
        return ret;

    /* A standalone old scale is unlinked; a dataset shared with
     * another variable (the former coordinate variable) stays. */
    if (old_dset >= 0 && old_dset != vinfo->hdf_dataset) {
        size_t i;
        shared_old = 0;
        for (i = 0; i < ncindexsize(grp->vars); i++) {
            NC_VAR_INFO_T *v = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
            NEXTCDF4_VAR_INFO_T *vi = v ? v->format_var_info : NULL;
            if (v && v != var && vi && vi->hdf_dataset == old_dset) {
                shared_old = 1;
                break;
            }
        }
        if (!shared_old) {
            char path[1024];
            ssize_t n = H5Iget_name(old_dset, path, sizeof(path));
            if (n > 0) {
                char *base = strrchr(path, '/');
                H5E_BEGIN_TRY {
                    H5Ldelete(hdf_grp, base ? base + 1 : path, H5P_DEFAULT);
                } H5E_END_TRY;
            }
            H5Dclose(old_dset);
        }
    }
    dinfo->hdf_dataset = vinfo->hdf_dataset;
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
        var->container == var->dim[0]->container &&
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
    NC_VAR_INFO_T *cvar;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;

    if ((ret = nc4_find_dim(grp, dimid, &dim, NULL)))
        return ret;
    if (!dim)
        return NC_EBADDIM;
    if (!strcmp(dim->hdr.name, name))
        return NC_NOERR;

    /* Only another dimension can conflict; renaming onto a variable
     * that uses this dimension at index 0 is the upstream
     * reform-coordinate-variable case. */
    if (ncindexlookup(dim->container->dim, (char *)name))
        return NC_ENAMEINUSE;

    dinfo = (NEXTCDF4_DIM_INFO_T *)dim->format_dim_info;
    ginfo = dim->container->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    oldname = dim->hdr.name;

    /* Find the coordinate variable before the dim name changes:
     * find_coord_var matches on the old name. */
    cvar = find_coord_var(dim->container, dim);

    /* A variable with the target name that uses this dimension at
     * index 0 reforms the coordinate-variable relationship. */
    {
        NC_VAR_INFO_T *rvar =
            (NC_VAR_INFO_T *)ncindexlookup(dim->container->vars,
                                           (char *)name);
        int reform = rvar && rvar->ndims == 1 && rvar->dim[0] == dim;

        if (cvar) {
            /* Upstream semantics: the shared dataset stays with the
             * variable under its old link name, and the dimension
             * moves onto the reforming variable's dataset or a
             * standalone scale. */
            if (reform) {
                H5E_BEGIN_TRY {
                    H5Adelete(dinfo->hdf_dataset, NEXTCDF4_DIMID_ATT);
                } H5E_END_TRY;
                if ((ret = nx4_reform_coord_var(file, dim->container,
                                                dim, rvar)))
                    return ret;
            } else if ((ret = nx4_break_coord_var(file, dim->container,
                                                  dim, cvar, name)))
                return ret;
        } else if (reform) {
            /* The standalone scale is unlinked inside reform. */
            if ((ret = nx4_reform_coord_var(file, dim->container,
                                            dim, rvar)))
                return ret;
        } else if (dinfo && dinfo->hdf_dataset >= 0) {
            if (H5Lmove(hdf_grp, oldname, hdf_grp, name, H5P_DEFAULT,
                        H5P_DEFAULT) < 0)
                return NC_EHDFERR;
            if (H5DSset_scale(dinfo->hdf_dataset, name) < 0)
                return NC_EHDFERR;
        }
    }

    free(dim->hdr.name);
    if (!(dim->hdr.name = strdup(name)))
        return NC_ENOMEM;
    if (!ncindexrebuild(dim->container->dim))
        return NC_EHDFERR;

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
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;

    if (varid < 0 || varid >= ncindexsize(grp->vars))
        return NC_ENOTVAR;
    var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid);
    if (!var)
        return NC_ENOTVAR;
    if (!strcmp(var->hdr.name, name))
        return NC_NOERR;

    /* Only another variable can conflict; renaming onto a dimension
     * the variable uses at index 0 is the upstream
     * reform-coordinate-variable case. */
    if (ncindexlookup(var->container->vars, (char *)name))
        return NC_ENAMEINUSE;

    vinfo = (NEXTCDF4_VAR_INFO_T *)var->format_var_info;
    ginfo = var->container->format_grp_info;
    hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    oldname = var->hdr.name;

    /* Detect the coordinate variable before the var name changes:
     * after the update the names can never match. */
    dim = coord_var_dim(var);

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

    /* A coordinate variable renamed away from its dimension breaks
     * the relationship upstream-style: the dimension keeps its name
     * and gets a standalone scale dataset under it (created now that
     * the variable's link has moved). */
    if (dim &&
        (ret = nx4_break_coord_var(file, var->container, dim, var,
                                   dim->hdr.name)))
        return ret;

    free(var->hdr.name);
    if (!(var->hdr.name = strdup(name)))
        return NC_ENOMEM;
    if (!ncindexrebuild(var->container->vars))
        return NC_EHDFERR;

    /* A non-coordinate variable renamed onto a dimension it uses at
     * index 0 reforms the relationship: its dataset becomes the
     * dimension's scale. */
    if (!dim && var->ndims == 1 && var->dim[0] &&
        var->dim[0]->hdr.name && !strcmp(name, var->dim[0]->hdr.name) &&
        (ret = nx4_reform_coord_var(file, var->container, var->dim[0],
                                    var)))
        return ret;

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
    if ((ret = NEXTCDF4_check_write_define(h5, file)))
        return ret;
    if (varid == NC_GLOBAL) {
        if ((ret = nc4_find_nc4_grp(ncid, &grp)))
            return ret;
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

    if (loc >= 0)
        H5E_BEGIN_TRY {
            H5Adelete(loc, name);
        } H5E_END_TRY;
    nx4_att_list_del(list, att);
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

    /* Upstream files may carry a scale with no `_Netcdf4Dimid`
     * attribute (a dimension without a coordinate variable). Collect
     * it anyway with dimid -1; load_dimensions() assigns a free id. */
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

    /* Assign free dimids to scales that store none (markerless
     * upstream scales), smallest-first in discovery order. */
    for (i = 0; i < dl.count; i++) {
        if (dl.dimids[i] < 0) {
            int cand, j;
            for (cand = 0; ; cand++) {
                int used = 0;
                for (j = 0; j < dl.count; j++)
                    if (j != i && dl.dimids[j] == cand) { used = 1; break; }
                if (!used)
                    break;
            }
            dl.dimids[i] = cand;
        }
    }

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
    if (!diminfo) {
        /* Upstream scales for dimensions without coordinate variables
         * carry a NAME of "This is a netCDF dimension but not a
         * netCDF variable." — useless for matching. The scale's own
         * link name is the dimension name instead. */
        ssize_t nlen = H5Iget_name(dsid, name, sizeof(name));
        if (nlen > 0) {
            char *base = strrchr(name, '/');
            diminfo = find_dim_by_name(r->grp, base ? base + 1 : name);
        }
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
        /* Not an atomic type: a variable created from a committed
         * user type resolves to the registered type record by HDF5
         * object identity. */
        NC_TYPE_INFO_T *utype = NULL;
        if (nx4_hdf_type_match(grp, ftype, &utype) || !utype) {
            ret = NC_EBADTYPE;
            goto done;
        }
        xtype = (nc_type)utype->hdr.id;
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
    if (xtype >= NC_FIRSTUSERTYPEID) {
        /* Share the committed type record; the reference count
         * balances the group's ownership with each variable using
         * it. */
        NC_TYPE_INFO_T *utype = NULL;
        if ((ret = nx4_find_type(grp, xtype, &utype)) || !utype) {
            free(vinfo);
            ret = ret ? ret : NC_EBADTYPE;
            goto done;
        }
        var->type_info = utype;
        utype->rc++;
    } else if ((ret = set_var_type(var, xtype))) {
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
                size_t fsize = var->type_info->size;
                if (var->fill_value) {
                    free(var->fill_value);
                    var->fill_value = NULL;
                }
                if (fsize && (var->fill_value = malloc(fsize))) {
                    /* NEP-only types: `H5Tget_native_type()` widens
                     * them to a larger native type (bfloat16 -> a
                     * 4-byte float), overflowing `var->fill_value`
                     * (#377) — the memory type keeps the on-disk
                     * size. User-defined types keep their own
                     * layout. */
                    hid_t mem_type = -1;
                    int rv;
                    if (xtype >= NC_FIRSTUSERTYPEID)
                        nx4_memory_type(grp, xtype, &mem_type);
                    else if (xtype > NC_STRING) {
                        int close_mem = 0;
                        mem_type = NEXTCDF4_memory_type(xtype, &close_mem);
                        if (mem_type >= 0 && !close_mem) {
                            hid_t c = H5Tcopy(mem_type);
                            mem_type = c;
                        }
                    } else {
                        mem_type = H5Tget_native_type(ftype, H5T_DIR_DEFAULT);
                    }
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

        H5E_BEGIN_TRY {
            order = H5Tget_order(ftype);
        } H5E_END_TRY;
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
    int is_var;         /**< Nonzero when iterating a variable's dataset. */
    NC_GRP_INFO_T *grp; /**< Group resolving user-defined datatypes. */
} att_iter_data_t;

/**
 * @brief Pick the `H5Aiterate2` index that preserves attribute
 * creation order when the object tracks it.
 *
 * NEXTCDF-4 and upstream files record `H5P_CRT_ORDER_TRACKED` on every
 * group and dataset; plain HDF5 objects without tracking fall back to
 * name order.
 *
 * @param obj HDF5 dataset or group identifier.
 * @param group Nonzero when @p obj is a group, zero for a dataset.
 * @return `H5_INDEX_CRT_ORDER` or `H5_INDEX_NAME`.
 */
static H5_index_t
att_iter_index(hid_t obj, int group)
{
    unsigned flags = 0;
    hid_t pl;
    H5_index_t idx = H5_INDEX_NAME;

    pl = group ? H5Gget_create_plist(obj) : H5Dget_create_plist(obj);
    if (pl >= 0) {
        if (H5Pget_attr_creation_order(pl, &flags) >= 0 &&
            (flags & H5P_CRT_ORDER_TRACKED))
            idx = H5_INDEX_CRT_ORDER;
        H5Pclose(pl);
    }
    return idx;
}

/**
 * @brief `H5Aiterate2` callback loading a single user attribute.
 *
 * Reserved (internal) attributes and attributes whose datatype
 * doesn't map to an atomic nc_type are skipped — neither should make
 * an otherwise-valid file unopenable. `_FillValue` on a variable is
 * the one exception: upstream exposes it as a normal, user-visible
 * attribute (#377).
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
    if (is_reserved_att(attr_name) &&
        !(ad->is_var && !strcmp(attr_name, NC_FillValue)))
        return 0;
    if (read_hdf5_att(loc, attr_name, ad->list, ad->container, ad->grp))
        return 0; /* skip attributes that cannot be mapped */
    return 0;
}

/**
 * @brief Mirror a loaded `_FillValue` attribute into
 * `var->fill_value`.
 *
 * `_FillValue` loads as a normal attribute; when present it is the
 * authoritative fill for `nc_inq_var_fill()` — the DCPL fill message
 * recovered earlier fills only the gap when no attribute exists.
 *
 * @param var Variable whose attribute index was just populated.
 */
static void
nx4_fill_from_att(NC_VAR_INFO_T *var)
{
    NC_ATT_INFO_T *att;
    size_t size;

    if (!(att = find_att(var->att, NC_FillValue)) || !att->data ||
        att->len < 1 || att->nc_typeid != var->type_info->hdr.id)
        return;
    if (att->nc_typeid == NC_STRING) {
        char *copy = strdup(*(char **)att->data);
        if (!copy)
            return;
        free(var->fill_value);
        var->fill_value = copy;
        return;
    }
    if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID) {
        /* Variable-sized types (vlens and compounds containing them)
         * deep-copy the descriptor and its payload so the fill value
         * owns its data outright. */
        NC_TYPE_INFO_T *ut = NULL;
        void *copy;
        if (!nx4_find_type(var->container, att->nc_typeid, &ut) || !ut ||
            !ut->varsized) {
            if (nx4_type_size(var->container, att->nc_typeid, &size))
                return;
            if (!(copy = malloc(size)))
                return;
            memcpy(copy, att->data, size);
        } else {
            size = var->type_info->size;
            if (!(copy = malloc(size ? size : 1)))
                return;
            if (NC_copy_data(var->container->nc4_info->controller,
                             att->nc_typeid, att->data, 1, copy)) {
                free(copy);
                return;
            }
        }
        free(var->fill_value);
        var->fill_value = copy;
        return;
    }
    if (NEXTCDF4_type_size(var->type_info->hdr.id, &size))
        return;
    {
        void *copy = malloc(size);
        if (!copy)
            return;
        memcpy(copy, att->data, size);
        free(var->fill_value);
        var->fill_value = copy;
    }
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
            NC_VAR_INFO_T *lvar;
            if ((ret = load_one_var(file, h5, grp, vl.names[i], vl.varids[i]))) {
                free_name_list(vl.names, vl.count);
                free(vl.varids);
                return ret;
            }
            lvar = (NC_VAR_INFO_T *)ncindexith(grp->vars, loaded);
            ad.loc = ((NEXTCDF4_VAR_INFO_T *)lvar->format_var_info)
                     ->hdf_dataset;
            ad.list = lvar->att;
            ad.container = &lvar->hdr;
            ad.is_var = 1;
            ad.grp = grp;
            H5Aiterate2(ad.loc, att_iter_index(ad.loc, 0), H5_ITER_INC,
                        NULL, load_att_cb, &ad);
            nx4_fill_from_att(lvar);
            loaded++;
        }
    } else {
        /* Stored varids present: load in id order so nc4_var_list_add
         * assigns the same ids. */
        for (id = 0; id <= maxid; id++) {
            for (i = 0; i < vl.count; i++) {
                if (vl.varids[i] == id) {
                    NC_VAR_INFO_T *lvar;
                    if ((ret = load_one_var(file, h5, grp, vl.names[i], vl.varids[i]))) {
                        free_name_list(vl.names, vl.count);
                        free(vl.varids);
                        return ret;
                    }
                    lvar = (NC_VAR_INFO_T *)ncindexith(grp->vars, loaded);
                    ad.loc = ((NEXTCDF4_VAR_INFO_T *)lvar->format_var_info)
                             ->hdf_dataset;
                    ad.list = lvar->att;
                    ad.container = &lvar->hdr;
                    ad.is_var = 1;
                    ad.grp = grp;
                    H5Aiterate2(ad.loc, att_iter_index(ad.loc, 0),
                                H5_ITER_INC, NULL, load_att_cb, &ad);
                    nx4_fill_from_att(lvar);
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
    ad.is_var = 0;
    ad.grp = grp;
    if (H5Aiterate2(hdf_grp, att_iter_index(hdf_grp, 1), H5_ITER_INC,
                    NULL, load_att_cb, &ad) < 0)
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
static int load_types(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
                      NC_GRP_INFO_T *grp);

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

    /* Register the record and class now; members are filled in the
     * second pass of load_types() so that a committed member type
     * appearing later in link order is already registered when its
     * container resolves members by object identity. */
    switch (cls) {
    case H5T_COMPOUND:
        type->nc_type_class = NC_COMPOUND;
        type->u.c.field = nclistnew();
        break;
    case H5T_ENUM:
        type->nc_type_class = NC_ENUM;
        type->u.e.enum_member = nclistnew();
        break;
    case H5T_OPAQUE:
        type->nc_type_class = NC_OPAQUE;
        break;
    case H5T_VLEN:
        type->nc_type_class = NC_VLEN;
        break;
    case H5T_STRING:
        type->nc_type_class = NC_STRING;
        break;
    default:
        ret = NC_EBADTYPE;
        goto fail;
    }
    type->committed = NC_TRUE;
    /* Mark vlens (and later, compounds containing them) variable-
     * sized so nc_copy_data/nc_reclaim_data walk their payloads. */
    NC4_set_varsize(type);

    return NC_NOERR;

fail:
    /* The record is already on the group's type index; leave it for
     * group teardown. The caller still owns @p htype and closes it —
     * drop only the private wrapper record. */
    if (type && type->format_type_info) {
        free(type->format_type_info);
        type->format_type_info = NULL;
    }
    return ret;
}

/**
 * @brief Fill the members of a registered type record from its HDF5
 * datatype.
 *
 * Second pass of load_types(): runs once every committed datatype in
 * the group has a registered record, so compound member types and
 * vlen bases resolve through nx4_hdf_type_match() regardless of the
 * order HDF5 links were iterated. Array members recover their rank
 * and shape via H5Tget_array_ndims()/H5Tget_array_dims().
 *
 * @param grp Group.
 * @param type Type record with `format_type_info` populated.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_type_members(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type)
{
    NEXTCDF4_TYPE_INFO_T *tinfo =
        (NEXTCDF4_TYPE_INFO_T *)type->format_type_info;
    hid_t htype = tinfo->hdf_type;
    size_t size = type->size;
    hid_t super = -1;
    nc_type xtype;
    int ret;

    switch (type->nc_type_class) {
    case NC_COMPOUND: {
        int nmem, m;
        if ((nmem = H5Tget_nmembers(htype)) < 0)
            return NC_EHDFERR;
        for (m = 0; m < nmem; m++) {
            char *mname = H5Tget_member_name(htype, (unsigned)m);
            size_t moff = H5Tget_member_offset(htype, (unsigned)m);
            hid_t mtype = -1;
            nc_type mxtype = NC_NAT;
            int mndims = 0;
            int mdims[NC_MAX_VAR_DIMS];

            if (!mname)
                return NC_EHDFERR;
            if ((mtype = H5Tget_member_type(htype, (unsigned)m)) < 0) {
                H5free_memory(mname);
                return NC_EHDFERR;
            }
            if (H5Tget_class(mtype) == H5T_ARRAY) {
                /* Fixed-size array member: record the shape and
                 * resolve the element type, atomic or user-defined. */
                hsize_t adims[NC_MAX_VAR_DIMS];
                hid_t base = -1;
                if ((mndims = H5Tget_array_ndims(mtype)) < 0 ||
                    mndims > NC_MAX_VAR_DIMS ||
                    H5Tget_array_dims(mtype, adims) < 0) {
                    H5Tclose(mtype);
                    H5free_memory(mname);
                    return NC_EHDFERR;
                }
                if ((base = H5Tget_super(mtype)) < 0) {
                    H5Tclose(mtype);
                    H5free_memory(mname);
                    return NC_EHDFERR;
                }
                if (map_nc_type(base, &mxtype)) {
                    NC_TYPE_INFO_T *ut = NULL;
                    if (nx4_hdf_type_match(grp, base, &ut) || !ut) {
                        H5Tclose(base);
                        H5Tclose(mtype);
                        H5free_memory(mname);
                        return NC_EBADTYPE;
                    }
                    mxtype = (nc_type)ut->hdr.id;
                }
                H5Tclose(base);
                for (ret = 0; ret < mndims; ret++)
                    mdims[ret] = (int)adims[ret];
                ret = NC_NOERR;
            } else if (map_nc_type(mtype, &mxtype)) {
                /* A committed member type resolves to its registered
                 * record by object identity. */
                NC_TYPE_INFO_T *ut = NULL;
                if (nx4_hdf_type_match(grp, mtype, &ut) || !ut) {
                    H5Tclose(mtype);
                    H5free_memory(mname);
                    return NC_EBADTYPE;
                }
                mxtype = (nc_type)ut->hdr.id;
            }
            H5Tclose(mtype);
            ret = nc4_field_list_add(type, mname, moff, mxtype, mndims,
                                     mndims ? mdims : NULL);
            H5free_memory(mname);
            if (ret)
                return ret;
            /* A member that is (transitively) variable-sized makes
             * the whole compound variable-sized. */
            NC4_recheck_varsize(type, mxtype);
        }
        break;
    }
    case NC_ENUM: {
        int nmem, m;
        if ((super = H5Tget_super(htype)) < 0)
            return NC_EHDFERR;
        if (map_nc_type(super, &xtype) == NC_NOERR)
            type->u.e.base_nc_typeid = xtype;
        else {
            H5Tclose(super);
            return NC_EBADTYPE;
        }
        H5Tclose(super);
        if ((nmem = H5Tget_nmembers(htype)) < 0)
            return NC_EHDFERR;
        for (m = 0; m < nmem; m++) {
            char *mname = H5Tget_member_name(htype, (unsigned)m);
            void *mval = NULL;
            if (!mname)
                return NC_EHDFERR;
            if (!(mval = malloc(size ? size : 1)) ||
                H5Tenum_valueof(htype, mname, mval) < 0) {
                free(mval);
                H5free_memory(mname);
                return NC_EHDFERR;
            }
            ret = nc4_enum_member_add(type, size, mname, mval);
            free(mval);
            H5free_memory(mname);
            if (ret)
                return ret;
        }
        break;
    }
    case NC_VLEN:
        if ((super = H5Tget_super(htype)) >= 0) {
            if (map_nc_type(super, &xtype) == NC_NOERR)
                type->u.v.base_nc_typeid = xtype;
            else {
                NC_TYPE_INFO_T *ut = NULL;
                if (!nx4_hdf_type_match(grp, super, &ut) && ut)
                    type->u.v.base_nc_typeid = (nc_type)ut->hdr.id;
            }
            H5Tclose(super);
        }
        break;
    default:
        /* OPAQUE and STRING carry no members. */
        break;
    }
    return NC_NOERR;
}

/**
 * @brief Load one group's full metadata: dimensions, committed
 * types, variables, attributes, then children recursively.
 *
 * Order matters — dimensions and committed types must exist before
 * variables resolve their dimids and datatypes, and children load
 * last so their parent's types are visible to them. An unmarked
 * root group with nothing loadable is rejected (`NC_ENOTNC`): a
 * plain HDF5 file with no netCDF structure is not a netCDF file.
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
    if ((ret = load_types(file, h5, grp)))
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
 * @brief `H5Literate` callback loading one committed datatype.
 *
 * Committed datatypes are the on-disk form of user-defined types.
 * They must all be registered — and their members populated — before
 * any variable or attribute that references them is loaded, so this
 * pass runs ahead of load_variables()/load_group_attributes().
 *
 * @param loc HDF5 location identifier.
 * @param name Name.
 * @param info Link info (unused).
 * @param op_data load_children_data_t iteration context.
 * @return 0 to continue iteration, -1 on load failure.
 */
static herr_t
load_type_cb(hid_t loc, const char *name, const H5L_info_t *info, void *op_data)
{
    load_children_data_t *ld = op_data;
    hid_t child_t = -1;
    (void)info;

    H5E_BEGIN_TRY {
        child_t = H5Topen2(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (child_t >= 0) {
        if (load_one_type(ld->parent, child_t, name)) {
            H5Tclose(child_t);
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Register every committed datatype in a group.
 * @param file NEXTCDF-4 file state.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
load_types(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5,
           NC_GRP_INFO_T *grp)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t hdf_grp = ginfo ? ginfo->hdf_group : file->rootid;
    load_children_data_t ld = {file, h5, grp, hdf_grp};

    if (H5Literate(hdf_grp, H5_INDEX_NAME, H5_ITER_INC, NULL,
                   load_type_cb, &ld) < 0)
        return NC_EHDFERR;

    /* Second pass: fill compound/enum/vlen members now that every
     * committed datatype in the group has a registered record. */
    {
        size_t i;
        for (i = 0; i < ncindexsize(grp->type); i++) {
            NC_TYPE_INFO_T *type =
                (NC_TYPE_INFO_T *)ncindexith(grp->type, i);
            int ret;
            if (type && (ret = load_type_members(grp, type)))
                return ret;
        }
    }
    return NC_NOERR;
}

/**
 * @brief `H5Literate` callback dispatching each child link to the
 * right loader.
 *
 * Links that open as groups become subgroups (load_one_group);
 * committed datatypes were already registered by the load_types()
 * pass and datasets by the variable pass. Anything else is skipped.
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

/**
 * @file nxt4type.c
 * @brief NEXTCDF-4 user-defined types and NC_STRING support.

 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nxt4internal.h"

/**
 * @brief Return the in-memory size of an atomic type — a thin public
 * wrapper over NEXTCDF4_type_size().
 * @param xtype NetCDF atomic type.
 * @param sizep Destination for the size.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_get_type_size(nc_type xtype, size_t *sizep)
{
    return NEXTCDF4_type_size(xtype, sizep);
}

/**
 * @brief Check that the file may have a user-defined type defined now.
 *
 * User-defined types (compound, vlen, enum, opaque) require define
 * mode and write access. They are an enhanced-model feature, so both
 * `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` files reject them —
 * NEXTCDF-4's netcdf-4-model mode deliberately supports only what
 * upstream netcdf-c already knows.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group (currently unused).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
check_type_write(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp)
{
    (void)grp;
    if (!file)
        return NC_EBADID;
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode)
        return NC_ENOTINDEFINE;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_ENOTNC4;
    if (file->netcdf4_model)
        return NC_ENOTNC4;
    return NC_NOERR;
}

/**
 * @brief Resolve file state and the group an ncid refers to.
 *
 * Shared prologue for the `def_*` functions: NEXTCDF4_get_file() maps
 * the ncid to file state, `nc4_find_nc4_grp` finds the in-memory
 * group (types are defined per-group in the enhanced model).
 *
 * @param ncid NetCDF file identifier.
 * @param h5 Destination for the NetCDF-4 file state.
 * @param grp Destination for the group.
 * @param filep Destination for the NEXTCDF-4 file state; may be NULL.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
get_grp_and_file(int ncid, NC_FILE_INFO_T **h5, NC_GRP_INFO_T **grp,
                 NEXTCDF4_FILE_INFO_T **filep)
{
    int ret;
    if ((ret = NEXTCDF4_get_file(ncid, h5, filep)))
        return ret;
    if ((ret = nc4_find_nc4_grp(ncid, grp)))
        return ret;
    return NC_NOERR;
}

/**
 * @brief Map an atomic nc_type to a fresh HDF5 datatype `hid_t`.
 *
 * Convenience wrapper over NEXTCDF4_map_hdf_type() returning -1
 * instead of an error code. The caller owns the returned datatype.
 *
 * @param xtype NetCDF atomic type.
 * @return HDF5 datatype, or negative on unsupported type.
 */
static hid_t
base_hdf_type(nc_type xtype)
{
    hid_t t;
    if (NEXTCDF4_map_hdf_type(xtype, &t))
        return -1;
    return t;
}

/**
 * @brief Record a built HDF5 datatype on a type, still uncommitted.
 *
 * HDF5 distinguishes a "transient" datatype (exists only in memory)
 * from a "committed" one (named and stored in the file). Types being
 * defined stay transient so members can still be inserted; they are
 * committed later at `_enddef` by NEXTCDF4_write_types() (or
 * immediately for vlen/opaque, which have no members).
 *
 * @param type In-memory type record.
 * @param hdf HDF5 datatype identifier to attach (ownership stays
 * with the record).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
attach_hdf_type(NC_TYPE_INFO_T *type, hid_t hdf)
{
    NEXTCDF4_TYPE_INFO_T *tinfo;

    if (!(tinfo = calloc(1, sizeof(*tinfo))))
        return NC_ENOMEM;
    tinfo->hdf_type = hdf;
    type->format_type_info = tinfo;
    type->committed = NC_FALSE;
    return NC_NOERR;
}

/**
 * @brief Commit (name and store) an HDF5 datatype into a group.
 *
 * Committed datatypes are named objects in the file, like variables —
 * this is what makes a user-defined type persistent. Falls back to
 * the root group when the group's own HDF5 handle isn't open yet.
 *
 * @param grp Group.
 * @param type In-memory type record.
 * @param hdf HDF5 datatype to commit.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_hdf_type(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type, hid_t hdf)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    hid_t commit_loc;
    int ret;

    if (!ginfo || ginfo->hdf_group < 0)
        commit_loc = grp->nc4_info->format_file_info ?
            ((NEXTCDF4_FILE_INFO_T *)grp->nc4_info->format_file_info)->rootid : -1;
    else
        commit_loc = ginfo->hdf_group;
    if (commit_loc < 0)
        return NC_EHDFERR;

    if (H5Tcommit2(commit_loc, type->hdr.name, hdf, H5P_DEFAULT,
                   H5P_DEFAULT, H5P_DEFAULT) < 0)
        return NC_EHDFERR;

    if ((ret = attach_hdf_type(type, hdf)))
        return ret;
    type->committed = NC_TRUE;
    return NC_NOERR;
}

/**
 * @brief Define a compound type — the `nc_def_compound()` dispatch
 * entry.
 *
 * A compound type is a C-struct-like record of named fields at byte
 * offsets. Creates an empty transient `H5T_COMPOUND`, registers the
 * `NC_TYPE_INFO_T`, and attaches the datatype uncommitted — fields
 * are added by NEXTCDF4_insert_compound() and the type is committed
 * at `_enddef`.
 *
 * @param ncid NetCDF file identifier.
 * @param size Size of the compound type in bytes.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type = NULL;
    hid_t hdf;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if ((ret = check_type_write(file, grp)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (nc4_check_dup_name(grp, (char *)name))
        return NC_ENAMEINUSE;

    if ((hdf = H5Tcreate(H5T_COMPOUND, size)) < 0)
        return NC_EHDFERR;

    if ((ret = nc4_type_list_add(grp, size, name, &type)))
        goto fail;
    type->nc_type_class = NC_COMPOUND;
    type->size = size;
    type->u.c.field = nclistnew();
    if ((ret = attach_hdf_type(type, hdf)))
        goto fail;

    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;

fail:
    H5Tclose(hdf);
    if (type)
        nc4_type_free(type);
    return ret;
}

/**
 * @brief Insert a scalar field into a compound type — the
 * `nc_insert_compound()` dispatch entry.
 *
 * Maps the field's atomic type to HDF5 and `H5Tinsert`s it at the
 * byte offset in the (still transient) compound datatype, then
 * records the field in the in-memory type.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param offset Field offset in bytes.
 * @param xtype NetCDF type of the field.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_insert_compound(int ncid, nc_type typeid, const char *name,
                         size_t offset, nc_type xtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    hid_t hdf;
    int ret;
    size_t size;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_COMPOUND)
        return NC_EBADTYPE;
    tinfo = type->format_type_info;
    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;

    if (NEXTCDF4_type_size(xtype, &size))
        return NC_EBADTYPE;

    hdf = base_hdf_type(xtype);
    if (hdf < 0)
        return NC_EBADTYPE;

    if (H5Tinsert(tinfo->hdf_type, name, offset, hdf) < 0) {
        H5Tclose(hdf);
        return NC_EHDFERR;
    }
    H5Tclose(hdf);

    if ((ret = nc4_field_list_add(type, name, offset, xtype, 0, NULL)))
        return ret;
    return NC_NOERR;
}

/**
 * @brief Insert an array field into a compound type; not implemented.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param offset Field offset in bytes.
 * @param xtype NetCDF type of the field elements.
 * @param ndims Number of array dimensions.
 * @param dim_sizesp Array dimension sizes.
 * @return `NC_ENOTBUILT` always.
 */
int
NEXTCDF4_insert_array_compound(int ncid, nc_type typeid, const char *name,
                               size_t offset, nc_type xtype, int ndims,
                               const int *dim_sizesp)
{
    (void)ncid;(void)typeid;(void)name;(void)offset;(void)xtype;(void)ndims;
    (void)dim_sizesp;
    return NC_ENOTBUILT;
}

/**
 * @brief Inquire a field of a compound type — the
 * `nc_inq_compound_field()` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param fieldid Field index.
 * @param name Destination for the field name.
 * @param offsetp Destination for the field offset.
 * @param fieldtypep Destination for the field type.
 * @param ndimsp Destination for the field rank.
 * @param dim_sizesp Destination for the field dimension sizes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_compound_field(int ncid, nc_type typeid, int fieldid, char *name,
                            size_t *offsetp, nc_type *fieldtypep, int *ndimsp,
                            int *dim_sizesp)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    NC_FIELD_INFO_T *field;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_COMPOUND)
        return NC_EBADTYPE;
    if (fieldid < 0 || (size_t)fieldid >= nclistlength(type->u.c.field))
        return NC_EBADFIELD;
    field = (NC_FIELD_INFO_T *)nclistget(type->u.c.field, fieldid);
    if (!field)
        return NC_EBADFIELD;
    if (name)
        strncpy(name, field->hdr.name, NC_MAX_NAME);
    if (offsetp)
        *offsetp = field->offset;
    if (fieldtypep)
        *fieldtypep = field->nc_typeid;
    if (ndimsp)
        *ndimsp = field->ndims;
    if (dim_sizesp && field->ndims > 0)
        memcpy(dim_sizesp, field->dim_size, field->ndims * sizeof(int));
    return NC_NOERR;
}

/**
 * @brief Get a compound field's index by name — the
 * `nc_inq_compound_fieldindex()` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param fieldidxp Destination for the field index.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_compound_fieldindex(int ncid, nc_type typeid, const char *name,
                                 int *fieldidxp)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    size_t i;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_COMPOUND)
        return NC_EBADTYPE;
    for (i = 0; i < nclistlength(type->u.c.field); i++) {
        NC_FIELD_INFO_T *field = (NC_FIELD_INFO_T *)nclistget(type->u.c.field, i);
        if (field && !strcmp(field->hdr.name, name)) {
            if (fieldidxp)
                *fieldidxp = (int)i;
            return NC_NOERR;
        }
    }
    return NC_EBADFIELD;
}

/**
 * @brief Define a variable-length type — the `nc_def_vlen()` dispatch
 * entry.
 *
 * A vlen variable stores a different number of elements per datum
 * (`hvl_t` handles in memory, heap-allocated on disk). Because vlens
 * have no fields to insert, the `H5Tvlen_create` datatype is
 * committed immediately rather than deferred to `_enddef`.
 *
 * @param ncid NetCDF file identifier.
 * @param name Type name.
 * @param base_typeid Base type of the vlen elements.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type = NULL;
    hid_t base, vlen;
    size_t size;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if ((ret = check_type_write(file, grp)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (nc4_check_dup_name(grp, (char *)name))
        return NC_ENAMEINUSE;

    if (NEXTCDF4_type_size(base_typeid, &size))
        return NC_EBADTYPE;

    base = base_hdf_type(base_typeid);
    if (base < 0)
        return NC_EBADTYPE;

    if ((vlen = H5Tvlen_create(base)) < 0) {
        H5Tclose(base);
        return NC_EHDFERR;
    }
    H5Tclose(base);

    if ((ret = nc4_type_list_add(grp, sizeof(hvl_t), name, &type)))
        goto fail;
    type->nc_type_class = NC_VLEN;
    type->size = sizeof(hvl_t);
    type->u.v.base_nc_typeid = base_typeid;
    if ((ret = commit_hdf_type(grp, type, vlen)))
        goto fail;

    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;

fail:
    H5Tclose(vlen);
    if (type)
        nc4_type_free(type);
    return ret;
}

/**
 * @brief Write a vlen element value (`nc_put_vlen_element`); not
 * implemented.
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Destination for the vlen element.
 * @param len Number of elements.
 * @param data Element data.
 * @return `NC_ENOTBUILT` always.
 */
int
NEXTCDF4_put_vlen_element(int ncid, int typeid, void *vlen_element,
                          size_t len, const void *data)
{
    (void)ncid;(void)typeid;(void)vlen_element;(void)len;(void)data;
    return NC_ENOTBUILT;
}

/**
 * @brief Read a vlen element value (`nc_get_vlen_element`); not
 * implemented.
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Vlen element to read.
 * @param lenp Destination for the element count.
 * @param data Destination for the element data.
 * @return `NC_ENOTBUILT` always.
 */
int
NEXTCDF4_get_vlen_element(int ncid, int typeid, const void *vlen_element,
                          size_t *lenp, void *data)
{
    (void)ncid;(void)typeid;(void)vlen_element;(void)lenp;(void)data;
    return NC_ENOTBUILT;
}

/**
 * @brief Define an enum type — the `nc_def_enum()` dispatch entry.
 *
 * An enum is named constants over an integer base type. Creates a
 * transient `H5Tenum_create` datatype and attaches it uncommitted;
 * members are added by NEXTCDF4_insert_enum() and the type is
 * committed at `_enddef`.
 *
 * @param ncid NetCDF file identifier.
 * @param base_typeid Integer base type id.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_enum(int ncid, nc_type base_typeid, const char *name, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type = NULL;
    hid_t base, en;
    size_t size;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if ((ret = check_type_write(file, grp)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (nc4_check_dup_name(grp, (char *)name))
        return NC_ENAMEINUSE;

    if (NEXTCDF4_type_size(base_typeid, &size))
        return NC_EBADTYPE;

    base = base_hdf_type(base_typeid);
    if (base < 0)
        return NC_EBADTYPE;

    if ((en = H5Tenum_create(base)) < 0) {
        H5Tclose(base);
        return NC_EHDFERR;
    }
    H5Tclose(base);

    if ((ret = nc4_type_list_add(grp, size, name, &type)))
        goto fail;
    type->nc_type_class = NC_ENUM;
    type->size = size;
    type->u.e.base_nc_typeid = base_typeid;
    type->u.e.enum_member = nclistnew();
    if ((ret = attach_hdf_type(type, en)))
        goto fail;

    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;

fail:
    H5Tclose(en);
    if (type)
        nc4_type_free(type);
    return ret;
}

/**
 * @brief Insert a member into an enum type — the `nc_insert_enum()`
 * dispatch entry.
 *
 * The member value arrives as raw bytes in the enum's base type; it
 * is widened to `long long` for `H5Tenum_insert` and stored
 * byte-for-byte in the in-memory record.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param name Member name.
 * @param value Member value.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_insert_enum(int ncid, nc_type typeid, const char *name, const void *value)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    int ret;
    long long v;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;
    tinfo = type->format_type_info;
    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;

    switch (type->u.e.base_nc_typeid) {
    case NC_BYTE: v = *(signed char *)value; break;
    case NC_UBYTE: v = *(unsigned char *)value; break;
    case NC_SHORT: v = *(short *)value; break;
    case NC_USHORT: v = *(unsigned short *)value; break;
    case NC_INT: v = *(int *)value; break;
    case NC_UINT: v = *(unsigned int *)value; break;
    case NC_INT64: v = *(long long *)value; break;
    case NC_UINT64: v = *(unsigned long long *)value; break;
    default: return NC_EBADTYPE;
    }

    if (H5Tenum_insert(tinfo->hdf_type, name, &v) < 0)
        return NC_EHDFERR;

    if ((ret = nc4_enum_member_add(type, type->size, name, value)))
        return ret;
    return NC_NOERR;
}

/**
 * @brief Inquire an enum member by index — the
 * `nc_inq_enum_member()` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param idx Member index.
 * @param name Destination for the member name.
 * @param value Destination for the member value.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_enum_member(int ncid, nc_type typeid, int idx, char *name, void *value)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    NC_ENUM_MEMBER_INFO_T *mem;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;
    if (idx < 0 || (size_t)idx >= nclistlength(type->u.e.enum_member))
        return NC_EINVAL;
    mem = (NC_ENUM_MEMBER_INFO_T *)nclistget(type->u.e.enum_member, idx);
    if (!mem)
        return NC_EINVAL;
    if (name)
        strncpy(name, mem->name, NC_MAX_NAME);
    if (value)
        memcpy(value, mem->value, type->size);
    return NC_NOERR;
}

/**
 * @brief Get an enum member name by value — the `nc_inq_enum_ident()`
 * dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param value Member value.
 * @param identifier Destination for the member name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_enum_ident(int ncid, nc_type typeid, long long value, char *identifier)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    size_t i;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;
    for (i = 0; i < nclistlength(type->u.e.enum_member); i++) {
        NC_ENUM_MEMBER_INFO_T *mem = (NC_ENUM_MEMBER_INFO_T *)nclistget(type->u.e.enum_member, i);
        if (mem) {
            long long v = 0;
            memcpy(&v, mem->value, type->size);
            if (v == value) {
                if (identifier)
                    strncpy(identifier, mem->name, NC_MAX_NAME);
                return NC_NOERR;
            }
        }
    }
    return NC_EINVAL;
}

/**
 * @brief Define an opaque type — the `nc_def_opaque()` dispatch
 * entry.
 *
 * An opaque type is a fixed-size blob the library does not interpret.
 * Like vlens it has no members, so the `H5T_OPAQUE` datatype is
 * committed immediately.
 *
 * @param ncid NetCDF file identifier.
 * @param size Size of the opaque data in bytes.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_opaque(int ncid, size_t size, const char *name, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type = NULL;
    hid_t op;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if ((ret = check_type_write(file, grp)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (nc4_check_dup_name(grp, (char *)name))
        return NC_ENAMEINUSE;

    if ((op = H5Tcreate(H5T_OPAQUE, size)) < 0)
        return NC_EHDFERR;

    if ((ret = nc4_type_list_add(grp, size, name, &type)))
        goto fail;
    type->nc_type_class = NC_OPAQUE;
    type->size = size;
    if ((ret = commit_hdf_type(grp, type, op)))
        goto fail;

    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;

fail:
    H5Tclose(op);
    if (type)
        nc4_type_free(type);
    return ret;
}

/**
 * @brief Commit one previously-attached-but-uncommitted type.
 *
 * Used at `_enddef` for compounds and enums, whose HDF5 datatypes
 * were built transient so members could be inserted during define
 * mode.
 *
 * @param grp Group the type belongs to.
 * @param type In-memory type record.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_one_type(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type)
{
    NEXTCDF4_TYPE_INFO_T *tinfo = type->format_type_info;
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t commit_loc;

    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EHDFERR;
    if (type->committed)
        return NC_NOERR;

    if (!ginfo || ginfo->hdf_group < 0)
        commit_loc = grp->nc4_info->format_file_info ?
            ((NEXTCDF4_FILE_INFO_T *)grp->nc4_info->format_file_info)->rootid : -1;
    else
        commit_loc = ginfo->hdf_group;
    if (commit_loc < 0)
        return NC_EHDFERR;

    if (H5Tcommit2(commit_loc, type->hdr.name, tinfo->hdf_type,
                   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0)
        return NC_EHDFERR;

    type->committed = NC_TRUE;
    return NC_NOERR;
}

/**
 * @brief Recursively commit all uncommitted types in a group tree.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_types_in_grp(NC_GRP_INFO_T *grp)
{
    size_t i;
    int ret;

    for (i = 0; i < ncindexsize(grp->type); i++) {
        NC_TYPE_INFO_T *type = (NC_TYPE_INFO_T *)ncindexith(grp->type, i);
        if (type && !type->committed) {
            if ((ret = commit_one_type(grp, type)))
                return ret;
        }
    }

    for (i = 0; i < ncindexsize(grp->children); i++) {
        NC_GRP_INFO_T *child = (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        if (child && (ret = commit_types_in_grp(child)))
            return ret;
    }
    return NC_NOERR;
}

/**
 * @brief Commit every pending user-defined type — called by
 * NEXTCDF4__enddef() before the metadata writer runs, so committed
 * datatypes exist when variable datasets are created.
 * @param h5 NetCDF-4 file state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_write_types(NC_FILE_INFO_T *h5)
{
    return commit_types_in_grp(h5->root_grp);
}

/**
 * @brief Inquire a user-defined type — the `nc_inq_user_type()`
 * dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param typeid Type id.
 * @param name Destination for the type name.
 * @param sizep Destination for the type size.
 * @param base_typep Destination for the base type id.
 * @param nfieldsp Destination for the number of fields/members.
 * @param classp Destination for the type class.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_user_type(int ncid, nc_type typeid, char *name, size_t *sizep,
                       nc_type *base_typep, size_t *nfieldsp, int *classp)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type)
        return NC_EBADTYPE;
    if (name)
        strncpy(name, type->hdr.name, NC_MAX_NAME);
    if (sizep)
        *sizep = type->size;
    if (classp)
        *classp = type->nc_type_class;
    if (nfieldsp)
        *nfieldsp = nclistlength(type->u.c.field);
    if (base_typep) {
        *base_typep = (type->nc_type_class == NC_ENUM) ? type->u.e.base_nc_typeid :
            (type->nc_type_class == NC_VLEN) ? type->u.v.base_nc_typeid : 0;
    }
    return NC_NOERR;
}

/**
 * @brief Get a type's id by name — the `nc_inq_typeid()` dispatch
 * entry.
 *
 * Type names are visible file-wide in netCDF semantics; the lookup
 * recurses through the group tree via netcdf-c's
 * `nc4_rec_find_named_type`.
 *
 * @param ncid NetCDF file identifier.
 * @param name Type name.
 * @param typeidp Destination for the type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_typeid(int ncid, const char *name, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    int ret;

    /* Atomic type names resolve before any file lookup, as upstream
     * does in NC4_inq_typeid. */
    {
        nc_type atype;
        if (NC4_inq_atomic_typeid(ncid, name, &atype) == NC_NOERR) {
            if (typeidp)
                *typeidp = atype;
            return NC_NOERR;
        }
    }
    if ((ret = get_grp_and_file(ncid, &h5, &grp, NULL)))
        return ret;
    type = nc4_rec_find_named_type(grp, (char *)name);
    if (!type)
        return NC_EBADTYPE;
    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;
}

/**
 * @brief Compare two types for equality — the `nc_inq_type_equal()`
 * dispatch entry.
 *
 * Two types count as equal when they are the same in-memory record or
 * share class and size; field-by-field structural comparison is not
 * attempted.
 *
 * @param ncid1 NetCDF file identifier of the first type.
 * @param typeid1 First type id.
 * @param ncid2 NetCDF file identifier of the second type.
 * @param typeid2 Second type id.
 * @param equalp Destination for the nonzero-if-equal flag.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, nc_type typeid2,
                        int *equalp)
{
    NC_FILE_INFO_T *h5_1, *h5_2;
    NC_TYPE_INFO_T *t1, *t2;
    int ret;

    if (equalp)
        *equalp = 0;
    if ((ret = NEXTCDF4_get_file(ncid1, &h5_1, NULL)))
        return ret;
    if ((ret = NEXTCDF4_get_file(ncid2, &h5_2, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5_1, typeid1, &t1)))
        return ret;
    if ((ret = nc4_find_type(h5_2, typeid2, &t2)))
        return ret;
    if (!t1 || !t2)
        return NC_EBADTYPE;
    if (equalp)
        *equalp = (t1 == t2) || (t1->nc_type_class == t2->nc_type_class &&
                                 t1->size == t2->size);
    return NC_NOERR;
}

/**
 * @brief Get the type ids of the user-defined types in a group — the
 * `nc_inq_typeids()` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param ntypes Destination for the number of types.
 * @param typeids Destination for the type ids.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_typeids(int ncid, int *ntypes, int *typeids)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    size_t i, n;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, NULL)))
        return ret;
    n = ncindexsize(grp->type);
    if (ntypes)
        *ntypes = (int)n;
    if (typeids) {
        for (i = 0; i < n; i++) {
            NC_TYPE_INFO_T *t = (NC_TYPE_INFO_T *)ncindexith(grp->type, i);
            if (t)
                typeids[i] = (int)t->hdr.id;
        }
    }
    return NC_NOERR;
}

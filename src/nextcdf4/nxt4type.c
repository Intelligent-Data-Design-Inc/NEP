/**
 * @file nxt4type.c
 * @brief NEXTCDF-4 user-defined types and NC_STRING support.
 *
 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nxt4internal.h"

/** @return The in-memory type size for any base nc_type. */
int
NEXTCDF4_get_type_size(nc_type xtype, size_t *sizep)
{
    return NEXTCDF4_type_size(xtype, sizep);
}

/** Validate a type for definition. */
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

/** @return The group associated with an ncid. */
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

/** Map an in-memory nc_type to an HDF5 atomic type. */
static hid_t
base_hdf_type(nc_type xtype)
{
    hid_t t;
    if (NEXTCDF4_map_hdf_type(xtype, &t))
        return -1;
    return t;
}

/** Record an HDF5 datatype in the in-memory type without committing. */
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

/** Commit an HDF5 datatype to its group. */
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

int
NEXTCDF4_insert_array_compound(int ncid, nc_type typeid, const char *name,
                               size_t offset, nc_type xtype, int ndims,
                               const int *dim_sizesp)
{
    (void)ncid;(void)typeid;(void)name;(void)offset;(void)xtype;(void)ndims;
    (void)dim_sizesp;
    return NC_ENOTBUILT;
}

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

int
NEXTCDF4_put_vlen_element(int ncid, int typeid, void *vlen_element,
                          size_t len, const void *data)
{
    (void)ncid;(void)typeid;(void)vlen_element;(void)len;(void)data;
    return NC_ENOTBUILT;
}

int
NEXTCDF4_get_vlen_element(int ncid, int typeid, const void *vlen_element,
                          size_t *lenp, void *data)
{
    (void)ncid;(void)typeid;(void)vlen_element;(void)lenp;(void)data;
    return NC_ENOTBUILT;
}

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

/** Commit a single previously-attached (but not committed) type. */
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

/** Recursively commit all uncommitted types in a group tree. */
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

int
NEXTCDF4_write_types(NC_FILE_INFO_T *h5)
{
    return commit_types_in_grp(h5->root_grp);
}

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

int
NEXTCDF4_inq_typeid(int ncid, const char *name, nc_type *typeidp)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    int ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, NULL)))
        return ret;
    type = nc4_rec_find_named_type(grp, (char *)name);
    if (!type)
        return NC_EBADTYPE;
    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    return NC_NOERR;
}

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

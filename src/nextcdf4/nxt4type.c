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
 * mode and write access. They are an enhanced-model feature, so
 * `NC_CLASSIC_MODEL` files reject them; `NC_NETCDF4_MODEL` files
 * permit the standard enhanced-model types because upstream netcdf-c
 * files can represent them.
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
 * @brief Find the user-defined type record an `nc_type` id refers to.
 *
 * Type visibility flows downward in the enhanced model: a type
 * defined in a group is usable in that group and its descendants,
 * never in siblings or ancestors. The search therefore walks the
 * group and its ancestors only; the returned record is borrowed —
 * the caller must not free it or alter its reference count.
 *
 * @param grp Group the type is being used from.
 * @param xtype Type id (must be `NC_FIRSTUSERTYPEID` or above).
 * @param typep Destination for the borrowed record.
 * @return `NC_NOERR` on success, `NC_EBADTYPE` for an atomic id, or
 * `NC_EBADTYPID` when no visible user type has the id.
 */
int
nx4_find_type(NC_GRP_INFO_T *grp, nc_type xtype, NC_TYPE_INFO_T **typep)
{
    NC_GRP_INFO_T *g;
    size_t i;

    if (typep)
        *typep = NULL;
    if (xtype < NC_FIRSTUSERTYPEID)
        return NC_EBADTYPE;
    for (g = grp; g; g = g->parent) {
        if (!g->type)
            continue;
        for (i = 0; i < ncindexsize(g->type); i++) {
            NC_TYPE_INFO_T *t = (NC_TYPE_INFO_T *)ncindexith(g->type, i);
            if (t && (nc_type)t->hdr.id == xtype) {
                if (typep)
                    *typep = t;
                return NC_NOERR;
            }
        }
    }
    return NC_EBADTYPID;
}

/**
 * @brief Resolve the in-memory element size of any supported type.
 *
 * Atomic ids delegate to NEXTCDF4_type_size(); ids at or above
 * `NC_FIRSTUSERTYPEID` resolve through nx4_find_type() and return
 * the type record's size.
 *
 * @param grp Group the type is being used from.
 * @param xtype Type id.
 * @param sizep Destination for the size in bytes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_type_size(NC_GRP_INFO_T *grp, nc_type xtype, size_t *sizep)
{
    NC_TYPE_INFO_T *type;

    if (xtype < NC_FIRSTUSERTYPEID)
        return NEXTCDF4_type_size(xtype, sizep);
    if (nx4_find_type(grp, xtype, &type))
        return NC_EBADTYPE;
    if (sizep)
        *sizep = type->size;
    return NC_NOERR;
}

/**
 * @brief Produce an owned HDF5 datatype with the type's file layout.
 *
 * Atomic types map through NEXTCDF4_map_hdf_type(); user-defined
 * types copy the record's committed (or still-transient) HDF5
 * datatype so the dataset or attribute links to the named type.
 * The caller owns the returned handle and must `H5Tclose` it.
 *
 * @param grp Group the type is being used from.
 * @param xtype Type id.
 * @param typep Destination for an owned HDF5 datatype.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_file_type(NC_GRP_INFO_T *grp, nc_type xtype, hid_t *typep)
{
    NC_TYPE_INFO_T *type;
    NEXTCDF4_TYPE_INFO_T *tinfo;

    if (xtype < NC_FIRSTUSERTYPEID)
        return NEXTCDF4_map_hdf_type(xtype, typep);
    if (!typep)
        return NC_EINVAL;
    if (nx4_find_type(grp, xtype, &type))
        return NC_EBADTYPE;
    tinfo = type->format_type_info;
    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;
    /* Once the type is committed, datasets and attributes must use
     * the committed datatype id itself — a `H5Tcopy()` would be a
     * transient anonymous type and the stored object would no longer
     * share the named type's identity (breaking reload). The
     * incremented reference keeps the caller's `H5Tclose` safe. */
    if (type->committed || H5Tcommitted(tinfo->hdf_type) > 0) {
        if (H5Iinc_ref(tinfo->hdf_type) < 0)
            return NC_EHDFERR;
        *typep = tinfo->hdf_type;
        return NC_NOERR;
    }
    if ((*typep = H5Tcopy(tinfo->hdf_type)) < 0)
        return NC_EHDFERR;
    return NC_NOERR;
}

/**
 * @brief Build an owned HDF5 memory datatype for a user-defined type.
 *
 * The memory layout uses native-endian atomic components so HDF5
 * converts between buffer and file representations:
 * compounds are rebuilt field-by-field at the offsets `nc_inq_compound_field`
 * reports (array fields wrapped in `H5Tarray_create`), enums are
 * rebuilt over the native integer base with the recorded member
 * values, opaques are plain `H5T_OPAQUE`, and vlens wrap the memory
 * base.
 *
 * @param grp Group the type is being used from.
 * @param type In-memory user-type record.
 * @param typep Destination for an owned HDF5 datatype.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
user_mem_type(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type, hid_t *typep)
{
    size_t i;
    hid_t t = -1;
    int ret;

    switch (type->nc_type_class) {
    case NC_COMPOUND: {
        /* The buffer uses the user's declared C layout, so the
         * memory datatype is rebuilt field-by-field at the recorded
         * offsets — including nested compound members, which must be
         * transient rebuilds rather than the committed datatype:
         * doing I/O through the committed datatype's packed native
         * type makes HDF5 insert alignment pads in the file
         * serialization, overflowing the dataset's storage. */
        if ((t = H5Tcreate(H5T_COMPOUND, type->size)) < 0)
            return NC_EHDFERR;
        for (i = 0; i < nclistlength(type->u.c.field); i++) {
            NC_FIELD_INFO_T *field =
                (NC_FIELD_INFO_T *)nclistget(type->u.c.field, i);
            hid_t mt = -1;
            if (!field)
                continue;
            if ((ret = nx4_memory_type(grp, field->nc_typeid, &mt))) {
                H5Tclose(t);
                return ret;
            }
            if (mt >= 0 && field->ndims > 0) {
                hsize_t adims[NC_MAX_VAR_DIMS];
                hid_t at;
                int d;
                for (d = 0; d < field->ndims; d++)
                    adims[d] = (hsize_t)field->dim_size[d];
                at = H5Tarray_create(mt, (unsigned)field->ndims, adims);
                H5Tclose(mt);
                mt = at;
            }
            if (mt < 0 ||
                H5Tinsert(t, field->hdr.name, field->offset, mt) < 0) {
                if (mt >= 0)
                    H5Tclose(mt);
                H5Tclose(t);
                return NC_EHDFERR;
            }
            H5Tclose(mt);
        }
        break;
    }
    case NC_ENUM: {
        hid_t base = -1;
        int close_base = 0;
        hid_t nbase;
        base = NEXTCDF4_memory_type(type->u.e.base_nc_typeid, &close_base);
        if (base < 0)
            return NC_EBADTYPE;
        if ((nbase = H5Tcopy(base)) < 0) {
            if (close_base)
                H5Tclose(base);
            return NC_EHDFERR;
        }
        if (close_base)
            H5Tclose(base);
        if ((t = H5Tenum_create(nbase)) < 0) {
            H5Tclose(nbase);
            return NC_EHDFERR;
        }
        H5Tclose(nbase);
        for (i = 0; i < nclistlength(type->u.e.enum_member); i++) {
            NC_ENUM_MEMBER_INFO_T *mem =
                (NC_ENUM_MEMBER_INFO_T *)nclistget(type->u.e.enum_member, i);
            if (mem && H5Tenum_insert(t, mem->name, mem->value) < 0) {
                H5Tclose(t);
                return NC_EHDFERR;
            }
        }
        break;
    }
    case NC_OPAQUE:
        if ((t = H5Tcreate(H5T_OPAQUE, type->size)) < 0)
            return NC_EHDFERR;
        break;
    case NC_VLEN: {
        hid_t mb = -1;
        if ((ret = nx4_memory_type(grp, type->u.v.base_nc_typeid, &mb)))
            return ret;
        t = H5Tvlen_create(mb);
        H5Tclose(mb);
        if (t < 0)
            return NC_EHDFERR;
        break;
    }
    default:
        return NC_EBADTYPE;
    }
    *typep = t;
    return NC_NOERR;
}

/**
 * @brief Produce an owned HDF5 memory datatype for any supported
 * type.
 *
 * Atomic types delegate to NEXTCDF4_memory_type(); ids at or above
 * `NC_FIRSTUSERTYPEID` resolve through nx4_find_type() and build the
 * memory layout described by user_mem_type(). The caller owns the
 * returned handle and must `H5Tclose` it.
 *
 * @param grp Group the type is being used from.
 * @param xtype Type id.
 * @param typep Destination for an owned HDF5 datatype.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_memory_type(NC_GRP_INFO_T *grp, nc_type xtype, hid_t *typep)
{
    NC_TYPE_INFO_T *type;
    hid_t t;
    int close_t = 0;

    if (!typep)
        return NC_EINVAL;
    if (xtype < NC_FIRSTUSERTYPEID) {
        if ((t = NEXTCDF4_memory_type(xtype, &close_t)) < 0)
            return NC_EBADTYPE;
        if (!close_t) {
            hid_t copy = H5Tcopy(t);
            if (copy < 0)
                return NC_EHDFERR;
            t = copy;
        }
        *typep = t;
        return NC_NOERR;
    }
    if (nx4_find_type(grp, xtype, &type))
        return NC_EBADTYPE;
    return user_mem_type(grp, type, typep);
}


/**
 * @brief Resolve an HDF5 datatype to a registered committed user
 * type by object identity.
 *
 * Datasets and attributes created from a committed datatype keep a
 * reference to that named object; comparing the HDF5 object token of
 * @p htype against every committed type visible from @p grp (the
 * group and its ancestors) recovers the exact `NC_TYPE_INFO_T` —
 * matching on `H5Tequal` alone would conflate distinct named types
 * with equal shapes.
 *
 * @param grp Group the datatype is being used from.
 * @param htype HDF5 datatype (need not be committed itself).
 * @param typep Destination for the borrowed type record.
 * @return `NC_NOERR` on success, `NC_EBADTYPID` when no visible
 * committed type is the same object.
 */
int
nx4_hdf_type_match(NC_GRP_INFO_T *grp, hid_t htype,
                   NC_TYPE_INFO_T **typep)
{
    NC_GRP_INFO_T *g;
    H5O_info2_t info;
    size_t i;
    int ret = NC_EBADTYPID;

    if (typep)
        *typep = NULL;
    if (htype < 0 || !typep)
        return NC_EBADTYPE;

    if (H5Oget_info3(htype, &info, H5O_INFO_BASIC) >= 0) {
        for (g = grp; g && ret; g = g->parent) {
            if (!g->type)
                continue;
            for (i = 0; i < ncindexsize(g->type); i++) {
                NC_TYPE_INFO_T *t = (NC_TYPE_INFO_T *)ncindexith(g->type, i);
                NEXTCDF4_TYPE_INFO_T *tinfo;
                H5O_info2_t tinfo_o;
                int cmp = 1;
                if (!t || !(tinfo = t->format_type_info) ||
                    tinfo->hdf_type < 0 || H5Tcommitted(tinfo->hdf_type) <= 0)
                    continue;
                if (H5Oget_info3(tinfo->hdf_type, &tinfo_o, H5O_INFO_BASIC) < 0)
                    continue;
                if (H5Otoken_cmp(htype, &info.token, &tinfo_o.token, &cmp) >= 0 &&
                    cmp == 0) {
                    *typep = t;
                    return NC_NOERR;
                }
            }
        }
    }
    /* Fallback: an object created from a transient copy of the named
     * type stores an anonymous datatype with no shared token. A
     * structural `H5Tequal` match still resolves it to the visible
     * registered type; ambiguous shapes resolve to the first match,
     * which is the scoping rule's closest definition anyway. */
    for (g = grp; g; g = g->parent) {
        if (!g->type)
            continue;
        for (i = 0; i < ncindexsize(g->type); i++) {
            NC_TYPE_INFO_T *t = (NC_TYPE_INFO_T *)ncindexith(g->type, i);
            NEXTCDF4_TYPE_INFO_T *tinfo;
            if (!t || !(tinfo = t->format_type_info) ||
                tinfo->hdf_type < 0)
                continue;
            if (H5Tequal(tinfo->hdf_type, htype) > 0) {
                *typep = t;
                return NC_NOERR;
            }
        }
    }
    return ret;
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
 * @brief Release a type's NEXTCDF-4-private record and its HDF5
 * datatype handle, if any.
 *
 * `nc4_type_free()` knows nothing about `format_type_info`, so every
 * definition failure path and the file-close walk must dispose of it
 * separately. Safe on a type with no private record.
 *
 * @param type In-memory type record.
 */
void
nx4_free_type_info(NC_TYPE_INFO_T *type)
{
    NEXTCDF4_TYPE_INFO_T *tinfo;

    if (!type || !(tinfo = type->format_type_info))
        return;
    H5E_BEGIN_TRY {
        if (tinfo->hdf_type >= 0)
            H5Tclose(tinfo->hdf_type);
    } H5E_END_TRY;
    free(tinfo);
    type->format_type_info = NULL;
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
    if (type) {
        nx4_free_type_info(type);
        nc4_type_free(type);
    }
    return ret;
}

/**
 * @brief Check whether @p target appears anywhere in @p t's member
 * graph — the cycle check for compound insertion and vlen bases.
 *
 * Member types form a DAG by construction (every insert runs this
 * check), so a depth-first walk through user-typed fields and vlen
 * bases is enough; @p seen bounds the recursion on a malformed graph.
 *
 * @param h5 NetCDF-4 file state used to resolve member type ids.
 * @param t Type record whose member graph is walked.
 * @param target Type record being searched for.
 * @param seen Scratch list of already-visited records (may be NULL;
 * allocated lazily by the caller).
 * @return Nonzero when @p target is reachable from @p t's members.
 */
static int
type_graph_contains(NC_FILE_INFO_T *h5, NC_TYPE_INFO_T *t,
                    NC_TYPE_INFO_T *target, NClist *seen)
{
    size_t i;

    if (!t)
        return 0;
    if (t == target)
        return 1;
    for (i = 0; i < nclistlength(seen); i++)
        if (nclistget(seen, i) == t)
            return 0;
    nclistpush(seen, t);
    if (t->nc_type_class == NC_VLEN) {
        /* A vlen's base is part of its type graph; nested vlens and
         * vlen-of-compound bases recurse the same way. */
        NC_TYPE_INFO_T *m = NULL;
        if (t->u.v.base_nc_typeid >= NC_FIRSTUSERTYPEID &&
            !nc4_find_type(h5, t->u.v.base_nc_typeid, &m) && m &&
            type_graph_contains(h5, m, target, seen))
            return 1;
        return 0;
    }
    if (t->nc_type_class != NC_COMPOUND)
        return 0;
    for (i = 0; i < nclistlength(t->u.c.field); i++) {
        NC_FIELD_INFO_T *f = (NC_FIELD_INFO_T *)nclistget(t->u.c.field, i);
        NC_TYPE_INFO_T *m = NULL;
        if (f && f->nc_typeid >= NC_FIRSTUSERTYPEID &&
            !nc4_find_type(h5, f->nc_typeid, &m) && m &&
            type_graph_contains(h5, m, target, seen))
            return 1;
    }
    return 0;
}

/**
 * @brief Check whether @p t's type graph contains a string or
 * reference member anywhere.
 *
 * `NC_STRING` values and `NC_STD_REF` tokens cannot legally nest
 * inside compounds or vlens (string member support is deferred;
 * references stay unsupported inside containers). The walk covers
 * compound fields, array elements and vlen bases recursively;
 * @p seen bounds the recursion on a malformed graph.
 *
 * @param h5 NetCDF-4 file state used to resolve member type ids.
 * @param t Type record whose member graph is walked.
 * @param seen Scratch list of already-visited records.
 * @return Nonzero when a forbidden type is reachable from @p t.
 */
static int
type_graph_has_forbidden(NC_FILE_INFO_T *h5, NC_TYPE_INFO_T *t,
                         NClist *seen)
{
    size_t i;

    if (!t)
        return 0;
    if (t->nc_type_class == NC_STRING)
        return 1;
    for (i = 0; i < nclistlength(seen); i++)
        if (nclistget(seen, i) == t)
            return 0;
    nclistpush(seen, t);
    if (t->nc_type_class == NC_VLEN) {
        if (t->u.v.base_nc_typeid == NC_STRING ||
            t->u.v.base_nc_typeid == NC_STD_REF)
            return 1;
        if (t->u.v.base_nc_typeid >= NC_FIRSTUSERTYPEID) {
            NC_TYPE_INFO_T *m = NULL;
            if (!nc4_find_type(h5, t->u.v.base_nc_typeid, &m) && m &&
                type_graph_has_forbidden(h5, m, seen))
                return 1;
        }
        return 0;
    }
    if (t->nc_type_class != NC_COMPOUND)
        return 0;
    for (i = 0; i < nclistlength(t->u.c.field); i++) {
        NC_FIELD_INFO_T *f = (NC_FIELD_INFO_T *)nclistget(t->u.c.field, i);
        NC_TYPE_INFO_T *m = NULL;
        if (!f)
            continue;
        if (f->nc_typeid == NC_STRING || f->nc_typeid == NC_STD_REF)
            return 1;
        if (f->nc_typeid >= NC_FIRSTUSERTYPEID &&
            !nc4_find_type(h5, f->nc_typeid, &m) && m &&
            type_graph_has_forbidden(h5, m, seen))
            return 1;
    }
    return 0;
}

/**
 * @brief Reject a type graph containing strings or references.
 * @param h5 NetCDF-4 file state.
 * @param t Type record to inspect.
 * @return `NC_NOERR` when the graph is clean, `NC_EBADTYPE` otherwise.
 */
static int
check_graph_clean(NC_FILE_INFO_T *h5, NC_TYPE_INFO_T *t)
{
    NClist *seen;
    int bad;

    if (!(seen = nclistnew()))
        return NC_ENOMEM;
    bad = type_graph_has_forbidden(h5, t, seen);
    nclistfree(seen);
    return bad ? NC_EBADTYPE : NC_NOERR;
}

/**
 * @brief Validate a compound member's element type.
 *
 * Atomic members use the file's compatibility rules with two fixed
 * exclusions — `NC_STRING` (Sprint 1P) and `NC_STD_REF` (documented
 * restriction). User-defined members may be any class except
 * `NC_STRING` — compounds, enums, opaques and vlens all nest — but
 * their member graph must contain no string or reference anywhere,
 * and must not contain the container itself.
 *
 * @param file NEXTCDF-4 file state.
 * @param grp Group the container type lives in (visibility scope).
 * @param xtype Candidate member element type.
 * @param container Compound record receiving the member.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
member_type_ok(NEXTCDF4_FILE_INFO_T *file, NC_GRP_INFO_T *grp,
               nc_type xtype, NC_TYPE_INFO_T *container)
{
    NC_TYPE_INFO_T *m = NULL;
    NClist *seen;
    int cyclic, ret;

    if (xtype < NC_FIRSTUSERTYPEID) {
        if (xtype == NC_STRING || xtype == NC_STD_REF)
            return NC_EBADTYPE;
        return NEXTCDF4_check_atomic_type(file, xtype);
    }
    if (nx4_find_type(grp, xtype, &m) || !m)
        return NC_EBADTYPE;
    if (m->nc_type_class == NC_STRING)
        return NC_EBADTYPE;
    if ((ret = check_graph_clean(grp->nc4_info, m)))
        return ret;
    if (!(seen = nclistnew()))
        return NC_ENOMEM;
    cyclic = type_graph_contains(grp->nc4_info, m, container, seen);
    nclistfree(seen);
    if (cyclic)
        return NC_EBADTYPE;
    return NC_NOERR;
}

/**
 * @brief Build the HDF5 datatype for one compound field.
 *
 * Resolves the element type (atomic via NEXTCDF4_map_hdf_type, or a
 * user-defined type's own HDF5 datatype) and wraps it in
 * `H5Tarray_create2` when the field is an array. The returned id is
 * owned by the caller only when `*ownedp` is nonzero; a borrowed
 * committed member datatype stays owned by its type record.
 *
 * @param grp Group resolving member type ids.
 * @param f Field record.
 * @param typep Destination for the HDF5 datatype.
 * @param ownedp Destination for the ownership flag.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
field_hdf_type(NC_GRP_INFO_T *grp, const NC_FIELD_INFO_T *f, hid_t *typep,
               int *ownedp)
{
    hid_t base = -1;
    int owned = 1;

    if (f->nc_typeid < NC_FIRSTUSERTYPEID) {
        if ((base = base_hdf_type(f->nc_typeid)) < 0)
            return NC_EBADTYPE;
    } else {
        NC_TYPE_INFO_T *m = NULL;
        NEXTCDF4_TYPE_INFO_T *mi;
        if (nx4_find_type(grp, f->nc_typeid, &m) || !m ||
            !(mi = m->format_type_info) || mi->hdf_type < 0)
            return NC_EBADTYPE;
        base = mi->hdf_type;
        owned = 0;
    }
    if (f->ndims > 0) {
        hsize_t adims[NC_MAX_VAR_DIMS];
        hid_t at;
        int d;
        for (d = 0; d < f->ndims; d++)
            adims[d] = (hsize_t)f->dim_size[d];
        at = H5Tarray_create2(base, (unsigned)f->ndims, adims);
        if (at < 0) {
            if (owned && base >= 0)
                H5Tclose(base);
            return NC_EHDFERR;
        }
        if (owned)
            H5Tclose(base);
        base = at;
        owned = 1;
    }
    *typep = base;
    *ownedp = owned;
    return NC_NOERR;
}

/**
 * @brief Insert a scalar field into a compound type — the
 * `nc_insert_compound()` dispatch entry.
 *
 * Thin wrapper over NEXTCDF4_insert_array_compound() with rank zero,
 * matching upstream's delegation.
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
    return NEXTCDF4_insert_array_compound(ncid, typeid, name, offset,
                                        xtype, 0, NULL);
}

/**
 * @brief Insert a scalar or fixed-size array field into a compound
 * type — the `nc_insert_array_compound()` dispatch entry.
 *
 * Members may be atomic types (excluding `NC_STRING` and
 * `NC_STD_REF`) or visible fixed-size user-defined types — compounds
 * nest. Validation happens entirely before `H5Tinsert` and the
 * metadata update, so a rejected call changes nothing.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param offset Field offset in bytes.
 * @param xtype NetCDF type of the field elements.
 * @param ndims Number of array dimensions; 0 inserts a scalar.
 * @param dim_sizesp Array dimension sizes (required when @p ndims>0).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_insert_array_compound(int ncid, nc_type typeid, const char *name,
                               size_t offset, nc_type xtype, int ndims,
                               const int *dim_sizesp)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_TYPE_INFO_T *type;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    NC_FIELD_INFO_T tmp;
    hid_t hdf = -1;
    int owned = 0;
    size_t esize, total = 1;
    int i, ret;

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode)
        return NC_ENOTINDEFINE;
    if (!name)
        return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_COMPOUND)
        return NC_EBADTYPE;
    /* A committed datatype is immutable in HDF5; member inserts must
     * happen before `_enddef` commits it. */
    if (type->committed)
        return NC_ENOTINDEFINE;
    tinfo = type->format_type_info;
    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;

    /* Array shape: rank zero is a scalar member. */
    if (ndims < 0 || ndims > NC_MAX_VAR_DIMS)
        return NC_EINVAL;
    if (ndims > 0) {
        if (!dim_sizesp)
            return NC_EINVAL;
        for (i = 0; i < ndims; i++) {
            if (dim_sizesp[i] <= 0)
                return NC_EINVAL;
            if (total > SIZE_MAX / (size_t)dim_sizesp[i])
                return NC_EINVAL;
            total *= (size_t)dim_sizesp[i];
        }
    }

    /* The member's element type must be legal here: standard atomic
     * (minus strings/references) or a visible fixed-size user type
     * that does not contain the container. */
    if ((ret = member_type_ok(file, grp, xtype, type)))
        return ret;
    if (nx4_type_size(grp, xtype, &esize))
        return NC_EBADTYPE;
    if (total > SIZE_MAX / esize)
        return NC_EINVAL;
    total *= esize;
    /* The field must fit inside the declared compound size. */
    if (offset > type->size || total > type->size - offset)
        return NC_EINVAL;
    for (i = 0; (size_t)i < nclistlength(type->u.c.field); i++) {
        NC_FIELD_INFO_T *f =
            (NC_FIELD_INFO_T *)nclistget(type->u.c.field, i);
        if (f && !strcmp(f->hdr.name, name))
            return NC_ENAMEINUSE;
    }

    /* All validation done — build the member datatype, insert, and
     * record the field. field_hdf_type borrows the member's own
     * datatype for user-defined elements so a committed member embeds
     * a shared reference. */
    memset(&tmp, 0, sizeof(tmp));
    tmp.nc_typeid = xtype;
    tmp.ndims = ndims;
    tmp.dim_size = (int *)dim_sizesp;
    if ((ret = field_hdf_type(grp, &tmp, &hdf, &owned)))
        return ret;
    if (H5Tinsert(tinfo->hdf_type, name, offset, hdf) < 0) {
        ret = NC_EHDFERR;
        goto done;
    }
    if ((ret = nc4_field_list_add(type, name, offset, xtype, ndims,
                                 dim_sizesp)))
        goto done;
    /* A member that is (transitively) variable-sized makes the whole
     * compound variable-sized — nc_copy_data/nc_reclaim_data need the
     * flag to walk its vlen fields. */
    NC4_recheck_varsize(type, xtype);

done:
    if (owned && hdf >= 0)
        H5Tclose(hdf);
    return ret;
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
 * (`nc_vlen_t` handles in memory, heap-allocated on disk). The base
 * type may be any supported atomic type except `NC_STRING` and
 * `NC_STD_REF`, or a visible user-defined type — compounds, enums,
 * opaques and other vlens all nest.
 *
 * When the base is still uncommitted (a compound whose fields may
 * still grow), the vlen's datatype stays transient and commit is
 * deferred to `_enddef`, where commit_type_tree() commits the base
 * first and rebuilds the vlen around it; vlens over committed bases
 * commit immediately, like opaques.
 *
 * The public `nc_vlen_t` descriptor and HDF5's `hvl_t` must have the
 * same layout — I/O moves descriptors verbatim.
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
    NC_TYPE_INFO_T *base_t = NULL;
    NEXTCDF4_TYPE_INFO_T *bi;
    hid_t base = -1, vlen = -1;
    int base_owned = 0;
    int ret;

    _Static_assert(sizeof(nc_vlen_t) == sizeof(hvl_t) &&
                   offsetof(nc_vlen_t, len) == offsetof(hvl_t, len) &&
                   offsetof(nc_vlen_t, p) == offsetof(hvl_t, p),
                   "nc_vlen_t and hvl_t layouts differ");

    if ((ret = get_grp_and_file(ncid, &h5, &grp, &file)))
        return ret;
    if ((ret = check_type_write(file, grp)))
        return ret;
    if (!name) return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if (nc4_check_dup_name(grp, (char *)name))
        return NC_ENAMEINUSE;

    /* Resolve the base: an atomic type under the file's compatibility
     * rules, or a visible user-defined type whose graph is free of
     * strings and references. A vlen cannot reference itself, so no
     * cycle is possible here. */
    if (base_typeid < NC_FIRSTUSERTYPEID) {
        if (base_typeid == NC_STRING || base_typeid == NC_STD_REF)
            return NC_EBADTYPE;
        if ((ret = NEXTCDF4_check_atomic_type(file, base_typeid)))
            return ret;
        if ((base = base_hdf_type(base_typeid)) < 0)
            return NC_EBADTYPE;
        base_owned = 1;
    } else {
        if (nx4_find_type(grp, base_typeid, &base_t) || !base_t)
            return NC_EBADTYPE;
        if (base_t->nc_type_class == NC_STRING)
            return NC_EBADTYPE;
        if ((ret = check_graph_clean(h5, base_t)))
            return ret;
        bi = base_t->format_type_info;
        if (!bi || bi->hdf_type < 0)
            return NC_EBADTYPE;
        base = bi->hdf_type; /* borrowed */
    }

    if ((vlen = H5Tvlen_create(base)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }

    if ((ret = nc4_type_list_add(grp, sizeof(nc_vlen_t), name, &type)))
        goto fail;
    type->nc_type_class = NC_VLEN;
    type->size = sizeof(nc_vlen_t);
    type->u.v.base_nc_typeid = base_typeid;
    NC4_set_varsize(type);
    if (base_t && !base_t->committed)
        ret = attach_hdf_type(type, vlen);
    else
        ret = commit_hdf_type(grp, type, vlen);
    if (ret)
        goto fail;

    if (typeidp)
        *typeidp = (nc_type)type->hdr.id;
    if (base_owned && base >= 0)
        H5Tclose(base);
    return NC_NOERR;

fail:
    if (base_owned && base >= 0)
        H5Tclose(base);
    H5Tclose(vlen);
    if (type) {
        nx4_free_type_info(type);
        nc4_type_free(type);
    }
    return ret;
}

/**
 * @brief Resolve a vlen type record from a typeid — shared prologue
 * for the vlen element helpers.
 * @param ncid NetCDF file identifier.
 * @param typeid Type id (must name a `NC_VLEN` user type).
 * @param h5p Destination for the NetCDF-4 file state.
 * @param typep Destination for the borrowed type record.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
find_vlen_type(int ncid, nc_type typeid, NC_FILE_INFO_T **h5p,
               NC_TYPE_INFO_T **typep)
{
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, NULL)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (h5p)
        *h5p = h5;
    if (!type || type->nc_type_class != NC_VLEN)
        return NC_EBADTYPE;
    if (typep)
        *typep = type;
    return NC_NOERR;
}

/**
 * @brief Write a vlen element value — the `nc_put_vlen_element()`
 * dispatch entry.
 *
 * Fills the caller's `nc_vlen_t` descriptor with the element count and
 * a **borrowed** pointer to @p data — nothing is copied and no
 * ownership is taken. The payload must stay valid until the write it
 * feeds (typically `nc_put_vara`) has completed.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Destination `nc_vlen_t` descriptor.
 * @param len Number of elements.
 * @param data Element data (required when @p len is nonzero).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_put_vlen_element(int ncid, int typeid, void *vlen_element,
                          size_t len, const void *data)
{
    nc_vlen_t *vl = (nc_vlen_t *)vlen_element;
    int ret;

    if (!vlen_element || (len && !data))
        return NC_EINVAL;
    if ((ret = find_vlen_type(ncid, typeid, NULL, NULL)))
        return ret;
    vl->len = len;
    vl->p = (void *)data;
    return NC_NOERR;
}

/**
 * @brief Read a vlen element value — the `nc_get_vlen_element()`
 * dispatch entry.
 *
 * Reports the descriptor's element count and copies the payload into
 * the caller's buffer using the base type's real in-memory size. The
 * copy is shallow: for a vlen whose base is itself dynamic (a nested
 * vlen, or a compound containing vlen members) the copied descriptors
 * still alias the source element's inner payloads. Those aliases must
 * not be reclaimed independently — callers wanting an owned deep copy
 * use `nc_copy_data()`.
 *
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Source `nc_vlen_t` descriptor.
 * @param lenp Destination for the element count (required).
 * @param data Destination for the element data (required when the
 * descriptor is non-empty).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_get_vlen_element(int ncid, int typeid, const void *vlen_element,
                          size_t *lenp, void *data)
{
    const nc_vlen_t *vl = (const nc_vlen_t *)vlen_element;
    NC_FILE_INFO_T *h5;
    NC_TYPE_INFO_T *type;
    NC_GRP_INFO_T *grp;
    size_t base_size;
    int ret;

    if (!lenp || !vlen_element)
        return NC_EINVAL;
    if ((ret = find_vlen_type(ncid, typeid, &h5, &type)))
        return ret;
    *lenp = vl->len;
    if (!vl->len)
        return NC_NOERR;
    if (!data || !vl->p)
        return NC_EINVAL;
    if ((ret = nc4_find_nc4_grp(ncid, &grp)))
        return ret;
    if ((ret = nx4_type_size(grp, type->u.v.base_nc_typeid, &base_size)))
        return ret;
    if (vl->len > SIZE_MAX / base_size)
        return NC_EINVAL;
    memcpy(data, vl->p, vl->len * base_size);
    return NC_NOERR;
}

/* Enum bases are restricted to integer types, upstream-style. */
#define NX4_ENUM_BASE_OK(t) \
    ((t) == NC_BYTE || (t) == NC_CHAR || (t) == NC_SHORT || \
     (t) == NC_INT || (t) == NC_UBYTE || (t) == NC_USHORT || \
     (t) == NC_UINT || (t) == NC_INT64 || (t) == NC_UINT64)

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

    /* Upstream restricts enum bases to integer types; in
     * netcdf-4-model files they are always standard anyway, but the
     * atomic check keeps the rule in one place. */
    if (!NX4_ENUM_BASE_OK(base_typeid))
        return NC_EBADTYPE;
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
    if (type) {
        nx4_free_type_info(type);
        nc4_type_free(type);
    }
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
    NEXTCDF4_FILE_INFO_T *file;
    NC_TYPE_INFO_T *type;
    NEXTCDF4_TYPE_INFO_T *tinfo;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode)
        return NC_ENOTINDEFINE;
    if (!name || !value)
        return NC_EINVAL;
    if ((ret = NC_check_name(name)))
        return ret;
    if ((ret = nc4_find_type(h5, typeid, &type)))
        return ret;
    if (!type || type->nc_type_class != NC_ENUM)
        return NC_EBADTYPE;
    if (type->committed)
        return NC_ENOTINDEFINE;
    tinfo = type->format_type_info;
    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;

    /* The value already carries the base type's in-memory
     * representation — `H5Tenum_insert` reads exactly the parent
     * datatype's size, preserving every bit of signed and unsigned
     * 64-bit members alike. */
    if (H5Tenum_insert(tinfo->hdf_type, name, (void *)value) < 0)
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
    /* Extend the 8-byte inquiry argument to the member's stored
     * representation: sign-extend for signed bases, zero-extend for
     * unsigned, so all unsigned 64-bit bits compare correctly. */
    {
        unsigned long long needle = (unsigned long long)value;
        int signed_base;
        switch (type->u.e.base_nc_typeid) {
        case NC_UBYTE: case NC_USHORT: case NC_UINT: case NC_UINT64:
            signed_base = 0;
            break;
        default:
            signed_base = 1;
        }
        for (i = 0; i < nclistlength(type->u.e.enum_member); i++) {
            NC_ENUM_MEMBER_INFO_T *mem = (NC_ENUM_MEMBER_INFO_T *)nclistget(type->u.e.enum_member, i);
            if (mem) {
                unsigned long long v = 0;
                memcpy(&v, mem->value, type->size);
                if (signed_base && type->size < sizeof(v) &&
                    (v & (1ULL << (type->size * 8 - 1))))
                    v |= ~((1ULL << (type->size * 8)) - 1);
                if (v == needle) {
                    if (identifier)
                        strncpy(identifier, mem->name, NC_MAX_NAME);
                    return NC_NOERR;
                }
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
    if (type) {
        nx4_free_type_info(type);
        nc4_type_free(type);
    }
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
 * @brief Rebuild a compound's transient HDF5 datatype from its field
 * metadata.
 *
 * Called at commit time, after the compound's dependency member types
 * have themselves been committed: building members from the now-
 * committed HDF5 datatypes makes the stored members shared datatype
 * references (as upstream netcdf-c writes), rather than anonymous
 * copies of what were transient ids at insert time.
 *
 * @param grp Group the type belongs to (visibility scope).
 * @param type Compound type record.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
rebuild_compound_transient(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type)
{
    NEXTCDF4_TYPE_INFO_T *tinfo = type->format_type_info;
    hid_t newt = -1;
    size_t i;
    int ret;

    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;
    if ((newt = H5Tcreate(H5T_COMPOUND, type->size)) < 0)
        return NC_EHDFERR;
    for (i = 0; i < nclistlength(type->u.c.field); i++) {
        NC_FIELD_INFO_T *f =
            (NC_FIELD_INFO_T *)nclistget(type->u.c.field, i);
        hid_t mt = -1;
        int owned = 0;
        if (!f)
            continue;
        if ((ret = field_hdf_type(grp, f, &mt, &owned)))
            goto fail;
        if (H5Tinsert(newt, f->hdr.name, f->offset, mt) < 0)
            ret = NC_EHDFERR;
        if (owned && mt >= 0)
            H5Tclose(mt);
        if (ret)
            goto fail;
    }
    H5Tclose(tinfo->hdf_type);
    tinfo->hdf_type = newt;
    return NC_NOERR;

fail:
    if (newt >= 0)
        H5Tclose(newt);
    return ret;
}

/**
 * @brief Find the group whose type index owns a record.
 * @param grp Group to walk recursively.
 * @param type Type record to locate.
 * @return Owning group, or NULL.
 */
static NC_GRP_INFO_T *
find_type_owner(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type)
{
    size_t i;

    for (i = 0; i < ncindexsize(grp->type); i++)
        if (ncindexith(grp->type, i) == (NC_OBJ *)type)
            return grp;
    for (i = 0; i < ncindexsize(grp->children); i++) {
        NC_GRP_INFO_T *child =
            (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        NC_GRP_INFO_T *found;
        if (child && (found = find_type_owner(child, type)))
            return found;
    }
    return NULL;
}

/**
 * @brief Rebuild a vlen's transient HDF5 datatype around its
 * committed base.
 *
 * Called at commit time when the vlen's base is a user-defined type:
 * building the vlen from the now-committed base datatype makes the
 * stored base a shared datatype reference (as upstream netcdf-c
 * writes), rather than an anonymous copy of the transient id the
 * base had when `nc_def_vlen` ran.
 *
 * @param grp Group the type belongs to (visibility scope).
 * @param type Vlen type record.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
rebuild_vlen_transient(NC_GRP_INFO_T *grp, NC_TYPE_INFO_T *type)
{
    NEXTCDF4_TYPE_INFO_T *tinfo = type->format_type_info;
    NC_TYPE_INFO_T *m = NULL;
    NEXTCDF4_TYPE_INFO_T *mi;
    hid_t newt;

    if (!tinfo || tinfo->hdf_type < 0)
        return NC_EBADTYPE;
    if (type->u.v.base_nc_typeid < NC_FIRSTUSERTYPEID)
        return NC_NOERR;
    if (nc4_find_type(grp->nc4_info, type->u.v.base_nc_typeid, &m) || !m)
        return NC_EBADTYPE;
    mi = m->format_type_info;
    if (!mi || mi->hdf_type < 0)
        return NC_EBADTYPE;
    if ((newt = H5Tvlen_create(mi->hdf_type)) < 0)
        return NC_EHDFERR;
    H5Tclose(tinfo->hdf_type);
    tinfo->hdf_type = newt;
    return NC_NOERR;
}

/**
 * @brief Commit one type after its dependencies.
 *
 * For compounds, every uncommitted user-defined member type is
 * committed first (in its own group), then the container's transient
 * datatype is rebuilt so stored members are shared references to the
 * committed datatypes. A vlen whose base is a user type gets the
 * same treatment: commit the base first, then rebuild the vlen
 * around it. Cycles cannot form — insertion rejects them — so plain
 * recursion is sufficient.
 *
 * @param h5 NetCDF-4 file state.
 * @param grp Group the type belongs to.
 * @param type Type record.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_type_tree(NC_FILE_INFO_T *h5, NC_GRP_INFO_T *grp,
                 NC_TYPE_INFO_T *type)
{
    size_t i;
    int ret;

    if (type->committed)
        return NC_NOERR;
    if (type->nc_type_class == NC_COMPOUND) {
        for (i = 0; i < nclistlength(type->u.c.field); i++) {
            NC_FIELD_INFO_T *f =
                (NC_FIELD_INFO_T *)nclistget(type->u.c.field, i);
            NC_TYPE_INFO_T *m = NULL;
            NC_GRP_INFO_T *og;
            if (!f || f->nc_typeid < NC_FIRSTUSERTYPEID)
                continue;
            if (nc4_find_type(h5, f->nc_typeid, &m) || !m)
                return NC_EBADTYPE;
            if (m->committed)
                continue;
            og = find_type_owner(h5->root_grp, m);
            if ((ret = commit_type_tree(h5, og ? og : grp, m)))
                return ret;
        }
        if ((ret = rebuild_compound_transient(grp, type)))
            return ret;
    } else if (type->nc_type_class == NC_VLEN) {
        NC_TYPE_INFO_T *m = NULL;
        if (type->u.v.base_nc_typeid >= NC_FIRSTUSERTYPEID) {
            if (nc4_find_type(h5, type->u.v.base_nc_typeid, &m) || !m)
                return NC_EBADTYPE;
            if (!m->committed) {
                NC_GRP_INFO_T *og = find_type_owner(h5->root_grp, m);
                if ((ret = commit_type_tree(h5, og ? og : grp, m)))
                    return ret;
            }
            if ((ret = rebuild_vlen_transient(grp, type)))
                return ret;
        }
    }
    return commit_one_type(grp, type);
}

/**
 * @brief Recursively commit all uncommitted types in a group tree.
 * @param h5 NetCDF-4 file state.
 * @param grp Group.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_types_in_grp(NC_FILE_INFO_T *h5, NC_GRP_INFO_T *grp)
{
    size_t i;
    int ret;

    for (i = 0; i < ncindexsize(grp->type); i++) {
        NC_TYPE_INFO_T *type = (NC_TYPE_INFO_T *)ncindexith(grp->type, i);
        if (type && (ret = commit_type_tree(h5, grp, type)))
            return ret;
    }

    for (i = 0; i < ncindexsize(grp->children); i++) {
        NC_GRP_INFO_T *child = (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        if (child && (ret = commit_types_in_grp(h5, child)))
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
    return commit_types_in_grp(h5, h5->root_grp);
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
        *nfieldsp = (type->nc_type_class == NC_COMPOUND) ?
            nclistlength(type->u.c.field) :
            (type->nc_type_class == NC_ENUM) ?
            nclistlength(type->u.e.enum_member) : 0;
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
 * @brief Structural comparison for `nc_inq_type_equal()`.
 *
 * Compares class, size and contents recursively — member type ids are
 * file-local, so user-defined members compare by structure through
 * each side's own type list, never by numeric id or by name.
 *
 * @param h5_1 File state of the first type.
 * @param t1 First type record.
 * @param h5_2 File state of the second type.
 * @param t2 Second type record.
 * @return Nonzero when the types are structurally equal.
 */
static int
types_equal(NC_FILE_INFO_T *h5_1, NC_TYPE_INFO_T *t1,
            NC_FILE_INFO_T *h5_2, NC_TYPE_INFO_T *t2)
{
    size_t i;

    if (t1 == t2)
        return 1;
    if (t1->nc_type_class != t2->nc_type_class || t1->size != t2->size)
        return 0;
    switch (t1->nc_type_class) {
    case NC_COMPOUND: {
        if (nclistlength(t1->u.c.field) != nclistlength(t2->u.c.field))
            return 0;
        for (i = 0; i < nclistlength(t1->u.c.field); i++) {
            NC_FIELD_INFO_T *f1 =
                (NC_FIELD_INFO_T *)nclistget(t1->u.c.field, i);
            NC_FIELD_INFO_T *f2 =
                (NC_FIELD_INFO_T *)nclistget(t2->u.c.field, i);
            int d;
            if (!f1 || !f2 || strcmp(f1->hdr.name, f2->hdr.name) ||
                f1->offset != f2->offset || f1->ndims != f2->ndims)
                return 0;
            for (d = 0; d < f1->ndims; d++)
                if (f1->dim_size[d] != f2->dim_size[d])
                    return 0;
            if (f1->nc_typeid >= NC_FIRSTUSERTYPEID ||
                f2->nc_typeid >= NC_FIRSTUSERTYPEID) {
                NC_TYPE_INFO_T *m1 = NULL, *m2 = NULL;
                if (f1->nc_typeid < NC_FIRSTUSERTYPEID ||
                    f2->nc_typeid < NC_FIRSTUSERTYPEID)
                    return 0;
                if (nc4_find_type(h5_1, f1->nc_typeid, &m1) || !m1 ||
                    nc4_find_type(h5_2, f2->nc_typeid, &m2) || !m2 ||
                    !types_equal(h5_1, m1, h5_2, m2))
                    return 0;
            } else if (f1->nc_typeid != f2->nc_typeid)
                return 0;
        }
        return 1;
    }
    case NC_ENUM: {
        if (t1->u.e.base_nc_typeid != t2->u.e.base_nc_typeid ||
            nclistlength(t1->u.e.enum_member) !=
            nclistlength(t2->u.e.enum_member))
            return 0;
        for (i = 0; i < nclistlength(t1->u.e.enum_member); i++) {
            NC_ENUM_MEMBER_INFO_T *m1 =
                (NC_ENUM_MEMBER_INFO_T *)nclistget(t1->u.e.enum_member, i);
            NC_ENUM_MEMBER_INFO_T *m2 =
                (NC_ENUM_MEMBER_INFO_T *)nclistget(t2->u.e.enum_member, i);
            if (!m1 || !m2 || strcmp(m1->name, m2->name) ||
                memcmp(m1->value, m2->value, t1->size))
                return 0;
        }
        return 1;
    }
    case NC_OPAQUE:
        return 1;
    case NC_VLEN: {
        if (t1->u.v.base_nc_typeid >= NC_FIRSTUSERTYPEID ||
            t2->u.v.base_nc_typeid >= NC_FIRSTUSERTYPEID) {
            NC_TYPE_INFO_T *m1 = NULL, *m2 = NULL;
            if (t1->u.v.base_nc_typeid < NC_FIRSTUSERTYPEID ||
                t2->u.v.base_nc_typeid < NC_FIRSTUSERTYPEID)
                return 0;
            if (nc4_find_type(h5_1, t1->u.v.base_nc_typeid, &m1) || !m1 ||
                nc4_find_type(h5_2, t2->u.v.base_nc_typeid, &m2) || !m2)
                return 0;
            return types_equal(h5_1, m1, h5_2, m2);
        }
        return t1->u.v.base_nc_typeid == t2->u.v.base_nc_typeid;
    }
    default:
        return 0;
    }
}

/**
 * @brief Compare two types for equality — the `nc_inq_type_equal()`
 * dispatch entry.
 *
 * Two types count as equal when they are the same in-memory record or
 * structurally equal: same class, size and member layout/base/member
 * values, recursively for nested members.
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
        *equalp = types_equal(h5_1, t1, h5_2, t2);
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

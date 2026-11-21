/**
 * @file nxt4ref.c
 * @brief NEXTCDF-4 reference API: create, dereference, and inspect
 * `NC_STD_REF` (`H5T_STD_REF`) tokens.
 *
 * The public functions declared in `nep.h` wrap the HDF5 2.x
 * reference API: tokens are 64-byte `H5R_ref_t` values carried in
 * `nc_std_ref_t` so `nep.h` need not include `hdf5.h`. One token
 * format covers object references (`H5Rcreate_object`), dataset
 * region references (`H5Rcreate_region`), and attribute references
 * (`H5Rcreate_attr`), including references whose target lives in a
 * different file. See @ref nextcdf4_refs for the user-facing
 * documentation.
 *
 * @author Edward Hartnett
 * @date 2026-11-21
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <hdf5_hl.h>
#include "nclist.h"
#include "nxt4internal.h"

/* nc_std_ref_t (nep.h) must stay layout-compatible with the HDF5
 * token so buffers can be shared without copying. */
_Static_assert(sizeof(nc_std_ref_t) == sizeof(H5R_ref_t),
               "nc_std_ref_t must match H5R_ref_t");

/**
 * @brief Resolve an ncid to NEXTCDF-4 file state, rejecting any file
 * that is not a NEXTCDF-4 file.
 *
 * @param ncid NetCDF file ID.
 * @param h5p Destination for `NC_FILE_INFO_T`, or NULL.
 * @param filep Destination for `NEXTCDF4_FILE_INFO_T`, or NULL.
 * @return `NC_NOERR` or `NC_ENOTNC4`.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
ref_file(int ncid, NC_FILE_INFO_T **h5p, NEXTCDF4_FILE_INFO_T **filep)
{
    NC *nc = NULL;
    NC_FILE_INFO_T *h5 = NULL;
    NC_GRP_INFO_T *grp = NULL;
    int ret;

    if ((ret = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return ret;
    if (!nc || nc->dispatch != NEXTCDF4_dispatch_table ||
        !h5 || !h5->format_file_info)
        return NC_ENOTNC4;
    if (h5p)
        *h5p = h5;
    if (filep)
        *filep = h5->format_file_info;
    return NC_NOERR;
}

/**
 * @brief Verify the file is in native mode — required by every
 * nc_ref_* create function since `NC_STD_REF` data cannot be stored
 * in compatibility-mode files.
 *
 * @param file NEXTCDF-4 file state.
 * @return `NC_NOERR` or `NC_ENOTNC4`.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
ref_check_native(NEXTCDF4_FILE_INFO_T *file)
{
    if (file->netcdf4_model || (file->mode & NC_CLASSIC_MODEL))
        return NC_ENOTNC4;
    return NC_NOERR;
}

/**
 * @brief Report whether a token points into a file other than the
 * one @p file belongs to.
 *
 * HDF5 stores the target file's name inside the token; for an
 * in-file reference it equals the containing file's own name.
 *
 * @param file NEXTCDF-4 file state.
 * @param ref Reference token.
 * @param extp Destination; nonzero when the target file differs.
 * @return `NC_NOERR` or an error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
ref_is_external(NEXTCDF4_FILE_INFO_T *file, const H5R_ref_t *ref, int *extp)
{
    char rname[2048], fname[2048];
    ssize_t n;

    if ((n = H5Rget_file_name(ref, rname, sizeof(rname))) < 0)
        return NC_EHDFERR;
    if (n >= (ssize_t)sizeof(rname))
        return NC_EINVAL;
    if (H5Fget_name(file->hdfid, fname, sizeof(fname)) < 0)
        return NC_EHDFERR;
    *extp = strcmp(rname, fname) != 0;
    return NC_NOERR;
}

/**
 * @brief Return the open HDF5 group handle for an in-memory group.
 *
 * The root group's handle is `file->rootid`; other groups carry
 * theirs in `format_grp_info`. A group defined but not yet
 * materialized to HDF5 reports `NC_EINVAL`.
 *
 * @param grp Group.
 * @param file NEXTCDF-4 file state.
 * @param gid Destination for the HDF5 group identifier.
 * @return `NC_NOERR` or `NC_EINVAL`.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
grp_hdf(NC_GRP_INFO_T *grp, NEXTCDF4_FILE_INFO_T *file, hid_t *gid)
{
    NEXTCDF4_GRP_INFO_T *ginfo = grp->format_grp_info;
    hid_t g = grp->parent ? (ginfo ? ginfo->hdf_group : -1) : file->rootid;

    if (g < 0)
        return NC_EINVAL;
    *gid = g;
    return NC_NOERR;
}

/**
 * @brief Resolve a group ncid to a full (file-qualified) ncid.
 *
 * Group ncids carry the file id in the upper 16 bits and the in-file
 * group id in the lower 16; the root group has id 0, so its ncid is
 * the file ncid itself.
 *
 * @param h5 File info.
 * @param grp Group.
 * @return The group's ncid.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
grp_ncid(NC_FILE_INFO_T *h5, NC_GRP_INFO_T *grp)
{
    return (h5->controller->ext_ncid & ~0xffff) | (int)grp->hdr.id;
}

/**
 * @brief Find the in-memory group at an absolute HDF5 path.
 *
 * Walks the `children` index one path component at a time, starting
 * at the root group; "/" itself resolves to root.
 *
 * @param h5 File info.
 * @param path Absolute HDF5 object path (for example "/g1/g2").
 * @param grpp Destination for the group.
 * @return `NC_NOERR` on success, `NC_EINVAL` when no group matches.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
grp_for_path(NC_FILE_INFO_T *h5, const char *path, NC_GRP_INFO_T **grpp)
{
    NC_GRP_INFO_T *grp = h5->root_grp;
    char *copy, *save = NULL, *tok;
    int ret = NC_NOERR;

    if (!path || !(copy = strdup(path)))
        return NC_EINVAL;
    for (tok = strtok_r(copy, "/", &save); tok;
         tok = strtok_r(NULL, "/", &save)) {
        NC_GRP_INFO_T *child = NULL;
        size_t i;

        for (i = 0; i < ncindexsize(grp->children); i++) {
            NC_GRP_INFO_T *g = (NC_GRP_INFO_T *)ncindexith(grp->children, i);
            if (g && g->hdr.name && !strcmp(g->hdr.name, tok)) {
                child = g;
                break;
            }
        }
        if (!child) {
            ret = NC_EINVAL;
            break;
        }
        grp = child;
    }
    free(copy);
    if (!ret)
        *grpp = grp;
    return ret;
}

/**
 * @brief Map an open HDF5 object back to its NetCDF kind and ids.
 *
 * `H5Iget_type` gives the object kind and `H5Iget_name` its absolute
 * path; the basename is then looked up in the containing group's
 * variable or type index, or the path itself resolves the group.
 * Works on the `NC_*_INFO_T` lists directly to avoid re-entering the
 * public `nc_*` API.
 *
 * @param h5 File info.
 * @param obj Open HDF5 object identifier.
 * @param obj_typep Destination for `NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP`, or `NC_REF_OBJ_TYPE`.
 * @param grpidp Destination for the containing group's ncid (the group itself for `NC_REF_OBJ_GRP`).
 * @param idp Destination for the varid, group ncid, or typeid.
 * @return `NC_NOERR` on success, `NC_EINVAL` for an unnamed or unmappable object, `NC_ENOMEM`, or `NC_EHDFERR`.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
obj_ids(NC_FILE_INFO_T *h5, hid_t obj, int *obj_typep, int *grpidp,
        int *idp)
{
    NC_GRP_INFO_T *grp;
    char name[NC_MAX_NAME + 2048], base[NC_MAX_NAME + 1];
    ssize_t n;
    int ret = NC_NOERR;

    switch (H5Iget_type(obj)) {
    case H5I_DATASET:  *obj_typep = NC_REF_OBJ_VAR;  break;
    case H5I_GROUP:    *obj_typep = NC_REF_OBJ_GRP;  break;
    case H5I_DATATYPE: *obj_typep = NC_REF_OBJ_TYPE; break;
    default:           return NC_EINVAL;
    }
    if ((n = H5Iget_name(obj, name, sizeof(name))) <= 0)
        return NC_EINVAL;
    name[n] = '\0';

    if (*obj_typep == NC_REF_OBJ_GRP) {
        if ((ret = grp_for_path(h5, name, &grp)))
            return ret;
        *grpidp = grp_ncid(h5, grp);
        *idp = *grpidp;
        return NC_NOERR;
    }

    {
        char *slash = strrchr(name, '/');
        char *dir;

        if (!slash)
            return NC_EINVAL;
        strncpy(base, slash + 1, sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
        *slash = '\0';
        dir = *name ? name : (char *)"/";
        if ((ret = grp_for_path(h5, dir, &grp)))
            return ret;
    }
    *grpidp = grp_ncid(h5, grp);

    if (*obj_typep == NC_REF_OBJ_VAR) {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexlookup(grp->vars, base);
        if (!var)
            return NC_EINVAL;
        *idp = (int)var->hdr.id;
    } else {
        NC_TYPE_INFO_T *type = (NC_TYPE_INFO_T *)ncindexlookup(grp->type, base);
        if (!type)
            return NC_EINVAL;
        *idp = (int)type->hdr.id;
    }
    return NC_NOERR;
}

/**
 * @brief Resolve a create-API `obj_type`/`objid` pair to the target
 * object's HDF5 handle, the HDF5 location the reference is created
 * in, and the object's name.
 *
 * Variable and type targets live in the file @p ncid belongs to. A
 * group target may name any group ncid — including one in a
 * different NEXTCDF-4 file, which produces a cross-file reference.
 * A non-NEXTCDF-4 file is rejected.
 *
 * @param ncid Group context for variable and type targets.
 * @param obj_type `NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP`, or `NC_REF_OBJ_TYPE`.
 * @param objid Varid, group ncid, or typeid per @p obj_type.
 * @param locp Destination for the `loc_id` argument of `H5Rcreate_*`.
 * @param objp Destination for the target object's own HDF5 handle (used for attribute checks), or NULL.
 * @param namep Destination for a malloc'd absolute path of the target; caller frees.
 * @return `NC_NOERR` on success, or an error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
obj_loc(int ncid, int obj_type, int objid, hid_t *locp, hid_t *objp,
        char **namep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t obj;
    char name[NC_MAX_NAME + 2048];
    ssize_t n;
    int ret = NC_NOERR;

    if (obj_type == NC_REF_OBJ_VAR) {
        NC_GRP_INFO_T *grp = NULL;
        NC_VAR_INFO_T *var = NULL;
        NEXTCDF4_VAR_INFO_T *vinfo;

        if ((ret = nc4_find_grp_h5_var(ncid, objid, &h5, &grp, &var)))
            return ret;
        file = h5->format_file_info;
        vinfo = var->format_var_info;
        obj = vinfo ? vinfo->hdf_dataset : -1;
    } else if (obj_type == NC_REF_OBJ_GRP) {
        NC *nc = NULL;
        NC_GRP_INFO_T *grp = NULL;

        /* objid is a group ncid; its file need not be ncid's file. */
        if ((ret = nc4_find_nc_grp_h5(objid, &nc, &grp, &h5)))
            return ret;
        if (!nc || nc->dispatch != NEXTCDF4_dispatch_table ||
            !h5 || !h5->format_file_info)
            return NC_ENOTNC4;
        file = h5->format_file_info;
        if ((ret = grp_hdf(grp, file, &obj)))
            return ret;
    } else if (obj_type == NC_REF_OBJ_TYPE) {
        NC_GRP_INFO_T *grp = NULL;
        NC_TYPE_INFO_T *type = NULL;
        NEXTCDF4_TYPE_INFO_T *tinfo;

        if ((ret = nc4_find_grp_h5(ncid, &grp, &h5)))
            return ret;
        file = h5->format_file_info;
        if ((ret = nc4_find_type(h5, objid, &type)))
            return ret;
        tinfo = type->format_type_info;
        obj = tinfo ? tinfo->hdf_type : -1;
    } else {
        return NC_EINVAL;
    }
    if (obj < 0)
        return NC_EINVAL;
    if ((n = H5Iget_name(obj, name, sizeof(name))) <= 0)
        return NC_EINVAL;
    name[n] = '\0';
    if (!(*namep = strdup(name)))
        return NC_ENOMEM;
    *locp = file->hdfid;
    if (objp)
        *objp = obj;
    return NC_NOERR;
}

/**
 * @brief Create an HDF5 object reference to a variable, group, or
 * user-defined type in a NEXTCDF-4 file.
 *
 * The object is identified by an `NC_REF_OBJ_*` kind and an id
 * interpreted per kind; the token is written to @p ref.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_object(int ncid, int obj_type, int objid, nc_std_ref_t *ref)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t loc = -1;
    char *name = NULL;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d obj_type %d objid %d", __func__, ncid, obj_type,
          objid));

    if (!ref)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, NULL, &file)))
        return ret;
    if ((ret = ref_check_native(file)))
        return ret;
    if ((ret = obj_loc(ncid, obj_type, objid, &loc, NULL, &name)))
        return ret;
    if (H5Rcreate_object(loc, name, H5P_DEFAULT, (H5R_ref_t *)ref) < 0)
        ret = NC_EHDFERR;
    free(name);
    return ret;
}

/**
 * @brief Shared dereference preamble: token-kind and external-target
 * checks.
 *
 * @param file NEXTCDF-4 file state.
 * @param ref Reference token.
 * @param expected Required `NC_STD_REF_*` kind value.
 * @return `NC_NOERR`, `NC_EINVAL` for a mismatched token kind, or `NC_ENOTNC4` for an external target.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
deref_check(NEXTCDF4_FILE_INFO_T *file, const H5R_ref_t *ref,
            int expected)
{
    int ext, kind;

    if (nc_ref_get_type((const nc_std_ref_t *)ref, &kind) ||
        kind != expected)
        return NC_EINVAL;
    {
        int ret = ref_is_external(file, ref, &ext);
        if (ret)
            return ret;
    }
    return ext ? NC_ENOTNC4 : NC_NOERR;
}

/**
 * @brief Resolve an object reference token back to NetCDF ids.
 *
 * Reports the target kind (`NC_REF_OBJ_VAR`, `NC_REF_OBJ_GRP`, or
 * `NC_REF_OBJ_TYPE`), the containing group ncid, and the object id.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_deref_object(int ncid, const nc_std_ref_t *ref, int *obj_typep,
                int *grpidp, int *idp)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t obj = -1;
    int obj_type = 0, grpid = 0, id = 0;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d", __func__, ncid));

    if (!ref)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, &h5, &file)))
        return ret;
    if ((ret = deref_check(file, (const H5R_ref_t *)ref, NC_STD_REF_OBJECT)))
        return ret;
    if ((obj = H5Ropen_object((H5R_ref_t *)ref, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return NC_EINVAL;
    ret = obj_ids(h5, obj, &obj_type, &grpid, &id);
    H5Oclose(obj);
    if (ret)
        return ret;
    if (obj_typep)
        *obj_typep = obj_type;
    if (grpidp)
        *grpidp = grpid;
    if (idp)
        *idp = id;
    return NC_NOERR;
}

/**
 * @brief Create a dataset region reference to a strided hyperslab
 * of a variable in a NEXTCDF-4 file.
 *
 * Only regular strided hyperslabs (`start`/`count`/`stride`) are
 * supported; irregular selections are rejected.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_region(int ncid, int varid, const size_t *startp,
              const size_t *countp, const ptrdiff_t *stridep,
              nc_std_ref_t *ref)
{
    NC_FILE_INFO_T *h5 = NULL;
    NC_GRP_INFO_T *grp = NULL;
    NC_VAR_INFO_T *var = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    NEXTCDF4_VAR_INFO_T *vinfo;
    hid_t space = -1, loc;
    hsize_t *hstart = NULL, *hcount = NULL, *hstride = NULL;
    char name[NC_MAX_NAME + 2048];
    ssize_t n;
    int ret = NC_NOERR;
    int d;

    ELOG((2, "%s: ncid %d varid %d", __func__, ncid, varid));

    if (!ref || !startp || !countp)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, &h5, &file)))
        return ret;
    if ((ret = ref_check_native(file)))
        return ret;
    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return ret;
    vinfo = var->format_var_info;
    if (!vinfo || vinfo->hdf_dataset < 0 || var->ndims <= 0)
        return NC_EINVAL;

    if (!(hstart = malloc(var->ndims * sizeof(hsize_t))) ||
        !(hcount = malloc(var->ndims * sizeof(hsize_t))) ||
        !(hstride = malloc(var->ndims * sizeof(hsize_t))))
        return NC_ENOMEM;
    for (d = 0; d < var->ndims; d++) {
        if (stridep && stridep[d] <= 0) {
            ret = NC_EINVAL;
            goto fail;
        }
        hstart[d] = (hsize_t)startp[d];
        hcount[d] = (hsize_t)countp[d];
        hstride[d] = stridep ? (hsize_t)stridep[d] : 1;
    }
    if ((space = H5Dget_space(vinfo->hdf_dataset)) < 0 ||
        H5Sselect_hyperslab(space, H5S_SELECT_SET, hstart, hstride,
                            hcount, NULL) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if ((n = H5Iget_name(vinfo->hdf_dataset, name, sizeof(name))) <= 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    name[n] = '\0';
    loc = file->hdfid;
    if (H5Rcreate_region(loc, name, space, H5P_DEFAULT,
                         (H5R_ref_t *)ref) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }

fail:
    if (space >= 0)
        H5Sclose(space);
    free(hstart);
    free(hcount);
    free(hstride);
    return ret;
}

/**
 * @brief Resolve a dataset region reference token back to its
 * variable and selection.
 *
 * Recovers the target variable and the stored `start`/`count`/`stride`
 * arrays.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_deref_region(int ncid, const nc_std_ref_t *ref, int *grpidp,
                int *varidp, int *ndimsp, size_t *startp,
                size_t *countp, ptrdiff_t *stridep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t space = -1, obj = -1;
    hsize_t *hstart = NULL, *hstride = NULL, *hcount = NULL,
        *hblock = NULL;
    int obj_type = 0, grpid = 0, varid = 0, ndims = 0;
    int ret = NC_NOERR;
    int d;

    ELOG((2, "%s: ncid %d", __func__, ncid));

    if (!ref)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, &h5, &file)))
        return ret;
    if ((ret = deref_check(file, (const H5R_ref_t *)ref, NC_STD_REF_REGION)))
        return ret;
    if ((space = H5Ropen_region((H5R_ref_t *)ref, H5P_DEFAULT,
                                H5P_DEFAULT)) < 0)
        return NC_EINVAL;
    if ((obj = H5Ropen_object((H5R_ref_t *)ref, H5P_DEFAULT,
                              H5P_DEFAULT)) < 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    if ((ret = obj_ids(h5, obj, &obj_type, &grpid, &varid)))
        goto fail;
    if (obj_type != NC_REF_OBJ_VAR) {
        ret = NC_EINVAL;
        goto fail;
    }
    if ((ndims = H5Sget_simple_extent_ndims(space)) < 0 ||
        H5Sis_regular_hyperslab(space) <= 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    if (!(hstart = malloc(ndims * sizeof(hsize_t))) ||
        !(hstride = malloc(ndims * sizeof(hsize_t))) ||
        !(hcount = malloc(ndims * sizeof(hsize_t))) ||
        !(hblock = malloc(ndims * sizeof(hsize_t)))) {
        ret = NC_ENOMEM;
        goto fail;
    }
    if (H5Sget_regular_hyperslab(space, hstart, hstride, hcount,
                                 hblock) < 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    for (d = 0; d < ndims; d++) {
        if (hblock[d] != 1) {
            ret = NC_EINVAL;
            goto fail;
        }
        if (startp)
            startp[d] = (size_t)hstart[d];
        if (countp)
            countp[d] = (size_t)hcount[d];
        if (stridep)
            stridep[d] = (ptrdiff_t)hstride[d];
    }
    if (grpidp)
        *grpidp = grpid;
    if (varidp)
        *varidp = varid;
    if (ndimsp)
        *ndimsp = ndims;

fail:
    if (obj >= 0)
        H5Oclose(obj);
    if (space >= 0)
        H5Sclose(space);
    free(hstart);
    free(hstride);
    free(hcount);
    free(hblock);
    return ret;
}

/**
 * @brief Create an HDF5 attribute reference to an attribute on a
 * variable, group, or committed datatype in a NEXTCDF-4 file.
 *
 * `obj_type`/`objid` are dispatched exactly as in nc_ref_object(),
 * so every attribute-bearing object kind is covered.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_attr(int ncid, int obj_type, int objid, const char *att_name,
            nc_std_ref_t *ref)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t loc = -1, obj = -1;
    char *name = NULL;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d obj_type %d objid %d att %s", __func__, ncid,
          obj_type, objid, att_name ? att_name : "NULL"));

    if (!ref || !att_name)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, NULL, &file)))
        return ret;
    if ((ret = ref_check_native(file)))
        return ret;
    if ((ret = obj_loc(ncid, obj_type, objid, &loc, &obj, &name)))
        return ret;
    if (H5Aexists(obj, att_name) <= 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    if (H5Rcreate_attr(loc, name, att_name, H5P_DEFAULT,
                       (H5R_ref_t *)ref) < 0)
        ret = NC_EHDFERR;

fail:
    free(name);
    return ret;
}

/**
 * @brief Resolve an attribute reference token back to its owner
 * object and attribute name.
 *
 * Reports the owner kind and ids as nc_deref_object() does, plus
 * the attribute name.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_deref_attr(int ncid, const nc_std_ref_t *ref, int *obj_typep,
              int *grpidp, int *idp, char *att_name)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t att = -1, obj = -1;
    int obj_type = 0, grpid = 0, id = 0;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d", __func__, ncid));

    if (!ref)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, &h5, &file)))
        return ret;
    if ((ret = deref_check(file, (const H5R_ref_t *)ref, NC_STD_REF_ATTR)))
        return ret;
    if ((att = H5Ropen_attr((H5R_ref_t *)ref, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return NC_EINVAL;
    if ((obj = H5Ropen_object((H5R_ref_t *)ref, H5P_DEFAULT,
                              H5P_DEFAULT)) < 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    if ((ret = obj_ids(h5, obj, &obj_type, &grpid, &id)))
        goto fail;
    if (att_name) {
        char buf[NC_MAX_NAME + 1];
        ssize_t n = H5Aget_name(att, sizeof(buf), buf);

        if (n < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        strcpy(att_name, buf);
    }
    if (obj_typep)
        *obj_typep = obj_type;
    if (grpidp)
        *grpidp = grpid;
    if (idp)
        *idp = id;

fail:
    if (obj >= 0)
        H5Oclose(obj);
    if (att >= 0)
        H5Aclose(att);
    return ret;
}

/**
 * @brief Validate the external-create arguments and open the destination
 * file read-write through raw HDF5.
 *
 * The token is created against the target object in @p filename;
 * HDF5 records the target file name inside it. @p path must be an
 * absolute HDF5 object path.
 *
 * @param ncid NetCDF file ID (NEXTCDF-4); only used to reject
 * non-NEXTCDF-4 and compatibility-mode files.
 * @param filename Path of the file holding the target.
 * @param path Absolute object path inside @p filename.
 * @param efp Destination for the open HDF5 file identifier; caller closes.
 * @return `NC_NOERR` on success, or an error code.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
ext_open(int ncid, const char *filename, const char *path, hid_t *efp)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t ef;
    int ret;

    if (!filename || !path)
        return NC_EINVAL;
    if ((ret = ref_file(ncid, NULL, &file)))
        return ret;
    if ((ret = ref_check_native(file)))
        return ret;
    if ((ef = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        return NC_EHDFERR;
    *efp = ef;
    return NC_NOERR;
}

/**
 * @brief Create an object reference pointing into another file.
 *
 * The token records @p filename so `H5Ropen_*` and the `nc_ref_get_*`
 * inspectors can resolve the external target.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_object_ext(int ncid, const char *filename, const char *path,
                  nc_std_ref_t *ref)
{
    hid_t ef = -1;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d file %s path %s", __func__, ncid,
          filename ? filename : "NULL", path ? path : "NULL"));

    if (!ref)
        return NC_EINVAL;
    if ((ret = ext_open(ncid, filename, path, &ef)))
        return ret;
    if (H5Rcreate_object(ef, path, H5P_DEFAULT, (H5R_ref_t *)ref) < 0)
        ret = NC_EHDFERR;
    H5Fclose(ef);
    return ret;
}

/**
 * @brief Create a dataset region reference pointing into another
 * file.
 *
 * The selection uses `start`/`count`/`stride` like nc_ref_region().
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_region_ext(int ncid, const char *filename, const char *path,
                  const size_t *startp, const size_t *countp,
                  const ptrdiff_t *stridep, nc_std_ref_t *ref)
{
    hid_t ef = -1, dset = -1, space = -1;
    hsize_t *hstart = NULL, *hcount = NULL, *hstride = NULL;
    int ret = NC_NOERR;
    int ndims, d;

    ELOG((2, "%s: ncid %d file %s path %s", __func__, ncid,
          filename ? filename : "NULL", path ? path : "NULL"));

    if (!ref || !startp || !countp)
        return NC_EINVAL;
    if ((ret = ext_open(ncid, filename, path, &ef)))
        return ret;
    if ((dset = H5Dopen2(ef, path, H5P_DEFAULT)) < 0 ||
        (space = H5Dget_space(dset)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if ((ndims = H5Sget_simple_extent_ndims(space)) <= 0) {
        ret = NC_EINVAL;
        goto fail;
    }
    if (!(hstart = malloc(ndims * sizeof(hsize_t))) ||
        !(hcount = malloc(ndims * sizeof(hsize_t))) ||
        !(hstride = malloc(ndims * sizeof(hsize_t)))) {
        ret = NC_ENOMEM;
        goto fail;
    }
    for (d = 0; d < ndims; d++) {
        if (stridep && stridep[d] <= 0) {
            ret = NC_EINVAL;
            goto fail;
        }
        hstart[d] = (hsize_t)startp[d];
        hcount[d] = (hsize_t)countp[d];
        hstride[d] = stridep ? (hsize_t)stridep[d] : 1;
    }
    if (H5Sselect_hyperslab(space, H5S_SELECT_SET, hstart, hstride,
                            hcount, NULL) < 0 ||
        H5Rcreate_region(ef, path, space, H5P_DEFAULT,
                         (H5R_ref_t *)ref) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }

fail:
    if (space >= 0)
        H5Sclose(space);
    if (dset >= 0)
        H5Dclose(dset);
    if (ef >= 0)
        H5Fclose(ef);
    free(hstart);
    free(hcount);
    free(hstride);
    return ret;
}

/**
 * @brief Create an attribute reference pointing into another file.
 *
 * The attribute is named on the object at @p path in @p filename.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_attr_ext(int ncid, const char *filename, const char *path,
                const char *att_name, nc_std_ref_t *ref)
{
    hid_t ef = -1;
    int ret = NC_NOERR;

    ELOG((2, "%s: ncid %d file %s path %s att %s", __func__, ncid,
          filename ? filename : "NULL", path ? path : "NULL",
          att_name ? att_name : "NULL"));

    if (!ref || !att_name)
        return NC_EINVAL;
    if ((ret = ext_open(ncid, filename, path, &ef)))
        return ret;
    if (H5Rcreate_attr(ef, path, att_name, H5P_DEFAULT,
                       (H5R_ref_t *)ref) < 0)
        ret = NC_EHDFERR;
    H5Fclose(ef);
    return ret;
}

/**
 * @brief Report the kind of a reference token.
 *
 * Returns `NC_STD_REF_OBJECT`, `NC_STD_REF_REGION`, or
 * `NC_STD_REF_ATTR`.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_get_type(const nc_std_ref_t *ref, int *ref_typep)
{
    if (!ref || !ref_typep)
        return NC_EINVAL;
    switch (H5Rget_type((const H5R_ref_t *)ref)) {
    case H5R_OBJECT1:
    case H5R_OBJECT2:          *ref_typep = NC_STD_REF_OBJECT; break;
    case H5R_DATASET_REGION1:
    case H5R_DATASET_REGION2:  *ref_typep = NC_STD_REF_REGION; break;
    case H5R_ATTR:             *ref_typep = NC_STD_REF_ATTR;   break;
    default:                   return NC_EINVAL;
    }
    return NC_NOERR;
}

/**
 * @brief Copy the file name stored in a reference token.
 *
 * Differs from the containing file only for external references.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_get_file_name(const nc_std_ref_t *ref, char *name, size_t size)
{
    ssize_t n;

    if (!ref || !name || !size)
        return NC_EINVAL;
    if ((n = H5Rget_file_name((const H5R_ref_t *)ref, name, size)) < 0)
        return NC_EHDFERR;
    return n >= (ssize_t)size ? NC_EINVAL : NC_NOERR;
}

/**
 * @brief Copy the object path stored in a reference token.
 *
 * For an attribute reference this is the path of the object that
 * carries the attribute.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_get_obj_name(const nc_std_ref_t *ref, char *name, size_t size)
{
    ssize_t n;

    if (!ref || !name || !size)
        return NC_EINVAL;
    if ((n = H5Rget_obj_name((H5R_ref_t *)ref, H5P_DEFAULT, name,
                             size)) < 0)
        return NC_EHDFERR;
    return n >= (ssize_t)size ? NC_EINVAL : NC_NOERR;
}

/**
 * @brief Release the resources held by a reference token.
 *
 * Wraps `H5Rdestroy()`; safe on uninitialized tokens.
 *
 * See nep.h for parameter and return-value documentation.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_ref_free(nc_std_ref_t *ref)
{
    if (!ref)
        return NC_EINVAL;
    return H5Rdestroy((H5R_ref_t *)ref) < 0 ? NC_EHDFERR : NC_NOERR;
}

/**
 * @file nxt4file.c
 * @brief File state, markers, synchronization, and cleanup for NEXTCDF-4.

 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nxt4internal.h"

/**
 * @brief Recursively close the HDF5 objects belonging to a group.
 *
 * At close time every open `hid_t` under a group must be released:
 * the group's own `NEXTCDF4_GRP_INFO_T::hdf_group` (skipping the root,
 * whose handle is closed separately via `rootid`), each variable's
 * dataset `hid_t`, each dimension's scale `hid_t`, then all children.
 * HDF5 calls are wrapped in `H5E_BEGIN_TRY`/`H5E_END_TRY` so a
 * partially-closed tree does not print spurious error stacks.
 *
 * @param grp Group.
 */
void
nx4_close_group_hdf(NC_GRP_INFO_T *grp)
{
    size_t i;
    NEXTCDF4_GRP_INFO_T *ginfo;

    if (!grp)
        return;
    if ((ginfo = grp->format_grp_info)) {
        if (ginfo->hdf_group >= 0 && ginfo->hdf_group !=
            ((NEXTCDF4_FILE_INFO_T *)grp->nc4_info->format_file_info)->rootid) {
            H5E_BEGIN_TRY {
                H5Gclose(ginfo->hdf_group);
            } H5E_END_TRY;
        }
        free(ginfo);
        grp->format_grp_info = NULL;
    }

    for (i = 0; i < ncindexsize(grp->vars); i++) {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        NEXTCDF4_VAR_INFO_T *vinfo;
        if (!var)
            continue;
        if ((vinfo = var->format_var_info)) {
            if (vinfo->hdf_dataset >= 0) {
                H5E_BEGIN_TRY {
                    H5Dclose(vinfo->hdf_dataset);
                } H5E_END_TRY;
            }
            free(vinfo);
            var->format_var_info = NULL;
        }
        NC4_hdf5_filter_freelist(var);
    }

    for (i = 0; i < ncindexsize(grp->dim); i++) {
        NC_DIM_INFO_T *dim = (NC_DIM_INFO_T *)ncindexith(grp->dim, i);
        NEXTCDF4_DIM_INFO_T *dinfo;
        if (dim && (dinfo = (NEXTCDF4_DIM_INFO_T *)dim->format_dim_info)) {
            if (dinfo->hdf_dataset >= 0) {
                H5E_BEGIN_TRY {
                    H5Dclose(dinfo->hdf_dataset);
                } H5E_END_TRY;
            }
            free(dinfo);
            dim->format_dim_info = NULL;
        }
    }

    /* Committed/transient datatypes close after every dependent
     * object (variables above) is finished, before the shared
     * metadata records are deleted by nc4_file_list_del(). */
    if (grp->type)
        for (i = 0; i < ncindexsize(grp->type); i++) {
            NC_TYPE_INFO_T *type =
                (NC_TYPE_INFO_T *)ncindexith(grp->type, i);
            if (type)
                nx4_free_type_info(type);
        }

    for (i = 0; i < ncindexsize(grp->children); i++)
        nx4_close_group_hdf((NC_GRP_INFO_T *)ncindexith(grp->children, i));
}

/**
 * @brief Close a `NEXTCDF4_FILE_INFO_T`'s HDF5 handles and free it.
 *
 * Companion to NEXTCDF4_add_file(): closes the root group and the
 * HDF5 file, then releases the path and the struct. Safe to call with
 * `NULL`. The `H5Fget_obj_count` log line is a leak check — anything
 * but 1 (the file itself) after `H5Fclose` means an object was left
 * open.
 *
 * @param file State to release; may be `NULL`.
 * @return `NC_NOERR` or `NC_EHDFERR` if an HDF5 close fails.
 */
int
NEXTCDF4_free_file(NEXTCDF4_FILE_INFO_T *file)
{
    int ret = NC_NOERR;

    if (!file)
        return NC_NOERR;
    ELOG((4, "%s: path %s", __func__, file->path ? file->path : ""));
    if (file->rootid >= 0 && H5Gclose(file->rootid) < 0)
        ret = NC_EHDFERR;
    if (file->hdfid >= 0 && H5Fclose(file->hdfid) < 0)
        ret = NC_EHDFERR;
    if (file->hdfid >= 0) {
        int nobj = H5Fget_obj_count(file->hdfid, H5F_OBJ_ALL);
        ELOG((3, "%s: hdfid=%ld nobj=%d ret=%d", __func__, (long)file->hdfid, nobj, ret));
    }
    free(file->path);
    free(file);
    return ret;
}

/**
 * @brief Allocate per-file state and attach it to netcdf-c's open-file
 * list.
 *
 * Called by both NEXTCDF4_create() and NEXTCDF4_open(). netcdf-c's
 * `nc4_file_list_add` creates the shared `NC_FILE_INFO_T` (with a
 * root `NC_GRP_INFO_T`); this function then allocates the
 * NEXTCDF-4-private `NEXTCDF4_FILE_INFO_T` and hangs it on
 * `format_file_info`, plus a `NEXTCDF4_GRP_INFO_T` on the root group.
 * New files start in define mode with `hdfid`/`rootid` unset.
 *
 * @param ncid NetCDF file identifier.
 * @param path File-system path.
 * @param mode Effective NetCDF mode.
 * @param filep Destination for the allocated state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_add_file(int ncid, const char *path, int mode,
                  NEXTCDF4_FILE_INFO_T **filep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    int ret;

    ELOG((4, "%s: ncid %d path %s mode 0x%x", __func__, ncid, path ? path : "", mode));

    if ((ret = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return ret;
    if (!(file = calloc(1, sizeof(*file)))) {
        nc4_file_list_del(ncid);
        return NC_ENOMEM;
    }
    file->hdfid = -1;
    file->rootid = -1;
    file->mode = mode;
    file->no_write = !(mode & NC_WRITE);
    file->define_mode = 1;
    file->netcdf4_model = !!(mode & NC_NETCDF4_MODEL);
    if (!(file->path = strdup(path))) {
        free(file);
        nc4_file_list_del(ncid);
        return NC_ENOMEM;
    }
    h5->no_write = file->no_write;
    h5->root_grp->atts_read = 1;
    h5->format_file_info = file;
    {
        NEXTCDF4_GRP_INFO_T *ginfo;
        if ((ginfo = calloc(1, sizeof(*ginfo))))
            ginfo->hdf_group = file->rootid;
        h5->root_grp->format_grp_info = ginfo;
    }
    *filep = file;
    return NC_NOERR;
}

/**
 * @brief Resolve both the shared and NEXTCDF-4-private file state for
 * an ncid.
 *
 * Almost every dispatch entry begins here: `nc4_find_grp_h5` maps the
 * ncid to netcdf-c's `NC_FILE_INFO_T`, and `format_file_info` then
 * yields the `NEXTCDF4_FILE_INFO_T`. A missing private record means
 * the ncid names a non-NEXTCDF-4 file.
 *
 * @param ncid NetCDF file identifier.
 * @param h5p Optional destination for common NetCDF-4 state.
 * @param filep Optional destination for NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EBADID` for invalid state.
 */
int
NEXTCDF4_get_file(int ncid, NC_FILE_INFO_T **h5p,
                  NEXTCDF4_FILE_INFO_T **filep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NC_GRP_INFO_T *grp = NULL;
    int ret;

    ELOG((4, "%s: ncid %d", __func__, ncid));

    if ((ret = nc4_find_grp_h5(ncid, &grp, &h5)))
        return ret;
    if (!h5 || !h5->format_file_info)
        return NC_EBADID;
    if (h5p)
        *h5p = h5;
    if (filep)
        *filep = h5->format_file_info;
    return NC_NOERR;
}

/**
 * @brief Write a null-terminated scalar HDF5 string attribute.
 *
 * Helper for NEXTCDF4_write_markers(): creates a scalar dataspace and
 * a fixed-size, null-terminated string type sized to the value, then
 * writes it. All HDF5 handles are closed on both paths.
 *
 * @param location HDF5 location identifier.
 * @param name Name.
 * @param value Data buffer.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
static int
write_string_att(hid_t location, const char *name, const char *value)
{
    hid_t space = -1;
    hid_t type = -1;
    hid_t attr = -1;
    int ret = NC_EHDFERR;

    if ((space = H5Screate(H5S_SCALAR)) < 0 ||
        (type = H5Tcopy(H5T_C_S1)) < 0 ||
        H5Tset_size(type, strlen(value) + 1) < 0 ||
        H5Tset_strpad(type, H5T_STR_NULLTERM) < 0 ||
        (attr = H5Acreate2(location, name, type, space, H5P_DEFAULT,
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
 * @brief Write the hidden marker attributes identifying the file as
 * NEXTCDF-4.
 *
 * Every NEXTCDF-4 file gets `_Nextcdf4Backend` = `"NEXTCDF-4/1.0"` on
 * the root group — this is what `NEXTCDF4_read_markers` checks at
 * open to distinguish NEXTCDF-4 files from upstream netCDF-4/HDF5
 * files. Files created with `NC_NETCDF4_MODEL` additionally get the
 * integer `_Nextcdf4Model` marker so the compatibility mode survives
 * reopen. See @ref nextcdf4_architecture.
 *
 * @param file Open writable NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
int
NEXTCDF4_write_markers(NEXTCDF4_FILE_INFO_T *file)
{
    ELOG((4, "%s", __func__));

    hid_t space = -1;
    hid_t attr = -1;
    int one = 1;
    int ret;

    if ((ret = write_string_att(file->rootid, NEXTCDF4_BACKEND_ATT,
                                NEXTCDF4_BACKEND_VALUE)))
        return ret;
    /* The model marker is only written when the file was created in
     * NC_NETCDF4_MODEL compatibility mode; a plain NEXTCDF-4 file gets
     * just the backend attribute. */
    if (!file->netcdf4_model)
        return NC_NOERR;
    if ((space = H5Screate(H5S_SCALAR)) < 0 ||
        (attr = H5Acreate2(file->rootid, NEXTCDF4_MODEL_ATT,
                          H5T_STD_I32LE, space, H5P_DEFAULT, H5P_DEFAULT)) < 0 ||
        H5Awrite(attr, H5T_NATIVE_INT, &one) < 0)
        ret = NC_EHDFERR;
    else
        ret = NC_NOERR;
    if (attr >= 0)
        H5Aclose(attr);
    if (space >= 0)
        H5Sclose(space);
    return ret;
}

/**
 * @brief Read and validate the hidden marker attributes at open.
 *
 * Sets `file->backend_marked` when `_Nextcdf4Backend` matches
 * `"NEXTCDF-4/1.0"`, and restores `netcdf4_model`/`NC_NETCDF4_MODEL`
 * when `_Nextcdf4Model` is present. A file without the backend marker
 * is treated as a plain HDF5/netCDF-4 file and opened anyway —
 * NEXTCDF-4 can read upstream netCDF-4 files.
 *
 * @param file Open NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EFILEMETA` for invalid markers.
 */
int
NEXTCDF4_read_markers(NEXTCDF4_FILE_INFO_T *file)
{
    ELOG((4, "%s", __func__));

    hid_t attr = -1;
    hid_t type = -1;
    char *value = NULL;
    size_t size;
    int model = 0;
    int ret = NC_EFILEMETA;

    if (H5Aexists(file->rootid, NEXTCDF4_BACKEND_ATT) <= 0 ||
        (attr = H5Aopen(file->rootid, NEXTCDF4_BACKEND_ATT, H5P_DEFAULT)) < 0 ||
        (type = H5Aget_type(attr)) < 0 || H5Tget_class(type) != H5T_STRING) {
        /* Missing or unreadable backend marker: this may be an upstream
         * NetCDF-4/HDF5 file. Proceed without setting the backend flag. */
        ret = NC_NOERR;
        goto done;
    }
    size = H5Tget_size(type);
    if (!size || !(value = calloc(size + 1, 1)) || H5Aread(attr, type, value) < 0)
        goto done;
    if (strcmp(value, NEXTCDF4_BACKEND_VALUE))
        goto done;
    H5Tclose(type);
    type = -1;
    H5Aclose(attr);
    attr = -1;

    if (H5Aexists(file->rootid, NEXTCDF4_MODEL_ATT) > 0) {
        if ((attr = H5Aopen(file->rootid, NEXTCDF4_MODEL_ATT, H5P_DEFAULT)) < 0 ||
            H5Aread(attr, H5T_NATIVE_INT, &model) < 0 || model != 1)
            goto done;
        file->netcdf4_model = 1;
        file->mode |= NC_NETCDF4_MODEL;
    }
    file->backend_marked = 1;
    ret = NC_NOERR;

done:
    free(value);
    if (type >= 0)
        H5Tclose(type);
    if (attr >= 0)
        H5Aclose(attr);
    return ret;
}

/**
 * @brief Flush pending writes — the `nc_sync()` dispatch entry.
 *
 * Read-only files are a no-op.
 *
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_sync(int ncid)
{
    ELOG((4, "%s: ncid %d", __func__, ncid));

    NEXTCDF4_FILE_INFO_T *file;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, NULL, &file)))
        return ret;
    if (file->no_write)
        return NC_NOERR;
    return H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0 ? NC_EHDFERR : NC_NOERR;
}

/**
 * @brief Flush, close, and release a file — the `nc_close()` dispatch
 * entry.
 *
 * A file still in define mode is `_enddef`'d first (materializing its
 * pending metadata), then all HDF5 handles are closed recursively,
 * the file is flushed if writable, and the state is freed and removed
 * from netcdf-c's open-file list.
 *
 * @param ncid NetCDF file identifier.
 * @param parameters Optional close parameters; currently ignored.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_close(int ncid, void *parameters)
{
    ELOG((2, "%s: ncid %d", __func__, ncid));

    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    int ret;
    int close_ret = NC_NOERR;

    (void)parameters;
    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if (file->define_mode) {
        if ((ret = NEXTCDF4__enddef(ncid, 0, 0, 0, 0)))
            return ret;
    }
    nx4_close_group_hdf(h5->root_grp);
    h5->format_file_info = NULL;
    if (!file->no_write && H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0)
        close_ret = NC_EHDFERR;
    ret = NEXTCDF4_free_file(file);
    nc4_file_list_del(ncid);
    return close_ret ? close_ret : ret;
}

/**
 * @brief Abort a file — the `nc_abort()` dispatch entry.
 *
 * For NEXTCDF-4 abort and close share the release path; HDF5 simply
 * discards unflushed state.
 *
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_abort(int ncid)
{
    return NEXTCDF4_close(ncid, NULL);
}

/**
 * @brief Report the public format class — the `nc_inq_format()`
 * dispatch entry.
 *
 * NEXTCDF-4 files report as `NC_FORMAT_NETCDF4`: they are HDF5 files
 * in the NetCDF-4 family, distinguishable only through
 * `nc_inq_format_extended()`.
 *
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for `NC_FORMAT_NETCDF4`.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_format(int ncid, int *formatp)
{
    ELOG((4, "%s: ncid %d", __func__, ncid));

    int ret = NEXTCDF4_get_file(ncid, NULL, NULL);
    if (ret)
        return ret;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @brief Report the extended format and effective mode — the
 * `nc_inq_format_extended()` dispatch entry.
 *
 * Returns `NC_FORMATX_NEXTCDF4` plus the mode the file actually runs
 * with: the create/open flags plus `NC_NEXTCDF4` and, for files
 * carrying the `_Nextcdf4Model` marker, `NC_NETCDF4_MODEL`.
 *
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for `NC_FORMATX_NEXTCDF4`.
 * @param modep Optional destination for the effective open/create mode.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_format_extended(int ncid, int *formatp, int *modep)
{
    ELOG((4, "%s: ncid %d", __func__, ncid));

    NEXTCDF4_FILE_INFO_T *file;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, NULL, &file)))
        return ret;
    if (formatp)
        *formatp = NC_FORMATX_NEXTCDF4;
    if (modep)
        *modep = file->mode | NC_NEXTCDF4;
    return NC_NOERR;
}

/**
 * @brief Rebuild dirty variable datasets and mark them committed.
 *
 * Def-var setters only record their settings and flag the variable
 * `dcpl_dirty`; `_enddef` performs the single rebuild per variable
 * that the immutable HDF5 creation properties require. Afterwards
 * every variable with a live dataset is marked `var->created`, so
 * post-commit setter calls fail with `NC_ELATEDEF`, matching
 * upstream.
 *
 * @param h5 NetCDF-4 file state.
 * @param file NEXTCDF-4 file state.
 * @param grp Group to walk (recursively).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
static int
commit_group_vars(NC_FILE_INFO_T *h5, NEXTCDF4_FILE_INFO_T *file,
                  NC_GRP_INFO_T *grp)
{
    size_t i;
    int ret;

    for (i = 0; i < ncindexsize(grp->vars); i++) {
        NC_VAR_INFO_T *var = (NC_VAR_INFO_T *)ncindexith(grp->vars, i);
        NEXTCDF4_VAR_INFO_T *vinfo;
        if (!var || !(vinfo = var->format_var_info))
            continue;
        if (vinfo->hdf_dataset < 0) {
            /* A user-typed variable's dataset is created only now,
             * after NEXTCDF4_write_types() committed its datatype. */
            if ((ret = nx4_create_var_dataset(var, h5, file, grp)))
                return ret;
            vinfo->dcpl_dirty = 0;
        } else if (vinfo->dcpl_dirty) {
            if ((ret = nx4_create_var_dataset(var, h5, file, grp)))
                return ret;
            vinfo->dcpl_dirty = 0;
        }
        if (vinfo->hdf_dataset >= 0) {
            /* The variable's metadata and attributes are fully in
             * memory — flag them read so upstream's lazy-load paths
             * (nc4_hdf5_find_grp_h5_var) leave them alone, then mark
             * the dataset committed. */
            var->meta_read = 1;
            var->atts_read = 1;
            var->created = 1;
        }
    }
    /* Attributes deferred on uncommitted types or deferred datasets
     * are materialized now, in definition order. */
    if ((ret = nx4_flush_pending_atts(grp)))
        return ret;
    for (i = 0; i < ncindexsize(grp->children); i++) {
        NC_GRP_INFO_T *child =
            (NC_GRP_INFO_T *)ncindexith(grp->children, i);
        if (child && (ret = commit_group_vars(h5, file, child)))
            return ret;
    }
    return NC_NOERR;
}

/**
 * @brief Materialize pending metadata and leave define mode — the
 * `nc_enddef()` dispatch entry.
 *
 * While a file is in define mode, `nc_def_dim`/`nc_def_var`/
 * `nc_put_att` only build the in-memory `NC_*_INFO_T` tree.
 * `_enddef` is where it all becomes HDF5: NEXTCDF4_write_types() and
 * the metadata writer in `nxt4meta.c` create the committed datatypes,
 * dimension scales, variable datasets and attributes, then the file
 * flushes and switches to data mode. The alignment arguments exist in
 * the dispatch signature for netcdf-c compatibility but are ignored.
 *
 * @param ncid NetCDF file identifier.
 * @param h_minfree Ignored (reserved for future alignment control).
 * @param v_align Ignored (reserved for future alignment control).
 * @param v_minfree Ignored (reserved for future alignment control).
 * @param r_align Ignored (reserved for future alignment control).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4__enddef(int ncid, size_t h_minfree, size_t v_align,
                 size_t v_minfree, size_t r_align)
{
    ELOG((3, "%s: ncid %d", __func__, ncid));

    NEXTCDF4_FILE_INFO_T *file;
    NC_FILE_INFO_T *h5;
    int ret;

    (void)h_minfree;
    (void)v_align;
    (void)v_minfree;
    (void)r_align;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if (file->no_write)
        return NC_EPERM;
    if (!file->define_mode)
        return NC_EINVAL;
    if ((ret = NEXTCDF4_write_types(h5)))
        return ret;
    if ((ret = commit_group_vars(h5, file, h5->root_grp)))
        return ret;
    if (H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0)
        return NC_EHDFERR;
    file->define_mode = 0;
    h5->flags &= ~NC_INDEF;
    return NC_NOERR;
}

/**
 * @brief Return the file to define mode — the `nc_redef()` dispatch
 * entry.
 *
 * Flips `define_mode` and netcdf-c's `NC_INDEF` flag; the next
 * `_enddef` writes any newly defined metadata to HDF5.
 *
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_redef(int ncid)
{
    ELOG((3, "%s: ncid %d", __func__, ncid));

    NEXTCDF4_FILE_INFO_T *file;
    NC_FILE_INFO_T *h5;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file)))
        return ret;
    if (file->no_write)
        return NC_EPERM;
    if (file->define_mode)
        return NC_EINVAL;
    file->define_mode = 1;
    h5->flags |= NC_INDEF;
    return NC_NOERR;
}

/**
 * @file nxt4file.c
 * @brief File state, markers, synchronization, and cleanup for NEXTCDF-4.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nxt4internal.h"

/** @return The NetCDF error code used for HDF5-layer failures. */
int
NEXTCDF4_hdf_error(void)
{
    return NC_EHDFERR;
}

/** Recursively close HDF5 group identifiers. */
static void
close_group_hdf(NC_GRP_INFO_T *grp)
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
    for (i = 0; i < ncindexsize(grp->children); i++)
        close_group_hdf((NC_GRP_INFO_T *)ncindexith(grp->children, i));
}

/**
 * Close owned HDF5 identifiers and release per-file memory.
 * @param file State to release; may be `NULL`.
 * @return `NC_NOERR` or `NC_EHDFERR` if an HDF5 close fails.
 */
int
NEXTCDF4_free_file(NEXTCDF4_FILE_INFO_T *file)
{
    int ret = NC_NOERR;

    if (!file)
        return NC_NOERR;
    if (file->rootid >= 0 && H5Gclose(file->rootid) < 0)
        ret = NC_EHDFERR;
    if (file->hdfid >= 0 && H5Fclose(file->hdfid) < 0)
        ret = NC_EHDFERR;
    free(file->path);
    free(file);
    return ret;
}

/**
 * Allocate NEXTCDF-4 state and attach it to netcdf-c's file list.
 * @param ncid NetCDF file identifier.
 * @param path File-system path.
 * @param mode Effective NetCDF mode.
 * @param filep Destination for allocated state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_add_file(int ncid, const char *path, int mode,
                  NEXTCDF4_FILE_INFO_T **filep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NEXTCDF4_FILE_INFO_T *file = NULL;
    int ret;

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
 * Resolve common and backend-specific state for an open NEXTCDF-4 file.
 * @param ncid NetCDF file identifier.
 * @param h5p Optional destination for common NetCDF-4 state.
 * @param filep Optional destination for NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_get_file(int ncid, NC_FILE_INFO_T **h5p,
                  NEXTCDF4_FILE_INFO_T **filep)
{
    NC_FILE_INFO_T *h5 = NULL;
    NC_GRP_INFO_T *grp = NULL;
    int ret;

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
 * @internal Write a null-terminated scalar HDF5 string attribute.
 * @param location HDF5 object receiving the attribute.
 * @param name Attribute name.
 * @param value Null-terminated attribute value.
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
 * Write hidden NEXTCDF-4 provenance and compatibility attributes.
 * @param file Open writable NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
int
NEXTCDF4_write_markers(NEXTCDF4_FILE_INFO_T *file)
{
    hid_t space = -1;
    hid_t attr = -1;
    int one = 1;
    int ret;

    if ((ret = write_string_att(file->rootid, NEXTCDF4_BACKEND_ATT,
                                NEXTCDF4_BACKEND_VALUE)))
        return ret;
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
 * Read and validate hidden NEXTCDF-4 provenance and compatibility attributes.
 * @param file Open NEXTCDF-4 state to update.
 * @return `NC_NOERR` on success, or `NC_EFILEMETA` for invalid metadata.
 */
int
NEXTCDF4_read_markers(NEXTCDF4_FILE_INFO_T *file)
{
    hid_t attr = -1;
    hid_t type = -1;
    char *value = NULL;
    size_t size;
    int model = 0;
    int ret = NC_EFILEMETA;

    if (H5Aexists(file->rootid, NEXTCDF4_BACKEND_ATT) <= 0 ||
        (attr = H5Aopen(file->rootid, NEXTCDF4_BACKEND_ATT, H5P_DEFAULT)) < 0 ||
        (type = H5Aget_type(attr)) < 0 || H5Tget_class(type) != H5T_STRING)
        goto done;
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
 * Flush pending writes for a writable NEXTCDF-4 file.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_sync(int ncid)
{
    NEXTCDF4_FILE_INFO_T *file;
    int ret;

    if ((ret = NEXTCDF4_get_file(ncid, NULL, &file)))
        return ret;
    if (file->no_write)
        return NC_NOERR;
    return H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0 ? NC_EHDFERR : NC_NOERR;
}

/**
 * Flush, close, and release a NEXTCDF-4 file.
 * @param ncid NetCDF file identifier.
 * @param parameters Optional close parameters; currently ignored.
 * @return `NC_NOERR` on success, or the first cleanup error.
 */
int
NEXTCDF4_close(int ncid, void *parameters)
{
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
    if (!file->no_write && H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0)
        close_ret = NC_EHDFERR;
    close_group_hdf(h5->root_grp);
    h5->format_file_info = NULL;
    ret = NEXTCDF4_free_file(file);
    return close_ret ? close_ret : ret;
}

/**
 * Abort a NEXTCDF-4 operation using the normal resource-release path.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_abort(int ncid)
{
    return NEXTCDF4_close(ncid, NULL);
}

/**
 * Report the public NetCDF-4 format class.
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for the format identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_format(int ncid, int *formatp)
{
    int ret = NEXTCDF4_get_file(ncid, NULL, NULL);
    if (ret)
        return ret;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * Report the extended NEXTCDF-4 format identifier and effective mode.
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for `NC_FORMATX_NEXTCDF4`.
 * @param modep Optional destination for the effective mode flags.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_format_extended(int ncid, int *formatp, int *modep)
{
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
 * Materialize pending root-group metadata and exit define mode.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4__enddef(int ncid, size_t h_minfree, size_t v_align,
                 size_t v_minfree, size_t r_align)
{
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
    if (H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0)
        return NC_EHDFERR;
    file->define_mode = 0;
    h5->flags &= ~NC_INDEF;
    return NC_NOERR;
}

/**
 * Return the file to define mode.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_redef(int ncid)
{
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

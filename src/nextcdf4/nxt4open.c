/**
 * @file nxt4open.c
 * @brief Empty-file open and validation for the NEXTCDF-4 backend.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <unistd.h>
#include "nxt4internal.h"

/*
 * Open and validate an empty NEXTCDF-4 HDF5 file.
 *
 * The file must be HDF5, contain a valid `_Nextcdf4Backend` marker, and have
 * no root-group links. Read/write access is selected from `NC_WRITE`.*/
int
NEXTCDF4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    NC_FILE_INFO_T *h5 = NULL;
    hid_t fapl = -1;
    unsigned flags;
    int ret;

    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;
    if (!path || !*path)
        return NC_EINVAL;
    if (access(path, F_OK) != 0)
        return NC_ENOTNC;
    H5E_BEGIN_TRY {
        ret = H5Fis_hdf5(path);
    } H5E_END_TRY;
    if (ret <= 0)
        return NC_ENOTNC;
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        return NC_EHDFERR;
    if (H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0) {
        H5Pclose(fapl);
        return NC_EHDFERR;
    }
    if ((ret = NEXTCDF4_add_file(ncid, path, mode, &file)))
        return ret;
    if ((ret = nc4_find_grp_h5(ncid, NULL, &h5)))
        goto fail;
    file->define_mode = 0;
    flags = (mode & NC_WRITE) ? H5F_ACC_RDWR : H5F_ACC_RDONLY;
    H5E_BEGIN_TRY {
        file->hdfid = H5Fopen(path, flags, fapl);
    } H5E_END_TRY;
    H5Pclose(fapl);
    if (file->hdfid < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if ((file->rootid = H5Gopen2(file->hdfid, "/", H5P_DEFAULT)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    if (h5->root_grp->format_grp_info)
        ((NEXTCDF4_GRP_INFO_T *)h5->root_grp->format_grp_info)->hdf_group = file->rootid;
    if ((ret = NEXTCDF4_read_markers(file)))
        goto fail;
    h5->root_grp->atts_read = 0;
    if ((ret = NEXTCDF4_load_metadata(file, h5)))
        goto fail;
    return NC_NOERR;

fail:
    NEXTCDF4_free_file(file);
    nc4_file_list_del(ncid);
    return ret;
}

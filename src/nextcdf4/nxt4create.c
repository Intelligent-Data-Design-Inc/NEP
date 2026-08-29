/**
 * @file nxt4create.c
 * @brief Empty-file creation for the NEXTCDF-4 backend.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <unistd.h>
#include "nxt4internal.h"

/**
 * @internal Validate NEXTCDF-4 create-mode combinations.
 * @param mode NetCDF create-mode flags.
 * @return `NC_NOERR` when valid, or `NC_EINVAL` for incompatible flags.
 */
static int
valid_create_mode(int mode)
{
    int incompatible = NC_64BIT_OFFSET | NC_64BIT_DATA;

    if ((mode & incompatible) ||
        ((mode & NC_CLASSIC_MODEL) && (mode & NC_NETCDF4_MODEL)))
        return NC_EINVAL;
    return NC_NOERR;
}

/**
 * Create an empty NEXTCDF-4 HDF5 file and register its in-memory state.
 *
 * Native and classic-model files use the latest HDF5 format bounds;
 * `NC_NETCDF4_MODEL` files use HDF5 1.10 compatibility bounds. The function
 * writes backend markers before returning a valid ncid.
 *
 * @param path File-system path to create.
 * @param cmode NetCDF create-mode flags.
 * @param initialsz Requested initial size; currently ignored.
 * @param basepe Base processing element; currently ignored.
 * @param chunksizehintp Chunk-size hint; currently ignored.
 * @param parameters Optional dispatch parameters; currently ignored.
 * @param dispatch Selected dispatch table.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                size_t *chunksizehintp, void *parameters,
                const NC_Dispatch *dispatch, int ncid)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t fapl = -1;
    H5F_libver_t bound;
    unsigned major;
    unsigned minor;
    unsigned release;
    unsigned flags;
    int existed;
    int ret;
    NC_FILE_INFO_T *h5 = NULL;

    (void)initialsz;
    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;
    if (!path || !*path)
        return NC_EINVAL;
    if ((ret = valid_create_mode(cmode)))
        return ret;
    if (H5get_libversion(&major, &minor, &release) < 0)
        return NC_EHDFERR;
    if (major < 1 || (major == 1 && minor < 14))
        return NC_ENOTBUILT;

    existed = access(path, F_OK) == 0;
    if ((ret = NEXTCDF4_add_file(ncid, path, cmode, &file)))
        return ret;
    if ((ret = nc4_find_grp_h5(ncid, NULL, &h5)))
        goto fail;
    h5->flags |= NC_INDEF;
    file->no_write = 0;
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    bound = file->netcdf4_model ? H5F_LIBVER_V110 : H5F_LIBVER_LATEST;
    if (H5Pset_libver_bounds(fapl, bound, bound) < 0) {
        ret = NC_EHDFERR;
        goto fail;
    }
    flags = (cmode & NC_NOCLOBBER) ? H5F_ACC_EXCL : H5F_ACC_TRUNC;
    H5E_BEGIN_TRY {
        file->hdfid = H5Fcreate(path, flags, H5P_DEFAULT, fapl);
    } H5E_END_TRY;
    if (file->hdfid < 0) {
        ret = existed && (cmode & NC_NOCLOBBER) ? NC_EEXIST : NC_EHDFERR;
        goto fail;
    }
    ret = NC_NOERR;
    if ((file->rootid = H5Gopen2(file->hdfid, "/", H5P_DEFAULT)) < 0 ||
        (ret = NEXTCDF4_write_markers(file)) ||
        H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0) {
        if (!ret)
            ret = NC_EHDFERR;
        goto fail;
    }
    if (h5->root_grp->format_grp_info)
        ((NEXTCDF4_GRP_INFO_T *)h5->root_grp->format_grp_info)->hdf_group = file->rootid;
    H5Pclose(fapl);
    return NC_NOERR;

fail:
    if (fapl >= 0)
        H5Pclose(fapl);
    NEXTCDF4_free_file(file);
    nc4_file_list_del(ncid);
    if (!existed && ret != NC_EEXIST)
        unlink(path);
    return ret;
}

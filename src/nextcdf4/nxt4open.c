/**
 * @file nxt4open.c
 * @brief Empty-file open and validation for the NEXTCDF-4 backend.

 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <unistd.h>
#include "nxt4internal.h"

/**
 * @brief Open an existing NEXTCDF-4 file.
 *
 * This is the dispatch-table entry netcdf-c calls when a user calls
 * `nc_open()` with `NC_NEXTCDF4` in the mode flags. The file must
 * already exist and be a real HDF5 file: NEXTCDF-4 files are ordinary
 * HDF5 files carrying a small set of hidden bookkeeping attributes,
 * so `H5Fis_hdf5()` is used as the file-type check.
 *
 * The function registers the file's in-memory state
 * (`NEXTCDF4_FILE_INFO_T`), opens the HDF5 file and its root group,
 * verifies the `_Nextcdf4Backend` marker attribute, and finally calls
 * NEXTCDF4_load_metadata() to rebuild the netCDF metadata tree
 * (dimensions, variables, attributes, types) by walking the HDF5
 * contents. See @ref nextcdf4_architecture for the big picture.
 *
 * @param path File-system path to open.
 * @param mode NetCDF open-mode flags (`NC_WRITE` selects read/write).
 * @param basepe Base processing element; currently ignored.
 * @param chunksizehintp Chunk-size hint; currently ignored.
 * @param parameters Optional dispatch parameters; currently ignored.
 * @param dispatch Dispatch table selected by netcdf-c.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, `NC_ENOTNC` if @p path is missing or
 * not HDF5, `NC_EINVAL` for a bad path, or another NetCDF error code.
 */
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

    ELOG((2, "%s: path %s mode 0x%x ncid %d", __func__, path ? path : "", mode, ncid));

    /* Validate the path, confirm the file is HDF5, register in-memory
     * state, open the HDF5 file and root group, read the hidden marker
     * attributes, then load the full metadata tree. */
    if (!path || !*path)
        return NC_EINVAL;
    if ((ret = NEXTCDF4_check_mode(mode)))
        return ret;
    if (access(path, F_OK) != 0)
        return NC_ENOTNC;
    H5E_BEGIN_TRY {
        ret = H5Fis_hdf5(path);
    } H5E_END_TRY;
    if (ret <= 0)
        return NC_ENOTNC;
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        EBAIL2(NC_EHDFERR);
    /* H5F_CLOSE_STRONG makes H5Fclose flush and invalidate every open
     * object in the file, which keeps the close path simple. */
    if (H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0)
        EBAIL2(NC_EHDFERR);
    if ((ret = NEXTCDF4_add_file(ncid, path, mode, &file)))
        return ret;
    if ((ret = nc4_find_grp_h5(ncid, NULL, &h5)))
        goto fail;
    file->define_mode = 0;
    flags = (mode & NC_WRITE) ? H5F_ACC_RDWR : H5F_ACC_RDONLY;
    H5E_BEGIN_TRY {
        file->hdfid = H5Fopen(path, flags, fapl);
    } H5E_END_TRY;
    if (file->hdfid < 0)
        EBAIL2(NC_EHDFERR);
    H5Pclose(fapl);
    fapl = -1;
    if ((file->rootid = H5Gopen2(file->hdfid, "/", H5P_DEFAULT)) < 0)
        EBAIL2(NC_EHDFERR);
    if (h5->root_grp->format_grp_info)
        ((NEXTCDF4_GRP_INFO_T *)h5->root_grp->format_grp_info)->hdf_group = file->rootid;
    if ((ret = NEXTCDF4_read_markers(file)))
        goto fail;
    if ((ret = nx4_read_ncproperties(file)))
        goto fail;
    h5->root_grp->atts_read = 0;
    if ((ret = NEXTCDF4_load_metadata(file, h5)))
        goto fail;
    return NC_NOERR;

fail:
    if (fapl >= 0)
        H5Pclose(fapl);
    if (h5) {
        nx4_close_group_hdf(h5->root_grp);
        nx4_scrub_extended_atomic_data(h5->root_grp);
    }
    NEXTCDF4_free_file(file);
    nc4_file_list_del(ncid);
    return ret;
}

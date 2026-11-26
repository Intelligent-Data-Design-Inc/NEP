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
 * @param parameters Non-NULL when the user called `nc_open_par()`:
 * points at an NX4_MPI_INFO carrying the MPI communicator and info
 * handle. `nc_open_par()` imposes no extra mode flag beyond
 * `NC_NEXTCDF4`.
 * @param dispatch Dispatch table selected by netcdf-c.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, `NC_ENOTNC` if @p path is missing or
 * not HDF5, `NC_EINVAL` for a bad path, `NC_ENOPAR` for a parallel
 * request when NEXTCDF-4 was built without parallel support,
 * `NC_EPARINIT` when the MPI-IO driver cannot be set, or another
 * NetCDF error code.
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
    (void)dispatch;

    ELOG((2, "%s: path %s mode 0x%x ncid %d", __func__, path ? path : "", mode, ncid));

    /* Validate the path, confirm the file is HDF5, register in-memory
     * state, open the HDF5 file and root group, read the hidden marker
     * attributes, then load the full metadata tree. */
    if (!path || !*path)
        return NC_EINVAL;
    if ((ret = NEXTCDF4_check_mode(mode)))
        return ret;
    if ((mode & NC_SWMR_WRITE) &&
        (!(mode & NC_WRITE) || parameters ||
         (mode & (NC_CLASSIC_MODEL | NC_NETCDF4_MODEL))))
        return NC_EINVAL;
    /* A SWMR reader is strictly read-only and serial: the flag pairs
     * with the writer's NC_SWMR_WRITE, so combining them, requesting
     * write access, a classic model or MPI parameters is invalid. A
     * NC_NETCDF4_MODEL file is acceptable here; whether it can actually
     * be SWMR-read is decided by its superblock version below. */
    if ((mode & NC_SWMR_READ) &&
        (parameters ||
         (mode & (NC_WRITE | NC_SWMR_WRITE | NC_CLASSIC_MODEL))))
        return NC_EINVAL;
#ifndef HAVE_NEXTCDF4_PARALLEL
    /* A non-NULL parameters pointer means nc_open_par was called;
     * without parallel support that is unanswerable. NC_INMEMORY also
     * uses parameters, but check_mode already rejected it above. */
    if (parameters)
        return NC_ENOPAR;
#endif
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
#ifdef HAVE_NEXTCDF4_PARALLEL
    /* Parallel open: switch the FAPL to the MPI-IO driver, request
     * collective metadata operations, and keep owned duplicates of the
     * communicator and info until close, as upstream's nc4_open_file
     * does. */
    if (parameters) {
        NX4_MPI_INFO *mpi = (NX4_MPI_INFO *)parameters;
        if (H5Pset_fapl_mpio(fapl, mpi->comm, mpi->info) < 0)
            EBAIL2(NC_EPARINIT);
        if (H5Pset_all_coll_metadata_ops(fapl, 1) < 0 ||
            H5Pset_coll_metadata_write(fapl, 1) < 0)
            EBAIL2(NC_EHDFERR);
        if (MPI_Comm_dup(mpi->comm, &file->comm) != MPI_SUCCESS)
            EBAIL(NC_EMPI);
        file->comm_duped = 1;
        if (mpi->info != MPI_INFO_NULL) {
            if (MPI_Info_dup(mpi->info, &file->info) != MPI_SUCCESS)
                EBAIL(NC_EMPI);
            file->info_duped = 1;
        } else
            file->info = MPI_INFO_NULL;
        file->parallel = 1;
    }
#endif
    flags = (mode & NC_WRITE) ? H5F_ACC_RDWR : H5F_ACC_RDONLY;
    /* SWMR reading is selected purely at H5Fopen: read-only access plus
     * H5F_ACC_SWMR_READ. HDF5 has no H5Fstart_swmr_read counterpart to
     * H5Fstart_swmr_write; nothing further is required to arm the
     * session, though the superblock version is verified below. */
    if (mode & NC_SWMR_READ)
        flags |= H5F_ACC_SWMR_READ;
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
    /* SWMR read requires a v3 superblock; older files (most
     * NC_NETCDF4_MODEL files included) are rejected without any
     * upgrade attempt. */
    if (file->swmr_read) {
        H5F_info2_t finfo;
        if (H5Fget_info2(file->hdfid, &finfo) < 0) {
            ret = NC_EHDFERR;
            goto fail;
        }
        if (finfo.super.version != 3) {
            ret = NC_EINVAL;
            goto fail;
        }
        /* Coordinate variables carry two handles to one dataset; in a
         * SWMR reader the duplicate poisons data visibility, so each
         * pair is collapsed to the variable's own handle. */
        if ((ret = nx4_swmr_read_aliases(h5->root_grp)))
            goto fail;
    }
    if (file->swmr_requested && (ret = nx4_swmr_activate(h5, file)))
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

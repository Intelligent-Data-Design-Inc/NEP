/**
 * @file nxt4create.c
 * @brief Empty-file creation for the NEXTCDF-4 backend.

 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "nxt4internal.h"

/**
 * @brief Reject create-mode flag combinations NEXTCDF-4 cannot honor.
 *
 * NEXTCDF-4 always writes HDF5 files, so the NetCDF-3 format-variant
 * flags `NC_64BIT_OFFSET` and `NC_64BIT_DATA` have no meaning here.
 * `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` select different data
 * models and different HDF5 superblock bounds, so they are mutually
 * exclusive. See @ref nextcdf4_architecture for the mode table.
 *
 * @param mode NetCDF mode flags.
 * @return `NC_NOERR` on success, or `NC_EINVAL`.
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
 * @brief Create an empty NEXTCDF-4 file.
 *
 * This is the dispatch-table entry netcdf-c calls when a user calls
 * `nc_create()` with `NC_NEXTCDF4` in the mode flags. It validates the
 * mode, requires HDF5 2.1.1 or newer at run time (the build enforces
 * the same minimum), registers the file's in-memory state, and
 * creates the HDF5 file with a superblock bound matching the
 * requested model: `H5F_LIBVER_LATEST` (superblock v3) for native and
 * classic-model files, `H5F_LIBVER_V110` for `NC_NETCDF4_MODEL` files
 * so upstream netcdf-c linked against HDF5 1.10 can read them.
 *
 * Only the hidden marker attributes are written here — dimensions,
 * variables and attributes are materialized later by `_enddef` (see
 * NEXTCDF4__enddef() and @ref nextcdf4_architecture).
 *
 * @param path File-system path to create.
 * @param cmode NetCDF create-mode flags.
 * @param initialsz Requested initial file size; currently ignored.
 * @param basepe Base processing element; currently ignored.
 * @param chunksizehintp Chunk-size hint; currently ignored.
 * @param parameters Non-NULL when the user called `nc_create_par()`:
 * points at an NX4_MPI_INFO carrying the MPI communicator and info
 * handle. `nc_create_par()` also requires `NC_NETCDF4` in the mode
 * flags (upstream rejects other creates without PnetCDF), so parallel
 * creates pass `NC_NEXTCDF4|NC_NETCDF4`.
 * @param dispatch Dispatch table selected by netcdf-c.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, `NC_EINVAL` for bad mode flags,
 * `NC_ENOTBUILT` when HDF5 is too old, `NC_EEXIST` for a
 * `NC_NOCLOBBER` collision, `NC_ENOPAR` for a parallel request when
 * NEXTCDF-4 was built without parallel support, `NC_EPARINIT` when the
 * MPI-IO driver cannot be set, or another NetCDF error code.
 */
int
NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                size_t *chunksizehintp, void *parameters,
                const NC_Dispatch *dispatch, int ncid)
{
    NEXTCDF4_FILE_INFO_T *file = NULL;
    hid_t fapl = -1;
    hid_t fcpl = -1;
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
    (void)dispatch;

    ELOG((2, "%s: path %s mode 0x%x ncid %d", __func__, path ? path : "", cmode, ncid));

    /* Validate path and mode, check the run-time HDF5 version, then
     * register state, create the HDF5 file with the right superblock
     * bound, open the root group, write the hidden marker attributes,
     * and flush. */
    if (!path || !*path)
        return NC_EINVAL;
    if ((ret = NEXTCDF4_check_mode(cmode)))
        return ret;
    if ((ret = valid_create_mode(cmode)))
        return ret;
    if ((cmode & NC_SWMR_WRITE) &&
        (parameters || (cmode & (NC_CLASSIC_MODEL | NC_NETCDF4_MODEL))))
        return NC_EINVAL;
#ifndef HAVE_NEXTCDF4_PARALLEL
    /* A non-NULL parameters pointer means nc_create_par was called;
     * without parallel support that is unanswerable. NC_INMEMORY also
     * uses parameters, but check_mode already rejected it above. */
    if (parameters)
        return NC_ENOPAR;
#endif
    if (H5get_libversion(&major, &minor, &release) < 0)
        return NC_EHDFERR;
    if (major < 2 || (major == 2 && minor < 1) || (major == 2 && minor == 1 && release < 1))
        return NC_ENOTBUILT;

    existed = access(path, F_OK) == 0;
    /* netcdf-c semantics: clobbering an existing path is the default,
     * so unlink it up front; with NC_NOCLOBBER we let H5Fcreate's
     * H5F_ACC_EXCL produce the NC_EEXIST instead. */
    if (existed && !(cmode & NC_NOCLOBBER)) {
        ELOG((3, "unlinking existing file %s", path));
        if (unlink(path) < 0)
            ELOG((0, "unlink %s failed: %s", path, strerror(errno)));
    }
    if ((ret = NEXTCDF4_add_file(ncid, path, cmode, &file)))
        return ret;
    if ((ret = nc4_find_grp_h5(ncid, NULL, &h5)))
        goto fail;
    h5->flags |= NC_INDEF;
    file->no_write = 0;
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        EBAIL2(NC_EHDFERR);
    if (H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0)
        EBAIL2(NC_EHDFERR);
    bound = file->netcdf4_model ? H5F_LIBVER_V110 : H5F_LIBVER_LATEST;
    if (H5Pset_libver_bounds(fapl, bound, bound) < 0)
        EBAIL2(NC_EHDFERR);
#ifdef HAVE_NEXTCDF4_PARALLEL
    /* Parallel create: switch the FAPL to the MPI-IO driver, request
     * collective metadata operations, and keep owned duplicates of the
     * communicator and info until close, as upstream's nc4_create_file
     * does. All ranks in the communicator must make identical
     * define-mode calls from here on. */
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
    flags = (cmode & NC_NOCLOBBER) ? H5F_ACC_EXCL : H5F_ACC_TRUNC;
    /* Track link and attribute creation order on the root group, as
     * upstream does: nc_inq_attname() returns creation order, and
     * readers iterate committed datatypes in commit order, which the
     * dependency-ordered type writer relies on for nested members. */
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0 ||
        H5Pset_link_creation_order(fcpl, H5P_CRT_ORDER_TRACKED |
                                   H5P_CRT_ORDER_INDEXED) < 0 ||
        H5Pset_attr_creation_order(fcpl, H5P_CRT_ORDER_TRACKED |
                                   H5P_CRT_ORDER_INDEXED) < 0)
        EBAIL2(NC_EHDFERR);
    H5E_BEGIN_TRY {
        file->hdfid = H5Fcreate(path, flags, fcpl, fapl);
    } H5E_END_TRY;
    H5Pclose(fcpl);
    fcpl = -1;
    if (file->hdfid < 0) {
        ret = existed && (cmode & NC_NOCLOBBER) ? NC_EEXIST : NC_EHDFERR;
        EBAIL2(ret);
    }
    {
        char fname[1024];
        ssize_t len = H5Fget_name(file->hdfid, fname, sizeof(fname));
        int nobj = H5Fget_obj_count(file->hdfid, H5F_OBJ_ALL);
        ELOG((3, "H5Fcreate succeeded hdfid=%ld path=%s nobj=%d",
             (long)file->hdfid, len > 0 ? fname : "?", nobj));
    }
    if ((file->rootid = H5Gopen2(file->hdfid, "/", H5P_DEFAULT)) < 0)
        EBAIL2(NC_EHDFERR);
    if ((ret = NEXTCDF4_write_markers(file)))
        EBAIL(ret);
    /* The _NCProperties string is built now so name lookups answer
     * during this session; the attribute itself is deferred to
     * _enddef/sync/close like upstream. */
    file->created_by_us = 1;
    if ((ret = nx4_build_ncproperties(&file->ncproperties)))
        EBAIL(ret);
    if (H5Fflush(file->hdfid, H5F_SCOPE_GLOBAL) < 0)
        EBAIL2(NC_EHDFERR);
    if (h5->root_grp->format_grp_info)
        ((NEXTCDF4_GRP_INFO_T *)h5->root_grp->format_grp_info)->hdf_group = file->rootid;
    H5Pclose(fapl);
    return NC_NOERR;

fail:
    if (fapl >= 0)
        H5Pclose(fapl);
    if (fcpl >= 0)
        H5Pclose(fcpl);
    if (h5) {
        nx4_close_group_hdf(h5->root_grp);
        nx4_scrub_extended_atomic_data(h5->root_grp);
    }
    NEXTCDF4_free_file(file);
    nc4_file_list_del(ncid);
    if (!existed && ret != NC_EEXIST)
        unlink(path);
    return ret;
}

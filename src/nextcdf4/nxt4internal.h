/**
 * @file nxt4internal.h
 * @brief Private state and shared helpers for the NEXTCDF-4 backend.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NXT4INTERNAL_H
#define NXT4INTERNAL_H

#include <hdf5.h>
#include "nep_nc4.h"
#include "nextcdf4dispatch.h"

/** Hidden root attribute identifying a NEXTCDF-4-created file. */
#define NEXTCDF4_BACKEND_ATT "_Nextcdf4Backend"
/** Hidden root attribute identifying NetCDF-4 compatibility mode. */
#define NEXTCDF4_MODEL_ATT "_Nextcdf4Model"
/** Backend version stored in the NEXTCDF-4 provenance attribute. */
#define NEXTCDF4_BACKEND_VALUE "NEXTCDF-4/1.0"

/** Per-file state owned by the NEXTCDF-4 dispatch layer. */
typedef struct NEXTCDF4_FILE_INFO
{
    hid_t hdfid;        /**< Open HDF5 file identifier, or a negative value. */
    hid_t rootid;       /**< Open HDF5 root-group identifier, or a negative value. */
    char *path;         /**< Owned copy of the file-system path. */
    int mode;           /**< Effective NetCDF create/open mode. */
    int no_write;       /**< Nonzero when the HDF5 file is read-only. */
    int define_mode;    /**< Nonzero while a newly created file is in define mode. */
    int netcdf4_model;  /**< Nonzero for `NC_NETCDF4_MODEL` compatibility mode. */
} NEXTCDF4_FILE_INFO_T;

/** Allocate and register per-file NEXTCDF-4 state.
 * @param ncid NetCDF file identifier.
 * @param path File-system path.
 * @param mode Effective NetCDF mode.
 * @param filep Destination for the allocated state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_add_file(int ncid, const char *path, int mode,
                      NEXTCDF4_FILE_INFO_T **filep);
/** Resolve registered NEXTCDF-4 state from a NetCDF identifier.
 * @param ncid NetCDF file identifier.
 * @param h5p Optional destination for common NetCDF-4 state.
 * @param filep Optional destination for NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EBADID` for invalid state.
 */
int NEXTCDF4_get_file(int ncid, NC_FILE_INFO_T **h5p,
                      NEXTCDF4_FILE_INFO_T **filep);
/** Close HDF5 identifiers and free per-file state.
 * @param file State to release; may be `NULL`.
 * @return `NC_NOERR` or `NC_EHDFERR` if an HDF5 close fails.
 */
int NEXTCDF4_free_file(NEXTCDF4_FILE_INFO_T *file);
/** Write NEXTCDF-4 provenance and compatibility markers.
 * @param file Open writable NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EHDFERR`.
 */
int NEXTCDF4_write_markers(NEXTCDF4_FILE_INFO_T *file);
/** Read and validate NEXTCDF-4 provenance and compatibility markers.
 * @param file Open NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_EFILEMETA` for invalid markers.
 */
int NEXTCDF4_read_markers(NEXTCDF4_FILE_INFO_T *file);
/** Return the NetCDF error used for an HDF5-layer failure.
 * @return `NC_EHDFERR`.
 */
int NEXTCDF4_hdf_error(void);

#endif

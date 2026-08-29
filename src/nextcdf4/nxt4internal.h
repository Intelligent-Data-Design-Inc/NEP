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

/** Hidden attribute storing a dimension's stable NetCDF dimid. */
#define NEXTCDF4_DIMID_ATT "_Netcdf4Dimid"
/** Hidden attribute storing a variable's stable NetCDF varid. */
#define NEXTCDF4_VARID_ATT "_Nextcdf4Varid"
/** Hidden attribute storing a variable's dimension id list. */
#define NEXTCDF4_VARDIMIDS_ATT "_Nextcdf4VarDimids"
/** HDF5 dimension scale class marker. */
#define NEXTCDF4_DIMCLASS "DIMENSION_SCALE"
/** Optional dimension scale name attribute. */
#define NEXTCDF4_DIMNAME_ATT "NAME"

/** Per-file state owned by the NEXTCDF-4 dispatch layer. */
typedef struct NEXTCDF4_FILE_INFO
{
    hid_t hdfid;        /**< Open HDF5 file identifier, or a negative value. */
    hid_t rootid;       /**< Open HDF5 root-group identifier, or a negative value. */
    char *path;         /**< Owned copy of the file-system path. */
    int mode;           /**< Effective NetCDF create/open mode. */
    int no_write;       /**< Nonzero when the HDF5 file is read-only. */
    int define_mode;    /**< Nonzero while the file is in define mode. */
    int netcdf4_model;  /**< Nonzero for `NC_NETCDF4_MODEL` compatibility mode. */
} NEXTCDF4_FILE_INFO_T;

/** Format-specific information attached to an NC_DIM_INFO_T. */
typedef struct NEXTCDF4_DIM_INFO
{
    hid_t hdf_dataset;  /**< HDF5 dimension scale dataset identifier. */
} NEXTCDF4_DIM_INFO_T;

/** Format-specific information attached to an NC_VAR_INFO_T. */
typedef struct NEXTCDF4_VAR_INFO
{
    hid_t hdf_dataset;  /**< HDF5 variable dataset identifier. */
} NEXTCDF4_VAR_INFO_T;

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

/** Verify the file is in define mode and not read-only.
 * @param file NEXTCDF-4 file state.
 * @return `NC_NOERR` or an appropriate NetCDF error code.
 */
int NEXTCDF4_check_write_define(NEXTCDF4_FILE_INFO_T *file);

/** Map a fixed-size NetCDF atomic type to its HDF5 datatype.
 * @param xtype NetCDF atomic type.
 * @param typep Destination for an HDF5 datatype identifier.
 * @return `NC_NOERR` or `NC_EBADTYPE`.
 */
int NEXTCDF4_map_hdf_type(nc_type xtype, hid_t *typep);

/** Return the in-memory size of a fixed-size NetCDF atomic type.
 * @param xtype NetCDF atomic type.
 * @param sizep Destination for the size in bytes.
 * @return `NC_NOERR` or `NC_EBADTYPE`.
 */
int NEXTCDF4_type_size(nc_type xtype, size_t *sizep);

/** Return a display name for a fixed-size NetCDF atomic type.
 * @param xtype NetCDF atomic type.
 * @return A pointer to a constant string.
 */
const char *NEXTCDF4_type_name(nc_type xtype);

/** Validate a fixed-size atomic type for the active model.
 * @param file NEXTCDF-4 file state.
 * @param xtype Candidate NetCDF type.
 * @return `NC_NOERR` or `NC_EBADTYPE`/`NC_ENOTNC4`.
 */
int NEXTCDF4_check_atomic_type(NEXTCDF4_FILE_INFO_T *file, nc_type xtype);

/** Load all root-group metadata from an open HDF5 file.
 * @param file Open NEXTCDF-4 state.
 * @param h5 Common NetCDF-4 file metadata.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_load_metadata(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5);

/** Materialize pending root-group metadata to HDF5 and exit define mode.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4__enddef(int ncid, size_t h_minfree, size_t v_align,
                     size_t v_minfree, size_t r_align);

/** Return the file to define mode.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_redef(int ncid);

/** Define a dimension in the root group.
 * @param ncid NetCDF file identifier.
 * @param name Dimension name.
 * @param len Dimension length, or 0 for unlimited.
 * @param idp Destination for the assigned dimension id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_dim(int ncid, const char *name, size_t len, int *idp);

/** Inquire a dimension by id.
 * @param ncid NetCDF file identifier.
 * @param dimid Dimension id.
 * @param name Destination for the dimension name.
 * @param lenp Destination for the dimension length.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_dim(int ncid, int dimid, char *name, size_t *lenp);

/** Define a fixed-size atomic variable in the root group.
 * @param ncid NetCDF file identifier.
 * @param name Variable name.
 * @param xtype NetCDF atomic type.
 * @param ndims Number of dimensions.
 * @param dimidsp Array of dimension ids.
 * @param varidp Destination for the assigned variable id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var(int ncid, const char *name, nc_type xtype, int ndims,
                     const int *dimidsp, int *varidp);

/** Inquire a variable by id.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name Destination for the variable name.
 * @param xtypep Destination for the variable type.
 * @param ndimsp Destination for the rank.
 * @param dimidsp Destination for the dimension ids.
 * @param nattsp Destination for the number of attributes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_var(int ncid, int varid, char *name, nc_type *xtypep,
                     int *ndimsp, int *dimidsp, int *nattsp);

/** Put an attribute on the root group or a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param xtype NetCDF atomic type.
 * @param len Number of elements.
 * @param value Attribute data.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_att(int ncid, int varid, const char *name, nc_type xtype,
                     size_t len, const void *value, nc_type memtype);

/** Get an attribute from the root group or a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param value Destination buffer.
 * @param memtype Requested in-memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_att(int ncid, int varid, const char *name, void *value,
                     nc_type memtype);

/** Rename an attribute.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Current attribute name.
 * @param newname New attribute name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_rename_att(int ncid, int varid, const char *name,
                        const char *newname);

/** Delete an attribute.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_del_att(int ncid, int varid, const char *name);

/** Write all in-memory attributes to their HDF5 group or dataset.
 * @param file NEXTCDF-4 file state.
 * @param h5 Common NetCDF-4 file metadata.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_write_attributes(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5);

#endif

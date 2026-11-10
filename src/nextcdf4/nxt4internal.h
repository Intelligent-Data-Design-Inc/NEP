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
#include "nep.h"
#include "nextcdf4dispatch.h"
#include "nxt4err.h"

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
#define NEXTCDF4_VARDIMIDS_ATT "_Netcdf4Coordinates"
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
    int backend_marked; /**< Nonzero when the NEXTCDF-4 backend marker is present. */
} NEXTCDF4_FILE_INFO_T;

/** Format-specific information attached to an NC_GRP_INFO_T. */
typedef struct NEXTCDF4_GRP_INFO
{
    hid_t hdf_group;    /**< Open HDF5 group identifier, or a negative value. */
} NEXTCDF4_GRP_INFO_T;

/** Format-specific information attached to an NC_DIM_INFO_T. */
typedef struct NEXTCDF4_DIM_INFO
{
    hid_t hdf_dataset;  /**< HDF5 dimension scale dataset identifier. */
} NEXTCDF4_DIM_INFO_T;

/** Format-specific information attached to an NC_VAR_INFO_T. */
typedef struct NEXTCDF4_VAR_INFO
{
    hid_t hdf_dataset;  /**< HDF5 variable dataset identifier. */
    int deflate_level;  /**< Deflate level, or -1 if deflate is not applied. */
    int shuffle;        /**< Non-zero if the shuffle filter is enabled. */
    int fletcher32;     /**< Non-zero if the fletcher32 checksum filter is enabled. */
} NEXTCDF4_VAR_INFO_T;

/** Format-specific information attached to an NC_TYPE_INFO_T. */
typedef struct NEXTCDF4_TYPE_INFO
{
    hid_t hdf_type;     /**< Open committed HDF5 datatype identifier, or a negative value. */
} NEXTCDF4_TYPE_INFO_T;

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
 * @param h_minfree Ignored (reserved for future alignment control).
 * @param v_align Ignored (reserved for future alignment control).
 * @param v_minfree Ignored (reserved for future alignment control).
 * @param r_align Ignored (reserved for future alignment control).
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

/** Variable storage-property dispatch callbacks. */
int NEXTCDF4_def_var_chunking(int ncid, int varid, int storage,
                              const size_t *chunksizesp);
/** Set shuffle and deflate compression on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param shuffle Nonzero to enable the shuffle filter.
 * @param deflate Nonzero to enable deflate compression.
 * @param deflate_level Deflate level (0-9).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_deflate(int ncid, int varid, int shuffle,
                             int deflate, int deflate_level);
/** Set the Fletcher32 checksum on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param fletcher32 `NC_FLETCHER32` to enable the checksum, `NC_NOCHECKSUM` to disable.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_fletcher32(int ncid, int varid, int fletcher32);
/** Set fill behavior and the fill value on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param no_fill Nonzero to suppress fill values.
 * @param fill_value Fill value, or NULL for the type default.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value);
/** Set the endianness on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param endianness `NC_ENDIAN_NATIVE`, `NC_ENDIAN_LITTLE` or `NC_ENDIAN_BIG`.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_endian(int ncid, int varid, int endianness);
/** Set quantization on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param quantize_mode `NC_NOQUANTIZE`, `NC_QUANTIZE_BITGROOM`, `NC_QUANTIZE_GRANULARBR` or `NC_QUANTIZE_BITROUND`.
 * @param nsd Number of significant digits.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_quantize(int ncid, int varid, int quantize_mode, int nsd);
/** Set an HDF5 filter on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param id HDF5 filter identifier.
 * @param nparams Number of filter parameters.
 * @param params Filter parameter values.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams,
                            const unsigned int *params);

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
/** Inquire all metadata and storage properties of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name Destination for the variable name.
 * @param xtypep Destination for the variable type.
 * @param ndimsp Destination for the rank.
 * @param dimidsp Destination for the dimension ids.
 * @param nattsp Destination for the number of attributes.
 * @param shufflep Destination for the shuffle flag.
 * @param deflatep Destination for the deflate flag.
 * @param deflate_levelp Destination for the deflate level.
 * @param fletcher32p Destination for the Fletcher32 flag.
 * @param contiguousp Destination for the contiguous-storage flag.
 * @param chunksizesp Destination for the chunk sizes.
 * @param no_fillp Destination for the no-fill flag.
 * @param fill_valuep Destination for the fill value.
 * @param endiannessp Destination for the endianness.
 * @param idp Destination for the filter ids.
 * @param nparamsp Destination for the filter parameter count.
 * @param params Destination for the filter parameters.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                         int *ndimsp, int *dimidsp, int *nattsp,
                         int *shufflep, int *deflatep, int *deflate_levelp,
                         int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                         int *no_fillp, void *fill_valuep, int *endiannessp,
                         unsigned int *idp, size_t *nparamsp, unsigned int *params);
/** Get the filter IDs applied to a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param nfiltersp Destination for the number of filters.
 * @param ids Destination for the filter ids.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_var_filter_ids(int ncid, int varid, size_t *nfiltersp,
                                unsigned int *ids);
/** Get the parameters of one filter on a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param id Filter identifier to inquire about.
 * @param nparamsp Destination for the number of filter parameters.
 * @param params Destination for the filter parameter values.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_var_filter_info(int ncid, int varid, unsigned int id,
                                 size_t *nparamsp, unsigned int *params);

/** Fixed-size atomic variable hyperslab I/O callbacks. */
int NEXTCDF4_get_vara(int ncid, int varid, const size_t *startp, const size_t *countp, void *data, nc_type memtype);
/** Write a hyperslab of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param data Source buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_vara(int ncid, int varid, const size_t *startp, const size_t *countp, const void *data, nc_type memtype);
/** Read a strided hyperslab of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param stridep Hyperslab strides, or NULL.
 * @param data Destination buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_vars(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, void *data, nc_type memtype);
/** Write a strided hyperslab of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param stridep Hyperslab strides, or NULL.
 * @param data Source buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_vars(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const void *data, nc_type memtype);
/** Read a mapped hyperslab of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param stridep Hyperslab strides, or NULL.
 * @param imapp Memory mapping offsets, or NULL.
 * @param data Destination buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_varm(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const ptrdiff_t *imapp, void *data, nc_type memtype);
/** Write a mapped hyperslab of a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param stridep Hyperslab strides, or NULL.
 * @param imapp Memory mapping offsets, or NULL.
 * @param data Source buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_varm(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const ptrdiff_t *imapp, const void *data, nc_type memtype);

/** Put an attribute on the root group or a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id or `NC_GLOBAL`.
 * @param name Attribute name.
 * @param xtype NetCDF atomic type.
 * @param len Number of elements.
 * @param value Attribute data.
 * @param memtype In-memory type for the attribute value.
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

/** Rename a dimension.
 * @param ncid NetCDF file identifier.
 * @param dimid Dimension id.
 * @param name New dimension name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_rename_dim(int ncid, int dimid, const char *name);

/** Rename a variable.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param name New variable name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_rename_var(int ncid, int varid, const char *name);

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

/* Group and type operations. */
/** Create a subgroup below a group.
 * @param ncid NetCDF file identifier of the parent group.
 * @param name Subgroup name.
 * @param grpidp Destination for the new subgroup ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_grp(int ncid, const char *name, int *grpidp);
/** Get the ncid of a named subgroup.
 * @param ncid NetCDF file identifier.
 * @param name Subgroup name.
 * @param grpidp Destination for the subgroup ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_ncid(int ncid, const char *name, int *grpidp);
/** Get the ncids of all subgroups.
 * @param ncid NetCDF file identifier.
 * @param numgrps Destination for the number of subgroups.
 * @param grpidsp Destination for the subgroup ncids.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_grps(int ncid, int *numgrps, int *grpidsp);
/** Get a group's name.
 * @param ncid NetCDF file identifier.
 * @param name Destination for the group name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_grpname(int ncid, char *name);
/** Get a group's full path name.
 * @param ncid NetCDF file identifier.
 * @param lenp Destination for the name length.
 * @param name Destination for the full group name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_grpname_full(int ncid, size_t *lenp, char *name);
/** Get a group's parent ncid.
 * @param ncid NetCDF file identifier.
 * @param parentidp Destination for the parent ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_grp_parent(int ncid, int *parentidp);
/** Get the ncid of a group given its full path.
 * @param ncid NetCDF file identifier.
 * @param full_name Full group path.
 * @param grpidp Destination for the group ncid.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_grp_full_ncid(int ncid, const char *full_name, int *grpidp);
/** Get the typeids of user-defined types in a group.
 * @param ncid NetCDF file identifier.
 * @param ntypes Destination for the number of types.
 * @param typeids Destination for the type ids.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_typeids(int ncid, int *ntypes, int *typeids);

/* User-defined type operations. */
/** Define a compound type.
 * @param ncid NetCDF file identifier.
 * @param size Size of the compound type in bytes.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp);
/** Insert a scalar field into a compound type.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param offset Field offset in bytes.
 * @param xtype NetCDF type of the field.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_insert_compound(int ncid, nc_type typeid, const char *name, size_t offset, nc_type xtype);
/** Insert an array field into a compound type.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param offset Field offset in bytes.
 * @param xtype NetCDF type of the field elements.
 * @param ndims Number of array dimensions.
 * @param dim_sizesp Array dimension sizes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_insert_array_compound(int ncid, nc_type typeid, const char *name,
                                   size_t offset, nc_type xtype, int ndims, const int *dim_sizesp);
/** Inquire a field of a compound type.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param fieldid Field index.
 * @param name Destination for the field name.
 * @param offsetp Destination for the field offset.
 * @param fieldtypep Destination for the field type.
 * @param ndimsp Destination for the field rank.
 * @param dim_sizesp Destination for the field dimension sizes.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_compound_field(int ncid, nc_type typeid, int fieldid, char *name,
                                size_t *offsetp, nc_type *fieldtypep, int *ndimsp, int *dim_sizesp);
/** Get a compound field's index by name.
 * @param ncid NetCDF file identifier.
 * @param typeid Compound type id.
 * @param name Field name.
 * @param fieldidxp Destination for the field index.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_compound_fieldindex(int ncid, nc_type typeid, const char *name, int *fieldidxp);
/** Define a variable-length type.
 * @param ncid NetCDF file identifier.
 * @param name Type name.
 * @param base_typeid Base type of the vlen elements.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *typeidp);
/** Write a vlen element value.
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Destination for the vlen element.
 * @param len Number of elements.
 * @param data Element data.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_vlen_element(int ncid, int typeid, void *vlen_element,
                              size_t len, const void *data);
/** Read a vlen element value.
 * @param ncid NetCDF file identifier.
 * @param typeid Vlen type id.
 * @param vlen_element Vlen element to read.
 * @param lenp Destination for the element count.
 * @param data Destination for the element data.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_vlen_element(int ncid, int typeid, const void *vlen_element,
                              size_t *lenp, void *data);
/** Define an enum type.
 * @param ncid NetCDF file identifier.
 * @param base_typeid Integer base type id.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_enum(int ncid, nc_type base_typeid, const char *name, nc_type *typeidp);
/** Insert a member into an enum type.
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param name Member name.
 * @param value Member value.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_insert_enum(int ncid, nc_type typeid, const char *name, const void *value);
/** Inquire an enum member by index.
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param idx Member index.
 * @param name Destination for the member name.
 * @param value Destination for the member value.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_enum_member(int ncid, nc_type typeid, int idx, char *name, void *value);
/** Get an enum member name by value.
 * @param ncid NetCDF file identifier.
 * @param typeid Enum type id.
 * @param value Member value.
 * @param identifier Destination for the member name.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_enum_ident(int ncid, nc_type typeid, long long value, char *identifier);
/** Define an opaque type.
 * @param ncid NetCDF file identifier.
 * @param size Size of the opaque data in bytes.
 * @param name Type name.
 * @param typeidp Destination for the new type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_def_opaque(int ncid, size_t size, const char *name, nc_type *typeidp);
/** Commit all user-defined types to the file.
 * @param h5 NetCDF-4 file state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_write_types(NC_FILE_INFO_T *h5);
/** Inquire a user-defined type.
 * @param ncid NetCDF file identifier.
 * @param typeid Type id.
 * @param name Destination for the type name.
 * @param sizep Destination for the type size.
 * @param base_typep Destination for the base type id.
 * @param nfieldsp Destination for the number of fields/members.
 * @param classp Destination for the type class.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_user_type(int ncid, nc_type typeid, char *name, size_t *sizep,
                           nc_type *base_typep, size_t *nfieldsp, int *classp);
/** Get a type's id by name.
 * @param ncid NetCDF file identifier.
 * @param name Type name.
 * @param typeidp Destination for the type id.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_typeid(int ncid, const char *name, nc_type *typeidp);
/** Compare two types for equality.
 * @param ncid1 NetCDF file identifier of the first type.
 * @param typeid1 First type id.
 * @param ncid2 NetCDF file identifier of the second type.
 * @param typeid2 Second type id.
 * @param equalp Destination for the nonzero-if-equal flag.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, nc_type typeid2, int *equalp);

/* String and user-defined type I/O helpers. */
int NEXTCDF4_get_type_size(nc_type xtype, size_t *sizep);

#endif

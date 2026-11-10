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
 */
int NEXTCDF4_add_file(int ncid, const char *path, int mode,
                      NEXTCDF4_FILE_INFO_T **filep);
/** Resolve registered NEXTCDF-4 state from a NetCDF identifier.
 */
int NEXTCDF4_get_file(int ncid, NC_FILE_INFO_T **h5p,
                      NEXTCDF4_FILE_INFO_T **filep);
/** Close HDF5 identifiers and free per-file state.
 */
int NEXTCDF4_free_file(NEXTCDF4_FILE_INFO_T *file);
/** Write NEXTCDF-4 provenance and compatibility markers.
 */
int NEXTCDF4_write_markers(NEXTCDF4_FILE_INFO_T *file);
/** Read and validate NEXTCDF-4 provenance and compatibility markers.
 */
int NEXTCDF4_read_markers(NEXTCDF4_FILE_INFO_T *file);

/** Verify the file is in define mode and not read-only.
 */
int NEXTCDF4_check_write_define(NEXTCDF4_FILE_INFO_T *file);

/** Map a fixed-size NetCDF atomic type to its HDF5 datatype.
 */
int NEXTCDF4_map_hdf_type(nc_type xtype, hid_t *typep);

/** Return the in-memory size of a fixed-size NetCDF atomic type.
 */
int NEXTCDF4_type_size(nc_type xtype, size_t *sizep);

/** Return a display name for a fixed-size NetCDF atomic type.
 */
const char *NEXTCDF4_type_name(nc_type xtype);

/** Validate a fixed-size atomic type for the active model.
 */
int NEXTCDF4_check_atomic_type(NEXTCDF4_FILE_INFO_T *file, nc_type xtype);

/** Load all root-group metadata from an open HDF5 file.
 */
int NEXTCDF4_load_metadata(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5);

/** Materialize pending root-group metadata to HDF5 and exit define mode.
 */
int NEXTCDF4__enddef(int ncid, size_t h_minfree, size_t v_align,
                     size_t v_minfree, size_t r_align);

/** Return the file to define mode.
 */
int NEXTCDF4_redef(int ncid);

/** Define a dimension in the root group.
 */
int NEXTCDF4_def_dim(int ncid, const char *name, size_t len, int *idp);

/** Inquire a dimension by id.
 */
int NEXTCDF4_inq_dim(int ncid, int dimid, char *name, size_t *lenp);

/** Define a fixed-size atomic variable in the root group.
 */
int NEXTCDF4_def_var(int ncid, const char *name, nc_type xtype, int ndims,
                     const int *dimidsp, int *varidp);

/** Variable storage-property dispatch callbacks. */
int NEXTCDF4_def_var_chunking(int ncid, int varid, int storage,
                              const size_t *chunksizesp);
/** Set shuffle and deflate compression on a variable.
 */
int NEXTCDF4_def_var_deflate(int ncid, int varid, int shuffle,
                             int deflate, int deflate_level);
/** Set the Fletcher32 checksum on a variable.
 */
int NEXTCDF4_def_var_fletcher32(int ncid, int varid, int fletcher32);
/** Set fill behavior and the fill value on a variable.
 */
int NEXTCDF4_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value);
/** Set the endianness on a variable.
 */
int NEXTCDF4_def_var_endian(int ncid, int varid, int endianness);
/** Set quantization on a variable.
 */
int NEXTCDF4_def_var_quantize(int ncid, int varid, int quantize_mode, int nsd);
/** Set an HDF5 filter on a variable.
 */
int NEXTCDF4_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams,
                            const unsigned int *params);

/** Inquire a variable by id.
 */
int NEXTCDF4_inq_var(int ncid, int varid, char *name, nc_type *xtypep,
                     int *ndimsp, int *dimidsp, int *nattsp);
/** Inquire all metadata and storage properties of a variable.
 */
int NEXTCDF4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                         int *ndimsp, int *dimidsp, int *nattsp,
                         int *shufflep, int *deflatep, int *deflate_levelp,
                         int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                         int *no_fillp, void *fill_valuep, int *endiannessp,
                         unsigned int *idp, size_t *nparamsp, unsigned int *params);
/** Get the filter IDs applied to a variable.
 */
int NEXTCDF4_inq_var_filter_ids(int ncid, int varid, size_t *nfiltersp,
                                unsigned int *ids);
/** Get the parameters of one filter on a variable.
 */
int NEXTCDF4_inq_var_filter_info(int ncid, int varid, unsigned int id,
                                 size_t *nparamsp, unsigned int *params);

/** Fixed-size atomic variable hyperslab I/O callbacks. */
int NEXTCDF4_get_vara(int ncid, int varid, const size_t *startp, const size_t *countp, void *data, nc_type memtype);
/** Write a hyperslab of a variable.
 */
int NEXTCDF4_put_vara(int ncid, int varid, const size_t *startp, const size_t *countp, const void *data, nc_type memtype);
/** Read a strided hyperslab of a variable.
 */
int NEXTCDF4_get_vars(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, void *data, nc_type memtype);
/** Write a strided hyperslab of a variable.
 */
int NEXTCDF4_put_vars(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const void *data, nc_type memtype);
/** Read a mapped hyperslab of a variable.
 */
int NEXTCDF4_get_varm(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const ptrdiff_t *imapp, void *data, nc_type memtype);
/** Write a mapped hyperslab of a variable.
 */
int NEXTCDF4_put_varm(int ncid, int varid, const size_t *startp, const size_t *countp, const ptrdiff_t *stridep, const ptrdiff_t *imapp, const void *data, nc_type memtype);

/** Put an attribute on the root group or a variable.
 */
int NEXTCDF4_put_att(int ncid, int varid, const char *name, nc_type xtype,
                     size_t len, const void *value, nc_type memtype);

/** Get an attribute from the root group or a variable.
 */
int NEXTCDF4_get_att(int ncid, int varid, const char *name, void *value,
                     nc_type memtype);

/** Rename an attribute.
 */
int NEXTCDF4_rename_att(int ncid, int varid, const char *name,
                        const char *newname);

/** Rename a dimension.
 */
int NEXTCDF4_rename_dim(int ncid, int dimid, const char *name);

/** Rename a variable.
 */
int NEXTCDF4_rename_var(int ncid, int varid, const char *name);

/** Delete an attribute.
 */
int NEXTCDF4_del_att(int ncid, int varid, const char *name);

/** Write all in-memory attributes to their HDF5 group or dataset.
 */
int NEXTCDF4_write_attributes(NEXTCDF4_FILE_INFO_T *file, NC_FILE_INFO_T *h5);

/* Group and type operations. */
/** Create a subgroup below a group.
 */
int NEXTCDF4_def_grp(int ncid, const char *name, int *grpidp);
/** Get the ncid of a named subgroup.
 */
int NEXTCDF4_inq_ncid(int ncid, const char *name, int *grpidp);
/** Get the ncids of all subgroups.
 */
int NEXTCDF4_inq_grps(int ncid, int *numgrps, int *grpidsp);
/** Get a group's name.
 */
int NEXTCDF4_inq_grpname(int ncid, char *name);
/** Get a group's full path name.
 */
int NEXTCDF4_inq_grpname_full(int ncid, size_t *lenp, char *name);
/** Get a group's parent ncid.
 */
int NEXTCDF4_inq_grp_parent(int ncid, int *parentidp);
/** Get the ncid of a group given its full path.
 */
int NEXTCDF4_inq_grp_full_ncid(int ncid, const char *full_name, int *grpidp);
/** Get the typeids of user-defined types in a group.
 */
int NEXTCDF4_inq_typeids(int ncid, int *ntypes, int *typeids);

/* User-defined type operations. */
/** Define a compound type.
 */
int NEXTCDF4_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp);
/** Insert a scalar field into a compound type.
 */
int NEXTCDF4_insert_compound(int ncid, nc_type typeid, const char *name, size_t offset, nc_type xtype);
/** Insert an array field into a compound type.
 */
int NEXTCDF4_insert_array_compound(int ncid, nc_type typeid, const char *name,
                                   size_t offset, nc_type xtype, int ndims, const int *dim_sizesp);
/** Inquire a field of a compound type.
 */
int NEXTCDF4_inq_compound_field(int ncid, nc_type typeid, int fieldid, char *name,
                                size_t *offsetp, nc_type *fieldtypep, int *ndimsp, int *dim_sizesp);
/** Get a compound field's index by name.
 */
int NEXTCDF4_inq_compound_fieldindex(int ncid, nc_type typeid, const char *name, int *fieldidxp);
/** Define a variable-length type.
 */
int NEXTCDF4_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *typeidp);
/** Write a vlen element value.
 */
int NEXTCDF4_put_vlen_element(int ncid, int typeid, void *vlen_element,
                              size_t len, const void *data);
/** Read a vlen element value.
 */
int NEXTCDF4_get_vlen_element(int ncid, int typeid, const void *vlen_element,
                              size_t *lenp, void *data);
/** Define an enum type.
 */
int NEXTCDF4_def_enum(int ncid, nc_type base_typeid, const char *name, nc_type *typeidp);
/** Insert a member into an enum type.
 */
int NEXTCDF4_insert_enum(int ncid, nc_type typeid, const char *name, const void *value);
/** Inquire an enum member by index.
 */
int NEXTCDF4_inq_enum_member(int ncid, nc_type typeid, int idx, char *name, void *value);
/** Get an enum member name by value.
 */
int NEXTCDF4_inq_enum_ident(int ncid, nc_type typeid, long long value, char *identifier);
/** Define an opaque type.
 */
int NEXTCDF4_def_opaque(int ncid, size_t size, const char *name, nc_type *typeidp);
/** Commit all user-defined types to the file.
 */
int NEXTCDF4_write_types(NC_FILE_INFO_T *h5);
/** Inquire a user-defined type.
 */
int NEXTCDF4_inq_user_type(int ncid, nc_type typeid, char *name, size_t *sizep,
                           nc_type *base_typep, size_t *nfieldsp, int *classp);
/** Get a type's id by name.
 */
int NEXTCDF4_inq_typeid(int ncid, const char *name, nc_type *typeidp);
/** Compare two types for equality.
 */
int NEXTCDF4_inq_type_equal(int ncid1, nc_type typeid1, int ncid2, nc_type typeid2, int *equalp);

/* String and user-defined type I/O helpers. */
int NEXTCDF4_get_type_size(nc_type xtype, size_t *sizep);

#endif

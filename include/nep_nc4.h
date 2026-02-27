/* Temporary abstraction shim for netcdf-c internal structure access.
 *
 * NEP UDF handlers must populate the netcdf-4 in-memory metadata model
 * (groups, variables, dimensions, attributes). This requires access to
 * private netcdf-c structures and functions that have no public API equivalent.
 *
 * This header centralizes that dependency to a single point. When netcdf-c
 * provides a public UDF metadata API, this header and the copied headers it
 * includes can be removed.
 *
 * Tracked at: https://github.com/Unidata/netcdf-c/issues/3277
 * See: docs/plan/v1.5.5-header-cleanup-map.md
 *
 * Do not add new private netcdf-c includes elsewhere in NEP source.
 * All private header access must go through this file.
 */

#ifndef NEP_NC4_H
#define NEP_NC4_H

/* Private netcdf-c headers copied into NEP's include/ directory.
 * See docs/plan/v1.5.5-header-cleanup-map.md for the elimination plan. */
#include "nc4internal.h"   /* NC_FILE_INFO_T, NC_GRP_INFO_T, NC_VAR_INFO_T, NC_ATT_INFO_T, NC_DIM_INFO_T */
#include "nc.h"            /* NC struct, NC_check_id() */
#include "ncdimscale.h"    /* HDF5 dimension scale constants (needed by grib2file.c) */

/* Functions from netcdf-c libsrc4 used by NEP UDF handlers.
 * These are not declared in any public netcdf-c header. */

/* Find group and file info from ncid. */
int nc4_find_grp_h5(int ncid, NC_GRP_INFO_T **grp, NC_FILE_INFO_T **h5);

/* Find NC, group, and file info from ncid. */
int nc4_find_nc_grp_h5(int ncid, NC **nc, NC_GRP_INFO_T **grp,
                        NC_FILE_INFO_T **h5);

/* Find file info, group, and variable from ncid + varid. */
int nc4_find_grp_h5_var(int ncid, int varid, NC_FILE_INFO_T **h5,
                         NC_GRP_INFO_T **grp, NC_VAR_INFO_T **var);

/* Add an attribute to an attribute index (group or variable att list). */
int nc4_att_list_add(NCindex *list, const char *name, NC_ATT_INFO_T **att);

/* Add a dimension to a group. */
int nc4_dim_list_add(NC_GRP_INFO_T *grp, const char *name, size_t len,
                     int assignedid, NC_DIM_INFO_T **dim);

/* Convert data between netcdf types. */
int nc4_convert_type(const void *src, void *dest, const nc_type src_type,
                     const nc_type dest_type, const size_t len,
                     int *range_error, const void *fill_value,
                     int strict_nc3, int quantize_mode, int nsd);

#endif /* NEP_NC4_H */

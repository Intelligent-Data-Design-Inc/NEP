/**
 * @file nep_nc4.h
 * @brief Temporary abstraction layer for netcdf-c internal structures.
 *
 * NEP UDF handlers must populate the netcdf-4 in-memory metadata model
 * (NC_FILE_INFO_T, NC_GRP_INFO_T, NC_VAR_INFO_T, etc.) during file open.
 * This requires access to private netcdf-c structures and functions that
 * have no public API equivalents.
 *
 * This header centralizes that dependency to a single point of contact.
 * When netcdf-c provides a public UDF metadata API (see upstream issue
 * https://github.com/Unidata/netcdf-c/issues/3277), this header and the
 * copied headers it includes can be replaced with a single public include.
 *
 * @note Do NOT add new direct includes of nc4internal.h, nc.h, or
 *       hdf5internal.h in NEP source files. Include this header instead.
 *
 * @see docs/plan/v1.5.5-header-cleanup-map.md for the full elimination plan.
 * @see docs/plan/v1.5.5-sprint2-abstraction-layer.md for Sprint 2 details.
 *
 * Copied headers version: netcdf-c v4.9.2 (NEP commit 2360497, 2025-09-29)
 *
 * @author Edward Hartnett
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#ifndef NEP_NC4_H
#define NEP_NC4_H

/* These are private netcdf-c headers copied into NEP's include/ directory.
 * See docs/plan/v1.5.5-header-cleanup-map.md for the elimination plan.
 *
 * nc4internal.h provides:
 *   - NC_FILE_INFO_T, NC_GRP_INFO_T, NC_VAR_INFO_T, NC_ATT_INFO_T,
 *     NC_DIM_INFO_T, NC_TYPE_INFO_T — the in-memory metadata model
 *   - nc4_file_list_add(), nc4_find_grp_h5(), nc4_find_nc_grp_h5(),
 *     nc4_find_grp_h5_var() — file/group/var lookup
 *   - nc4_dim_list_add(), nc4_var_list_add(), nc4_var_set_ndims(),
 *     nc4_att_list_add(), nc4_type_new() — metadata construction
 *   - nc4_nc4f_list_del() — file cleanup on close
 *   - nc4_convert_type() — type conversion for get_vara
 *
 * nc.h provides:
 *   - NC struct — the top-level file handle
 *   - NC_check_id() — look up NC struct from ncid
 *   Elimination: replace with nc_udf_file_open() when upstream API exists.
 *
 * ncdimscale.h provides:
 *   - HDF5 dimension scale constants used by grib2file.c
 *   Elimination: constants can be defined locally once grib2 is fully
 *   implemented and the dependency is confirmed minimal.
 */
#include "nc4internal.h"
#include "nc.h"
#include "ncdimscale.h"

#endif /* NEP_NC4_H */

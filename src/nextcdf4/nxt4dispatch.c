/**
 * @file nxt4dispatch.c
 * @brief UDF9 dispatch table registration for the NEXTCDF-4 backend.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include "nextcdf4dispatch.h"
#include "nc4dispatch.h"
#include "hdf5dispatch.h"
#include "netcdf_filter.h"
#include "nxt4internal.h"

/**
 * @internal Report that variable reads are not implemented yet.
 * @param ncid NetCDF file identifier.
 * @param varid NetCDF variable identifier.
 * @param start Hyperslab start indices.
 * @param count Hyperslab element counts.
 * @param value Destination data buffer.
 * @param memtype Requested in-memory NetCDF type.
 * @return `NC_ENOTBUILT`.
 */
static int
NEXTCDF4_get_vara(int ncid, int varid, const size_t *start,
                  const size_t *count, void *value, nc_type memtype)
{
    (void)ncid;
    (void)varid;
    (void)start;
    (void)count;
    (void)value;
    (void)memtype;
    return NC_ENOTBUILT;
}

/** Complete NetCDF dispatch table for NEXTCDF-4 UDF slot 9. */
static const NC_Dispatch NEXTCDF4_dispatcher = {
    NC_FORMATX_NEXTCDF4,
    NC_DISPATCH_VERSION,
    NEXTCDF4_create,
    NEXTCDF4_open,
    NC_RO_redef,
    NC_RO__enddef,
    NEXTCDF4_sync,
    NEXTCDF4_abort,
    NEXTCDF4_close,
    NC_RO_set_fill,
    NEXTCDF4_inq_format,
    NEXTCDF4_inq_format_extended,
    NC4_inq,
    NC4_inq_type,
    NC_RO_def_dim,
    NC4_inq_dimid,
    HDF5_inq_dim,
    NC4_inq_unlimdim,
    NC_RO_rename_dim,
    NC4_inq_att,
    NC4_inq_attid,
    NC4_inq_attname,
    NC_RO_rename_att,
    NC_RO_del_att,
    NC4_get_att,
    NC_RO_put_att,
    NC_RO_def_var,
    NC4_inq_varid,
    NC_RO_rename_var,
    NEXTCDF4_get_vara,
    NC_RO_put_vara,
    NCDEFAULT_get_vars,
    NCDEFAULT_put_vars,
    NCDEFAULT_get_varm,
    NCDEFAULT_put_varm,
    NC4_inq_var_all,
    NC_NOTNC4_var_par_access,
    NC_RO_def_var_fill,
    NC4_show_metadata,
    NC4_inq_unlimdims,
    NC4_inq_ncid,
    NC4_inq_grps,
    NC4_inq_grpname,
    NC4_inq_grpname_full,
    NC4_inq_grp_parent,
    NC4_inq_grp_full_ncid,
    NC4_inq_varids,
    NC4_inq_dimids,
    NC4_inq_typeids,
    NC4_inq_type_equal,
    NC_NOTNC4_def_grp,
    NC_NOTNC4_rename_grp,
    NC_NOTNC4_inq_user_type,
    NC_NOTNC4_inq_typeid,
    NC_NOTNC4_def_compound,
    NC_NOTNC4_insert_compound,
    NC_NOTNC4_insert_array_compound,
    NC_NOTNC4_inq_compound_field,
    NC_NOTNC4_inq_compound_fieldindex,
    NC_NOTNC4_def_vlen,
    NC_NOTNC4_put_vlen_element,
    NC_NOTNC4_get_vlen_element,
    NC_NOTNC4_def_enum,
    NC_NOTNC4_insert_enum,
    NC_NOTNC4_inq_enum_member,
    NC_NOTNC4_inq_enum_ident,
    NC_NOTNC4_def_opaque,
    NC_NOTNC4_def_var_deflate,
    NC_NOTNC4_def_var_fletcher32,
    NC_NOTNC4_def_var_chunking,
    NC_NOTNC4_def_var_endian,
    NC_NOTNC4_def_var_filter,
    NC_NOTNC4_set_var_chunk_cache,
    NC_NOTNC4_get_var_chunk_cache,
    NC_NOOP_inq_var_filter_ids,
    NC_NOOP_inq_var_filter_info,
    NC_NOTNC4_def_var_quantize,
    NC_NOTNC4_inq_var_quantize,
    NC_NOOP_inq_filter_avail
};

/** Active NEXTCDF-4 dispatch table, populated during initialization. */
const NC_Dispatch *NEXTCDF4_dispatch_table = NULL;

/**
 * Register the NEXTCDF-4 dispatch table in NetCDF UDF slot 9.
 * @return Pointer to the NEXTCDF-4 dispatch table.
 */
NC_Dispatch *
NC_NEXTCDF4_initialize(void)
{
    NEXTCDF4_dispatch_table = &NEXTCDF4_dispatcher;
    nc_def_user_format(NEP_UDF_NEXTCDF4, (NC_Dispatch *)NEXTCDF4_dispatch_table, NULL);
    return (NC_Dispatch *)&NEXTCDF4_dispatcher;
}

/**
 * Finalize process-wide NEXTCDF-4 dispatch state.
 * @return `NC_NOERR`.
 */
int
NC_NEXTCDF4_finalize(void)
{
    return NC_NOERR;
}

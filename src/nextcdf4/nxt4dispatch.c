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

/*
 * Complete NetCDF dispatch table for NEXTCDF-4 UDF slot 9.
 */
static const NC_Dispatch NEXTCDF4_dispatcher = {
    NC_FORMATX_NEXTCDF4,
    NC_DISPATCH_VERSION,
    NEXTCDF4_create,
    NEXTCDF4_open,
    NEXTCDF4_redef,
    NEXTCDF4__enddef,
    NEXTCDF4_sync,
    NEXTCDF4_abort,
    NEXTCDF4_close,
    NC_RO_set_fill,
    NEXTCDF4_inq_format,
    NEXTCDF4_inq_format_extended,
    NC4_inq,
    NC4_inq_type,
    NEXTCDF4_def_dim,
    NC4_inq_dimid,
    NEXTCDF4_inq_dim,
    NC4_inq_unlimdim,
    NEXTCDF4_rename_dim,
    NC4_inq_att,
    NC4_inq_attid,
    NC4_inq_attname,
    NEXTCDF4_rename_att,
    NEXTCDF4_del_att,
    NEXTCDF4_get_att,
    NEXTCDF4_put_att,
    NEXTCDF4_def_var,
    NC4_inq_varid,
    NEXTCDF4_rename_var,
    NEXTCDF4_get_vara,
    NEXTCDF4_put_vara,
    NEXTCDF4_get_vars,
    NEXTCDF4_put_vars,
    NEXTCDF4_get_varm,
    NEXTCDF4_put_varm,
    NEXTCDF4_inq_var_all,
    NC_NOTNC4_var_par_access,
    NEXTCDF4_def_var_fill,
    NC4_show_metadata,
    NC4_inq_unlimdims,
    NEXTCDF4_inq_ncid,
    NEXTCDF4_inq_grps,
    NEXTCDF4_inq_grpname,
    NEXTCDF4_inq_grpname_full,
    NEXTCDF4_inq_grp_parent,
    NEXTCDF4_inq_grp_full_ncid,
    NC4_inq_varids,
    NC4_inq_dimids,
    NEXTCDF4_inq_typeids,
    NEXTCDF4_inq_type_equal,
    NEXTCDF4_def_grp,
    NC_NOTNC4_rename_grp,
    NEXTCDF4_inq_user_type,
    NEXTCDF4_inq_typeid,
    NEXTCDF4_def_compound,
    NEXTCDF4_insert_compound,
    NEXTCDF4_insert_array_compound,
    NEXTCDF4_inq_compound_field,
    NEXTCDF4_inq_compound_fieldindex,
    NEXTCDF4_def_vlen,
    NEXTCDF4_put_vlen_element,
    NEXTCDF4_get_vlen_element,
    NEXTCDF4_def_enum,
    NEXTCDF4_insert_enum,
    NEXTCDF4_inq_enum_member,
    NEXTCDF4_inq_enum_ident,
    NEXTCDF4_def_opaque,
    NEXTCDF4_def_var_deflate,
    NEXTCDF4_def_var_fletcher32,
    NEXTCDF4_def_var_chunking,
    NEXTCDF4_def_var_endian,
    NEXTCDF4_def_var_filter,
    NC_NOTNC4_set_var_chunk_cache,
    NC_NOTNC4_get_var_chunk_cache,
    NEXTCDF4_inq_var_filter_ids,
    NEXTCDF4_inq_var_filter_info,
    NEXTCDF4_def_var_quantize,
    NC4_inq_var_quantize,
    NC_NOOP_inq_filter_avail
};

/*
 * Active NEXTCDF-4 dispatch table, populated during initialization.
 */
const NC_Dispatch *NEXTCDF4_dispatch_table = NULL;

/*
 * Register the NEXTCDF-4 dispatch table in NetCDF UDF slot 9.
 */
NC_Dispatch *
NC_NEXTCDF4_initialize(void)
{
    NEXTCDF4_dispatch_table = &NEXTCDF4_dispatcher;
    nc_def_user_format(NEP_UDF_NEXTCDF4, (NC_Dispatch *)NEXTCDF4_dispatch_table, NULL);
    return (NC_Dispatch *)&NEXTCDF4_dispatcher;
}

/*
 * Finalize process-wide NEXTCDF-4 dispatch state.
 */
int
NC_NEXTCDF4_finalize(void)
{
    return NC_NOERR;
}

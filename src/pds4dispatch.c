/**
 * @file
 * @internal Dispatch code for PDS4. PDS4 access is read-only.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include "pds4dispatch.h"
#include "nc4dispatch.h"
#include "hdf5dispatch.h"
#include "netcdf_filter.h"

/* This is the dispatch object that holds pointers to all the
 * functions that make up the PDS4 dispatch interface. */
static const NC_Dispatch PDS4_dispatcher = {

    NC_FORMATX_NC_PDS4,
    NC_DISPATCH_VERSION,

    NC_RO_create,
    NC_PDS4_open,

    NC_RO_redef,
    NC_RO__enddef,
    NC_RO_sync,
    NC_PDS4_abort,
    NC_PDS4_close,
    NC_RO_set_fill,
    NC_PDS4_inq_format,
    NC_PDS4_inq_format_extended,

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
    NC_PDS4_get_vara,
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
    NC4_inq_user_type,
    NC4_inq_typeid,

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

    NC_NOOP_inq_filter_avail,
};

const NC_Dispatch *PDS4_dispatch_table = NULL;

/**
 * Initialize the PDS4 dispatch layer.
 *
 * Registers the PDS4 handler via `nc_def_user_format()` for direct
 * calls, and returns the dispatch table pointer for the self-registration
 * (`HAVE_NETCDF_UDF_SELF_REGISTRATION`) path. Safe to call when the
 * handler has already been registered via `.ncrc` autoload; in that
 * case `NC_EINVAL` from `nc_def_user_format()` is silently ignored.
 *
 * @return Pointer to PDS4 dispatch table, or NULL on failure.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
NC_Dispatch*
NC_PDS4_initialize(void)
{
    PDS4_dispatch_table = &PDS4_dispatcher;
    nc_def_user_format(NEP_UDF_PDS4, (NC_Dispatch *)PDS4_dispatch_table,
                       NEP_MAGIC_PDS4);
    return (NC_Dispatch*)&PDS4_dispatcher;
}

/**
 * Finalize the PDS4 dispatch layer.
 *
 * No-op in the current implementation.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 * @date 2026-07-08
 */
int
NC_PDS4_finalize(void)
{
    return NC_NOERR;
}

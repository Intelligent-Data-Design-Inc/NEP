/**
 * @file
 * @brief Legacy PDB dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include "pdbdispatch.h"
#include "nc4dispatch.h"
#include "hdf5dispatch.h"
#include "netcdf_filter.h"

/* This is the dispatch object that holds pointers to all the
 * functions that make up the legacy PDB dispatch interface. */
static const NC_Dispatch PDB_dispatcher = {

    NC_FORMATX_NC_PDB,
    NC_DISPATCH_VERSION,

    NC_RO_create,
    NC_PDB_open,

    NC_RO_redef,
    NC_RO__enddef,
    NC_RO_sync,
    NC_PDB_abort,
    NC_PDB_close,
    NC_RO_set_fill,
    NC_PDB_inq_format,
    NC_PDB_inq_format_extended,

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
    NC_PDB_get_vara,
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

const NC_Dispatch *PDB_dispatch_table = NULL;

/**
 * @internal Initialize the legacy PDB dispatch layer.
 *
 * Registers the PDB handler via `nc_def_user_format()` for direct
 * calls, and returns the dispatch table pointer for the self-registration
 * (`HAVE_NETCDF_UDF_SELF_REGISTRATION`) path. Safe to call when the
 * handler has already been registered via `.ncrc` autoload; in that
 * case `NC_EINVAL` from `nc_def_user_format()` is silently ignored.
 *
 * @return Pointer to PDB dispatch table, or NULL on failure.
 * @author Edward Hartnett
 */
NC_Dispatch*
NC_PDB_initialize(void)
{
    PDB_dispatch_table = &PDB_dispatcher;
    nc_def_user_format(NEP_UDF_PDB, (NC_Dispatch *)PDB_dispatch_table,
                       NEP_MAGIC_PDB);
    return (NC_Dispatch*)&PDB_dispatcher;
}

/**
 * @internal Finalize the legacy PDB dispatch layer.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_finalize(void)
{
    return NC_NOERR;
}

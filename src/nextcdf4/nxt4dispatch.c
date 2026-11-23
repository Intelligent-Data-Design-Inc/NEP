/**
 * @file nxt4dispatch.c
 * @brief UDF9 dispatch table registration for the NEXTCDF-4 backend.

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
 * @brief The complete netCDF dispatch table for NEXTCDF-4.
 *
 * A dispatch table is a struct of function pointers, one per netCDF
 * API operation; netcdf-c routes every `nc_*` call through the table
 * of the file it was called on. Entries fall into three groups:
 * functions implemented by NEXTCDF-4 itself (`NEXTCDF4_*`), shared
 * netcdf-c helpers that work on the common `NC_*_INFO_T` metadata
 * (`NC4_*`), and stubs for unsupported operations (`NC_NOTNC4_*`,
 * `NC_NOOP_*`). Fill defaults, group rename and chunk-cache settings
 * use NEXTCDF-4 entries rather than unsupported-operation stubs.
 *
 * The struct has no designated initializers, so entries must appear
 * in the exact positional order netcdf-c defines. See
 * @ref nextcdf4_architecture.
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
    NEXTCDF4_set_fill,
    NEXTCDF4_inq_format,
    NEXTCDF4_inq_format_extended,
    NC4_inq,
    NC4_inq_type,
    NEXTCDF4_def_dim,
    NC4_inq_dimid,
    NEXTCDF4_inq_dim,
    NC4_inq_unlimdim,
    NEXTCDF4_rename_dim,
    NEXTCDF4_inq_att,
    NEXTCDF4_inq_attid,
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
    NEXTCDF4_rename_grp,
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
    NEXTCDF4_set_var_chunk_cache,
    NEXTCDF4_get_var_chunk_cache,
    NEXTCDF4_inq_var_filter_ids,
    NEXTCDF4_inq_var_filter_info,
    NEXTCDF4_def_var_quantize,
    NC4_inq_var_quantize,
    NEXTCDF4_inq_filter_avail
};

/**
 * @brief The active NEXTCDF-4 dispatch table.
 *
 * Set by NC_NEXTCDF4_initialize(); `NULL` until the backend has been
 * registered.
 */
const NC_Dispatch *NEXTCDF4_dispatch_table = NULL;

/**
 * @brief Register the NEXTCDF-4 dispatch table with netcdf-c.
 *
 * Installs #NEXTCDF4_dispatcher into UDF slot 9 by calling
 * netcdf-c's `nc_def_user_format()`. Applications must call this once
 * before using `NC_NEXTCDF4` (unless the backend is autoloaded via
 * `.ncrc`). Also reads the `NEP_LOG_LEVEL` environment variable to
 * set the initial log level when logging is compiled in.
 *
 * @return Pointer to the NEXTCDF-4 dispatch table.
 */
NC_Dispatch *
NC_NEXTCDF4_initialize(void)
{
    NEXTCDF4_dispatch_table = &NEXTCDF4_dispatcher;
    nc_def_user_format(NEP_UDF_NEXTCDF4, (NC_Dispatch *)NEXTCDF4_dispatch_table, NULL);

#ifdef NEP_LOGGING
    {
        const char *env = getenv("NEP_LOG_LEVEL");
        if (env) {
            int level = atoi(env);
            if (level >= 0)
                nep_set_log_level(level);
        }
    }
#endif /* NEP_LOGGING */

    return (NC_Dispatch *)&NEXTCDF4_dispatcher;
}

/**
 * @brief Finalize process-wide NEXTCDF-4 dispatch state.
 *
 * NEXTCDF-4 currently keeps no state that needs teardown, so this is
 * a placeholder for symmetry with NC_NEXTCDF4_initialize() and the
 * other NEP backends.
 *
 * @return `NC_NOERR`.
 */
int
NC_NEXTCDF4_finalize(void)
{
    return NC_NOERR;
}

/**
 * @file
 * @internal Dispatch code for GRIB2. GRIB2 access is read-only.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include "grib2dispatch.h"
#include "nc4dispatch.h"
#include "hdf5dispatch.h"
#include "netcdf_filter.h"

/* This is the dispatch object that holds pointers to all the
 * functions that make up the GRIB2 dispatch interface. */
static const NC_Dispatch GRIB2_dispatcher = {

    NC_FORMATX_NC_GRIB2,
    NC_DISPATCH_VERSION,

    NC_RO_create,
    NC_GRIB2_open,

    NC_RO_redef,
    NC_RO__enddef,
    NC_RO_sync,
    NC_GRIB2_abort,
    NC_GRIB2_close,
    NC_RO_set_fill,
    NC_GRIB2_inq_format,
    NC_GRIB2_inq_format_extended,

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
    NC_GRIB2_get_vara,
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

const NC_Dispatch *GRIB2_dispatch_table = NULL;

/**
 * @internal Initialize GRIB2 dispatch layer.
 *
 * @return Dispatch table pointer for self-loading, or NULL on error.
 * @author Edward Hartnett
 */
NC_Dispatch*
NC_GRIB2_initialize(void)
{
    GRIB2_dispatch_table = &GRIB2_dispatcher;
    return (NC_Dispatch*)&GRIB2_dispatcher;
}

/**
 * @internal Finalize GRIB2 dispatch layer.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_GRIB2_finalize(void)
{
    return NC_NOERR;
}

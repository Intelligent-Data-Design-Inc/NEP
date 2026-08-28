#include "config.h"
#include "nextcdf4dispatch.h"
#include "nc4dispatch.h"
#include "hdf5dispatch.h"
#include "netcdf_filter.h"

int
NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                size_t *chunksizehintp, void *parameters,
                const NC_Dispatch *dispatch, int ncid)
{
    (void)path;
    (void)cmode;
    (void)initialsz;
    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;
    (void)ncid;
    return NC_ENOTBUILT;
}

int
NEXTCDF4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    (void)path;
    (void)mode;
    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;
    (void)ncid;
    return NC_ENOTBUILT;
}

static int
NEXTCDF4_abort(int ncid)
{
    (void)ncid;
    return NC_ENOTBUILT;
}

static int
NEXTCDF4_close(int ncid, void *parameters)
{
    (void)ncid;
    (void)parameters;
    return NC_ENOTBUILT;
}

static int
NEXTCDF4_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

static int
NEXTCDF4_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NEXTCDF4;
    if (modep)
        *modep = NC_NEXTCDF4;
    return NC_NOERR;
}

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

static const NC_Dispatch NEXTCDF4_dispatcher = {
    NC_FORMATX_NEXTCDF4,
    NC_DISPATCH_VERSION,
    NEXTCDF4_create,
    NEXTCDF4_open,
    NC_RO_redef,
    NC_RO__enddef,
    NC_RO_sync,
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

const NC_Dispatch *NEXTCDF4_dispatch_table = NULL;

NC_Dispatch *
NC_NEXTCDF4_initialize(void)
{
    NEXTCDF4_dispatch_table = &NEXTCDF4_dispatcher;
    nc_def_user_format(NEP_UDF_NEXTCDF4, (NC_Dispatch *)NEXTCDF4_dispatch_table, NULL);
    return (NC_Dispatch *)&NEXTCDF4_dispatcher;
}

int
NC_NEXTCDF4_finalize(void)
{
    return NC_NOERR;
}

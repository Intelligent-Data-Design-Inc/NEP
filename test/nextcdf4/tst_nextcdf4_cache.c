/* Effective HDF5 cache controls, including coordinate aliases.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include "nxt4internal.h"

#define CHECK(call) do { int r = (call); if (r != NC_NOERR) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(r)); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)

static int
check_cache(int ncid, int varid, size_t bytes, size_t slots, float weight)
{
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NEXTCDF4_VAR_INFO_T *info;
    size_t b, s;
    float w;
    hid_t dapl;
    double dw;

    CHECK(nc_get_var_chunk_cache(ncid, varid, &b, &s, &w));
    REQUIRE(b == bytes && s == slots && w == weight);
    CHECK(nc_get_var_chunk_cache(ncid, varid, NULL, NULL, NULL));
    CHECK(nc4_find_nc4_grp(ncid, &grp));
    var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid);
    info = var->format_var_info;
    if (info->hdf_dataset >= 0 && var->storage == NC_CHUNKED) {
        REQUIRE((dapl = H5Dget_access_plist(info->hdf_dataset)) >= 0);
        REQUIRE(H5Pget_chunk_cache(dapl, &s, &b, &dw) >= 0);
        REQUIRE(H5Pclose(dapl) >= 0);
        REQUIRE(b == bytes && s == slots && (float)dw == weight);
        if (var->ndims == 1 && !strcmp(var->hdr.name, var->dim[0]->hdr.name)) {
            NEXTCDF4_DIM_INFO_T *dim = var->dim[0]->format_dim_info;
            REQUIRE(dim->hdf_dataset != info->hdf_dataset);
            REQUIRE((dapl = H5Dget_access_plist(dim->hdf_dataset)) >= 0);
            REQUIRE(H5Pget_chunk_cache(dapl, &s, &b, &dw) >= 0);
            REQUIRE(H5Pclose(dapl) >= 0);
            REQUIRE(b == bytes && s == slots && (float)dw == weight);
        }
    }
    return 0;
}

static int
run_mode(int mode)
{
    int ncid, dim, coord, data, pending = -1, scalar, plain, out[3];
    int values[3] = {1, 2, 3}, attr = 71;
    size_t start = 0, count = 3, chunk = 2;
    nc_type type;
    size_t default_bytes, default_slots;
    float default_weight;
    nc_vlen_t input = {3, values}, output = {0, NULL};

    CHECK(nc_get_chunk_cache(&default_bytes, &default_slots, &default_weight));
    CHECK(nc_set_chunk_cache(2097152, 1009, 0.5f));
    CHECK(nc_create("cache.nc", NC_NEXTCDF4 | mode, &ncid));
    CHECK(nc_def_dim(ncid, "x", NC_UNLIMITED, &dim));
    CHECK(nc_def_var(ncid, "x", NC_INT, 1, &dim, &coord));
    CHECK(nc_def_var(ncid, "data", NC_INT, 1, &dim, &data));
    CHECK(nc_def_var(ncid, "scalar", NC_INT, 0, NULL, &scalar));
    CHECK(nc_def_dim(ncid, "fixed", 3, &plain));
    CHECK(nc_def_var(ncid, "plain", NC_INT, 1, &plain, &plain));
    REQUIRE(!check_cache(ncid, data, 2097152, 1009, 0.5f));
    CHECK(nc_set_var_chunk_cache(ncid, coord, 1048576, 521, 0.25f));
    CHECK(nc_set_var_chunk_cache(ncid, data, 4194304, 1031, 1.0f));
    CHECK(nc_set_var_chunk_cache(ncid, scalar, 1000, 17, 0.0f));
    CHECK(nc_set_var_chunk_cache(ncid, plain, 1000, 17, 0.0f));
    if (!(mode & NC_CLASSIC_MODEL)) {
        CHECK(nc_def_var_chunking(ncid, coord, NC_CHUNKED, &chunk));
        CHECK(nc_def_var_deflate(ncid, data, 1, 1, 1));
        CHECK(nc_def_vlen(ncid, "ints", NC_INT, &type));
        CHECK(nc_def_var(ncid, "pending", type, 1, &dim, &pending));
        CHECK(nc_set_var_chunk_cache(ncid, pending, 1048576, 521, 0.5f));
        REQUIRE(!check_cache(ncid, pending, 1048576, 521, 0.5f));
    }
    CHECK(nc_put_att_int(ncid, data, "tag", NC_INT, 1, &attr));
    CHECK(nc_enddef(ncid));
    REQUIRE(!check_cache(ncid, coord, 1048576, 521, 0.25f));
    REQUIRE(!check_cache(ncid, scalar, 1000, 17, 0.0f));
    REQUIRE(!check_cache(ncid, plain, 1000, 17, 0.0f));
    CHECK(nc_put_vara_int(ncid, coord, &start, &count, values));
    CHECK(nc_put_vara_int(ncid, data, &start, &count, values));
    CHECK(nc_set_var_chunk_cache(ncid, coord, 8388608, 2003, 1.0f));
    CHECK(nc_set_var_chunk_cache(ncid, data, 0, 0, 0.0f));
    REQUIRE(!check_cache(ncid, coord, 8388608, 2003, 1.0f));
    REQUIRE(!check_cache(ncid, data, 0, 0, 0.0f));
    CHECK(nc_get_var_int(ncid, data, out));
    REQUIRE(!memcmp(out, values, sizeof(out)));
    CHECK(nc_get_var_int(ncid, coord, out));
    REQUIRE(!memcmp(out, values, sizeof(out)));
    CHECK(nc_get_att_int(ncid, data, "tag", out));
    REQUIRE(out[0] == attr);
    if (pending >= 0) {
        int shuffle, deflate, level;
        count = 1;
        CHECK(nc_put_vara(ncid, pending, &start, &count, &input));
        CHECK(nc_set_var_chunk_cache(ncid, pending, 524288, 101, 0.0f));
        REQUIRE(!check_cache(ncid, pending, 524288, 101, 0.0f));
        CHECK(nc_get_vara(ncid, pending, &start, &count, &output));
        REQUIRE(output.len == 3 && !memcmp(output.p, values, sizeof(values)));
        CHECK(nc_reclaim_data(ncid, type, &output, 1));
        CHECK(nc_inq_var_deflate(ncid, data, &shuffle, &deflate, &level));
        REQUIRE(shuffle && deflate && level == 1);
    }
    REQUIRE(nc_set_var_chunk_cache(ncid, data, 1, 1, NAN) == NC_EINVAL);
    REQUIRE(nc_set_var_chunk_cache(ncid, data, 1, 1, -0.1f) == NC_EINVAL);
    REQUIRE(nc_set_var_chunk_cache(ncid, data, 1, 1, 1.1f) == NC_EINVAL);
    REQUIRE(nc_set_var_chunk_cache(ncid, -1, 1, 1, 0) == NC_ENOTVAR);
    REQUIRE(nc_get_var_chunk_cache(ncid, -1, NULL, NULL, NULL) == NC_ENOTVAR);
    CHECK(nc_close(ncid));
    CHECK(nc_open("cache.nc", NC_NEXTCDF4, &ncid));
    CHECK(nc_inq_varid(ncid, "x", &coord));
    REQUIRE(!check_cache(ncid, coord, 2097152, 1009, 0.5f));
    CHECK(nc_set_var_chunk_cache(ncid, coord, 1048576, 521, 0.25f));
    REQUIRE(!check_cache(ncid, coord, 1048576, 521, 0.25f));
    CHECK(nc_get_var_int(ncid, coord, out));
    REQUIRE(!memcmp(out, values, sizeof(out)));
    CHECK(nc_close(ncid));
    REQUIRE(nc_get_var_chunk_cache(ncid, 0, NULL, NULL, NULL) == NC_EBADID);
    REQUIRE(nc_set_var_chunk_cache(ncid, 0, 1, 1, 0) == NC_EBADID);
    CHECK(nc_set_chunk_cache(default_bytes, default_slots, default_weight));
    return 0;
}

int
main(void)
{
    if (!NC_NEXTCDF4_initialize())
        return 1;
    return run_mode(0) || run_mode(NC_NETCDF4_MODEL) || run_mode(NC_CLASSIC_MODEL);
}

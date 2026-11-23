/* File-level fill defaults and variable overrides.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include "nxt4internal.h"

#define CHECK(call) do { int r = (call); if (r != NC_NOERR) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(r)); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)

static int
check_fill(int ncid, int varid, int expected)
{
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NEXTCDF4_VAR_INFO_T *info;
    H5D_fill_time_t when;
    hid_t dcpl;
    int nofill;

    CHECK(nc_inq_var_fill(ncid, varid, &nofill, NULL));
    REQUIRE(nofill == expected);
    CHECK(nc4_find_nc4_grp(ncid, &grp));
    var = (NC_VAR_INFO_T *)ncindexith(grp->vars, varid);
    info = var->format_var_info;
    if (info->hdf_dataset >= 0) {
        REQUIRE((dcpl = H5Dget_create_plist(info->hdf_dataset)) >= 0);
        REQUIRE(H5Pget_fill_time(dcpl, &when) >= 0);
        REQUIRE(H5Pclose(dcpl) >= 0);
        REQUIRE((when == H5D_FILL_TIME_NEVER) == expected);
    }
    return 0;
}

static int
run_mode(int mode)
{
    int ncid, dim, before, after, override, later, old = -1, group;
    int v, value = 42, got[4], custom = -17;
    size_t start = 2, count = 1;
    const char *path = "fill_mode.nc";

    CHECK(nc_create(path, NC_NEXTCDF4 | mode, &ncid));
    CHECK(nc_def_dim(ncid, "x", NC_UNLIMITED, &dim));
    CHECK(nc_def_var(ncid, "before", NC_INT, 1, &dim, &before));
    CHECK(nc_set_fill(ncid, NC_NOFILL, &old));
    REQUIRE(old == NC_FILL);
    CHECK(nc_set_fill(ncid, NC_NOFILL, &old));
    REQUIRE(old == NC_NOFILL);
    REQUIRE(nc_set_fill(ncid, 777, &old) == NC_EINVAL);
    CHECK(nc_def_var(ncid, "after", NC_INT, 1, &dim, &after));
    CHECK(nc_def_var(ncid, "override", NC_INT, 1, &dim, &override));
    CHECK(nc_def_var_fill(ncid, override, 0, &custom));
    if (!(mode & NC_CLASSIC_MODEL)) {
        nc_type type;
        CHECK(nc_def_grp(ncid, "child", &group));
        CHECK(nc_set_fill(group, NC_FILL, &old));
        REQUIRE(old == NC_NOFILL);
        CHECK(nc_set_fill(group, NC_NOFILL, NULL));
        CHECK(nc_def_vlen(ncid, "ints", NC_INT, &type));
        CHECK(nc_def_var(ncid, "dynamic", type, 1, &dim, &v));
        REQUIRE(!check_fill(ncid, v, 0));
        CHECK(nc_def_opaque(ncid, sizeof(int), "fixed", &type));
        CHECK(nc_def_var(ncid, "opaque", type, 1, &dim, &v));
        REQUIRE(!check_fill(ncid, v, 0));
        if (!mode) {
            CHECK(nc_def_var(ncid, "bf16", NC_BFLOAT16, 1, &dim, &v));
            REQUIRE(!check_fill(ncid, v, 1));
            CHECK(nc_def_var(ncid, "ref", NC_STD_REF, 1, &dim, &v));
            REQUIRE(!check_fill(ncid, v, 0));
        }
    }
    CHECK(nc_set_fill(ncid, NC_FILL, &old));
    REQUIRE(old == NC_NOFILL);
    CHECK(nc_def_var(ncid, "later", NC_INT, 1, &dim, &later));
    REQUIRE(!check_fill(ncid, before, 0));
    REQUIRE(!check_fill(ncid, after, 1));
    CHECK(nc_enddef(ncid));
    REQUIRE(!check_fill(ncid, override, 0));
    CHECK(nc_put_vara_int(ncid, after, &start, &count, &value));
    CHECK(nc_get_vara_int(ncid, after, &start, &count, got));
    REQUIRE(got[0] == value);
    CHECK(nc_get_var_int(ncid, before, got));
    REQUIRE(got[0] == NC_FILL_INT && got[2] == NC_FILL_INT);
    CHECK(nc_get_var_int(ncid, override, got));
    REQUIRE(got[0] == custom && got[2] == custom);
    CHECK(nc_set_fill(ncid, NC_NOFILL, &old));
    REQUIRE(old == NC_FILL);
    CHECK(nc_redef(ncid));
    REQUIRE(nc_def_var_fill(ncid, before, 1, NULL) == NC_ELATEDEF);
    CHECK(nc_close(ncid));
    CHECK(nc_open(path, NC_NEXTCDF4 | NC_WRITE, &ncid));
    CHECK(nc_set_fill(ncid, NC_FILL, &old));
    REQUIRE(old == NC_FILL);
    CHECK(nc_inq_varid(ncid, "after", &after));
    REQUIRE(!check_fill(ncid, after, 1));
    CHECK(nc_close(ncid));
    CHECK(nc_open(path, NC_NEXTCDF4, &ncid));
    REQUIRE(nc_set_fill(ncid, NC_NOFILL, NULL) == NC_EPERM);
    CHECK(nc_close(ncid));
    REQUIRE(nc_set_fill(ncid, NC_FILL, NULL) == NC_EBADID);
    return 0;
}

int
main(void)
{
    if (!NC_NEXTCDF4_initialize())
        return 1;
    return run_mode(0) || run_mode(NC_NETCDF4_MODEL) || run_mode(NC_CLASSIC_MODEL);
}

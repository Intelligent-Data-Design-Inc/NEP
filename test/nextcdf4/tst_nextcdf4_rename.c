/**
 * @file tst_nextcdf4_rename.c
 * @brief Tests for dimension and variable renaming in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_rename.h5"

static int
create_file_with_dim_and_var(void)
{
    int ncid, dimid, varid;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;
    return 0;
}

/** @return Zero when a dimension can be renamed. */
static int
test_rename_dim(void)
{
    int ncid, dimid;
    char name[NC_MAX_NAME + 1];
    size_t len;

    if (create_file_with_dim_and_var())
        return 1;
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_redef(ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "x", &dimid) != NC_NOERR)
        return 1;
    if (nc_rename_dim(ncid, dimid, "longitude") != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "longitude", &dimid) != NC_NOERR)
        return 1;
    if (nc_inq_dim(ncid, dimid, name, &len) != NC_NOERR)
        return 1;
    if (strcmp(name, "longitude") || len != 10)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when renaming a coordinate variable breaks the link
 * and leaves the dimension name unchanged (upstream semantics). */
static int
test_rename_coord_var(void)
{
    int ncid, dimid, varid;
    char name[NC_MAX_NAME + 1];
    size_t len;
    int varid2;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "x", NC_INT, 1, &dimid, &varid) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_redef(ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "x", &varid2) != NC_NOERR)
        return 1;
    if (nc_rename_var(ncid, varid2, "longitude") != NC_NOERR)
        return 1;
    /* Upstream semantics: renaming a coordinate variable breaks the
     * link — the dimension keeps its name. */
    if (nc_inq_dimid(ncid, "x", &dimid) != NC_NOERR)
        return 1;
    if (nc_inq_dimname(ncid, dimid, name) != NC_NOERR)
        return 1;
    if (strcmp(name, "x"))
        return 1;
    if (nc_inq_varid(ncid, "longitude", &varid2) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "x", &dimid) != NC_NOERR)
        return 1;
    if (nc_inq_dim(ncid, dimid, name, &len) != NC_NOERR)
        return 1;
    if (strcmp(name, "x") || len != 10)
        return 1;
    if (nc_inq_varid(ncid, "longitude", &varid2) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when renaming a dimension breaks the coordinate-variable
 * link and leaves the variable name unchanged (upstream semantics). */
static int
test_rename_dim_with_coord(void)
{
    int ncid, dimid, varid;
    char name[NC_MAX_NAME + 1];
    size_t len;
    int varid2;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "x", NC_INT, 1, &dimid, &varid) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_redef(ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "x", &dimid) != NC_NOERR)
        return 1;
    if (nc_rename_dim(ncid, dimid, "longitude") != NC_NOERR)
        return 1;
    /* Upstream semantics: renaming the dimension breaks the link —
     * the variable keeps its name. */
    if (nc_inq_varid(ncid, "x", &varid2) != NC_NOERR)
        return 1;
    if (nc_inq_varname(ncid, varid2, name) != NC_NOERR)
        return 1;
    if (strcmp(name, "x"))
        return 1;
    if (nc_inq_dimid(ncid, "longitude", &dimid) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "x", &varid2) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "longitude", &dimid) != NC_NOERR)
        return 1;
    if (nc_inq_dim(ncid, dimid, name, &len) != NC_NOERR)
        return 1;
    if (strcmp(name, "longitude") || len != 10)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when a shared dimension can be renamed. */
static int
test_rename_shared_dim(void)
{
    int ncid, dimid, varid1, varid2;
    char name[NC_MAX_NAME + 1];
    size_t len;
    int v1id, v2id;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "a", NC_INT, 1, &dimid, &varid1) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "b", NC_INT, 1, &dimid, &varid2) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_redef(ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "x", &dimid) != NC_NOERR)
        return 1;
    if (nc_rename_dim(ncid, dimid, "longitude") != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_dimid(ncid, "longitude", &dimid) != NC_NOERR)
        return 1;
    if (nc_inq_dim(ncid, dimid, name, &len) != NC_NOERR)
        return 1;
    if (strcmp(name, "longitude") || len != 10)
        return 1;
    if (nc_inq_varid(ncid, "a", &v1id) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "b", &v2id) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when a regular variable can be renamed. */
static int
test_rename_var(void)
{
    int ncid, varid;
    int varid2;

    if (create_file_with_dim_and_var())
        return 1;
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_redef(ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "data", &varid) != NC_NOERR)
        return 1;
    if (nc_rename_var(ncid, varid, "values") != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "values", &varid2) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
static int
test_rename_groups(void)
{
    int modes[] = {0, NC_NETCDF4_MODEL};
    size_t m;
    for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
        int ncid, grp, child, sibling, dim, var, id, value = 19, out;
        nc_type type;
        nc_std_ref_t ref = {0};
        char longname[201], path[1024], expected[1024];
        size_t len;
        memset(longname, 'a', 200);
        longname[200] = '\0';
#define GROUP_CHECK(call) do { int r = (call); if (r) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(r)); return 1; } } while (0)
#define GROUP_REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)
        GROUP_CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | modes[m], &ncid));
        GROUP_CHECK(nc_def_dim(ncid, "x", 1, &dim));
        GROUP_CHECK(nc_def_opaque(ncid, sizeof(int), "root_type", &type));
        GROUP_CHECK(nc_def_grp(ncid, "original", &grp));
        GROUP_CHECK(nc_def_grp(ncid, "sibling", &sibling));
        GROUP_CHECK(nc_def_grp(grp, longname, &child));
        GROUP_CHECK(nc_def_var(child, "data", NC_INT, 1, &dim, &var));
        GROUP_CHECK(nc_put_att_int(grp, NC_GLOBAL, "tag", NC_INT, 1, &value));
        GROUP_REQUIRE(nc_rename_grp(ncid, "root") == NC_EBADGRPID);
        GROUP_REQUIRE(nc_rename_grp(grp, "sibling") == NC_ENAMEINUSE);
        GROUP_REQUIRE(nc_rename_grp(grp, "root_type") == NC_ENAMEINUSE);
        GROUP_REQUIRE(nc_rename_grp(grp, "bad/name") == NC_EBADNAME);
        GROUP_REQUIRE(nc_rename_grp(grp, NULL) == NC_EINVAL);
        GROUP_REQUIRE(nc_rename_grp(grp, "original") == NC_ENAMEINUSE);
        GROUP_CHECK(nc_rename_grp(grp, "renamed"));
        GROUP_CHECK(nc_enddef(ncid));
        GROUP_CHECK(nc_put_var_int(child, var, &value));
        if (!modes[m])
            GROUP_CHECK(nc_ref_object(ncid, NC_REF_OBJ_GRP, child, &ref));
        GROUP_CHECK(nc_rename_grp(grp, longname));
        snprintf(expected, sizeof(expected), "/%s/%s", longname, longname);
        GROUP_CHECK(nc_inq_grpname_full(child, &len, path));
        GROUP_REQUIRE(len == strlen(expected) && !strcmp(path, expected));
        GROUP_CHECK(nc_inq_grp_full_ncid(ncid, expected, &id));
        GROUP_REQUIRE(id == child);
        if (!modes[m]) {
            int kind, target;
            GROUP_CHECK(nc_deref_object(ncid, &ref, &kind, &target, &id));
            GROUP_REQUIRE(kind == NC_REF_OBJ_GRP && target == child && id == child);
            GROUP_CHECK(nc_ref_free(&ref));
        }
        GROUP_REQUIRE(nc_inq_ncid(ncid, "original", &id) != NC_NOERR);
        GROUP_REQUIRE(nc_inq_ncid(ncid, "renamed", &id) != NC_NOERR);
        GROUP_CHECK(nc_enddef(ncid));
        GROUP_CHECK(nc_get_var_int(child, var, &out));
        GROUP_REQUIRE(out == value);
        GROUP_CHECK(nc_get_att_int(grp, NC_GLOBAL, "tag", &out));
        GROUP_REQUIRE(out == value);
        GROUP_CHECK(nc_close(ncid));
        GROUP_CHECK(nc_open(FILE_NAME, NC_NEXTCDF4, &ncid));
        GROUP_CHECK(nc_inq_grp_full_ncid(ncid, expected, &child));
        GROUP_CHECK(nc_inq_varid(child, "data", &var));
        GROUP_CHECK(nc_get_var_int(child, var, &out));
        GROUP_REQUIRE(out == value);
        GROUP_REQUIRE(nc_rename_grp(child, "denied") == NC_EPERM);
        GROUP_CHECK(nc_close(ncid));
        if (modes[m]) {
            GROUP_CHECK(nc_open(FILE_NAME, NC_NOWRITE, &ncid));
            GROUP_CHECK(nc_inq_grp_full_ncid(ncid, expected, &child));
            GROUP_CHECK(nc_inq_varid(child, "data", &var));
            GROUP_CHECK(nc_get_var_int(child, var, &out));
            GROUP_REQUIRE(out == value);
            GROUP_CHECK(nc_close(ncid));
            GROUP_CHECK(nc_create(FILE_NAME, NC_NETCDF4, &ncid));
            GROUP_CHECK(nc_def_grp(ncid, "upstream", &grp));
            GROUP_CHECK(nc_def_var(grp, "data", NC_INT, 0, NULL, &var));
            GROUP_CHECK(nc_put_var_int(grp, var, &value));
            GROUP_CHECK(nc_close(ncid));
            GROUP_CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid));
            GROUP_CHECK(nc_inq_ncid(ncid, "upstream", &grp));
            GROUP_CHECK(nc_rename_grp(grp, "new"));
            GROUP_CHECK(nc_close(ncid));
            GROUP_CHECK(nc_open(FILE_NAME, NC_NOWRITE, &ncid));
            GROUP_CHECK(nc_inq_ncid(ncid, "new", &grp));
            GROUP_CHECK(nc_get_var_int(grp, var, &out));
            GROUP_REQUIRE(out == value);
            GROUP_CHECK(nc_close(ncid));
        }
#undef GROUP_CHECK
#undef GROUP_REQUIRE
    }
    return 0;
}

int
main(void)
{
    struct {
        const char *name;
        int (*fn)(void);
    } tests[] = {
        {"test_rename_groups", test_rename_groups},
        {"test_rename_dim", test_rename_dim},
        {"test_rename_coord_var", test_rename_coord_var},
        {"test_rename_dim_with_coord", test_rename_dim_with_coord},
        {"test_rename_shared_dim", test_rename_shared_dim},
        {"test_rename_var", test_rename_var},
    };
    size_t i;
    int failed = 0;

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        int ret = tests[i].fn();
        if (ret) {
            fprintf(stderr, "%s failed: %d\n", tests[i].name, ret);
            failed = 1;
        }
    }
    return failed;
}

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

/** @return Zero when a coordinate variable rename also renames the dimension. */
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
    if (nc_inq_varid(ncid, "longitude", &varid2) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when a dimension with a coordinate variable is renamed. */
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
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "longitude", &varid2) != NC_NOERR)
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
    int ncid, dimid, varid;
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

int
main(void)
{
    struct {
        const char *name;
        int (*fn)(void);
    } tests[] = {
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

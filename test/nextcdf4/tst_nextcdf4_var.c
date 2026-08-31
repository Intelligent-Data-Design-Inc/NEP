/**
 * @file tst_nextcdf4_var.c
 * @brief Tests variable I/O and dimension scales for NEXTCDF-4 Sprint 4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

/** NEXTCDF-4 file used for I/O tests. */
#define FILE_NAME "tst_nextcdf4_var.h5"

/** @return Zero when a scalar variable round-trips correctly. */
static int
test_scalar(void)
{
    int ncid;
    int varid;
    int in = 42;
    int out = 0;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_var(ncid, "scalar", NC_INT, 0, NULL, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, varid, &in))
        return 1;
    if (nc_get_var_int(ncid, varid, &out))
        return 1;
    if (out != in)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a fixed one-dimensional variable round-trips. */
static int
test_1d(void)
{
    int ncid;
    int dimid;
    int varid;
    unsigned int data[5] = {1, 2, 3, 4, 5};
    unsigned int out[5] = {0};
    int ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        { fprintf(stderr, "create: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_def_dim(ncid, "x", 5, &dimid)))
        { fprintf(stderr, "def_dim: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_def_var(ncid, "data", NC_UINT, 1, &dimid, &varid)))
        { fprintf(stderr, "def_var: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_enddef(ncid)))
        { fprintf(stderr, "enddef: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_put_var_uint(ncid, varid, data)))
        { fprintf(stderr, "put_var: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_get_var_uint(ncid, varid, out)))
        { fprintf(stderr, "get_var: %s\n", nc_strerror(ret)); return 1; }
    if (memcmp(data, out, sizeof(data)))
        { fprintf(stderr, "data mismatch\n"); return 1; }
    if ((ret = nc_close(ncid)))
        { fprintf(stderr, "close: %s\n", nc_strerror(ret)); return 1; }
    return 0;
}

/** @return Zero when a two-dimensional hyperslab read/write works. */
static int
test_2d_hyperslab(void)
{
    int ncid;
    int dims[2];
    int varid;
    int data[12];
    int out[12] = {0};
    size_t start[2] = {1, 1};
    size_t count[2] = {2, 3};
    size_t ostart[2] = {0, 0};
    int i, j;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "y", 4, &dims[0]))
        return 1;
    if (nc_def_dim(ncid, "x", 5, &dims[1]))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 2, dims, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    for (i = 0; i < 12; i++)
        data[i] = i + 1;
    if (nc_put_vara_int(ncid, varid, start, count, data))
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(int) * 6))
        return 1;
    ostart[0] = 1; ostart[1] = 1;
    if (nc_get_var1_int(ncid, varid, ostart, &j))
        return 1;
    if (j != 1)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when an unlimited dimension can be extended. */
static int
test_unlimited(void)
{
    int ncid;
    int dimid;
    int varid;
    size_t start[1] = {0};
    size_t count[1] = {3};
    int data[3] = {10, 20, 30};
    int out[3] = {0};
    size_t len;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "rec", NC_UNLIMITED, &dimid))
        return 1;
    if (nc_def_var(ncid, "rec", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_vara_int(ncid, varid, start, count, data))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len))
        return 1;
    if (len != 3)
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(data)))
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Reopen and check the extension survived. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len))
        return 1;
    if (len != 3)
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(data)))
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when strided read/write round-trips. */
static int
test_strided(void)
{
    int ncid;
    int dimid;
    int varid;
    int data[6] = {1, 2, 3, 4, 5, 6};
    int out[3] = {0};
    size_t start[1] = {0};
    size_t count[1] = {3};
    ptrdiff_t stride[1] = {2};
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "s", 6, &dimid))
        return 1;
    if (nc_def_var(ncid, "s", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_get_vars_int(ncid, varid, start, count, stride, out))
        return 1;
    for (i = 0; i < 3; i++)
        if (out[i] != data[i * 2])
            return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when mapped I/O returns the expected not-built error. */
static int
test_varm_not_built(void)
{
    int ncid;
    int dimid;
    int varid;
    int data[4] = {1, 2, 3, 4};
    int out[4] = {0};
    size_t start[1] = {0};
    size_t count[1] = {2};
    ptrdiff_t stride[1] = {1};
    ptrdiff_t imap[1] = {1};

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "m", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "m", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_varm_int(ncid, varid, start, count, stride, imap, data)
        != NC_ENOTBUILT)
        return 1;
    if (nc_get_varm_int(ncid, varid, start, count, stride, imap, out)
        != NC_ENOTBUILT)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a coordinate variable reuses the dimension scale dataset. */
static int
test_coordinate(void)
{
    int ncid;
    int dimid;
    int varid;
    float vals[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float out[4] = {0.0f};
    size_t start[1] = {0};
    size_t count[1] = {4};

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "lat", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "lat", NC_FLOAT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_vara_float(ncid, varid, start, count, vals))
        return 1;
    if (nc_get_vara_float(ncid, varid, start, count, out))
        return 1;
    if (memcmp(vals, out, sizeof(vals)))
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when all Sprint 4 checks pass. */
int
main(void)
{
    if (!NC_NEXTCDF4_initialize()) {
        fprintf(stderr, "initialize failed\n");
        return 1;
    }
    if (test_scalar()) { fprintf(stderr, "scalar test failed\n"); return 1; }
    if (test_1d()) { fprintf(stderr, "1d test failed\n"); return 1; }
    if (test_2d_hyperslab()) { fprintf(stderr, "2d hyperslab test failed\n"); return 1; }
    if (test_unlimited()) { fprintf(stderr, "unlimited test failed\n"); return 1; }
    if (test_strided()) { fprintf(stderr, "strided test failed\n"); return 1; }
    if (test_varm_not_built()) { fprintf(stderr, "varm test failed\n"); return 1; }
    if (test_coordinate()) { fprintf(stderr, "coordinate test failed\n"); return 1; }
    unlink(FILE_NAME);
    printf("OK: NEXTCDF-4 variable I/O and dimension scales\n");
    return 0;
}

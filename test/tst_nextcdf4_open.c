/**
 * @file tst_nextcdf4_open.c
 * @brief Tests for opening existing and populated NEXTCDF-4 and upstream NetCDF-4 files.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"

/** NEXTCDF-4 test file. */
#define FILE_NAME "tst_nextcdf4_open.h5"
/** Upstream NetCDF-4 test file. */
#define UPSTREAM_NAME "tst_nextcdf4_open_upstream.h5"

/** @return Zero when a NEXTCDF-4 file can be reopened. */
static int
test_nextcdf4_reopen(void)
{
    int ncid, ncid2;
    int dimid, varid;
    int ndims, nvars;
    int data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int data_in[10];
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid) != NC_NOERR)
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid) != NC_NOERR)
        return 1;
    if (nc_enddef(ncid) != NC_NOERR)
        return 1;
    if (nc_put_var_int(ncid, varid, data) != NC_NOERR)
        return 1;
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid2) != NC_NOERR)
        return 1;
    if (nc_inq(ncid2, &ndims, &nvars, NULL, NULL) != NC_NOERR)
        return 1;
    if (ndims != 1 || nvars != 1)
        return 1;
    if (nc_inq_varid(ncid2, "data", &varid) != NC_NOERR)
        return 1;
    if (nc_get_var_int(ncid2, varid, data_in) != NC_NOERR)
        return 1;
    for (i = 0; i < 10; i++)
        if (data_in[i] != data[i])
            return 1;
    if (nc_close(ncid2) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when an upstream NetCDF-4/HDF5 file can be opened. */
static int
test_upstream_netcdf4(void)
{
    int ncid, ncid2;
    int dimid, varid;
    int ndims, nvars;
    float data[5] = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
    float data_in[5];
    int i;
    int ret;

    unlink(UPSTREAM_NAME);
    if ((ret = nc_create(UPSTREAM_NAME, NC_NETCDF4 | NC_CLOBBER, &ncid)) != NC_NOERR) {
        fprintf(stderr, "nc_create: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_dim(ncid, "y", 5, &dimid)) != NC_NOERR) {
        fprintf(stderr, "nc_def_dim: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_var(ncid, "temp", NC_FLOAT, 1, &dimid, &varid)) != NC_NOERR) {
        fprintf(stderr, "nc_def_var: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_enddef(ncid)) != NC_NOERR) {
        fprintf(stderr, "nc_enddef: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_var_float(ncid, varid, data)) != NC_NOERR) {
        fprintf(stderr, "nc_put_var_float: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_close(ncid)) != NC_NOERR) {
        fprintf(stderr, "nc_close: %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = nc_open(UPSTREAM_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid2)) != NC_NOERR) {
        fprintf(stderr, "nc_open: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_inq(ncid2, &ndims, &nvars, NULL, NULL)) != NC_NOERR) {
        fprintf(stderr, "nc_inq: %s\n", nc_strerror(ret));
        return 1;
    }
    if (ndims != 1 || nvars != 1) {
        fprintf(stderr, "ndims=%d nvars=%d\n", ndims, nvars);
        return 1;
    }
    if ((ret = nc_inq_varid(ncid2, "temp", &varid)) != NC_NOERR) {
        fprintf(stderr, "nc_inq_varid: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_get_var_float(ncid2, varid, data_in)) != NC_NOERR) {
        fprintf(stderr, "nc_get_var_float: %s\n", nc_strerror(ret));
        return 1;
    }
    for (i = 0; i < 5; i++)
        if (data_in[i] != data[i]) {
            fprintf(stderr, "data[%d] %f != %f\n", i, data_in[i], data[i]);
            return 1;
        }
    if ((ret = nc_close(ncid2)) != NC_NOERR) {
        fprintf(stderr, "nc_close 2: %s\n", nc_strerror(ret));
        return 1;
    }

    unlink(UPSTREAM_NAME);
    return 0;
}

int
main(void)
{
    int failed = 0;
    int ret;

    if ((ret = test_nextcdf4_reopen()) != 0) {
        fprintf(stderr, "test_nextcdf4_reopen failed: %d\n", ret);
        failed = 1;
    }
    if ((ret = test_upstream_netcdf4()) != 0) {
        fprintf(stderr, "test_upstream_netcdf4 failed: %d\n", ret);
        failed = 1;
    }
    return failed;
}

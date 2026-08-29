/**
 * @file tst_nextcdf4_complex.c
 * @brief Tests for complex number types in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_complex.h5"

static int
test_complex(nc_type xtype, size_t esize)
{
    int ncid, dimid, varid;
    size_t start[1] = {0};
    size_t count[1] = {4};
    nc_type xtype_in;
    float _Complex fdata[4] = {1.0f + 2.0f*I, 3.0f + 4.0f*I,
                               -1.0f + 0.5f*I, 0.0f + 0.0f*I};
    float _Complex fdata_in[4] = {0};
    double _Complex ddata[4] = {1.0 + 2.0*I, 3.0 + 4.0*I,
                                -1.0 + 0.5*I, 0.0 + 0.0*I};
    double _Complex ddata_in[4] = {0};
    void *data = (xtype == NC_COMPLEX) ? (void *)fdata : (void *)ddata;
    void *data_in = (xtype == NC_COMPLEX) ? (void *)fdata_in : (void *)ddata_in;
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "c", xtype, 1, &dimid, &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_enddef(ncid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_put_vara(ncid, varid, start, count, data) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "c", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_inq_var(ncid, varid, NULL, &xtype_in, NULL, NULL, NULL) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (xtype_in != xtype) {
        fprintf(stderr, "type mismatch: got %d, expected %d\n", xtype_in, xtype);
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start, count, data_in) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (xtype == NC_COMPLEX) {
            if (crealf(fdata[i]) != crealf(fdata_in[i]) ||
                cimagf(fdata[i]) != cimagf(fdata_in[i])) {
                fprintf(stderr, "complex mismatch at %d\n", i);
                nc_close(ncid);
                return 1;
            }
        } else {
            if (creal(ddata[i]) != creal(ddata_in[i]) ||
                cimag(ddata[i]) != cimag(ddata_in[i])) {
                fprintf(stderr, "doublecomplex mismatch at %d\n", i);
                nc_close(ncid);
                return 1;
            }
        }
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

static int
test_rejected_in_model(int mode)
{
    int ncid, dimid, varid;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "c", NC_COMPLEX, 1, &dimid, &varid) == NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    nc_close(ncid);
    unlink(FILE_NAME);
    return 0;
}

int
main(void)
{
    int failed = 0;

    if (test_complex(NC_COMPLEX, 2 * sizeof(float))) {
        fprintf(stderr, "NC_COMPLEX test failed\n");
        failed = 1;
    }
    if (test_complex(NC_DOUBLECOMPLEX, 2 * sizeof(double))) {
        fprintf(stderr, "NC_DOUBLECOMPLEX test failed\n");
        failed = 1;
    }
    if (test_rejected_in_model(NC_NETCDF4_MODEL)) {
        fprintf(stderr, "NC_NETCDF4_MODEL rejection failed\n");
        failed = 1;
    }
    if (test_rejected_in_model(NC_CLASSIC_MODEL)) {
        fprintf(stderr, "NC_CLASSIC_MODEL rejection failed\n");
        failed = 1;
    }
    return failed;
}

/**
 * @file tst_nextcdf4_float16.c
 * @brief Tests for small floating-point types in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_float16.h5"

typedef struct {
    const char *name;
    nc_type xtype;
    int esize;
} test_case_t;

static const test_case_t tests[] = {
    {"float16", NC_FLOAT16, 2},
    {"bfloat16", NC_BFLOAT16, 2},
    {"float8_e4m3", NC_FLOAT8_E4M3, 1},
    {"float8_e5m2", NC_FLOAT8_E5M2, 1},
    {"float6_e2m3", NC_FLOAT6_E2M3, 1},
    {"float6_e3m2", NC_FLOAT6_E3M2, 1},
    {"float4_e2m1", NC_FLOAT4_E2M1, 1},
};

static int
test_round_trip(const test_case_t *t)
{
    int ncid, dimid, varid;
    uint16_t data16[4] = {0x1234, 0x5678, 0x9abc, 0xdef0};
    uint8_t data8[4] = {0x12, 0x34, 0x56, 0x78};
    uint16_t data16_in[4] = {0};
    uint8_t data8_in[4] = {0};
    void *data = (t->esize == 2) ? (void *)data16 : (void *)data8;
    void *data_in = (t->esize == 2) ? (void *)data16_in : (void *)data8_in;
    nc_type xtype_in;
    size_t start[1] = {0};
    size_t count[1] = {4};
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR) {
        fprintf(stderr, "nc_create failed for %s\n", t->name);
        return 1;
    }
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        fprintf(stderr, "nc_def_dim failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, t->name, t->xtype, 1, &dimid, &varid) != NC_NOERR) {
        fprintf(stderr, "nc_def_var failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (nc_enddef(ncid) != NC_NOERR) {
        fprintf(stderr, "nc_enddef failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (nc_put_vara(ncid, varid, start, count, data) != NC_NOERR) {
        fprintf(stderr, "nc_put_vara failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "nc_close failed for %s\n", t->name);
        return 1;
    }

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR) {
        fprintf(stderr, "nc_open failed for %s\n", t->name);
        return 1;
    }
    if (nc_inq_varid(ncid, t->name, &varid) != NC_NOERR) {
        fprintf(stderr, "nc_inq_varid failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (nc_inq_var(ncid, varid, NULL, &xtype_in, NULL, NULL, NULL) != NC_NOERR) {
        fprintf(stderr, "nc_inq_var failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    if (xtype_in != t->xtype) {
        fprintf(stderr, "type mismatch for %s: got %d, expected %d\n",
                t->name, xtype_in, t->xtype);
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start, count, data_in) != NC_NOERR) {
        fprintf(stderr, "nc_get_vara failed for %s\n", t->name);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (t->esize == 2) {
            if (data16_in[i] != data16[i]) {
                fprintf(stderr, "data mismatch for %s at %d: %u != %u\n",
                        t->name, i, (unsigned)data16_in[i], (unsigned)data16[i]);
                nc_close(ncid);
                return 1;
            }
        } else {
            if (data8_in[i] != data8[i]) {
                fprintf(stderr, "data mismatch for %s at %d: %u != %u\n",
                        t->name, i, (unsigned)data8_in[i], (unsigned)data8[i]);
                nc_close(ncid);
                return 1;
            }
        }
    }
    if (nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "nc_close 2 failed for %s\n", t->name);
        return 1;
    }

    unlink(FILE_NAME);
    return 0;
}

static int
test_rejected_in_model(int mode)
{
    int ncid, dimid, varid;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid) != NC_NOERR) {
        fprintf(stderr, "nc_create with mode %d failed\n", mode);
        return 1;
    }
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "f", NC_FLOAT16, 1, &dimid, &varid) == NC_NOERR) {
        fprintf(stderr, "NC_FLOAT16 accepted in mode %d\n", mode);
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
    size_t i;
    int failed = 0;

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        if (test_round_trip(&tests[i])) {
            fprintf(stderr, "test_round_trip failed for %s\n", tests[i].name);
            failed = 1;
        }
    }
    if (test_rejected_in_model(NC_NETCDF4_MODEL)) {
        fprintf(stderr, "test_rejected_in_model NC_NETCDF4_MODEL failed\n");
        failed = 1;
    }
    if (test_rejected_in_model(NC_CLASSIC_MODEL)) {
        fprintf(stderr, "test_rejected_in_model NC_CLASSIC_MODEL failed\n");
        failed = 1;
    }
    return failed;
}

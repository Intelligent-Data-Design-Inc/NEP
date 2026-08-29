/**
 * @file tst_nextcdf4_bitfield.c
 * @brief Tests for bitfield types in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_bitfield.h5"

typedef struct {
    const char *name;
    nc_type xtype;
    size_t esize;
} test_case_t;

static const test_case_t tests[] = {
    {"bf8",  NC_BITFIELD8,  1},
    {"bf16", NC_BITFIELD16, 2},
    {"bf32", NC_BITFIELD32, 4},
    {"bf64", NC_BITFIELD64, 8},
};

static int
test_bitfield(const test_case_t *t)
{
    int ncid, dimid, varid;
    size_t start[1] = {0};
    size_t count[1] = {4};
    nc_type xtype_in;
    uint8_t  data8[4]  = {0x01, 0xAD, 0xBE, 0xF0};
    uint16_t data16[4] = {0x0001, 0xDEAD, 0xBABE, 0xDEF0};
    uint32_t data32[4] = {0x00000001, 0x0000DEAD, 0xCAFEBABE, 0x9ABCDEF0};
    uint64_t data64[4] = {0x0000000000000001ULL, 0x000000000000DEADULL,
                          0x00000000CAFEBABEULL, 0x123456789ABCDEF0ULL};
    uint8_t  data8_in[4]  = {0};
    uint16_t data16_in[4] = {0};
    uint32_t data32_in[4] = {0};
    uint64_t data64_in[4] = {0};
    void *data, *data_in;
    int i;

    switch (t->esize) {
    case 1:  data = data8;  data_in = data8_in;  break;
    case 2:  data = data16; data_in = data16_in; break;
    case 4:  data = data32; data_in = data32_in; break;
    case 8:  data = data64; data_in = data64_in; break;
    default: return 1;
    }

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, t->name, t->xtype, 1, &dimid, &varid) != NC_NOERR) {
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
    if (nc_inq_varid(ncid, t->name, &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_inq_var(ncid, varid, NULL, &xtype_in, NULL, NULL, NULL) != NC_NOERR) {
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
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 4; i++) {
        int ok = 0;
        switch (t->esize) {
        case 1:
            ok = (data8_in[i] == data8[i]); break;
        case 2:
            ok = (data16_in[i] == data16[i]); break;
        case 4:
            ok = (data32_in[i] == data32[i]); break;
        case 8:
            ok = (data64_in[i] == data64[i]); break;
        }
        if (!ok) {
            fprintf(stderr, "data mismatch for %s at %d\n", t->name, i);
            nc_close(ncid);
            return 1;
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
    if (nc_def_var(ncid, "bf", NC_BITFIELD16, 1, &dimid, &varid) == NC_NOERR) {
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
        if (test_bitfield(&tests[i])) {
            fprintf(stderr, "%s test failed\n", tests[i].name);
            failed = 1;
        }
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

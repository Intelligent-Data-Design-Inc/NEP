/**
 * @file tst_explore.c
 * @brief Create a simple NEXTCDF-4 file, write data, reopen, and print metadata.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "explore.h5"

static int
write_and_close(void)
{
    int ncid;
    int latid, lonid, heightid, timeid;
    int tempid, uwindid, vwindid, mixid;
    int dims3[3], dims4[4];
    size_t start3[3] = {0, 0, 0};
    size_t count3[3] = {2, 2, 1};
    size_t start4[4] = {0, 0, 0, 0};
    size_t count4[4] = {2, 2, 2, 1};
    float data3[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    uint16_t data4[8] = {0x3C00, 0x4000, 0x4200, 0x4400,
                         0x4500, 0x4600, 0x4700, 0x4800};

    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;

    if (nc_def_dim(ncid, "lat", 2, &latid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_dim(ncid, "lon", 2, &lonid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_dim(ncid, "height", 2, &heightid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_dim(ncid, "time", 0, &timeid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    dims3[0] = latid;
    dims3[1] = lonid;
    dims3[2] = timeid;

    if (nc_def_var(ncid, "temp", NC_FLOAT, 3, dims3, &tempid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "uwind", NC_FLOAT, 3, dims3, &uwindid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "vwind", NC_FLOAT, 3, dims3, &vwindid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    dims4[0] = latid;
    dims4[1] = lonid;
    dims4[2] = heightid;
    dims4[3] = timeid;

    if (nc_def_var(ncid, "mix", NC_FLOAT16, 4, dims4, &mixid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    if (nc_enddef(ncid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    if (nc_put_vara(ncid, tempid, start3, count3, data3) != NC_NOERR ||
        nc_put_vara(ncid, uwindid, start3, count3, data3) != NC_NOERR ||
        nc_put_vara(ncid, vwindid, start3, count3, data3) != NC_NOERR ||
        nc_put_vara(ncid, mixid, start4, count4, data4) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    if (nc_close(ncid) != NC_NOERR)
        return 1;
    return 0;
}

static int
read_and_print(void)
{
    int ncid;
    int ndims, nvars, ngatts, unlimid;
    float data3[4] = {0};
    float data3_in[4] = {0};
    uint16_t data4[8] = {0};
    uint16_t data4_in[8] = {0};
    size_t start3[3] = {0, 0, 0};
    size_t count3[3] = {2, 2, 1};
    size_t start4[4] = {0, 0, 0, 0};
    size_t count4[4] = {2, 2, 2, 1};
    int i;
    int varid;
    char name[NC_MAX_NAME + 1];
    nc_type xtype;
    int var_ndims;
    int dimids[NC_MAX_VAR_DIMS];
    int natt;
    size_t len;

    data3[0] = 1.0f; data3[1] = 2.0f; data3[2] = 3.0f; data3[3] = 4.0f;
    data4[0] = 0x3C00; data4[1] = 0x4000; data4[2] = 0x4200; data4[3] = 0x4400;
    data4[4] = 0x4500; data4[5] = 0x4600; data4[6] = 0x4700; data4[7] = 0x4800;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;

    if (nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    printf("Metadata for %s:\n", FILE_NAME);
    printf("  ndims=%d nvars=%d ngatts=%d unlimdimid=%d\n",
           ndims, nvars, ngatts, unlimid);

    printf("  dimensions:\n");
    for (i = 0; i < ndims; i++) {
        if (nc_inq_dim(ncid, i, name, &len) != NC_NOERR) {
            nc_close(ncid);
            return 1;
        }
        printf("    %s (id=%d) len=%zu%s\n", name, i, len,
               i == unlimid ? " unlimited" : "");
    }

    printf("  variables:\n");
    for (i = 0; i < nvars; i++) {
        if (nc_inq_var(ncid, i, name, &xtype, &var_ndims, dimids, &natt) != NC_NOERR) {
            nc_close(ncid);
            return 1;
        }
        printf("    %s (id=%d) type=%d ndims=%d natt=%d\n",
               name, i, xtype, var_ndims, natt);
    }

    if (nc_inq_varid(ncid, "temp", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start3, count3, data3_in) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    if (nc_inq_varid(ncid, "mix", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start4, count4, data4_in) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }

    for (i = 0; i < 4; i++) {
        if (data3_in[i] != data3[i]) {
            fprintf(stderr, "temp data mismatch at %d: %f != %f\n",
                    i, data3_in[i], data3[i]);
            nc_close(ncid);
            return 1;
        }
    }
    for (i = 0; i < 8; i++) {
        if (data4_in[i] != data4[i]) {
            fprintf(stderr, "mix data mismatch at %d: %u != %u\n",
                    i, (unsigned)data4_in[i], (unsigned)data4[i]);
            nc_close(ncid);
            return 1;
        }
    }

    if (nc_close(ncid) != NC_NOERR)
        return 1;
    return 0;
}

int
main(void)
{
    unlink(FILE_NAME);
    if (write_and_close())
        return 1;
    if (read_and_print())
        return 1;
    return 0;
}

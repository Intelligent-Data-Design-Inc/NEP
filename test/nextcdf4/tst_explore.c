/**
 * @file tst_explore.c
 * @brief Create a simple NEXTCDF-4 file and leave it in the build directory.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "explore.h5"

int
main(void)
{
    int ncid;
    int latid, lonid, heightid, timeid;
    int tempid, uwindid, vwindid, mixid;
    int dims3[3], dims4[4];

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
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    return 0;
}

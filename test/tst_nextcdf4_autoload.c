/**
 * @file tst_nextcdf4_autoload.c
 * @brief Tests `.ncrc` autoload and public lifecycle calls for NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-28
 */
#include <stdio.h>
#include <unistd.h>
#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** @return Zero when autoload registration and lifecycle checks pass. */
int
main(void)
{
    NC_Dispatch *dispatch = NULL;
    int ncid;
    int ret;

    ret = nc_initialize();
    if (ret != NC_NOERR) {
        fprintf(stderr, "nc_initialize failed: %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_inq_user_format(NEP_UDF_NEXTCDF4, &dispatch, NULL);
    if (ret != NC_NOERR || !dispatch || dispatch->model != NC_FORMATX_UDF9) {
        fprintf(stderr, "NEXTCDF-4 UDF9 autoload failed: %s\n", nc_strerror(ret));
        return 1;
    }

    unlink("tst_nextcdf4_autoload.h5");
    ret = nc_create("tst_nextcdf4_autoload.h5", NC_NEXTCDF4 | NC_CLOBBER, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "autoload create failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR ||
        nc_open("tst_nextcdf4_autoload.h5", NC_NEXTCDF4, &ncid) != NC_NOERR ||
        nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "autoload lifecycle failed\n");
        return 1;
    }
    unlink("tst_nextcdf4_autoload.h5");

    printf("OK: NEXTCDF-4 UDF9 autoload lifecycle\n");
    return 0;
}

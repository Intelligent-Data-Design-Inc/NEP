#include <stdio.h>
#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nextcdf4dispatch.h"
#include "nep_meta.h"

int
main(void)
{
    NC_Dispatch *dispatch = NULL;
    NC_Dispatch *registered = NULL;
    int ret;

    if (NEP_HAS_NEXTCDF4 != 1) {
        fprintf(stderr, "NEP_HAS_NEXTCDF4 is not enabled\n");
        return 1;
    }

    dispatch = NC_NEXTCDF4_initialize();
    if (!dispatch || dispatch->model != NC_FORMATX_UDF9 ||
        dispatch->dispatch_version != NC_DISPATCH_VERSION) {
        fprintf(stderr, "NEXTCDF-4 returned an invalid dispatch table\n");
        return 1;
    }

    ret = nc_inq_user_format(NEP_UDF_NEXTCDF4, &registered, NULL);
    if (ret != NC_NOERR || registered != dispatch) {
        fprintf(stderr, "UDF9 registration failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if (NC_NEXTCDF4_initialize() != dispatch) {
        fprintf(stderr, "repeated initialization changed the dispatch table\n");
        return 1;
    }

    ret = dispatch->create("tst_nextcdf4_not_created.nc", NC_NEXTCDF4, 0, 0,
                           NULL, NULL, dispatch, -1);
    if (ret != NC_ENOTBUILT) {
        fprintf(stderr, "create returned %s instead of NC_ENOTBUILT\n", nc_strerror(ret));
        return 1;
    }

    ret = NC_NEXTCDF4_finalize();
    if (ret != NC_NOERR || NC_NEXTCDF4_finalize() != NC_NOERR) {
        fprintf(stderr, "NEXTCDF-4 finalization failed\n");
        return 1;
    }

    printf("OK: NEXTCDF-4 UDF9 dispatch foundation\n");
    return 0;
}

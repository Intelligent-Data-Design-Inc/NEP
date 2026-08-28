#include <stdio.h>
#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

int
main(void)
{
    NC_Dispatch *dispatch = NULL;
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

    printf("OK: NEXTCDF-4 UDF9 autoload\n");
    return 0;
}

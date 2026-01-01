/**
 * @file @internal CDF functions.
 *
 * @author Edward Hartnett
 * @date 2025-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "nc4internal.h"
#include "cdfdispatch.h"
/* Suppress warnings from external CDF library header */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <cdf.h>
#pragma GCC diagnostic pop

/**
 * @internal Get the format (i.e. NC_FORMAT_NC_CDF) of an open CDF
 * file.
 *
 * @param ncid File ID (ignored).
 * @param formatp Pointer that gets the constant indicating format.

 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_CDF_inq_format(int ncid, int *formatp)
{
    /* Function body commented out for v1.3.0 Sprint 3 - UDF skeleton only */
    /* Implementation will be added in Sprint 4 */
    /* CDF is the format. */
    /* if (formatp) */
    /*     *formatp = NC_FORMATX_NC_CDF; */

    return NC_ENOTNC4; /* Placeholder return */
}

/**
 * @internal Return the extended format (i.e. the dispatch model),
 * plus the mode associated with an open file.
 *
 * @param ncid File ID.
 * @param formatp a pointer that gets the extended format. CDF files
 * will always get NC_FORMATX_NC_CDF.
 * @param modep a pointer that gets the open/create mode associated with
 * this file. Ignored if NULL.

 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_CDF_inq_format_extended(int ncid, int *formatp, int *modep)
{
    /* Function body commented out for v1.3.0 Sprint 3 - UDF skeleton only */
    /* Implementation will be added in Sprint 4 */
    /* NC *nc; */
    /* int retval; */

    /* LOG((2, "%s: ncid 0x%x", __func__, ncid)); */

    /* if ((retval = nc4_find_nc_grp_h5(ncid, &nc, NULL, NULL))) */
    /*     return NC_EBADID; */

    /* if (modep) */
    /*     *modep = nc->mode|NC_NETCDF4; */

    /* if (formatp) */
    /*     *formatp = NC_FORMATX_NC_CDF; */

    return NC_ENOTNC4; /* Placeholder return */
}

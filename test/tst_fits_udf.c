/**
 * @file tst_fits_udf.c
 * @brief Test for FITS User Defined Format (UDF) handler.
 *
 * Validates that a FITS file can be opened and closed through the
 * standard NetCDF API using the FITS UDF handler, without reading
 * any CFITSIO metadata.
 *
 * This test covers v2.0.0 Sprint 2.
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_FITS

#include <stdio.h>
#include <netcdf.h>
#include "fitsdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the FITS test data file (relative to test build directory). */
#define FITS_TEST_FILE "data/WFPC2u5780205r_c0fx.fits"

int
main(void)
{
    int ncid;
    int retval;

    /* Ensure the FITS UDF handler is registered (also covers builds where
       .ncrc autoload is not active). */
    if ((retval = NC_FITS_initialize()) && retval != NC_EINVAL)
        ERR(retval);

    /* Open a valid FITS file read-only via .ncrc autoload. */
    if ((retval = nc_open(FITS_TEST_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open FITS file\n");

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close FITS file\n");

    return 0;
}

#endif /* HAVE_FITS */

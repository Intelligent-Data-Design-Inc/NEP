/**
 * @file tst_mmcif_udf.c
 * @brief Smoke test for the PDBx/mmCIF User-Defined Format (UDF) handler.
 *
 * V3.4.0 Sprint 1: verifies that the no-op mmCIF dispatch layer can
 * open and close a real `.cif` file through the NetCDF API. No format
 * detection or record parsing is exercised yet; that is Sprint 2.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_MMCIF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "mmcifdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the first PDBx/mmCIF test file. */
#define MMCIF_TEST_FILE_1 "data/mmCIF/1J7W.cif"

/**
 * @internal Open and close a real PDBx/mmCIF file via the UDF handler.
 */
int
main(int argc, char **argv)
{
    int ncid, retval;

    (void)argc;
    (void)argv;

    /* Register the mmCIF UDF handler. */
    if (!NC_MMCIF_initialize())
    {
        fprintf(stderr, "Error initializing mmCIF UDF handler\n");
        return 1;
    }

    /* Open the file read-only. The magic "data_" selects UDF slot 8. */
    if ((retval = nc_open(MMCIF_TEST_FILE_1, NC_NOWRITE, &ncid)))
        ERR(retval);

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);

    printf("Done.\n");
    return 0;
}

#else /* !HAVE_MMCIF */

int
main(void)
{
    printf("mmCIF support not enabled; test skipped.\n");
    return 0;
}

#endif /* HAVE_MMCIF */

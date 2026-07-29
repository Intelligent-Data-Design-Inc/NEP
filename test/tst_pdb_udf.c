/**
 * @file tst_pdb_udf.c
 * @brief Test for the legacy PDB User-Defined Format (UDF) handler.
 *
 * V3.3.0 Sprint 1: infrastructure-only smoke test. Verifies that the
 * no-op PDB dispatch layer can open and close real legacy PDB files
 * (test/data/PDB/1J7W.pdb and 4HHB.pdb, sourced from https://www.rcsb.org)
 * without parsing any PDB records. Real record parsing is added in
 * Sprint 2.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_PDB

#include <stdio.h>
#include <netcdf.h>
#include "pdbdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the first legacy PDB test file. */
#define PDB_TEST_FILE_1 "data/PDB/1J7W.pdb"

/** Path to the second legacy PDB test file. */
#define PDB_TEST_FILE_2 "data/PDB/4HHB.pdb"

int
main(void)
{
    int ncid, retval;

    /* Ensure the PDB UDF handler is registered. */
    if (!NC_PDB_initialize())
        ERR(NC_EINVAL);

    /* Open and close the first test file. PDB files have no reliable
     * byte-0 magic, so force the UDF7 slot rather than relying on
     * NetCDF-C's first-byte magic matching. */
    if ((retval = nc_open(PDB_TEST_FILE_1, NC_UDF7, &ncid)))
        ERR(retval);
    printf("PASS: nc_open %s\n", PDB_TEST_FILE_1);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close %s\n", PDB_TEST_FILE_1);

    /* Open and close the second test file. */
    if ((retval = nc_open(PDB_TEST_FILE_2, NC_UDF7, &ncid)))
        ERR(retval);
    printf("PASS: nc_open %s\n", PDB_TEST_FILE_2);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close %s\n", PDB_TEST_FILE_2);

    printf("Done.\n");
    return 0;
}

#else  /* !HAVE_PDB */

int
main(void)
{
    printf("PDB support not built; test skipped.\n");
    return 0;
}

#endif /* HAVE_PDB */

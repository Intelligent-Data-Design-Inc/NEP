/* This is part of the netCDF package.
   Copyright 2018 University Corporation for Atmospheric Research/Unidata
   See COPYRIGHT file for conditions of use.

   NEP shim providing the nc_tests.h defines used by the ported
   nc_test4 tests, so the ports can stay near-verbatim. TEST_PNETCDF is
   deliberately not defined: NEXTCDF-4 does parallel I/O through the
   HDF5 MPI-IO path only, so upstream PnetCDF test sections compile
   out.

   Author: Edward Hartnett, Intelligent Data Design, Inc.
*/

#ifndef _NC_TESTS_H
#define _NC_TESTS_H

#include <netcdf.h>
#include "nep.h"
#include "err_macros.h"

#define NC_TESTS_MAX_DIMS 1024

#define MAX_NUM_FORMATS 5 /**< Max number of available binary formats. */

/** Useful define for tests. */
#define MEGABYTE 1048576
#define HALF_MEG (MEGABYTE/2)
#define MILLION 1000000
#define SIXTEEN_MEG 16777216
#define FOUR_MEG (SIXTEEN_MEG/4)
#define THIRTY_TWO_MEG (SIXTEEN_MEG * 2)
#define SIXTY_FOUR_MEG (SIXTEEN_MEG * 4)
#define ONE_TWENTY_EIGHT_MEG (SIXTEEN_MEG * 8)
#define TEST_VAL_42 42
#define BAD_NAME "dd//d/  "
#define NUM_CLASSIC_TYPES 6
#define NUM_NETCDF_TYPES 12
#define NC_MAX_FILENAME 4096

/* Sprint 1N4 test-only mode selector: returns NC_NEXTCDF4 for
 * NEP_NC_TEST4_MODEL unset, empty or "native", and
 * NC_NEXTCDF4 | NC_NETCDF4_MODEL for "netcdf4". Any other value is an
 * error so tests fail loudly instead of silently selecting a mode. */
static inline int
nep_nc_test4_create_mode(void)
{
    const char *mode = getenv("NEP_NC_TEST4_MODEL");

    if (!mode || !*mode || !strcmp(mode, "native"))
        return NC_NEXTCDF4;
    if (!strcmp(mode, "netcdf4"))
        return NC_NEXTCDF4 | NC_NETCDF4_MODEL;
    fprintf(stderr, "nep_nc_test4_create_mode: bad NEP_NC_TEST4_MODEL "
            "'%s' (want 'native' or 'netcdf4')\n", mode);
    exit(2);
}

/* Sprint 1V3 parallel create-mode selector for the ported
 * tst_parallel* tests: like nep_nc_test4_create_mode() but always ORs
 * in NC_NETCDF4, because upstream nc_create_par() rejects
 * non-NC_NETCDF4 create modes when netcdf-c is built without
 * PnetCDF. */
static inline int
nep_nc_test4_par_create_mode(void)
{
    return nep_nc_test4_create_mode() | NC_NETCDF4;
}

/* Sprint 1V3 parallel open-mode selector: NEXTCDF-4 dispatch is
 * selected by the NC_NEXTCDF4 flag at open; the compatibility model
 * comes from the file marker, not a flag. Combine with
 * NC_NOWRITE/NC_WRITE as upstream combines bare mode flags. */
static inline int
nep_nc_test4_par_open_mode(void)
{
    return NC_NEXTCDF4;
}

#endif /* _NC_TESTS_H */

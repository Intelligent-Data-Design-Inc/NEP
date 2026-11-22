/* This is part of the netCDF package.
   Copyright 2018 University Corporation for Atmospheric Research/Unidata
   See COPYRIGHT file for conditions of use.

   NEP shim providing the nc_tests.h defines used by the ported
   nc_test4 tests, so the ports can stay near-verbatim. TEST_PNETCDF is
   deliberately not defined: NEXTCDF-4 has no parallel I/O, so upstream
   parallel test sections compile out.

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

#endif /* _NC_TESTS_H */

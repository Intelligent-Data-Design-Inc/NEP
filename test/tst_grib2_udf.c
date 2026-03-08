/**
 * @file tst_grib2_udf.c
 * @brief Placeholder test for GRIB2 User Defined Format (UDF) handler.
 *
 * This test will validate that GRIB2 files can be opened and read through
 * the standard NetCDF API using the GRIB2 UDF handler. Full implementation
 * follows in a subsequent sprint once NC_GRIB2_open() is complete.
 *
 * This test is part of v1.7.0 Sprint 1.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#ifdef HAVE_GRIB2

#include <stdio.h>
#include <netcdf.h>
#include "grib2dispatch.h"

int
main(void)
{
    printf("GRIB2 UDF placeholder test\n");
    return 0;
}

#else

int
main(void)
{
    printf("GRIB2 support not enabled - skipping\n");
    return 0;
}

#endif /* HAVE_GRIB2 */

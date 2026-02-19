/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */
/* Copied from netcdf-c v4.9.2 (commit 2360497 in NEP, 2025-09-29).
 * Source: https://github.com/Unidata/netcdf-c/blob/main/libhdf5/ncdimscale.h
 * Do not edit. See docs/plan/v1.5.5-header-cleanup-map.md for elimination plan.
 * Category: DIRECT - used by grib2file.c for HDF5 dimension scale constants.
 * Elimination path: Requires upstream netcdf-c public UDF metadata API. */
/**
 * @file
 * @internal Includes for some HDF5 stuff needed by libhdf5 code and
 * also some h5_test tests.
 *
 * @author Ed Hartnett
 */

#ifndef _NCDIMSCALE_H_
#define _NCDIMSCALE_H_

#ifdef USE_HDF5
#include <hdf5.h>

/* This is used to uniquely identify datasets, so we can keep track of
 * dimscales. */
typedef struct hdf5_objid
{
#if H5_VERSION_GE(1,12,0)
    unsigned long fileno; /* file number */
    H5O_token_t token; /* token */
#else
    unsigned long fileno[2]; /* file number */
    haddr_t objno[2]; /* object number */
#endif
} HDF5_OBJID_T;

#endif /* USE_HDF5 */

#endif

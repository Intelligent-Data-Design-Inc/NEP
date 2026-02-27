/* Copied from netcdf-c v4.9.2 on 2025-11-01.
 * Source: https://github.com/Unidata/netcdf-c/blob/main/include/ncdimscale.h
 * Category: DIRECT — included by grib2file.c
 * Elimination path: REQUIRES UPSTREAM CHANGE — HDF5 dimension scale constants
 *   needed for GRIB2 metadata mapping. See https://github.com/Unidata/netcdf-c/issues/3277
 * Do not edit. See docs/plan/v1.5.5-header-cleanup-map.md for elimination plan. */
/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */
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

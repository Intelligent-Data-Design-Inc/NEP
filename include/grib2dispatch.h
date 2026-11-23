/**
 * @file grib2dispatch.h
 * @brief Public declarations for the GRIB2 UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _GRIB2DISPATCH_H
#define _GRIB2DISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** GRIB2 format uses UDF2 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF2
#define NC_FORMATX_NC_GRIB2 NC_FORMATX_UDF2
#else
#define NC_FORMATX_NC_GRIB2 NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the GRIB2 dispatch table in its UDF slot. */
extern NC_Dispatch *NC_GRIB2_initialize(void);

/** @brief Finalize process-wide GRIB2 dispatch state. */
extern int NC_GRIB2_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _GRIB2DISPATCH_H */

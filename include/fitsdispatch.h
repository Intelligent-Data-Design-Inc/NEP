/**
 * @file fitsdispatch.h
 * @brief Public declarations for the FITS UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _FITSDISPATCH_H
#define _FITSDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** FITS format uses UDF3 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF3
#define NC_FORMATX_NC_FITS NC_FORMATX_UDF3
#else
#define NC_FORMATX_NC_FITS NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the FITS dispatch table in its UDF slot. */
extern NC_Dispatch *NC_FITS_initialize(void);

/** @brief Finalize process-wide FITS dispatch state. */
extern int NC_FITS_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _FITSDISPATCH_H */

/**
 * @file cdfdispatch.h
 * @brief Public declarations for the CDF UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2025-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _CDFDISPATCH_H
#define _CDFDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** CDF format uses UDF4 slot for dispatch table model field (see NEP.h for slot allocation) */
#ifdef NC_FORMATX_UDF4
#define NC_FORMATX_NC_CDF NC_FORMATX_UDF4
#else
#define NC_FORMATX_NC_CDF NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the CDF dispatch table in its UDF slot. */
extern NC_Dispatch *NC_CDF_initialize(void);

/** @brief Finalize process-wide CDF dispatch state. */
extern int NC_CDF_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /*_CDFDISPATCH_H */

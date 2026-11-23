/**
 * @file pds4dispatch.h
 * @brief Public declarations for the PDS4 UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-07-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _PDS4DISPATCH_H
#define _PDS4DISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** PDS4 format uses UDF5 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF5
#define NC_FORMATX_NC_PDS4 NC_FORMATX_UDF5
#else
#define NC_FORMATX_NC_PDS4 NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the PDS4 dispatch table in its UDF slot. */
extern NC_Dispatch *NC_PDS4_initialize(void);

/** @brief Finalize process-wide PDS4 dispatch state. */
extern int NC_PDS4_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PDS4DISPATCH_H */

/**
 * @file pdbdispatch.h
 * @brief Public declarations for the legacy PDB UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _PDBDISPATCH_H
#define _PDBDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** Legacy PDB format uses UDF7 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF7
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF7
#else
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the legacy PDB dispatch table in its UDF slot. */
extern NC_Dispatch *NC_PDB_initialize(void);

/** @brief Finalize process-wide legacy PDB dispatch state. */
extern int NC_PDB_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PDBDISPATCH_H */

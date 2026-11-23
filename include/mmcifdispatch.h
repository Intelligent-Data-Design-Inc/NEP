/**
 * @file mmcifdispatch.h
 * @brief Public declarations for the PDBx/mmCIF UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _MMCIFDISPATCH_H
#define _MMCIFDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** PDBx/mmCIF format uses UDF8 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF8
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF8
#else
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the PDBx/mmCIF dispatch table in its UDF slot. */
extern NC_Dispatch *NC_MMCIF_initialize(void);

/** @brief Finalize process-wide PDBx/mmCIF dispatch state. */
extern int NC_MMCIF_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _MMCIFDISPATCH_H */

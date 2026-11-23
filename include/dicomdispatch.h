/**
 * @file dicomdispatch.h
 * @brief Public declarations for the DICOM UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-07-25
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _DICOMDISPATCH_H
#define _DICOMDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** DICOM format uses UDF6 slot for dispatch table model field. */
#ifdef NC_FORMATX_UDF6
#define NC_FORMATX_NC_DICOM NC_FORMATX_UDF6
#else
#define NC_FORMATX_NC_DICOM NC_FORMATX_UDF0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the DICOM dispatch table in its UDF slot. */
extern NC_Dispatch *NC_DICOM_initialize(void);

/** @brief Finalize process-wide DICOM dispatch state. */
extern int NC_DICOM_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /* _DICOMDISPATCH_H */

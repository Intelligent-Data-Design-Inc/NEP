/**
 * @file geotiffdispatch.h
 * @brief Public declarations for the GeoTIFF UDF backend.
 *
 * This header declares only what applications need — the
 * initialize/finalize pair and the extended format identifier.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2025-12-26
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _GEOTIFFDISPATCH_H
#define _GEOTIFFDISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** GeoTIFF format uses UDF1 slot for dispatch table model field (see NEP.h for slot allocation) */
#define NC_FORMATX_NC_GEOTIFF NC_FORMATX_UDF1

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the GeoTIFF dispatch table in its UDF slot. */
extern NC_Dispatch *NC_GEOTIFF_initialize(void);

/** @brief Finalize process-wide GeoTIFF dispatch state. */
extern int NC_GEOTIFF_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif /*_GEOTIFFDISPATCH_H */

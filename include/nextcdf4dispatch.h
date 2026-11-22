/**
 * @file nextcdf4dispatch.h
 * @brief Public declarations for the NEXTCDF-4 UDF backend.
 *
 * NEXTCDF-4 is a netCDF User Defined Format (UDF) backend: a
 * plug-in file format registered in UDF slot 9 (`NC_NEXTCDF4`).
 * This installed header declares only what applications need —
 * the initialize/finalize pair and the extended format identifier.
 * `nc_create()`/`nc_open()` with the `NC_NEXTCDF4` mode flag
 * routes to the backend through netcdf-c; see
 * @ref nextcdf4_architecture.
 *
 * This header may be included by downstream projects: it depends
 * only on installed netcdf-c headers and `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NEXTCDF4DISPATCH_H
#define NEXTCDF4DISPATCH_H

#include <netcdf.h>
#include <netcdf_dispatch.h>
#include "nep.h"

/** @brief Extended format identifier reported by
 * `nc_inq_format_extended()` for NEXTCDF-4 files. */
#define NC_FORMATX_NEXTCDF4 NC_FORMATX_UDF9

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Register the NEXTCDF-4 dispatch table in UDF slot 9.
 *
 * Applications call this once before using `NC_NEXTCDF4` (unless the
 * backend is autoloaded via `.ncrc`). Also reads `NEP_LOG_LEVEL`.
 */
extern NC_Dispatch *NC_NEXTCDF4_initialize(void);

/** @brief Finalize process-wide NEXTCDF-4 dispatch state. */
extern int NC_NEXTCDF4_finalize(void);

#if defined(__cplusplus)
}
#endif

#endif

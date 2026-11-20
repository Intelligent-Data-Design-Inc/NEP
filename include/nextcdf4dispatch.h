/**
 * @file nextcdf4dispatch.h
 * @brief Public dispatch declarations for the NEXTCDF-4 UDF backend.
 *
 * NEXTCDF-4 is a netCDF User Defined Format (UDF) backend: a
 * plug-in file format registered in UDF slot 9 (`NC_NEXTCDF4`).
 * These declarations are the backend's dispatch-table entry points
 * and its initialize/finalize pair. Most applications never call
 * them directly — `nc_create()`/`nc_open()` with the `NC_NEXTCDF4`
 * mode flag routes here through netcdf-c. See
 * @ref nextcdf4_architecture.
 *
 * @author Edward Hartnett
 * @date 2026-08-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NEXTCDF4DISPATCH_H
#define NEXTCDF4DISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** @brief Extended format identifier reported by
 * `nc_inq_format_extended()` for NEXTCDF-4 files. */
#define NC_FORMATX_NEXTCDF4 NC_FORMATX_UDF9

#if defined(__cplusplus)
extern "C" {
#endif

/** @brief Dispatch-table entry behind `nc_create()` with
 * `NC_NEXTCDF4`: creates the HDF5 file and writes the backend marker
 * attributes.
 */
extern int NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                           size_t *chunksizehintp, void *parameters,
                           const NC_Dispatch *dispatch, int ncid);

/** @brief Dispatch-table entry behind `nc_open()` with `NC_NEXTCDF4`:
 * opens the HDF5 file, validates the marker attributes and loads all
 * metadata.
 */
extern int NEXTCDF4_open(const char *path, int mode, int basepe,
                         size_t *chunksizehintp, void *parameters,
                         const NC_Dispatch *dispatch, int ncid);

/** @brief Dispatch-table entry behind `nc_abort()`: releases the
 * file's resources; shares the close path.
 */
extern int NEXTCDF4_abort(int ncid);

/** @brief Dispatch-table entry behind `nc_close()`: ends define mode
 * if needed, flushes, closes all HDF5 objects and frees state.
 */
extern int NEXTCDF4_close(int ncid, void *parameters);

/** @brief Dispatch-table entry behind `nc_sync()`: flushes a
 * writable file to storage.
 */
extern int NEXTCDF4_sync(int ncid);

/** @brief Dispatch-table entry behind `nc_inq_format()`: reports
 * `NC_FORMAT_NETCDF4` — NEXTCDF-4 files are in the NetCDF-4/HDF5
 * family.
 */
extern int NEXTCDF4_inq_format(int ncid, int *formatp);

/** @brief Dispatch-table entry behind `nc_inq_format_extended()`:
 * reports `NC_FORMATX_NEXTCDF4` and the effective mode flags.
 */
extern int NEXTCDF4_inq_format_extended(int ncid, int *formatp, int *modep);

/** @brief Register the NEXTCDF-4 dispatch table in UDF slot 9.
 *
 * Applications call this once before using `NC_NEXTCDF4` (unless the
 * backend is autoloaded via `.ncrc`). Also reads `NEP_LOG_LEVEL`.
 */
extern NC_Dispatch *NC_NEXTCDF4_initialize(void);

/** @brief Finalize process-wide NEXTCDF-4 dispatch state. */
extern int NC_NEXTCDF4_finalize(void);

/** @brief Active NEXTCDF-4 dispatch table, or `NULL` before
 * initialization. */
extern const NC_Dispatch *NEXTCDF4_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif

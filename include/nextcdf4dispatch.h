/**
 * @file nextcdf4dispatch.h
 * @brief Public dispatch declarations for the NEXTCDF-4 UDF backend.
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

/** Extended format identifier used by the NEXTCDF-4 dispatch table. */
#define NC_FORMATX_NEXTCDF4 NC_FORMATX_UDF9

#if defined(__cplusplus)
extern "C" {
#endif

/** Create an empty NEXTCDF-4 file.
 * @param path File-system path to create.
 * @param cmode NetCDF create-mode flags.
 * @param initialsz Requested initial file size; currently ignored.
 * @param basepe Base processing element; currently ignored.
 * @param chunksizehintp Chunk-size hint; currently ignored.
 * @param parameters Optional dispatch parameters; currently ignored.
 * @param dispatch Dispatch table selected by netcdf-c.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                           size_t *chunksizehintp, void *parameters,
                           const NC_Dispatch *dispatch, int ncid);

/** Open an empty, marked NEXTCDF-4 file.
 * @param path File-system path to open.
 * @param mode NetCDF open-mode flags.
 * @param basepe Base processing element; currently ignored.
 * @param chunksizehintp Chunk-size hint; currently ignored.
 * @param parameters Optional dispatch parameters; currently ignored.
 * @param dispatch Dispatch table selected by netcdf-c.
 * @param ncid NetCDF file identifier allocated by netcdf-c.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_open(const char *path, int mode, int basepe,
                         size_t *chunksizehintp, void *parameters,
                         const NC_Dispatch *dispatch, int ncid);

/** Abort a NEXTCDF-4 file operation and release its resources.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_abort(int ncid);

/** Close a NEXTCDF-4 file and release its resources.
 * @param ncid NetCDF file identifier.
 * @param parameters Optional close parameters; currently ignored.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_close(int ncid, void *parameters);

/** Flush a writable NEXTCDF-4 file to storage.
 * @param ncid NetCDF file identifier.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_sync(int ncid);

/** Inquire the public NetCDF format class.
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for `NC_FORMAT_NETCDF4`.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_inq_format(int ncid, int *formatp);

/** Inquire the extended NEXTCDF-4 format and effective mode.
 * @param ncid NetCDF file identifier.
 * @param formatp Optional destination for `NC_FORMATX_NEXTCDF4`.
 * @param modep Optional destination for the effective open/create mode.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
extern int NEXTCDF4_inq_format_extended(int ncid, int *formatp, int *modep);

/** Register the NEXTCDF-4 dispatch table in UDF slot 9.
 * @return Pointer to the NEXTCDF-4 dispatch table.
 */
extern NC_Dispatch *NC_NEXTCDF4_initialize(void);

/** Finalize process-wide NEXTCDF-4 dispatch state.
 * @return `NC_NOERR`.
 */
extern int NC_NEXTCDF4_finalize(void);

/** Active NEXTCDF-4 dispatch table, or `NULL` before initialization. */
extern const NC_Dispatch *NEXTCDF4_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif

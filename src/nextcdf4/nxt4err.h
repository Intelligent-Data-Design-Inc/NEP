/**
 * @file nxt4err.h
 * @brief Error-handling macros for the NEXTCDF-4 backend.
 *
 * These macros mirror the NetCDF-C BAIL/BAIL2 pattern used in
 * libsrc4 and libhdf5, adapted to the local cleanup style (`ret` and
 * `fail:`) used by the NEXTCDF-4 source files.
 *
 * @author Edward Hartnett
 * @date 2026-08-31
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NXT4ERR_H
#define NXT4ERR_H

#include "config.h"
#include "nc_logging.h"
#include <netcdf.h>

#ifdef LOGGING
/* Dump the HDF5 error stack after a NetCDF HDF5 failure. */
extern void nc_log_hdf5(void);
#endif /* LOGGING */

/**
 * @internal Log a NetCDF error, set `ret`, and jump to the `fail:`
 * cleanup label. Use this for errors that are not HDF5-specific (for
 * example, invalid arguments or missing state).
 */
#ifdef BAIL
#undef BAIL
#endif
#define BAIL(e) \
   do { \
      BAILLOG(e); \
      ret = e; \
      goto fail; \
   } while (0)

/**
 * @internal Log a NetCDF error, dump the HDF5 error stack, set `ret`,
 * and jump to the `fail:` cleanup label. Use this after an HDF5 API
 * call fails.
 */
#ifdef LOGGING
#define BAIL2(e) \
   do { \
      BAILLOG(e); \
      nc_log_hdf5(); \
      ret = e; \
      goto fail; \
   } while (0)
#else /* LOGGING */
#define BAIL2(e) BAIL(e)
#endif /* LOGGING */

#endif /* NXT4ERR_H */

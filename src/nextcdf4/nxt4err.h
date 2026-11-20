/**
 * @file nxt4err.h
 * @brief Error-handling macros for the NEXTCDF-4 backend.
 *
 * These macros mirror the NetCDF-C BAIL/BAIL2 pattern used in
 * libsrc4 and libhdf5, adapted to the local cleanup style (`ret` and
 * `fail:`) used by the NEXTCDF-4 source files. See
 * @ref nextcdf4_architecture for the convention in context.
 *
 * @author Edward Hartnett
 * @date 2026-08-31
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NXT4ERR_H
#define NXT4ERR_H

#include "config.h"
#include "nep_logging.h"
#include <netcdf.h>
#include <hdf5.h>

/**
 * @brief Log a NetCDF error, set `ret`, and jump to the `fail:`
 * cleanup label.
 *
 * Use this for errors that are not HDF5-specific (for example,
 * invalid arguments or missing state). Every function using it must
 * declare `int ret = NC_NOERR;` and provide a `fail:` label.
 */
#ifdef EBAIL
#undef EBAIL
#endif
#define EBAIL(e) \
   do { \
      EBAILLOG(e); \
      ret = e; \
      goto fail; \
   } while (0)

/**
 * @brief Log a NetCDF error, dump the HDF5 error stack, set `ret`,
 * and jump to the `fail:` cleanup label.
 *
 * Use this after an HDF5 API call fails — HDF5 records its own
 * detailed error stack, which `H5Eprint2` dumps to stderr. Without
 * logging compiled in (`NEP_ENABLE_LOGGING=OFF`) it degrades to
 * EBAIL().
 */
#ifdef NEP_LOGGING
#define EBAIL2(e) \
   do { \
      EBAILLOG(e); \
      H5Eprint2(H5E_DEFAULT, stderr); \
      ret = e; \
      goto fail; \
   } while (0)
#else /* NEP_LOGGING */
#define EBAIL2(e) EBAIL(e)
#endif /* NEP_LOGGING */

#endif /* NXT4ERR_H */

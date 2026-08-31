/**
 * @file nep_logging.h
 * @brief Internal NEP diagnostic-logging macros.
 *
 * This header is intended for use inside the NEP libraries and UDF
 * handlers. It mirrors the NetCDF-C `LOG(())` macro convention used in
 * `netcdf-c/libsrc4` and `netcdf-c/libhdf5`, but routes output through
 * NEP's own `nep_log()` instead of NetCDF-C's `nc_log()`. This makes NEP
 * logging independent of whether the underlying NetCDF-C build has
 * logging enabled.
 *
 * The public API for controlling logging is `nep_set_log_level()`, declared
 * in `nep.h`.
 *
 * @author Edward Hartnett
 * @date 2026-08-31
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef NEP_LOGGING_H
#define NEP_LOGGING_H

#include <stdlib.h>
#include <assert.h>

/** Value passed to nep_set_log_level() to disable logging. */
#define NEP_TURN_OFF_LOGGING -1

/** Default logging level for NEP builds with logging enabled. */
#define NEP_DEFAULT_LOG_LEVEL 4

#ifdef LOGGING

/* Implemented in src/nep.c (the core NEP library). */
void nep_log(int severity, const char *fmt, ...);

/** Emit a diagnostic message if LOGGING is enabled. */
#define LOG(e) nep_log e

/**
 * Log an error message including the source location and NetCDF error
 * string for @p e.
 */
#define BAILLOG(e) \
   do { \
      LOG((0, "file %s, line %d.\n%s", __FILE__, __LINE__, nc_strerror(e))); \
   } while (0)

/**
 * Set retval to @p e and jump to the `exit:` label without emitting an
 * error message.
 */
#define BAIL_QUIET(e) \
   do { \
      retval = e; \
      goto exit; \
   } while (0)

#else /* LOGGING */

/** Suppress diagnostic messages when logging is disabled. */
#define LOG(e)

/** No-op when logging is disabled. */
#define BAILLOG(e) \
   do { \
   } while (0)

/**
 * When logging is disabled, BAIL_QUIET is equivalent to BAIL because no
 * message is emitted either way.
 */
#define BAIL_QUIET BAIL

#endif /* LOGGING */

/**
 * Log an error message (if logging is enabled), set retval to @p e, and
 * jump to the `exit:` cleanup label.
 */
#define BAIL(e) \
   do { \
      BAILLOG(e); \
      retval = e; \
      goto exit; \
   } while (0)

#endif /* NEP_LOGGING_H */

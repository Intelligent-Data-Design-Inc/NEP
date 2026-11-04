/**
 * @file
 * This is the code file for nep.
 *
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "nep.h"
#include "nep_logging.h"
#include <stdarg.h>
#include <stdio.h>
#ifdef NEP_LOGGING
/**
 * Current severity threshold for NEP diagnostic messages. Messages with
 * severity greater than this value are suppressed. Initialized to
 * NEP_DEFAULT_LOG_LEVEL so that logging is active by default in builds
 * where NEP_LOGGING is enabled.
 */
static int nep_log_level = NEP_DEFAULT_LOG_LEVEL;

/**
 * Write a NEP diagnostic message to stderr.
 *
 * @param severity Message severity (0 for errors, higher for less important
 * messages).
 * @param fmt printf-style format string.
 */
void
nep_log(int severity, const char *fmt, ...)
{
    va_list ap;

    if (severity > nep_log_level)
        return;

    if (severity == 0)
        fprintf(stderr, "ERROR: ");
    else
        fprintf(stderr, "%d: ", severity);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");
    fflush(stderr);
}
#endif /* NEP_LOGGING */

/**
 * Set the NEP logging threshold.
 *
 * @param new_level The new logging level. Use -1 to disable logging,
 * 0 for errors only, and higher numbers for progressively more verbose output.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
nep_set_log_level(int new_level)
{
#ifdef NEP_LOGGING
    nep_log_level = new_level;
#endif /* NEP_LOGGING */
    return NC_NOERR;
}

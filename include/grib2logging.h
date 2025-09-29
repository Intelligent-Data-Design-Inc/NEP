/**
 * @file Header file for logging for the GRIB2 dispatch layer library.
 *
 * @author Ed Hartnett
*/

#ifndef _GRIB2LOGGING_
#define _GRIB2LOGGING_

#include <stdlib.h>
#include <assert.h>

/* Set the log level. 0 shows only errors, 1 only major messages,
 * etc., to 5, which shows way too much information. */
int grib2_set_log_level(int new_level);

/* To log something... */
void grib2_log(int severity, const char *fmt, ...);

/* Use this to turn off logging by calling
   grib2_log_level(NC_TURN_OFF_LOGGING) */
#define GRIB2_TURN_OFF_LOGGING (-1)

#ifdef GRIB2_LOGGING
#define LOG(e) grib2_log e
#else /* GRIB2_LOGGING */
#define LOG(e)
#endif /* GRIB2_LOGGING */

#endif /* _GRIB2LOGGING_ */


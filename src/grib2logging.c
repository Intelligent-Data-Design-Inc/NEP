/**
 * @file
 * Logging function for the GRIB2 dispatch layer library.
 *
 * @author Ed Hartnett
*/

#include "config.h"
#include "grib2logging.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"

int grib2_log_level = GRIB2_TURN_OFF_LOGGING;

/**
 * This function prints out a message, if the severity of the message
 * is lower than the global grib2_log_level. To use it, do something like
 *
 * grib2_log(0, "this computer will explode in %d seconds", i);
 *
 * After the first arg (the severity), use the rest like a normal
 * printf statement. Output will appear on stdout.
 *
 * @param severity Print if this is > grib2_log_level.
 * @param fmt Format string as from printf statement.
 *
 * @author Ed Hartnett
 */
void 
grib2_log(int severity, const char *fmt, ...)
{
#ifdef GRIB2_LOGGING
   va_list argp;
   int t;

   /* If the severity is greater than the log level, we don' care to
      print this message. */
   if (severity > grib2_log_level)
      return;

   /* If the severity is zero, this is an error. Otherwise insert that
      many tabs before the message. */
   if (!severity)
      fprintf(stdout, "ERROR: ");
   for (t=0; t<severity; t++)
      fprintf(stdout, "\t");

   /* Print out the variable list of args with vprintf. */
   va_start(argp, fmt);
   vfprintf(stdout, fmt, argp);
   va_end(argp);
   
   /* Put on a final linefeed. */
   fprintf(stdout, "\n");
   fflush(stdout);
#endif /* ifdef GRIB2_LOGGING */
}

/**
 * Use this to set the global log level. Set it to GRIB2_TURN_OFF_LOGGING
 * (-1) to turn off all logging. Set it to 0 to show only errors, and
 * to higher numbers to show more and more logging details.
 *
 * @param new_level The new logging level.
 *
 * @return NC_NOERR No error.
 * @author Ed Hartnett
 */
int
grib2_set_log_level(int new_level)
{
#ifdef GRIB2_LOGGING
   /* Remember the new level. */
   grib2_log_level = new_level;
   LOG((4, "log_level changed to %d", grib2_log_level));
#endif /* ifdef GRIB2_LOGGING */   
   return 0;
}




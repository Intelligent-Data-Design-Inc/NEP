/**
 * @file
 * @internal This header file contains the prototypes for the
 * FITS versions of the netCDF functions. This is part of the FITS
 * dispatch layer and this header should not be included by any file
 * outside the libncfits directory.
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _FITSDISPATCH_H
#define _FITSDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/* Include CFITSIO header if available */
#ifdef HAVE_FITS
#include <fitsio.h>
#endif

/** FITS format uses UDF3 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF3
#define NC_FORMATX_NC_FITS NC_FORMATX_UDF3
#else
#define NC_FORMATX_NC_FITS NC_FORMATX_UDF0
#endif

/** FITS magic number length (6 bytes: "SIMPLE") */
#define FITS_MAGIC_LEN 6

/** Per-variable FITS info: which HDU and (for tables) which column. */
typedef struct NC_FITS_VAR_INFO
{
    int hdu_num;    /**< 1-based HDU number in the FITS file */
    int col_num;    /**< 1-based column number; 0 = image variable */
} NC_FITS_VAR_INFO_T;

/** Per-file FITS state with CFITSIO integration */
typedef struct NC_FITS_FILE_INFO
{
#ifdef HAVE_FITS
    fitsfile *fptr;               /**< CFITSIO fitsfile pointer */
#else
    void *fptr;                   /**< Placeholder when CFITSIO not available */
#endif
    char *path;                   /**< Path to the open FITS file */
    int num_hdus;                 /**< Total number of HDUs in the file */
} NC_FITS_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_FITS_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                 void *parameters, const NC_Dispatch *, int);

    extern int
    NC_FITS_abort(int ncid);

    extern int
    NC_FITS_close(int ncid, void *ignore);

    extern int
    NC_FITS_inq_format(int ncid, int *formatp);

    extern int
    NC_FITS_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_FITS_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                     void *value, nc_type);

    extern int
    NC_FITS_initialize(void);

    extern int
    NC_FITS_finalize(void);

#define FITS_INIT_OK() (NC_FITS_initialize() == NC_NOERR)
#define FITS_INIT_AND_ASSIGN(ret) do { \
        (ret) = NC_FITS_initialize(); \
    } while(0)

    extern const NC_Dispatch *FITS_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _FITSDISPATCH_H */

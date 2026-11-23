/**
 * @file fitsinternal.h
 * @brief Private state and shared helpers for the FITS UDF backend.
 *
 * @author Edward Hartnett
 * @date 2026-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef FITSINTERNAL_H
#define FITSINTERNAL_H

#include "config.h"
#include "ncdispatch.h"
#include "fitsdispatch.h"

/* Include CFITSIO header if available */
#ifdef HAVE_FITS
#include <fitsio.h>
#endif

/** FITS magic number length (6 bytes: "SIMPLE") */
#define FITS_MAGIC_LEN 6

/** Per-variable FITS info: which HDU and (for tables) which column. */
typedef struct NC_FITS_VAR_INFO
{
    int hdu_num;    /**< 1-based HDU number in the FITS file */
    int col_num;    /**< 1-based column number; 0 = image variable */
    int col_width;  /**< bytes per string element (TSTRING columns only; 0 otherwise) */
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

#define FITS_INIT_OK() (NC_FITS_initialize() != NULL)
#define FITS_INIT_AND_ASSIGN(ret) do { \
        (ret) = (NC_FITS_initialize() != NULL) ? NC_NOERR : NC_EINVAL; \
    } while(0)

    extern const NC_Dispatch *FITS_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* FITSINTERNAL_H */

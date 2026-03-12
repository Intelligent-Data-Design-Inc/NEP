/**
 * @file
 * @internal This header file contains the prototypes for the
 * GRIB2 versions of the netCDF functions. This is part of the GRIB2
 * dispatch layer and this header should not be included by any file
 * outside the libgrib2 directory.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _GRIB2DISPATCH_H
#define _GRIB2DISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** GRIB2 format uses UDF2 slot for dispatch table model field (see nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF2
#define NC_FORMATX_NC_GRIB2 NC_FORMATX_UDF2
#else
#define NC_FORMATX_NC_GRIB2 NC_FORMATX_UDF0
#endif

/** Maximum number of GRIB2 messages supported per file */
#define NC_MAX_GRIB2_MESSAGES 4096

/** GRIB2 magic number length (4 bytes: "GRIB") */
#define GRIB2_MAGIC_LEN 4

/** Maximum length of a g2c parameter abbreviation string */
#define NC_GRIB2_ABBREV_LEN 64

/** Per-product inventory entry (one per GRIB2 product/field) */
typedef struct NC_GRIB2_PROD_INFO
{
    int msg_index;                    /**< Zero-based GRIB2 message index */
    int prod_index;                   /**< Zero-based product index within message */
    unsigned char discipline;         /**< GRIB2 discipline (Table 0.0) */
    int category;                     /**< GRIB2 parameter category */
    int param_number;                 /**< GRIB2 parameter number */
    size_t nx;                        /**< Grid size in X (longitude) direction */
    size_t ny;                        /**< Grid size in Y (latitude) direction */
    char abbrev[NC_GRIB2_ABBREV_LEN]; /**< Parameter abbreviation from g2c_param_abbrev() */
} NC_GRIB2_PROD_INFO_T;

/** Per-variable format-specific information (used as NC_VAR_INFO_T.format_var_info) */
typedef struct NC_VAR_GRIB2_INFO
{
    int msg_index;       /**< Zero-based index of the GRIB2 message */
    int prod_index;      /**< Zero-based product index within message */
    int discipline;      /**< GRIB2 discipline (Table 0.0) */
    int category;        /**< GRIB2 parameter category */
    int param_number;    /**< GRIB2 parameter number */
} NC_VAR_GRIB2_INFO_T;

/** Per-file GRIB2 state */
typedef struct NC_GRIB2_FILE_INFO
{
    int g2cid;                    /**< File ID returned by g2c_open() */
    int num_messages;             /**< Number of GRIB2 messages in the file */
    int num_products;             /**< Total number of products across all messages */
    size_t num_y;                 /**< Grid size in Y (latitude) direction */
    size_t num_x;                 /**< Grid size in X (longitude) direction */
    char *path;                   /**< Path to the open GRIB2 file */
    NC_GRIB2_PROD_INFO_T *products; /**< Per-product inventory array (num_products entries) */
} NC_GRIB2_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_GRIB2_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                  void *parameters, const NC_Dispatch *, int);

    extern int
    NC_GRIB2_abort(int ncid);

    extern int
    NC_GRIB2_close(int ncid, void *ignore);

    extern int
    NC_GRIB2_inq_format(int ncid, int *formatp);

    extern int
    NC_GRIB2_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_GRIB2_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type);

    extern NC_Dispatch*
    NC_GRIB2_initialize(void);

    extern int
    NC_GRIB2_finalize(void);

#define GRIB2_INIT_OK() (NC_GRIB2_initialize() != NULL)
#define GRIB2_INIT_AND_ASSIGN(ret) do { \
        NC_Dispatch *_d = NC_GRIB2_initialize(); \
        (ret) = (_d != NULL) ? NC_NOERR : NC_ENOTNC; \
    } while(0)

    extern const NC_Dispatch *GRIB2_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _GRIB2DISPATCH_H */

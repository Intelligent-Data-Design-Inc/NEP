/**
 * @file
 * @brief Public types and prototypes for the DICOM UDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-07-25
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _DICOMDISPATCH_H
#define _DICOMDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/* Include libdicom header if available */
#ifdef HAVE_DICOM
#include <dicom/dicom.h>
#endif

/** DICOM format uses UDF6 slot for dispatch table model field. */
#ifdef NC_FORMATX_UDF6
#define NC_FORMATX_NC_DICOM NC_FORMATX_UDF6
#else
#define NC_FORMATX_NC_DICOM NC_FORMATX_UDF0
#endif

/** DICOM magic number length (4 bytes: "DICM") */
#define DICOM_MAGIC_LEN 4

/** Per-variable DICOM info: frame index (1-based for pixel_data). */
typedef struct NC_DICOM_VAR_INFO
{
    int frame_index;    /**< 1-based frame number for pixel data variables */
} NC_DICOM_VAR_INFO_T;

/** Per-file DICOM state with libdicom integration. */
typedef struct NC_DICOM_FILE_INFO
{
#ifdef HAVE_DICOM
    DcmFilehandle *filehandle;  /**< libdicom file handle */
    const DcmDataSet *metadata; /**< Borrowed metadata subset */
#endif
    char *path;                 /**< Path to the open DICOM file */
    char *transfer_syntax_uid;  /**< Transfer Syntax UID string */
    int encapsulated;           /**< Non-zero for compressed transfer syntaxes */
    int jpeg_lossless;          /**< Non-zero for JPEG Lossless (Process 14) */
    int nframes;                  /**< Number of frames (Sprint 1: always 1) */
    size_t rows;                /**< Image rows */
    size_t columns;               /**< Image columns */
    int samples_per_pixel;        /**< Samples per pixel (1 or 3) */
    int bits_allocated;           /**< Bits allocated per sample */
    int bits_stored;              /**< Bits stored per sample */
    int high_bit;                 /**< Zero-based high bit index */
    int pixel_representation;     /**< 0 = unsigned, 1 = signed */
    int planar_configuration;   /**< 0 = color-by-pixel, 1 = color-by-plane */
    nc_type xtype;                /**< NetCDF type for pixel data */
    size_t type_size;             /**< Size in bytes of one pixel sample */
    int frame_dim_index;          /**< Index of frame dimension (always 0) */
    int row_dim_index;            /**< Index of row dimension */
    int col_dim_index;            /**< Index of column dimension */
    int sample_dim_index;         /**< Index of sample dimension, or -1 */
    int color_dim_index;          /**< Deprecated; kept for compatibility */
} NC_DICOM_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_DICOM_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                  void *parameters, const NC_Dispatch *, int);

    extern int
    NC_DICOM_abort(int ncid);

    extern int
    NC_DICOM_close(int ncid, void *ignore);

    extern int
    NC_DICOM_inq_format(int ncid, int *formatp);

    extern int
    NC_DICOM_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_DICOM_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type);

    extern NC_Dispatch*
    NC_DICOM_initialize(void);

    extern int
    NC_DICOM_finalize(void);

#define DICOM_INIT_OK() (NC_DICOM_initialize() != NULL)
#define DICOM_INIT_AND_ASSIGN(ret) do { \
        (ret) = (NC_DICOM_initialize() != NULL) ? NC_NOERR : NC_EINVAL; \
    } while(0)

    extern const NC_Dispatch *DICOM_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _DICOMDISPATCH_H */

/**
 * @file
 * @internal This header file contains the prototypes for the
 * GeoTIFF versions of the netCDF functions. This is part of the GeoTIFF
 * dispatch layer and this header should not be included by any file
 * outside the libgeotiff directory.
 *
 * @author Edward Hartnett
 * @date 2025-12-26
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _GEOTIFFDISPATCH_H
#define _GEOTIFFDISPATCH_H

#include "config.h"
#include "ncdispatch.h"

/** GeoTIFF format uses UDF1 slot in NetCDF-C dispatch table */
#define NC_FORMATX_NC_GEOTIFF NC_FORMATX_UDF1

/** TIFF magic numbers for format detection */
#define TIFF_MAGIC_LE 0x4949  /* Little-endian "II" */
#define TIFF_MAGIC_BE 0x4D4D  /* Big-endian "MM" */

/** TIFF version numbers */
#define TIFF_VERSION_CLASSIC 42
#define TIFF_VERSION_BIGTIFF 43

/** Minimum TIFF header size for validation */
#define TIFF_HEADER_SIZE 8

typedef struct NC_VAR_GEOTIFF_INFO
{
    int band_num;
    int geotiff_data_type;
} NC_VAR_GEOTIFF_INFO_T;

typedef struct NC_GEOTIFF_FILE_INFO
{
    void *tiff_handle;
    void *gtif_handle;
    char *path;
    int is_little_endian;
} NC_GEOTIFF_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_GEOTIFF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                    void *parameters, const NC_Dispatch *, int);

    extern int
    NC_GEOTIFF_abort(int ncid);

    extern int
    NC_GEOTIFF_close(int ncid, void *ignore);

    extern int
    NC_GEOTIFF_inq_format(int ncid, int *formatp);

    extern int
    NC_GEOTIFF_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_GEOTIFF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                        void *value, nc_type);

    extern int
    NC_GEOTIFF_initialize(void);

    extern int
    NC_GEOTIFF_finalize(void);

    extern int
    NC_GEOTIFF_detect_format(const char *path, int *is_geotiff);

    extern int
    NC_GEOTIFF_extract_metadata(struct NC_FILE_INFO *h5, NC_GEOTIFF_FILE_INFO_T *geotiff_info);

    extern const NC_Dispatch *GEOTIFF_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /*_GEOTIFFDISPATCH_H */

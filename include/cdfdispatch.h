/**
 * @file @internal This header file contains the prototypes for the
 * CDF versions of the netCDF functions. This is part of the CDF
 * dispatch layer and this header should not be included by any file
 * outside the libhdf4 directory.
 *
 * @author Edward Hartnett
 * @date 2025-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _CDFDISPATCH_H
#define _CDFDISPATCH_H

#include "config.h"
#include "ncdispatch.h"

/** This is the max number of dimensions for a CDF SD dataset (from
 * CDF documentation). */
#define NC_MAX_CDF_DIMS 32

/* Stuff below is for hdf4 files. */
typedef struct NC_VAR_CDF_INFO
{
    int sdsid;
    int hdf4_data_type;
} NC_VAR_CDF_INFO_T;

typedef struct NC_CDF_FILE_INFO
{
    int sdid;
} NC_CDF_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_CDF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                 void *parameters, const NC_Dispatch *, int);

    extern int
    NC_CDF_abort(int ncid);

    extern int
    NC_CDF_close(int ncid, void *ignore);

    extern int
    NC_CDF_inq_format(int ncid, int *formatp);

    extern int
    NC_CDF_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_CDF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                     void *value, nc_type);

#if defined(__cplusplus)
}
#endif

#endif /*_CDFDISPATCH_H */

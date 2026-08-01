/**
 * @file
 * @brief Public types and prototypes for the PDBx/mmCIF UDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _MMCIFDISPATCH_H
#define _MMCIFDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** PDBx/mmCIF format uses UDF8 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF8
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF8
#else
#define NC_FORMATX_NC_MMCIF NC_FORMATX_UDF0
#endif

/** Per-file PDBx/mmCIF state.
 *
 * V3.4.0 Sprint 1: placeholder struct for the no-op dispatch skeleton.
 * The full parsed in-memory representation will be added in Sprint 2.
 */
typedef struct NC_MMCIF_FILE_INFO
{
    char *path;               /**< Path to the open mmCIF file */
} NC_MMCIF_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_MMCIF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                  void *parameters, const NC_Dispatch *, int);

    extern int
    NC_MMCIF_abort(int ncid);

    extern int
    NC_MMCIF_close(int ncid, void *ignore);

    extern int
    NC_MMCIF_inq_format(int ncid, int *formatp);

    extern int
    NC_MMCIF_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_MMCIF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type);

    extern NC_Dispatch*
    NC_MMCIF_initialize(void);

    extern int
    NC_MMCIF_finalize(void);

    extern const NC_Dispatch *MMCIF_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _MMCIFDISPATCH_H */

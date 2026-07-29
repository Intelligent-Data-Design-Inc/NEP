/**
 * @file
 * @brief Public types and prototypes for the legacy PDB UDF dispatch layer.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _PDBDISPATCH_H
#define _PDBDISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

/** Legacy PDB format uses UDF7 slot for dispatch table model field (see
 * nep.h for slot allocation) */
#ifdef NC_FORMATX_UDF7
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF7
#else
#define NC_FORMATX_NC_PDB NC_FORMATX_UDF0
#endif

/** Per-file legacy PDB state. Sprint 1 is a no-op dispatch layer: only
 * the file path is stored, no record parsing occurs yet. */
typedef struct NC_PDB_FILE_INFO
{
    char *path;   /**< Path to the open PDB file */
} NC_PDB_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_PDB_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                void *parameters, const NC_Dispatch *, int);

    extern int
    NC_PDB_abort(int ncid);

    extern int
    NC_PDB_close(int ncid, void *ignore);

    extern int
    NC_PDB_inq_format(int ncid, int *formatp);

    extern int
    NC_PDB_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_PDB_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                    void *value, nc_type);

    extern NC_Dispatch*
    NC_PDB_initialize(void);

    extern int
    NC_PDB_finalize(void);

    extern const NC_Dispatch *PDB_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif /* _PDBDISPATCH_H */

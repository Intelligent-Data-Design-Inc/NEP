#ifndef NEXTCDF4DISPATCH_H
#define NEXTCDF4DISPATCH_H

#include "config.h"
#include "ncdispatch.h"
#include "nep.h"

#define NC_FORMATX_NEXTCDF4 NC_FORMATX_UDF9

#if defined(__cplusplus)
extern "C" {
#endif

extern int NEXTCDF4_create(const char *path, int cmode, size_t initialsz, int basepe,
                           size_t *chunksizehintp, void *parameters,
                           const NC_Dispatch *dispatch, int ncid);
extern int NEXTCDF4_open(const char *path, int mode, int basepe,
                         size_t *chunksizehintp, void *parameters,
                         const NC_Dispatch *dispatch, int ncid);
extern NC_Dispatch *NC_NEXTCDF4_initialize(void);
extern int NC_NEXTCDF4_finalize(void);
extern const NC_Dispatch *NEXTCDF4_dispatch_table;

#if defined(__cplusplus)
}
#endif

#endif

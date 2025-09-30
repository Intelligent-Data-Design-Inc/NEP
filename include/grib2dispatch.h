/**
 * @file
 * This header file contains the prototypes for the GRIB2 versions
 * of the netCDF functions.
 *
 * Ed Hartnett
 */
#ifndef _GRIB2DISPATCH_H
#define _GRIB2DISPATCH_H

#include "config.h"
#include <stddef.h> /* size_t, ptrdiff_t */
#include <stdio.h>
#include <netcdf.h>
#include <netcdf_dispatch.h>
#include <assert.h>
#include "grib2logging.h"

/* Stuff below is for grib2 files. */
typedef struct NC_VAR_GRIB2_INFO
{
    int sdsid;
    int grib2_data_type;
} NC_VAR_GRIB2_INFO_T;

typedef struct NC_GRIB2_FILE_INFO
{
    int sdid;
} NC_GRIB2_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int NC_GRIB2_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
			     void *parameters, const NC_Dispatch *, int);

    extern int NC_GRIB2_abort(int ncid);

    extern int NC_GRIB2_close(int ncid, void *);

    extern int GRIB2_inq_format(int ncid, int *formatp);

    extern int GRIB2_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int GRIB2_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
				 void *value, nc_type);

    extern int nc_grib2_set_log_level(int new_level);

    /* From netcdf-c. */
    extern int
    NC4_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp);
    extern int
    NC4_inq_type(int, nc_type, char *, size_t *);

    /* Begin _dim */

    extern int
    NC4_inq_dimid(int ncid, const char *name, int *idp);

    extern int
    NC4_inq_unlimdim(int ncid, int *unlimdimidp);

    /* End _dim */
    /* Begin _att */

    extern int
    NC4_inq_att(int ncid, int varid, const char *name,
                nc_type *xtypep, size_t *lenp);

    extern int
    NC4_inq_attid(int ncid, int varid, const char *name, int *idp);

    extern int
    NC4_inq_attname(int ncid, int varid, int attnum, char *name);

    /* End _att */
    /* Begin {put,get}_att */

    extern int
    NC4_get_att(int ncid, int varid, const char *name, void *value, nc_type);
    
    extern int
    NC4_inq_varid(int ncid, const char *name, int *varidp);

    EXTERNL int
    NC4_HDF5_get_att(int ncid, int varid, const char *name, void *value, nc_type);

    EXTERNL int
    HDF5_inq_dim(int ncid, int dimid, char *name, size_t *lenp);
   
    /* Expose the default vars and varm dispatch entries */
    EXTERNL int NCDEFAULT_get_vars(int, int, const size_t*,
				   const size_t*, const ptrdiff_t*, void*, nc_type);
    EXTERNL int NCDEFAULT_put_vars(int, int, const size_t*,
				   const size_t*, const ptrdiff_t*, const void*, nc_type);
    EXTERNL int NCDEFAULT_get_varm(int, int, const size_t*,
				   const size_t*, const ptrdiff_t*, const ptrdiff_t*,
				   void*, nc_type);
    EXTERNL int NCDEFAULT_put_varm(int, int, const size_t*,
				   const size_t*, const ptrdiff_t*, const ptrdiff_t*,
				   const void*, nc_type);

    EXTERNL int
    NC4_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
                    int *ndimsp, int *dimidsp, int *nattsp,
                    int *shufflep, int *deflatep, int *deflate_levelp,
                    int *fletcher32p, int *contiguousp, size_t *chunksizesp,
                    int *no_fill, void *fill_valuep, int *endiannessp,
                    unsigned int* idp, size_t* nparamsp, unsigned int* params);
    EXTERNL int
    NC4_show_metadata(int);
    EXTERNL int
    NC4_inq_unlimdims(int, int *, int *);
    EXTERNL int
    NC4_inq_ncid(int, const char *, int *);

    EXTERNL int
    NC4_inq_grps(int, int *, int *);

    EXTERNL int
    NC4_inq_grpname(int, char *);

    EXTERNL int
    NC4_inq_grpname_full(int, size_t *, char *);

    EXTERNL int
    NC4_inq_grp_parent(int, int *);

    EXTERNL int
    NC4_inq_grp_full_ncid(int, const char *, int *);

    EXTERNL int
    NC4_inq_varids(int, int * nvars, int *);

    EXTERNL int
    NC4_inq_dimids(int, int * ndims, int *, int);

    EXTERNL int
    NC4_inq_typeids(int, int * ntypes, int *);

    EXTERNL int
    NC4_inq_type_equal(int, nc_type, int, nc_type, int *);

    EXTERNL int
    NC4_inq_user_type(int, nc_type, char *, size_t *, nc_type *,
                      size_t *, int *);

    EXTERNL int
    NC4_inq_typeid(int, const char *, nc_type *);
    
    
#if defined(__cplusplus)
}
#endif

#endif /*_GRIB2DISPATCH_H */

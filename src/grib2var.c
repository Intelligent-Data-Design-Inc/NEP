/**
 * @file
 */

#include <nc4internal.h>
#include "nc4dispatch.h"

/**
 * @internal Get data from an HDF4 SD dataset.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Array of start indicies.
 * @param countp Array of counts.
 * @param mem_nc_type The type of these data after it is read into memory.
 * @param is_long Ignored for HDF4.
 * @param data pointer that gets the data.
 * @returns ::NC_NOERR for success
 * @author Ed Hartnett
 */
/* static int */
/* get_grib2_vara(NC *nc, int ncid, int varid, const size_t *startp, */
/*             const size_t *countp, nc_type mem_nc_type, int is_long, void *data) */
/* { */
/*    NC_GRP_INFO_T *grp; */
/*    NC_HDF5_FILE_INFO_T *h5; */
/*    NC_VAR_INFO_T *var; */
/*    /\* int  d; *\/ */
/*    int retval; */

/*    /\* Find our metadata for this file, group, and var. *\/ */
/*    assert(nc); */
/*    if ((retval = nc4_find_g_var_nc(nc, ncid, varid, &grp, &var))) */
/*       return retval; */
/*    h5 = NC4_DATA(nc); */
/*    assert(grp && h5 && var && var->hdr.name); */

/*    /\* for (d = 0; d < var->ndims; d++) *\/ */
/*    /\* { *\/ */
/*    /\*    start32[d] = startp[d]; *\/ */
/*    /\*    edge32[d] = countp[d]; *\/ */
/*    /\* } *\/ */

/*    /\* if (SDreaddata(var->sdsid, start32, NULL, edge32, data)) *\/ */
/*    /\*    return NC_EHDFERR; *\/ */

/*    return NC_NOERR; */
/* } */

/**
 * Read an array of values. This is called by nc_get_vara() for
 * netCDF-4 files, as well as all the other nc_get_vara_*
 * functions. HDF4 files are handled as a special case.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Array of start indicies.
 * @param countp Array of counts.
 * @param ip pointer that gets the data.
 * @param memtype The type of these data after it is read into memory.

 * @returns ::NC_NOERR for success
 * @author Ed Hartnett, Dennis Heimbigner
 */
int
GRIB2_get_vara(int ncid, int varid, const size_t *startp,
              const size_t *countp, void *ip, int memtype)
{
   NC *nc;
   /* NC_HDF5_FILE_INFO_T* h5; */

   LOG((2, "%s: ncid 0x%x varid %d memtype %d", __func__, ncid, varid,
        memtype));

   /* if (!(nc = nc4_find_nc_file(ncid, &h5))) */
   /*    return NC_EBADID; */

   /* /\* Handle HDF4 cases. *\/ */
   /* return get_grib2_vara(nc, ncid, varid, startp, countp, memtype, */
   /*                    0, (void *)ip); */
   return 0;
}

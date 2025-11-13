/**
 * @file 
 * @internal This file handles groups for the GRIB2 dispatch layer. All
 * functions return NC_ENOTNC4.
 * 
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "nc4internal.h"
#include "grib2dispatch.h"
#include "grib2logging.h"

/**
 * @internal Create a group. Its ncid is returned in the new_ncid
 * pointer. 
 *
 * @param parent_ncid Parent group.
 * @param name Name of new group.
 * @param new_ncid Pointer that gets ncid for new group.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @return NC_ESTRICTNC3 Classic model in use for this file.
 * @return NC_ENOTNC4 Not a netCDF-4 file.
 * @author Ed Hartnett
*/
int
GRIB2_def_grp(int parent_ncid, const char *name, int *new_ncid)
{
   return NC_EPERM;
}

/**
 * @internal Rename a group. 
 *
 * @param grpid Group ID.
 * @param name New name for group.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @return NC_ENOTNC4 Not a netCDF-4 file.
 * @return NC_EPERM File opened read-only.
 * @return NC_EBADGRPID Renaming root forbidden.
 * @return NC_EHDFERR HDF5 function returned error.
 * @return NC_ENOMEM Out of memory.
 * @author Ed Hartnett
*/
int
GRIB2_rename_grp(int grpid, const char *name)
{
   return NC_EPERM;   
}



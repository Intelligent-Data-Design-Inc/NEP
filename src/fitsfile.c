/**
 * @file
 * @internal The FITS file functions. These provide a read-only
 * interface to FITS astronomical data files via CFITSIO.
 *
 * Sprint 2: no-op implementations. The open function only creates the
 * internal NetCDF-4 file skeleton and stores the path; no CFITSIO
 * calls are made yet.
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nep_nc4.h"
#include "fitsdispatch.h"

/**
 * @internal Open a FITS file for read-only access.
 *
 * @param path Path to the FITS file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid parameters or mode flags.
 * @return ::NC_EPERM Write mode requested.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_FITS_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
             void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_FITS_FILE_INFO_T *fits_file;
    int retval;

    assert(basepe || !basepe);
    assert(chunksizehintp || !chunksizehintp);
    assert(parameters || !parameters);
    assert(dispatch);

    if (!path)
        return NC_EINVAL;

    /* Only read-only access is supported. */
    if (mode & NC_WRITE)
        return NC_EPERM;

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Allocate FITS-specific file info (placeholder for future CFITSIO state). */
    if (!(fits_file = calloc(1, sizeof(NC_FITS_FILE_INFO_T))))
        return NC_ENOMEM;
    fits_file->fptr = NULL;
    if (!(fits_file->path = strdup(path)))
    {
        free(fits_file);
        return NC_ENOMEM;
    }
    h5->format_file_info = fits_file;

    return NC_NOERR;
}

/**
 * @internal Close a FITS file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_FITS_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_FITS_FILE_INFO_T *fits_file;
    int retval;

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get FITS-specific info. */
    fits_file = (NC_FITS_FILE_INFO_T *)h5->format_file_info;
    if (!fits_file)
        return NC_NOERR;

    /* Free FITS-specific info. No CFITSIO calls in Sprint 2. */
    free(fits_file->path);
    free(fits_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Abort opening a FITS file.
 *
 * @param ncid NetCDF ID.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_FITS_abort(int ncid)
{
    (void)ncid;
    return NC_NOERR;
}

/**
 * @internal Inquire the format of a FITS file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_FITS_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a FITS file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_FITS_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_FITS;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab of data from a FITS variable.
 *
 * Sprint 2: no-op. No variables are defined yet.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices.
 * @param count Counts.
 * @param value Buffer.
 * @param memtype Memory type.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_FITS_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                 void *value, nc_type memtype)
{
    (void)ncid;
    (void)varid;
    (void)start;
    (void)count;
    (void)value;
    (void)memtype;
    return NC_NOERR;
}

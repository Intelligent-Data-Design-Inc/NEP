/**
 * @file
 * @internal The GRIB2 file functions. These provide a read-only
 * interface to GRIB2 meteorological data files via NCEPLIBS-g2c.
 *
 * @author Edward Hartnett
 * @date 2026-03-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include "nep_nc4.h"
#include "grib2dispatch.h"
#include <grib2.h>

/** @internal These flags may not be set for open mode. */
static const int
ILLEGAL_OPEN_FLAGS = (NC_MMAP|NC_64BIT_OFFSET|NC_DISKLESS|NC_WRITE);

/**
 * @internal Open a GRIB2 file for read-only access.
 *
 * @param path Path to the GRIB2 file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid parameters or mode flags.
 * @return ::NC_ENOTNC File is not a valid GRIB2 file.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_GRIB2_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_GRIB2_FILE_INFO_T *grib2_file;
    int g2cid, num_msg;
    int retval;

    assert(basepe || !basepe);
    assert(chunksizehintp || !chunksizehintp);
    assert(parameters || !parameters);
    assert(dispatch);

    if (!path)
        return NC_EINVAL;

    if (mode & ILLEGAL_OPEN_FLAGS)
        return NC_EINVAL;

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Open the GRIB2 file with g2c. */
    if (g2c_open(path, G2C_NOWRITE, &g2cid))
        return NC_ENOTNC;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
    {
        g2c_close(g2cid);
        return retval;
    }
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Query number of GRIB2 messages. */
    num_msg = 0;
    g2c_inq(g2cid, &num_msg);

    /* Allocate GRIB2-specific file info. */
    if (!(grib2_file = calloc(1, sizeof(NC_GRIB2_FILE_INFO_T))))
    {
        g2c_close(g2cid);
        return NC_ENOMEM;
    }
    grib2_file->g2cid = g2cid;
    grib2_file->num_messages = num_msg;
    h5->format_file_info = grib2_file;

    return NC_NOERR;
}

/**
 * @internal Abort (close) a GRIB2 file.
 *
 * For read-only files, abort is identical to close.
 *
 * @param ncid NetCDF ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_GRIB2_abort(int ncid)
{
    return NC_GRIB2_close(ncid, NULL);
}

/**
 * @internal Close a GRIB2 file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_GRIB2_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_GRIB2_FILE_INFO_T *grib2_file;
    int retval;

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get GRIB2-specific info. */
    grib2_file = (NC_GRIB2_FILE_INFO_T *)h5->format_file_info;
    if (!grib2_file)
        return NC_NOERR;

    /* Close the g2c file handle. */
    g2c_close(grib2_file->g2cid);

    /* Free GRIB2-specific info. */
    free(grib2_file->path);
    free(grib2_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Inquire the format of a GRIB2 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    (void)formatp;
    return NC_ENOTBUILT;
}

/**
 * @internal Inquire the extended format of a GRIB2 file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    (void)formatp;
    (void)modep;
    return NC_ENOTBUILT;
}

/**
 * @internal Read a hyperslab of data from a GRIB2 variable.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices of the hyperslab.
 * @param count Count of elements along each dimension.
 * @param value Pointer to buffer for the read data.
 * @param memtype Memory type for the data.
 *
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                  void *value, nc_type memtype)
{
    (void)ncid;
    (void)varid;
    (void)start;
    (void)count;
    (void)value;
    (void)memtype;
    return NC_ENOTBUILT;
}

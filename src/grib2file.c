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
#include "nep_nc4.h"
#include "grib2dispatch.h"

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
 * @return ::NC_EINVAL Invalid mode flags.
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    (void)path;
    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;
    (void)ncid;

    if (mode & ILLEGAL_OPEN_FLAGS)
        return NC_EINVAL;

    return NC_ENOTBUILT;
}

/**
 * @internal Abort a GRIB2 file open operation.
 *
 * @param ncid NetCDF ID.
 *
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_abort(int ncid)
{
    (void)ncid;
    return NC_ENOTBUILT;
}

/**
 * @internal Close a GRIB2 file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return ::NC_ENOTBUILT GRIB2 support not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GRIB2_close(int ncid, void *ignore)
{
    (void)ncid;
    (void)ignore;
    return NC_ENOTBUILT;
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

/**
 * @file mmciffile.c
 * @brief PDBx/mmCIF User-Defined Format (UDF) dispatch layer.
 *
 * V3.4.0 Sprint 1: no-op dispatch skeleton for PDBx/mmCIF files. Opens
 * and closes a real `.cif` file but does not parse any records. The
 * custom STAR/CIF tokenizer and NetCDF metadata model will be added in
 * Sprint 2.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "nep_nc4.h"
#include "mmcifdispatch.h"

/**
 * @internal Open a PDBx/mmCIF file.
 *
 * V3.4.0 Sprint 1: creates the minimal NetCDF-4 file metadata structs
 * and stores the file path, but does not read or parse the file
 * contents. Read-only access is enforced.
 *
 * @param path Path to the mmCIF file.
 * @param mode Open mode (must not include NC_WRITE).
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_MMCIF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    int retval;

    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;

    if (!path)
        return NC_EINVAL;

    /* Only read-only access is supported. */
    if (mode & NC_WRITE)
        return NC_EPERM;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Allocate the minimal file-specific state. */
    if (!(mmcif_file = calloc(1, sizeof(NC_MMCIF_FILE_INFO_T))))
    {
        nc4_file_list_del(ncid);
        return NC_ENOMEM;
    }

    if (!(mmcif_file->path = strdup(path)))
    {
        free(mmcif_file);
        nc4_file_list_del(ncid);
        return NC_ENOMEM;
    }

    h5->format_file_info = mmcif_file;

    return NC_NOERR;
}

/**
 * @internal Close a PDBx/mmCIF file.
 *
 * Frees the file-specific state allocated in NC_MMCIF_open().
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_MMCIF_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_MMCIF_FILE_INFO_T *mmcif_file;
    int retval;

    (void)ignore;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get mmCIF-specific info. */
    mmcif_file = (NC_MMCIF_FILE_INFO_T *)h5->format_file_info;
    if (mmcif_file)
    {
        free(mmcif_file->path);
        free(mmcif_file);
        h5->format_file_info = NULL;
    }

    return NC_NOERR;
}

/**
 * @internal Abort opening a PDBx/mmCIF file.
 *
 * V3.4.0 Sprint 1: no resources need to be cleaned up beyond close.
 *
 * @param ncid NetCDF ID.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_abort(int ncid)
{
    (void)ncid;
    return NC_NOERR;
}

/**
 * @internal Inquire the format of a PDBx/mmCIF file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a PDBx/mmCIF file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_MMCIF;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab of data from a PDBx/mmCIF variable.
 *
 * V3.4.0 Sprint 1: no variables are defined yet, so this is a no-op
 * that always succeeds.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices (ignored).
 * @param count Counts (ignored).
 * @param value Output buffer (ignored).
 * @param memtype Requested memory type (ignored).
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_MMCIF_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
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

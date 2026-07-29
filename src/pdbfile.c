/**
 * @file pdbfile.c
 * @brief Legacy PDB User-Defined Format (UDF) dispatch layer.
 *
 * V3.3.0 Sprint 1: no-op infrastructure only. NC_PDB_open() creates the
 * internal netCDF-4 file/group skeleton and stores the file path, but does
 * not read or parse any PDB records. Real fixed-column record parsing is
 * added in Sprint 2 (see .devin/skills/pdb-legacy/SKILL.md and
 * docs/plan/v3.3.0-sprint1-pdb-infrastructure.md).
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nep_nc4.h"
#include "pdbdispatch.h"

/**
 * @internal Open a legacy PDB file for read-only access.
 *
 * Sprint 1 is a no-op: it validates the mode, builds the internal
 * netCDF-4 file/group skeleton, and stores the file path. No PDB record
 * parsing happens yet.
 *
 * @param path Path to the PDB file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters or mode flags.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_PDB_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
            void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_PDB_FILE_INFO_T *pdb_file;
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

    /* Allocate PDB-specific file info. Sprint 1 stores only the path;
     * no record parsing occurs yet. */
    if (!(pdb_file = calloc(1, sizeof(NC_PDB_FILE_INFO_T))))
        return NC_ENOMEM;

    if (!(pdb_file->path = strdup(path)))
    {
        free(pdb_file);
        return NC_ENOMEM;
    }

    h5->format_file_info = pdb_file;

    return NC_NOERR;
}

/**
 * @internal Close a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_PDB_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDB_FILE_INFO_T *pdb_file;
    int retval;

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get PDB-specific info. */
    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    if (!pdb_file)
        return NC_NOERR;

    free(pdb_file->path);
    free(pdb_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Abort opening a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_abort(int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_PDB_FILE_INFO_T *pdb_file;
    int retval;

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get PDB-specific info. */
    pdb_file = (NC_PDB_FILE_INFO_T *)h5->format_file_info;
    if (!pdb_file)
        return NC_NOERR;

    free(pdb_file->path);
    free(pdb_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Inquire the format of a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a legacy PDB file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_PDB;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

/**
 * @internal Read a slab of data from a legacy PDB variable.
 *
 * Sprint 1 is a no-op: no variables exist yet, so this is never called
 * in practice, but it must be present in the dispatch table.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_PDB_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
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

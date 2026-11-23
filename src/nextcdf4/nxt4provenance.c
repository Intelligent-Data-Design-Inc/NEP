/**
 * @file nxt4provenance.c
 * @brief Upstream-compatible provenance attributes for NEXTCDF-4.
 *
 * Writes the same hidden root-group attributes upstream netcdf-c
 * produces (`_NCProperties`, `_nc3_strict`) and answers name lookups
 * for the virtual attributes (`_IsNetcdf4`, `_SuperblockVersion`,
 * `_NCProperties`) that upstream synthesizes in
 * `nc4_get_att_special()` rather than storing. The stored attributes
 * are deferred to `_enddef`/`nc_sync`/`nc_close`, never overwrite an
 * existing stamp, and are only written for files NEXTCDF-4 created —
 * a foreign file opened for write keeps whatever provenance it had.
 * See @ref nextcdf4_architecture.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "nxt4internal.h"
#include "nep_meta.h"

/** @brief Upstream's provenance attribute names (nc_provenance.h). */
#define NX4_NCPROPS "_NCProperties"
#define NX4_NC3_STRICT "_nc3_strict"
#define NX4_ISNETCDF4 "_IsNetcdf4"
#define NX4_SUPERBLOCK "_SuperblockVersion"

/** @brief Hidden attribute names that mark an object as NetCDF-4
 * bookkeeping; two or more anywhere in the file mean "written by
 * netcdf-4", matching upstream's `NC4_isnetcdf4` walk. */
static const char *isnetcdf4_markers[] = {
    "CLASS",
    "NAME",
    "REFERENCE_LIST",
    "DIMENSION_LIST",
    "_Netcdf4Coordinates",
    "_Netcdf4Dimid",
    NULL
};

/**
 * @brief Build the `_NCProperties` value for a new file.
 *
 * Produces `version=2,netcdf=<netcdf-c version>,hdf5=<hdf5 version>,
 * nextcdf4=<nep version>` — upstream's property string plus a
 * `nextcdf4` key, which the version-2 format tolerates. The netcdf-c
 * version is the first token of `nc_inq_libvers()`; the HDF5 version
 * comes from `H5get_libversion`; the NEP version is the build-time
 * `NEP_VERSION`.
 *
 * @param out Destination for a `malloc`'d string the caller frees.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_build_ncproperties(char **out)
{
    const char *vers;
    char ncvers[64];
    unsigned major, minor, release;
    size_t n;

    if (!out)
        return NC_EINVAL;
    *out = NULL;
    vers = nc_inq_libvers();
    if (!vers)
        return NC_EINTERNAL;
    /* nc_inq_libvers() returns e.g. "4.10.2-development of Nov 23
     * 2026 ..." — keep only the version token. */
    n = strcspn(vers, " ");
    if (n >= sizeof(ncvers))
        n = sizeof(ncvers) - 1;
    memcpy(ncvers, vers, n);
    ncvers[n] = '\0';
    if (H5get_libversion(&major, &minor, &release) < 0)
        return NC_EHDFERR;
    {
        int len = snprintf(NULL, 0, "version=2,netcdf=%s,hdf5=%u.%u.%u,nextcdf4=%s",
                           ncvers, major, minor, release, NEP_VERSION);
        if (len <= 0 || !(*out = malloc((size_t)len + 1)))
            return NC_ENOMEM;
        snprintf(*out, (size_t)len + 1,
                 "version=2,netcdf=%s,hdf5=%u.%u.%u,nextcdf4=%s",
                 ncvers, major, minor, release, NEP_VERSION);
    }
    return NC_NOERR;
}

/**
 * @brief Read an existing `_NCProperties` attribute at open.
 *
 * Populates `file->ncproperties` when the root group carries the
 * attribute. A missing or malformed attribute is tolerated — the
 * field stays NULL, matching upstream's "invalid attribute: ignored"
 * leniency.
 *
 * @param file Open NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or `NC_ENOMEM`.
 */
int
nx4_read_ncproperties(NEXTCDF4_FILE_INFO_T *file)
{
    hid_t attr = -1;
    hid_t type = -1;
    char *value = NULL;
    size_t size;
    int ret = NC_NOERR;

    if (H5Aexists(file->rootid, NX4_NCPROPS) <= 0)
        return NC_NOERR;
    if ((attr = H5Aopen(file->rootid, NX4_NCPROPS, H5P_DEFAULT)) < 0 ||
        (type = H5Aget_type(attr)) < 0 || H5Tget_class(type) != H5T_STRING) {
        ELOG((0, "Invalid _NCProperties attribute: ignored"));
        ret = NC_NOERR;
        goto done;
    }
    size = H5Tget_size(type);
    if (!size || !(value = calloc(size + 1, 1)) ||
        H5Aread(attr, type, value) < 0) {
        ELOG((0, "Could not read _NCProperties attribute: ignored"));
        ret = value ? NC_NOERR : NC_ENOMEM;
        goto done;
    }
    file->ncproperties = value;
    value = NULL;

done:
    free(value);
    if (type >= 0)
        H5Tclose(type);
    if (attr >= 0)
        H5Aclose(attr);
    return ret;
}

/**
 * @brief Write `_NCProperties` and `_nc3_strict` to a file
 * NEXTCDF-4 created.
 *
 * Called from `NEXTCDF4__enddef`, `NEXTCDF4_sync` and `NEXTCDF4_close`
 * on writable files — the same deferred points upstream's
 * `NC4_write_provenance` runs. `_NCProperties` is stored exactly as
 * upstream does: a scalar fixed-length ASCII string of `strlen`
 * bytes with `H5T_STR_NULLTERM` padding. `_nc3_strict` is a scalar
 * `H5T_NATIVE_INT` = 1 written only for `NC_CLASSIC_MODEL` files.
 * Neither attribute is ever overwritten, so an existing stamp
 * survives a write-open update. Files NEXTCDF-4 did not create are
 * never stamped.
 *
 * @param file Open writable NEXTCDF-4 state.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_write_provenance(NEXTCDF4_FILE_INFO_T *file)
{
    hid_t space = -1;
    hid_t type = -1;
    hid_t attr = -1;
    int ret = NC_NOERR;

    if (!file || file->no_write)
        return NC_NOERR;

    if (file->created_by_us && file->ncproperties &&
        H5Aexists(file->rootid, NX4_NCPROPS) == 0) {
        size_t len = strlen(file->ncproperties);
        /* Byte parity with upstream's NC4_write_ncproperties: the
         * stored size is strlen, with NULLTERM padding. */
        if ((space = H5Screate(H5S_SCALAR)) < 0 ||
            (type = H5Tcopy(H5T_C_S1)) < 0 ||
            H5Tset_strpad(type, H5T_STR_NULLTERM) < 0 ||
            H5Tset_cset(type, H5T_CSET_ASCII) < 0 ||
            H5Tset_size(type, len) < 0 ||
            (attr = H5Acreate2(file->rootid, NX4_NCPROPS, type, space,
                               H5P_DEFAULT, H5P_DEFAULT)) < 0 ||
            H5Awrite(attr, type, file->ncproperties) < 0)
            ret = NC_EFILEMETA;
        if (attr >= 0)
            H5Aclose(attr);
        attr = -1;
        if (type >= 0)
            H5Tclose(type);
        type = -1;
        if (space >= 0)
            H5Sclose(space);
        space = -1;
        if (ret)
            return ret;
    }

    if (file->created_by_us && (file->mode & NC_CLASSIC_MODEL) &&
        H5Aexists(file->rootid, NX4_NC3_STRICT) == 0) {
        int one = 1;
        if ((space = H5Screate(H5S_SCALAR)) < 0 ||
            (attr = H5Acreate2(file->rootid, NX4_NC3_STRICT,
                               H5T_NATIVE_INT, space, H5P_DEFAULT,
                               H5P_DEFAULT)) < 0 ||
            H5Awrite(attr, H5T_NATIVE_INT, &one) < 0)
            ret = NC_EFILEMETA;
        if (attr >= 0)
            H5Aclose(attr);
        if (space >= 0)
            H5Sclose(space);
    }
    return ret;
}

/**
 * @brief Count objects carrying NetCDF-4 bookkeeping attributes.
 *
 * `H5Ovisit` callback for nx4_isnetcdf4(): increments the counter
 * when the visited object has any of the hidden marker attributes —
 * upstream's threshold of two distinguishes a netCDF-4 file from a
 * plain HDF5 file that happens to carry one such name.
 */
static herr_t
count_marker_atts(hid_t loc, const char *name, const H5O_info_t *info,
                  void *op_data)
{
    int *count = (int *)op_data;
    hid_t obj = -1;
    size_t i;

    /* The callback receives the anchor id, not the visited object's —
     * open each object by name before checking its attributes. */
    (void)info;
    H5E_BEGIN_TRY {
        obj = H5Oopen(loc, name, H5P_DEFAULT);
    } H5E_END_TRY;
    if (obj < 0)
        return 0;
    for (i = 0; isnetcdf4_markers[i]; i++)
        if (H5Aexists(obj, isnetcdf4_markers[i]) > 0) {
            (*count)++;
            break;
        }
    H5Oclose(obj);
    return *count >= 2 ? 1 : 0;
}

/**
 * @brief Whether the open file was written by a netCDF-4 library.
 *
 * Mirrors upstream's `NC4_isnetcdf4`: 1 when `_nc3_strict` or
 * `_NCProperties` exists on the root group, else a whole-file walk
 * that requires at least two objects carrying NetCDF-4 marker
 * attributes.
 *
 * @param file Open NEXTCDF-4 state.
 * @return 1 for a netCDF-4 file, 0 otherwise.
 */
static int
nx4_isnetcdf4(NEXTCDF4_FILE_INFO_T *file)
{
    int count = 0;

    /* A file created in this session is netCDF-4 even before its
     * deferred _NCProperties stamp reaches storage. */
    if (file->created_by_us)
        return 1;
    if (H5Aexists(file->rootid, NX4_NC3_STRICT) > 0 ||
        H5Aexists(file->rootid, NX4_NCPROPS) > 0)
        return 1;
    H5E_BEGIN_TRY {
        H5Ovisit(file->hdfid, H5_INDEX_NAME, H5_ITER_NATIVE,
                 count_marker_atts, &count, H5O_INFO_ALL);
    } H5E_END_TRY;
    return count >= 2;
}

/**
 * @brief Answer a name lookup for a provenance attribute.
 *
 * NEXTCDF-4's `nc4_get_att_special` equivalent: `_NCProperties` is
 * answered from the in-memory copy (built at create or read at
 * open), while `_IsNetcdf4` and `_SuperblockVersion` are synthesized
 * — they exist only as answers to name lookups, never as stored
 * attributes and never in `nc_inq_natts`/`nc_inq_attname` results.
 *
 * @param file Open NEXTCDF-4 state.
 * @param name Attribute name.
 * @param xtypep Optional destination for the file type.
 * @param mem_type Requested in-memory type (`NC_NAT` allowed).
 * @param lenp Optional destination for the attribute length.
 * @param attnump Non-NULL asks for the attribute's index, which
 * virtual attributes do not have.
 * @param data Optional destination for the attribute's value.
 * @return `NC_NOERR` on success, `NC_EATTMETA` for an attnum inquiry,
 * `NC_ENOTATT` when the attribute does not exist, `NC_ECHAR` for a
 * non-character read of `_NCProperties`, `NC_ERANGE` for a
 * non-integer memtype of the integer attributes, or a NetCDF error
 * code.
 */
int
nx4_get_att_special(NEXTCDF4_FILE_INFO_T *file, const char *name,
                    nc_type *xtypep, nc_type mem_type, size_t *lenp,
                    int *attnump, void *data)
{
    if (attnump)
        return NC_EATTMETA;

    if (!strcmp(name, NX4_NCPROPS)) {
        size_t len;
        if (!file->ncproperties)
            return NC_ENOTATT;
        if (mem_type == NC_NAT)
            mem_type = NC_CHAR;
        if (mem_type != NC_CHAR)
            return NC_ECHAR;
        if (xtypep)
            *xtypep = NC_CHAR;
        len = strlen(file->ncproperties);
        if (lenp)
            *lenp = len;
        if (data)
            memcpy(data, file->ncproperties, len + 1);
        return NC_NOERR;
    }

    if (!strcmp(name, NX4_ISNETCDF4) || !strcmp(name, NX4_SUPERBLOCK)) {
        unsigned long long iv = 0;
        if (!strcmp(name, NX4_SUPERBLOCK)) {
            hid_t plist;
            unsigned super;
            if ((plist = H5Fget_create_plist(file->hdfid)) < 0)
                return NC_EHDFERR;
            if (H5Pget_version(plist, &super, NULL, NULL, NULL) < 0) {
                H5Pclose(plist);
                return NC_EHDFERR;
            }
            H5Pclose(plist);
            iv = (unsigned long long)super;
        } else {
            iv = (unsigned long long)nx4_isnetcdf4(file);
        }
        if (xtypep)
            *xtypep = NC_INT;
        if (lenp)
            *lenp = 1;
        if (mem_type == NC_NAT)
            mem_type = NC_INT;
        if (data)
            switch (mem_type) {
            case NC_BYTE: *((char *)data) = (char)iv; break;
            case NC_SHORT: *((short *)data) = (short)iv; break;
            case NC_INT: *((int *)data) = (int)iv; break;
            case NC_UBYTE: *((unsigned char *)data) = (unsigned char)iv; break;
            case NC_USHORT: *((unsigned short *)data) = (unsigned short)iv; break;
            case NC_UINT: *((unsigned int *)data) = (unsigned int)iv; break;
            case NC_INT64: *((long long *)data) = (long long)iv; break;
            case NC_UINT64: *((unsigned long long *)data) = iv; break;
            default:
                return NC_ERANGE;
            }
        return NC_NOERR;
    }
    return NC_ENOTATT;
}

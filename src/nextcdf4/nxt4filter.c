/**
 * @file nxt4filter.c
 * @brief HDF5 filter pipeline support for the NEXTCDF-4 backend.
 *
 * NEXTCDF-4 stores each variable's filter pipeline in the shared
 * `NC_VAR_INFO_T::filters` list — an `NClist` of `struct
 * NC_HDF5_Filter` — and manipulates it through netcdf-c's exported
 * `NC4_hdf5_*` helpers, exactly as the built-in HDF5 backend does in
 * `libhdf5/hdf5filter.c`. The list order is the pipeline order:
 * fletcher32 is always first, shuffle first-or-second, and every
 * other filter appears in definition order.
 *
 * @author Edward Hartnett
 * @date 2026-11-20
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <hdf5_hl.h>
#include "nclist.h"
#include "nxt4internal.h"
#include "netcdf_filter.h"

/**
 * @brief Define an HDF5 filter on a variable — the
 * `nc_def_var_filter()` dispatch entry.
 *
 * Follows upstream `NC4_hdf5_def_var_filter()`: the filter must be
 * available to HDF5, the variable must be non-scalar and not yet
 * committed to disk (`nx4_find_var_for_write` rejects that with
 * `NC_ELATEDEF`), and deflate/szip carry the usual parameter and
 * mutual-exclusion rules. A re-defined filter id replaces its
 * parameters in place. Defining any filter forces chunked storage and
 * marks the variable `dcpl_dirty`, since HDF5 creation properties
 * are immutable — `_enddef` rebuilds the dataset once.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param id HDF5 filter identifier.
 * @param nparams Number of filter parameters.
 * @param params Filter parameter values.
 * @return `NC_NOERR` on success, `NC_ENOFILTER` when the filter is
 * unavailable, `NC_ELATEDEF` on a committed variable, or a NetCDF
 * error code.
 */
int
NEXTCDF4_def_var_filter(int ncid, int varid, unsigned int id,
                        size_t nparams, const unsigned int *params)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    struct NC_HDF5_Filter *oldspec = NULL;
    htri_t avail;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_EINVAL;
    if (!var->ndims)
        return NC_EINVAL;
    if (nparams > 0 && !params)
        return NC_EINVAL;

    if ((avail = H5Zfilter_avail(id)) < 0)
        return NC_EHDFERR;
    if (!avail)
        return NC_ENOFILTER;

    switch ((ret = NC4_hdf5_filter_lookup(var, id, &oldspec))) {
    case NC_NOERR:
    case NC_ENOFILTER:
        break;
    default:
        return ret;
    }

    /* Conflict and parameter checks apply only when the filter is
     * newly defined; re-defining just replaces the parameters. */
    if (!oldspec) {
        if (id == H5Z_FILTER_DEFLATE) {
            if (nparams != 1)
                return NC_EFILTER;
            if (params[0] > NC_MAX_DEFLATE_LEVEL)
                return NC_EINVAL;
            if (!NC4_hdf5_filter_lookup(var, H5Z_FILTER_SZIP, NULL))
                return NC_EINVAL;
        }
        if (id == H5Z_FILTER_SZIP) {
            if (nparams != 2)
                return NC_EFILTER;
            /* Pixels per block must be an even number, <= 32. */
            if (params[1] % 2 || params[1] > NC_MAX_PIXELS_PER_BLOCK)
                return NC_EINVAL;
            if (!NC4_hdf5_filter_lookup(var, H5Z_FILTER_DEFLATE, NULL))
                return NC_EINVAL;
        }
        var->storage = NC_CHUNKED;
    }

    /* For szip, the pixels_per_block parameter must not be greater
     * than the number of elements in the variable. */
    if (id == H5Z_FILTER_SZIP) {
        size_t num_elem = 1;
        int d;
        for (d = 0; d < (int)var->ndims; d++)
            if (var->dim[d]->len)
                num_elem *= var->dim[d]->len;
        if (params[1] > num_elem)
            return NC_EINVAL;
    }

    if ((ret = NC4_hdf5_addfilter(var, id, nparams, params, 0)))
        return ret;

    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
}

/**
 * @brief Set deflate compression — the `nc_def_var_deflate()`
 * dispatch entry.
 *
 * Upstream `NC4_def_var_deflate` semantics: shuffle is set first,
 * then deflate is added only when `deflate` is nonzero and
 * `deflate_level` is 1-9 — level 0 means "no compression" and adds
 * no filter. Once defined, a filter cannot be removed; re-calling
 * only replaces parameters.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param shuffle Nonzero to enable the shuffle filter.
 * @param deflate Nonzero to enable deflate compression.
 * @param deflate_level Deflate level (0-9).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_deflate(int ncid, int varid, int shuffle, int deflate,
                         int deflate_level)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    unsigned int level = (unsigned int)deflate_level;
    int changed = 0;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_EINVAL;
    if (!var->ndims)
        return NC_EINVAL;
    if (deflate_level < NC_MIN_DEFLATE_LEVEL ||
        deflate_level > NC_MAX_DEFLATE_LEVEL)
        return NC_EINVAL;

    if (shuffle) {
        if ((ret = NC4_hdf5_addfilter(var, H5Z_FILTER_SHUFFLE, 0, NULL, 0)))
            return ret;
        changed = 1;
    }
    if (deflate && deflate_level) {
        if ((ret = NC4_hdf5_addfilter(var, H5Z_FILTER_DEFLATE, 1, &level, 0)))
            return ret;
        changed = 1;
    }
    if (!changed)
        return NC_NOERR;

    var->storage = NC_CHUNKED;
    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
}

/**
 * @brief Enable the Fletcher32 checksum — the
 * `nc_def_var_fletcher32()` dispatch entry.
 *
 * `NC_FLETCHER32` adds the checksum filter (always first in the
 * pipeline); `NC_NOCHECKSUM` is a no-op, matching upstream — filters
 * cannot be removed once defined.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param fletcher32 `NC_FLETCHER32` to enable, `NC_NOCHECKSUM` ignored.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_def_var_fletcher32(int ncid, int varid, int fletcher32)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int ret;

    if ((ret = nx4_find_var_for_write(ncid, varid, &h5, &file, &grp, &var)))
        return ret;
    if (file->mode & NC_CLASSIC_MODEL)
        return NC_EINVAL;
    if (!var->ndims)
        return NC_EINVAL;
    if (!fletcher32)
        return NC_NOERR;

    if ((ret = NC4_hdf5_addfilter(var, H5Z_FILTER_FLETCHER32, 0, NULL, 0)))
        return ret;
    var->storage = NC_CHUNKED;
    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->dcpl_dirty = 1;
    return NC_NOERR;
}

/**
 * @brief Inquire the filter ids applied to a variable — the
 * `nc_inq_var_filter_ids()` dispatch entry.
 *
 * Reports the pipeline in order. `NC_NOERR` with a count of zero is
 * returned for a filterless variable, matching upstream.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param nfiltersp Destination for the number of filters.
 * @param ids Destination for the filter ids, or NULL for count only.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
NEXTCDF4_inq_var_filter_ids(int ncid, int varid, size_t *nfiltersp,
                            unsigned int *ids)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NClist *flist;
    size_t n;
    int ret;

    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return ret;

    flist = (NClist *)var->filters;
    n = (size_t)nclistlength(flist);
    if (ids)
        for (size_t k = 0; k < n; k++)
            ids[k] = ((struct NC_HDF5_Filter *)nclistget(flist, k))->filterid;
    if (nfiltersp)
        *nfiltersp = n;
    return NC_NOERR;
}

/**
 * @brief Inquire the parameters of one variable filter — the
 * `nc_inq_var_filter_info()` dispatch entry.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param id Filter identifier to inquire about.
 * @param nparamsp Destination for the number of filter parameters.
 * @param params Destination for the filter parameter values.
 * @return `NC_NOERR` on success, `NC_ENOFILTER` when the filter is
 * not applied, or a NetCDF error code.
 */
int
NEXTCDF4_inq_var_filter_info(int ncid, int varid, unsigned int id,
                             size_t *nparamsp, unsigned int *params)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    struct NC_HDF5_Filter *spec = NULL;
    int ret;

    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return ret;
    if ((ret = NC4_hdf5_filter_lookup(var, id, &spec)))
        return ret;

    if (nparamsp)
        *nparamsp = spec->nparams;
    if (params && spec->nparams > 0)
        memcpy(params, spec->params, spec->nparams * sizeof(unsigned int));
    return NC_NOERR;
}

/**
 * @brief Test whether an HDF5 filter is available — the
 * `nc_inq_filter_avail()` dispatch entry.
 *
 * @param ncid NetCDF file identifier (ignored; availability is
 * process-wide).
 * @param id HDF5 filter identifier.
 * @return `NC_NOERR` when available, `NC_ENOFILTER` when not, or a
 * NetCDF error code.
 */
int
NEXTCDF4_inq_filter_avail(int ncid, unsigned int id)
{
    htri_t avail;

    (void)ncid;
    if ((avail = H5Zfilter_avail(id)) < 0)
        return NC_EHDFERR;
    if (!avail)
        return NC_ENOFILTER;
    return NC_NOERR;
}

/**
 * @brief Apply a variable's filter list to a dataset creation
 * property list.
 *
 * Iterates `var->filters` in pipeline order: fletcher32, shuffle,
 * deflate and szip get their dedicated `H5Pset_*` calls; every other
 * filter is attached with `H5Pset_filter()` and `H5Z_FLAG_OPTIONAL`
 * so HDF5 skips it silently on types it cannot process (matching
 * upstream).
 *
 * @param dcpl Dataset creation property list identifier.
 * @param var Variable.
 * @return `NC_NOERR` on success, `NC_EFILTER` on bad parameters, or
 * `NC_EHDFERR` on an HDF5 failure.
 */
int
nx4_apply_filters(hid_t dcpl, NC_VAR_INFO_T *var)
{
    NClist *flist = (NClist *)var->filters;
    size_t j;

    for (j = 0; j < (size_t)nclistlength(flist); j++) {
        struct NC_HDF5_Filter *fi = (struct NC_HDF5_Filter *)nclistget(flist, j);
        if (fi->filterid == H5Z_FILTER_FLETCHER32) {
            if (H5Pset_fletcher32(dcpl) < 0)
                return NC_EHDFERR;
        } else if (fi->filterid == H5Z_FILTER_SHUFFLE) {
            if (H5Pset_shuffle(dcpl) < 0)
                return NC_EHDFERR;
        } else if (fi->filterid == H5Z_FILTER_DEFLATE) {
            if (fi->nparams != 1)
                return NC_EFILTER;
            if (H5Pset_deflate(dcpl, fi->params[0]) < 0)
                return NC_EFILTER;
        } else if (fi->filterid == H5Z_FILTER_SZIP) {
            if (fi->nparams != 2)
                return NC_EFILTER;
            if (H5Pset_szip(dcpl, fi->params[0], fi->params[1]) < 0)
                return NC_EFILTER;
        } else {
            if (H5Pset_filter(dcpl, fi->filterid, H5Z_FLAG_OPTIONAL,
                              fi->nparams, fi->params) < 0)
                return NC_EFILTER;
        }
    }
    return NC_NOERR;
}

/**
 * @brief Read a variable's filter pipeline from a dataset creation
 * property list.
 *
 * Port of upstream `get_filter_info()` (`libhdf5/hdf5open.c`): every
 * filter in the DCPL is recorded on `var->filters` in order, with its
 * stored parameters — even when the filter plugin is unavailable, so
 * metadata stays readable. An unavailable filter marks the spec and
 * the variable `NC_HDF5_VAR_FILTER_MISSING` (data reads then fail
 * with `NC_ENOFILTER`); a mandatory filter on a variable-sized type
 * marks the variable the same way. Deflate's parameters are
 * validated, and szip's HDF5-expanded 4-parameter form is folded
 * back to the 2 netcdf-c reports.
 *
 * @param dcpl Dataset creation property list identifier.
 * @param var Variable.
 * @param vinfo NEXTCDF-4 variable state (flags destination).
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int
nx4_load_filters(hid_t dcpl, NC_VAR_INFO_T *var,
                 NEXTCDF4_VAR_INFO_T *vinfo)
{
    unsigned int *cd_values = NULL;
    int nfilter, varsized, ret = NC_NOERR;
    int j;

    if ((nfilter = H5Pget_nfilters(dcpl)) < 0)
        return NC_EHDFERR;
    varsized = NC4_var_varsized(var);

    for (j = 0; j < nfilter; j++) {
        unsigned int flags = 0;
        size_t cd_nelems = 0;
        H5Z_filter_t fid;
        htri_t avail;

        if ((fid = H5Pget_filter2(dcpl, (unsigned)j, &flags, &cd_nelems,
                                  NULL, 0, NULL, NULL)) < 0) {
            ret = NC_ENOFILTER;
            goto done;
        }
        if ((avail = H5Zfilter_avail(fid)) < 0) {
            ret = NC_EHDFERR;
            goto done;
        }
        if (!avail) {
            flags |= NC_HDF5_FILTER_MISSING;
            vinfo->flags |= NC_HDF5_VAR_FILTER_MISSING;
        }
        /* A mandatory filter on a variable-sized type is unreadable. */
        if (varsized && (flags & H5Z_FLAG_MANDATORY))
            vinfo->flags |= NC_HDF5_VAR_FILTER_MISSING;

        if (!(cd_values = calloc(cd_nelems ? cd_nelems : 1,
                                 sizeof(unsigned int)))) {
            ret = NC_ENOMEM;
            goto done;
        }
        if (H5Pget_filter2(dcpl, (unsigned)j, NULL, &cd_nelems,
                           cd_values, 0, NULL, NULL) < 0) {
            ret = NC_EHDFERR;
            goto done;
        }

        switch (fid) {
        case H5Z_FILTER_DEFLATE:
            if (cd_nelems != CD_NELEMS_ZLIB ||
                cd_values[0] > NC_MAX_DEFLATE_LEVEL) {
                ret = NC_EHDFERR;
                goto done;
            }
            break;
        case H5Z_FILTER_SZIP:
            /* HDF5 expands szip's 2 stored parameters to 4 and
             * widens the options mask; fold back to the 2 values
             * netcdf-c reports. */
            if (cd_nelems == 0)
                break;
            if (cd_nelems != 4) {
                ret = NC_EHDFERR;
                goto done;
            }
            cd_nelems = 2;
            cd_values[0] &= H5_SZIP_ALL_MASKS;
            break;
        default:
            break;
        }

        if ((ret = NC4_hdf5_addfilter(var, (unsigned int)fid, cd_nelems,
                                      cd_values, (int)flags)))
            goto done;
        free(cd_values);
        cd_values = NULL;
    }

done:
    free(cd_values);
    return ret;
}

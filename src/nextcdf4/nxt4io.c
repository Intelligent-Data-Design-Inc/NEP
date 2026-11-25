/**
 * @file nxt4io.c
 * @brief Hyperslab variable I/O and memtype checks for fixed-size NEXTCDF-4 atomic types.

 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "nxt4internal.h"
#include "nclist.h"

/**
 * @brief Map a netCDF atomic type to the HDF5 datatype describing the
 * caller's memory buffer.
 *
 * `H5Dread`/`H5Dwrite` take a "memory datatype" describing the layout
 * of the buffer in RAM — this is what lets HDF5 convert on the fly
 * (for example `nc_get_vara_float` on an `NC_SHORT` variable). Most
 * netCDF types map to a predefined `H5T_NATIVE_*` constant that must
 * not be closed; types with no native constant (`NC_CHAR`, small
 * floats, strings, references) are produced by `H5Tcopy` of a base
 * type, and the caller owns the copy — @p closep says which.
 *
 * `NC_BFLOAT16` copies little-endian bfloat16 and then adopts the
 * machine's uint16 byte order, because HDF5 2.1.1 has no native-order
 * bfloat16 constant. `NC_CHAR` is a fixed-length 1-byte string;
 * `NC_STRING` is a variable-length string.
 *
 * @param type NetCDF atomic type (the `nc_type` of the memory buffer).
 * @param closep Set nonzero when the returned type must be closed
 * with `H5Tclose()`; may be NULL.
 * @return HDF5 memory datatype, or negative on error.
 */
hid_t
NEXTCDF4_memory_type(nc_type type, int *closep)
{
    if (closep)
        *closep = 0;
    switch (type) {
    case NC_BYTE: return H5T_NATIVE_SCHAR;
    case NC_UBYTE: return H5T_NATIVE_UCHAR;
    case NC_CHAR:
        if (closep)
            *closep = 1;
        {
            hid_t t = H5Tcopy(H5T_C_S1);
            if (t >= 0) {
                H5Tset_size(t, 1);
                H5Tset_strpad(t, H5T_STR_NULLPAD);
            }
            return t;
        }
    case NC_SHORT: return H5T_NATIVE_SHORT;
    case NC_USHORT: return H5T_NATIVE_USHORT;
    case NC_INT: return H5T_NATIVE_INT;
    case NC_UINT: return H5T_NATIVE_UINT;
    case NC_INT64: return H5T_NATIVE_LLONG;
    case NC_UINT64: return H5T_NATIVE_ULLONG;
    case NC_FLOAT: return H5T_NATIVE_FLOAT;
    case NC_DOUBLE: return H5T_NATIVE_DOUBLE;
    case NC_FLOAT16: return H5T_NATIVE_FLOAT16;
    case NC_BFLOAT16:
        if (closep)
            *closep = 1;
        {
            hid_t t = H5Tcopy(H5T_FLOAT_BFLOAT16LE);
            if (t >= 0 && H5Tset_order(t, H5Tget_order(H5T_NATIVE_USHORT)) < 0) {
                H5Tclose(t);
                return -1;
            }
            return t;
        }
    case NC_FLOAT8_E4M3:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_FLOAT_F8E4M3);
    case NC_FLOAT8_E5M2:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_FLOAT_F8E5M2);
    case NC_FLOAT6_E2M3:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_FLOAT_F6E2M3);
    case NC_FLOAT6_E3M2:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_FLOAT_F6E3M2);
    case NC_FLOAT4_E2M1:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_FLOAT_F4E2M1);
    case NC_COMPLEX: return H5T_NATIVE_FLOAT_COMPLEX;
    case NC_DOUBLECOMPLEX: return H5T_NATIVE_DOUBLE_COMPLEX;
    case NC_BITFIELD8: return H5T_NATIVE_B8;
    case NC_BITFIELD16: return H5T_NATIVE_B16;
    case NC_BITFIELD32: return H5T_NATIVE_B32;
    case NC_BITFIELD64: return H5T_NATIVE_B64;
    case NC_STD_REF:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_STD_REF);
    case NC_STRING:
        if (closep)
            *closep = 1;
        {
            hid_t t = H5Tcopy(H5T_C_S1);
            if (t >= 0) {
                if (H5Tset_size(t, H5T_VARIABLE) < 0 ||
                    H5Tset_strpad(t, H5T_STR_NULLTERM) < 0 ||
                    H5Tset_cset(t, H5T_CSET_UTF8) < 0) {
                    H5Tclose(t);
                    return -1;
                }
            }
            return t;
        }
    default: return -1;
    }
}

/**
 * @brief Return non-zero if @p xtype is a NEXTCDF-4 small
 * floating-point type.
 *
 * Small floats are `NC_FLOAT16`, `NC_BFLOAT16`, and the HDF5 2.x
 * sub-16-bit types `NC_FLOAT8_E4M3`/`E5M2`, `NC_FLOAT6_E2M3`/`E3M2`
 * and `NC_FLOAT4_E2M1`. They get special memtype rules in
 * check_memtype().
 *
 * @param xtype The netCDF type.
 * @return 1 for a small floating-point type, 0 otherwise.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
is_small_float(nc_type xtype)
{
    switch (xtype) {
    case NC_FLOAT16: case NC_BFLOAT16:
    case NC_FLOAT8_E4M3: case NC_FLOAT8_E5M2:
    case NC_FLOAT6_E2M3: case NC_FLOAT6_E3M2: case NC_FLOAT4_E2M1:
        return 1;
    default:
        return 0;
    }
}

/**
 * @brief Decide whether a typed I/O call is allowed on a variable.
 *
 * netCDF lets users read/write a variable with a buffer of a
 * different (compatible) type — "the memory type" — and the library
 * converts. NEXTCDF-4's rules:
 *
 * - Untyped or same-type calls always pass (raw bit patterns, no
 *   conversion).
 * - Standard types keep netcdf-c behavior: anything goes, the
 *   internal engine converts.
 * - `NC_FLOAT16`/`NC_BFLOAT16` additionally accept `NC_FLOAT` and
 *   `NC_DOUBLE` buffers, converted by the internal engine.
 * - The FP8/FP6/FP4 types accept only their own type, because HDF5
 *   2.1.1 converts them incorrectly (an HDF5 Known Problem).
 * - Text (`NC_CHAR`) calls on small floats return `NC_ECHAR`,
 *   matching netcdf-c's char/non-char convention.
 *
 * @param xtype The variable's type in the file.
 * @param memtype The memory type of the I/O call.
 * @return NC_NOERR The memtype is allowed.
 * @return NC_ECHAR A text call on a small floating-point variable.
 * @return NC_EBADTYPE Any other memtype not allowed for xtype.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
check_memtype(nc_type xtype, nc_type memtype)
{
    /* Raw I/O: the memory type is the file type, so HDF5 does no
     * conversion. */
    if (memtype == xtype || memtype == NC_NAT)
        return NC_NOERR;

    /* Neither side is a small float: leave the existing behavior for
     * the standard types (HDF5 converts) unchanged. */
    if (!is_small_float(xtype) && !is_small_float(memtype))
        return NC_NOERR;

    /* A text call involving a small float; NetCDF-C uses NC_ECHAR for
     * char/non-char mismatches. */
    if (memtype == NC_CHAR)
        return NC_ECHAR;

    /* float16 and bfloat16 widen exactly into float and double, and
     * the internal engine's conversion back is round-to-nearest-even
     * (documented in NEXTCDF4_float16.md). */
    if ((xtype == NC_FLOAT16 || xtype == NC_BFLOAT16) &&
        (memtype == NC_FLOAT || memtype == NC_DOUBLE))
        return NC_NOERR;

    /* Everything else: integer, string or a different small float, and
     * any typed call on FP8/FP6/FP4, which HDF5 2.1.1 converts
     * incorrectly. */
    return NC_EBADTYPE;
}

/**
 * @brief Select a variable's session-only parallel transfer mode.
 *
 * All ranks must select the same mode and follow the same collective call
 * sequence. Filtered variables cannot use independent transfers. This does
 * not change storage or enter define mode, and works on read-only sessions.
 *
 * @param ncid File or group identifier.
 * @param varid Group-local variable identifier.
 * @param par_access NC_COLLECTIVE or NC_INDEPENDENT.
 * @return NC_NOERR on success, NC_ENOPAR for a serial session/build,
 * NC_EINVAL for an invalid mode or independent filtered access, or the
 * identifier lookup error.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
NEXTCDF4_var_par_access(int ncid, int varid, int par_access)
{
#ifdef HAVE_NEXTCDF4_PARALLEL
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    int ret;

    if (par_access != NC_INDEPENDENT && par_access != NC_COLLECTIVE)
        return NC_EINVAL;
    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file))) return ret;
    if (!file->parallel) return NC_ENOPAR;
    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var))) return ret;
    if (par_access == NC_INDEPENDENT && nclistlength((NClist *)var->filters))
        return NC_EINVAL;
    ((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->parallel_access = par_access;
    return NC_NOERR;
#else
    (void)ncid;
    (void)varid;
    (void)par_access;
    return NC_ENOPAR;
#endif
}

/**
 * @brief Common hyperslab I/O behind the `nc_get/put_vara` and
 * `nc_get/put_vars` dispatch entries.
 *
 * A hyperslab is the rectangular (possibly strided) region of a
 * variable an I/O call touches. This function resolves the variable's
 * HDF5 dataset, validates the memtype against check_memtype(),
 * bounds-checks the selection (extending unlimited dimensions on
 * writes past the current extent), performs any type conversion or
 * quantization through the internal engine nx4_convert_type(), and
 * runs `H5Dread`/`H5Dwrite` with a matching memory dataspace.
 *
 * When a conversion overflows (`NC_ERANGE` semantics, issue #381),
 * the I/O still completes — the engine records the range error and
 * the function returns `NC_ERANGE` after the data lands.
 *
 * Parallel sessions use an owned MPI transfer list. Collective empty/fill-only
 * selections still enter HDF5 with none-selected spaces. Unlimited writes
 * negotiate extents across the file communicator; independent growth fails
 * before mutation. The caller supplies matching collective order and valid
 * rank-local arguments. Dynamic/reference transfers remain serial-only.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @param ncid NetCDF file identifier.
 * @param varid Variable identifier.
 * @param startp Hyperslab start indices.
 * @param countp Hyperslab counts.
 * @param stridep Hyperslab strides, or NULL for contiguous.
 * @param data Data buffer.
 * @param memtype NetCDF atomic memory type (`NC_NAT` resolves to the
 * variable's own type for the untyped `nc_get_var`/`nc_put_var`,
 * `var1` and `vars` calls).
 * @param writing Nonzero for a write, zero for a read.
 * @return `NC_NOERR` on success, `NC_ERANGE` when data landed but a
 * conversion overflowed, or another NetCDF error code.
 */
static int
var_io(int ncid, int varid, const size_t *startp, const size_t *countp,
       const ptrdiff_t *stridep, void *data, nc_type memtype, int writing)
{
    NC_FILE_INFO_T *h5;
    NEXTCDF4_FILE_INFO_T *file;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NEXTCDF4_VAR_INFO_T *vinfo;
    hid_t fspace = -1, mspace = -1;
    hid_t mtype = -1;
    int close_mtype = 0;
    hsize_t dims[NC_MAX_VAR_DIMS], newdims[NC_MAX_VAR_DIMS];
    hsize_t start[NC_MAX_VAR_DIMS], count[NC_MAX_VAR_DIMS], stride[NC_MAX_VAR_DIMS];
    hsize_t rcount[NC_MAX_VAR_DIMS];
    hsize_t memory_dims[NC_MAX_VAR_DIMS], origin[NC_MAX_VAR_DIMS] = {0};
    hsize_t total = 1;
    int provide_fill = 0, no_read = 0, collective = 0, parallel = 0, empty = 0;
    hid_t dxpl = H5P_DEFAULT;
    unsigned char dummy = 0;
    int ndims, i, extend = 0, ret = NC_NOERR, range_error = 0;
    void *converted = NULL, *io_data = data;
    nc_type xtype, io_type;
    int converting, dynamic = 0, read_started = 0;
    char **string_write = NULL;
    size_t xtype_size;

    /* Resolve state, validate mode and memtype, bounds-check and
     * select the hyperslab, convert if needed, run the HDF5 I/O, then
     * convert on the way out for reads. All errors funnel to `done`
     * so every HDF5 handle is closed exactly once. */
    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file))) return ret;
    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var))) return ret;
    if (!var || !(vinfo = var->format_var_info)) return NC_ENOTVAR;
    /* A variable opened with a missing mandatory filter plugin keeps
     * its metadata readable but rejects all data I/O. */
    if (vinfo->flags & NC_HDF5_VAR_FILTER_MISSING) return NC_ENOFILTER;
    if (writing && file->no_write) return NC_EPERM;
#ifdef HAVE_NEXTCDF4_PARALLEL
    parallel = file->parallel;
    collective = parallel && vinfo->parallel_access == NC_COLLECTIVE;
    if (parallel && (var->type_info->hdr.id == NC_STRING ||
                     var->type_info->hdr.id == NC_STD_REF ||
                     (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID && var->type_info->varsized)))
        return NC_ENOTBUILT;
    /* HDF5 requires collective access for filtered writes; filtered
     * reads are permitted independently. */
    if (parallel && writing && !collective && nclistlength((NClist *)var->filters))
        return NC_EINVAL;
#endif
    /* Upstream leaves define mode automatically when a write arrives;
     * nc_close does the same below. Reads in define mode keep NEP's
     * NC_EINDEFINE rule. The auto-enddef must run before the dataset
     * check: a user-typed variable's dataset exists only after the
     * deferred creation `_enddef` performs. */
    if (file->define_mode) {
        if (!writing) return NC_EINDEFINE;
        if ((ret = NEXTCDF4__enddef(ncid, 0, 0, 0, 0))) return ret;
    }
    if (vinfo->hdf_dataset < 0) return NC_ENOTVAR;
    /* The untyped nc_put_var/nc_get_var, var1 and vars calls pass
     * NC_NAT, meaning "the variable's own type"; resolve it so
     * NEXTCDF4_memory_type() gets a real type. */
    if (memtype == NC_NAT) memtype = (nc_type)var->type_info->hdr.id;
    /* Reject memtypes not allowed for small-float variables before any
     * HDF5 call, so a rejected call touches neither buffer nor file. */
    if ((memtype == NC_CHAR) != (var->type_info->hdr.id == NC_CHAR)) return NC_ECHAR;
    if (var->type_info->hdr.id >= NC_FIRSTUSERTYPEID) {
        /* User-defined types take matching-type or untyped I/O only;
         * there is no numeric conversion. */
        if (memtype != var->type_info->hdr.id) return NC_EBADTYPE;
    } else if ((ret = check_memtype((nc_type)var->type_info->hdr.id, memtype))) return ret;
    if (memtype == NC_STRING) {
        if ((mtype = H5Dget_type(vinfo->hdf_dataset)) < 0) return NC_EHDFERR;
    } else if ((ret = nx4_memory_type(grp, memtype, &mtype))) return ret;
    close_mtype = 1;
    ndims = (int)var->ndims;
    if ((fspace = H5Dget_space(vinfo->hdf_dataset)) < 0) { ret = NC_EHDFERR; goto done; }
    if (!ndims) goto prepare;
    if (!startp || !countp) { ret = NC_EINVAL; goto done; }
    if (H5Sget_simple_extent_dims(fspace, dims, NULL) < 0) { ret = NC_EHDFERR; goto done; }
    for (i = 0; i < ndims; i++)
        if (!countp[i]) empty = 1;
    for (i = 0; i < ndims; i++) {
        size_t last;
        if (stridep && stridep[i] <= 0) { ret = NC_ESTRIDE; goto done; }
        start[i] = startp[i]; count[i] = countp[i]; stride[i] = stridep ? (hsize_t)stridep[i] : 1;
        newdims[i] = dims[i];
        rcount[i] = countp[i];
        memory_dims[i] = countp[i];
        if (!countp[i]) { total = 0; continue; }
        if (countp[i] - 1 > (SIZE_MAX - startp[i]) / (size_t)stride[i]) { ret = NC_EEDGE; goto done; }
        last = startp[i] + (countp[i] - 1) * (size_t)stride[i];
        if (last == SIZE_MAX) { ret = NC_EEDGE; goto done; }
        if ((size_t)(hsize_t)last != last || (hsize_t)last == H5S_UNLIMITED) {
            ret = NC_EEDGE; goto done;
        }
        if (!empty && last >= dims[i]) {
            if (!var->dim[i]->unlimited) { ret = NC_EEDGE; goto done; }
            if (writing) {
                newdims[i] = (hsize_t)last + 1; extend = 1;
            } else {
                /* Reads legitimately select past the physical extent
                 * on an unlimited dim: clip the hyperslab to what the
                 * dataset actually holds and fill the tail with the
                 * variable's fill value, as upstream does. */
                if (start[i] < dims[i])
                    rcount[i] = (dims[i] - (hsize_t)start[i] +
                                 stride[i] - 1) / stride[i];
                else
                    rcount[i] = 0;
                count[i] = rcount[i];
                provide_fill = 1;
            }
        }
        if (total && countp[i] > SIZE_MAX / total) { ret = NC_EINVAL; goto done; }
        total *= countp[i];
    }
    if (!total && !collective) { ret = NC_NOERR; goto done; }
#ifdef HAVE_NEXTCDF4_PARALLEL
    if (writing && collective) {
        int unlimited = 0;
        unsigned long long extents[NC_MAX_VAR_DIMS];
        for (i = 0; i < ndims; i++) {
            unlimited |= var->dim[i]->unlimited;
            extents[i] = (unsigned long long)newdims[i];
            if ((hsize_t)extents[i] != newdims[i]) { ret = NC_EEDGE; goto done; }
        }
        if (unlimited) {
            if (MPI_Allreduce(MPI_IN_PLACE, extents, ndims, MPI_UNSIGNED_LONG_LONG,
                              MPI_MAX, file->comm) != MPI_SUCCESS) {
                ret = NC_EMPI; goto done;
            }
            extend = 0;
            for (i = 0; i < ndims; i++) {
                newdims[i] = (hsize_t)extents[i];
                if ((unsigned long long)newdims[i] != extents[i] ||
                    (unsigned long long)(size_t)extents[i] != extents[i]) {
                    ret = NC_EEDGE; goto done;
                }
                if (newdims[i] > dims[i]) extend = 1;
            }
        }
    }
#endif
    if (parallel && !collective && extend) { ret = NC_ECANTEXTEND; goto done; }
prepare:
    if (total && !data) { ret = NC_EINVAL; goto done; }
    xtype = (nc_type)var->type_info->hdr.id;
    converting = memtype != xtype || (writing && var->quantize_mode != NC_NOQUANTIZE);
    io_type = converting ? xtype : memtype;
    if ((ret = nx4_type_size(grp, xtype, &xtype_size))) goto done;
    if (total > SIZE_MAX / xtype_size) { ret = NC_EINVAL; goto done; }
    if (converting && total) {
        /* A dry-run call with no buffers lets the engine report
         * NC_EBADTYPE etc. before the (possibly large) scratch buffer
         * is allocated; the real call fills `converted` on writes or
         * after the read below. */
        nc_type source = writing ? memtype : xtype;
        nc_type target = writing ? xtype : memtype;
        int mode = writing ? var->quantize_mode : NC_NOQUANTIZE;
        if ((ret = nx4_convert_type(NULL, NULL, source, target, 0, &range_error,
                                    NULL, 0, mode, var->nsd, 1))) goto done;
        if (!(converted = malloc((size_t)total * xtype_size))) { ret = NC_ENOMEM; goto done; }
        io_data = converted;
        if (writing && (ret = nx4_convert_type(data, converted, source, target, (size_t)total,
                                               &range_error, var->fill_value, 0, mode, var->nsd, 1))) goto done;
        if (close_mtype) { H5Tclose(mtype); close_mtype = 0; }
        if ((ret = nx4_memory_type(grp, io_type, &mtype))) goto done;
        close_mtype = 1;
    }
    if (!converting && xtype >= NC_FIRSTUSERTYPEID) {
        /* A compound variable whose stored datatype is anonymous was
         * written by a foreign library (upstream netCDF stores an
         * anonymous copy on its datasets). Those files use HDF5's
         * serialized-compound layout, which decodes correctly only
         * through the packed native datatype — the same memtype
         * upstream itself uses. Our own datasets keep the committed
         * datatype and the declared-layout memory type. */
        NC_TYPE_INFO_T *ut = NULL;
        hid_t dt;
        if (!nx4_find_type(grp, xtype, &ut) && ut &&
            ut->nc_type_class == NC_COMPOUND &&
            (dt = H5Dget_type(vinfo->hdf_dataset)) >= 0) {
            if (H5Tcommitted(dt) <= 0) {
                if (close_mtype)
                    H5Tclose(mtype);
                if ((mtype = H5Tget_native_type(dt, H5T_DIR_DEFAULT)) < 0) {
                    H5Tclose(dt);
                    ret = NC_EHDFERR;
                    goto done;
                }
                close_mtype = 1;
            }
            H5Tclose(dt);
        }
    }
    dynamic = xtype == NC_STRING ||
              (xtype >= NC_FIRSTUSERTYPEID && var->type_info->varsized);
    if (dynamic) {
        hid_t stored = H5Dget_type(vinfo->hdf_dataset), matched = -1;
        if (stored < 0) { ret = NC_EHDFERR; goto done; }
        ret = nx4_storage_memory_type(mtype, stored, &matched);
        H5Tclose(stored);
        if (ret) goto done;
        H5Tclose(mtype);
        mtype = matched;
    }
    if (!writing && dynamic) {
        memset(io_data, 0, (size_t)total * xtype_size);
        read_started = 1;
    }
    if (writing && xtype == NC_STRING) {
        if (!(string_write = malloc((size_t)total * sizeof(char *)))) {
            ret = NC_ENOMEM; goto done;
        }
        for (size_t k = 0; k < (size_t)total; k++)
            string_write[k] = ((char **)data)[k] ? ((char **)data)[k] : "";
        io_data = string_write;
    }
#ifdef HAVE_NEXTCDF4_PARALLEL
    if (parallel) {
        if ((dxpl = H5Pcreate(H5P_DATASET_XFER)) < 0) { ret = NC_EHDFERR; goto done; }
        if (H5Pset_dxpl_mpio(dxpl, collective ? H5FD_MPIO_COLLECTIVE : H5FD_MPIO_INDEPENDENT) < 0) {
            ret = NC_EPARINIT; goto done;
        }
    }
#endif
    if (extend) {
        if (H5Dset_extent(vinfo->hdf_dataset, newdims) < 0) { ret = NC_EHDFERR; goto done; }
        H5Sclose(fspace);
        if ((fspace = H5Dget_space(vinfo->hdf_dataset)) < 0) { ret = NC_EHDFERR; goto done; }
        for (i = 0; i < ndims; i++) {
            NEXTCDF4_DIM_INFO_T *di;
            if (!var->dim[i]->unlimited || newdims[i] <= var->dim[i]->len)
                continue;
            var->dim[i]->len = (size_t)newdims[i];
            /* Grow the dimension's scale dataset too so the
             * recovered length matches on reopen — unless the scale
             * is the variable's own dataset (coordinate variable,
             * already extended above). */
            di = (NEXTCDF4_DIM_INFO_T *)var->dim[i]->format_dim_info;
            if (di && di->hdf_dataset >= 0 &&
                di->hdf_dataset != vinfo->hdf_dataset) {
                hsize_t dlen = newdims[i];
                if (H5Dset_extent(di->hdf_dataset, &dlen) < 0)
                    { ret = NC_EHDFERR; goto done; }
            }
        }
    }
    if (!writing) {
        /* A zero count on any dimension means nothing to read from
         * disk — the whole region is fill. */
        for (i = 0, no_read = 0; i < ndims; i++)
            if (!rcount[i]) no_read = 1;
    }
    mspace = ndims ? H5Screate_simple(ndims, memory_dims, NULL) : H5Screate(H5S_SCALAR);
    if (mspace < 0) { ret = NC_EHDFERR; goto done; }
    if ((!total || no_read) &&
        (H5Sselect_none(fspace) < 0 || H5Sselect_none(mspace) < 0)) {
        ret = NC_EHDFERR; goto done;
    }
    if (!total) io_data = &dummy;
    if (ndims && total && !no_read &&
        (H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, stride, count, NULL) < 0 ||
         H5Sselect_hyperslab(mspace, H5S_SELECT_SET, origin, NULL, count, NULL) < 0)) {
        ret = NC_EHDFERR; goto done;
    }
    if (writing) {
        ret = H5Dwrite(vinfo->hdf_dataset, mtype, ndims ? mspace : H5S_ALL,
                       fspace, dxpl, io_data) < 0 ? NC_EHDFERR : NC_NOERR;
        if (!ret)
            var->written_to = 1;
    }
    else {
        if (!no_read || collective) {
            ret = H5Dread(vinfo->hdf_dataset, mtype, ndims ? mspace : H5S_ALL,
                          fspace, dxpl, io_data) < 0 ? NC_EHDFERR : NC_NOERR;
            /* A failed read of a variable-sized type (strings, vlens
             * and compounds containing them) is reclaimed at done,
             * including values copied while supplying fill. */
        }
        if (!ret && provide_fill) {
            /* Supply each missing logical element independently;
             * multidimensional clipped selections are not a flat tail.
             * Dynamic values receive independently owned payloads. */
            void *zero_fill = NULL;
            const void *fillvalue = var->fill_value;
            char *empty = "";
            if (!fillvalue && xtype < NC_STRING) {
                if ((ret = nc4_get_fill_value(h5, var, &zero_fill)))
                    goto done;
                fillvalue = zero_fill;
            }
            if (!fillvalue && xtype == NC_STRING) fillvalue = &empty;
            if (!fillvalue &&
                !(fillvalue = zero_fill = calloc(1, xtype_size))) {
                ret = NC_ENOMEM; goto done;
            }
            for (size_t k = 0; k < (size_t)total; k++) {
                size_t index = k;
                int missing = 0;
                for (i = ndims - 1; i >= 0; i--) {
                    if (index % countp[i] >= rcount[i]) missing = 1;
                    index /= countp[i];
                }
                if (!missing) continue;
                void *dst = (char *)io_data + k * xtype_size;
                if (xtype > NC_STRING && xtype < NC_FIRSTUSERTYPEID)
                    memcpy(dst, fillvalue, xtype_size);
                else if ((ret = nx4_copy_data(h5->controller, xtype,
                                             fillvalue, 1, dst))) break;
            }
            free(zero_fill);
        }
        if (!ret && converting && total)
            ret = nx4_convert_type(converted, data, xtype, memtype, (size_t)total,
                                    &range_error, NULL, 0, NC_NOQUANTIZE, 0, 1);
    }
done:
    if (read_started && ret) {
        NC_reclaim_data(h5->controller, xtype, io_data, (size_t)total);
        memset(io_data, 0, (size_t)total * xtype_size);
    }
    if (read_started && !ret && xtype == NC_STRING) {
        for (size_t k = 0; k < (size_t)total; k++)
            if (!((char **)io_data)[k] && !(((char **)io_data)[k] = strdup(""))) {
                NC_reclaim_data(h5->controller, xtype, io_data, (size_t)total);
                memset(io_data, 0, (size_t)total * xtype_size);
                ret = NC_ENOMEM;
                break;
            }
    }
    if (dxpl > 0) H5Pclose(dxpl);
    free(string_write);
    free(converted);
    if (mspace >= 0) H5Sclose(mspace);
    if (fspace >= 0) H5Sclose(fspace);
    if (mtype >= 0 && close_mtype) H5Tclose(mtype);
    return ret ? ret : range_error ? NC_ERANGE : NC_NOERR;
}

/**
 * @brief Read a hyperslab — the `nc_get_vara_*` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param start Hyperslab start indices.
 * @param count Hyperslab counts.
 * @param value Destination buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, NULL, value, memtype, 0); }
/**
 * @brief Write a hyperslab — the `nc_put_vara_*` dispatch entry.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param start Hyperslab start indices.
 * @param count Hyperslab counts.
 * @param value Source buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      const void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, NULL, (void *)value, memtype, 1); }
/**
 * @brief Read a strided hyperslab — the `nc_get_vars_*` dispatch
 * entry.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param start Hyperslab start indices.
 * @param count Hyperslab counts.
 * @param stride Hyperslab strides.
 * @param value Destination buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_get_vars(int ncid, int varid, const size_t *start, const size_t *count,
                      const ptrdiff_t *stride, void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, stride, value, memtype, 0); }
/**
 * @brief Write a strided hyperslab — the `nc_put_vars_*` dispatch
 * entry.
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param start Hyperslab start indices.
 * @param count Hyperslab counts.
 * @param stride Hyperslab strides.
 * @param value Source buffer.
 * @param memtype NetCDF atomic memory type.
 * @return `NC_NOERR` on success, or a NetCDF error code.
 */
int NEXTCDF4_put_vars(int ncid, int varid, const size_t *start, const size_t *count,
                      const ptrdiff_t *stride, const void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, stride, (void *)value, memtype, 1); }
/**
 * @brief Mapped hyperslab read (`nc_get_varm_*`); not implemented.
 *
 * varm calls add a memory-layout mapping to the strided selection.
 * The feature is deprecated upstream and deliberately unimplemented
 * here; every call returns `NC_ENOTBUILT`.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param s Hyperslab start indices.
 * @param c Hyperslab counts.
 * @param st Hyperslab strides.
 * @param im Memory mapping offsets.
 * @param v Destination buffer.
 * @param t NetCDF atomic memory type.
 * @return `NC_ENOTBUILT` always.
 */
int NEXTCDF4_get_varm(int ncid, int varid, const size_t *s, const size_t *c,
                      const ptrdiff_t *st, const ptrdiff_t *im, void *v, nc_type t)
{ (void)ncid;(void)varid;(void)s;(void)c;(void)st;(void)im;(void)v;(void)t; return NC_ENOTBUILT; }
/**
 * @brief Mapped hyperslab write (`nc_put_varm_*`); not implemented.
 *
 * See NEXTCDF4_get_varm(): always returns `NC_ENOTBUILT`.
 *
 * @param ncid NetCDF file identifier.
 * @param varid Variable id.
 * @param s Hyperslab start indices.
 * @param c Hyperslab counts.
 * @param st Hyperslab strides.
 * @param im Memory mapping offsets.
 * @param v Source buffer.
 * @param t NetCDF atomic memory type.
 * @return `NC_ENOTBUILT` always.
 */
int NEXTCDF4_put_varm(int ncid, int varid, const size_t *s, const size_t *c,
                      const ptrdiff_t *st, const ptrdiff_t *im, const void *v, nc_type t)
{ (void)ncid;(void)varid;(void)s;(void)c;(void)st;(void)im;(void)v;(void)t; return NC_ENOTBUILT; }

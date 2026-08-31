/**
 * @file nxt4io.c
 * @brief Hyperslab variable I/O for fixed-size NEXTCDF-4 atomic types.
 *
 * @author Edward Hartnett
 * @date 2026-08-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include <limits.h>
#include <stdlib.h>
#include "nxt4internal.h"

/*
 * Return the native HDF5 memory datatype for a standard atomic type.
 */
static hid_t
memory_type(nc_type type, int *closep)
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
        return H5Tcopy(H5T_FLOAT_BFLOAT16LE);
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
    case NC_REF_OBJECT:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_STD_REF_OBJ);
    case NC_REF_REGION:
        if (closep)
            *closep = 1;
        return H5Tcopy(H5T_STD_REF_DSETREG);
    case NC_STRING:
        if (closep)
            *closep = 1;
        {
            hid_t t = H5Tcopy(H5T_C_S1);
            if (t >= 0) {
                H5Tset_size(t, H5T_VARIABLE);
                H5Tset_strpad(t, H5T_STR_NULLTERM);
            }
            return t;
        }
    default: return -1;
    }
}

/*
 * Common HDF5 hyperslab implementation used by vara and vars dispatch calls.
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
    hsize_t total = 1;
    int ndims, i, extend = 0, ret;

    if ((ret = NEXTCDF4_get_file(ncid, &h5, &file))) return ret;
    if ((ret = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var))) return ret;
    if (!var || !(vinfo = var->format_var_info) || vinfo->hdf_dataset < 0) return NC_ENOTVAR;
    if (writing && file->no_write) return NC_EPERM;
    if (writing && file->define_mode) return NC_EINDEFINE;
    if (!writing && file->define_mode) return NC_EINDEFINE;
    if (!data) return NC_EINVAL;
    if ((mtype = memory_type(memtype, &close_mtype)) < 0) return NC_EBADTYPE;
    ndims = (int)var->ndims;
    if ((fspace = H5Dget_space(vinfo->hdf_dataset)) < 0) return NC_EHDFERR;
    if (ndims == 0) {
        if (writing)
            ret = H5Dwrite(vinfo->hdf_dataset, mtype, H5S_ALL, H5S_ALL,
                           H5P_DEFAULT, data) < 0 ? NC_EHDFERR : NC_NOERR;
        else
            ret = H5Dread(vinfo->hdf_dataset, mtype, H5S_ALL, H5S_ALL,
                          H5P_DEFAULT, data) < 0 ? NC_EHDFERR : NC_NOERR;
        H5Sclose(fspace);
        return ret;
    }
    if (!startp || !countp) { H5Sclose(fspace); return NC_EINVAL; }
    if (H5Sget_simple_extent_dims(fspace, dims, NULL) < 0) { H5Sclose(fspace); return NC_EHDFERR; }
    for (i = 0; i < ndims; i++) {
        size_t last;
        if (stridep && stridep[i] <= 0) { ret = NC_ESTRIDE; goto done; }
        start[i] = startp[i]; count[i] = countp[i]; stride[i] = stridep ? (hsize_t)stridep[i] : 1;
        newdims[i] = dims[i];
        if (!countp[i]) { total = 0; continue; }
        if (countp[i] - 1 > (SIZE_MAX - startp[i]) / (size_t)stride[i]) { ret = NC_EEDGE; goto done; }
        last = startp[i] + (countp[i] - 1) * (size_t)stride[i];
        if (last >= dims[i]) {
            if (!writing || !var->dim[i]->unlimited) { ret = NC_EEDGE; goto done; }
            newdims[i] = (hsize_t)last + 1; extend = 1;
        }
        if (countp[i] > SIZE_MAX / total) { ret = NC_EINVAL; goto done; }
        total *= countp[i];
    }
    if (!total) { ret = NC_NOERR; goto done; }
    if (extend) {
        if (H5Dset_extent(vinfo->hdf_dataset, newdims) < 0) { ret = NC_EHDFERR; goto done; }
        H5Sclose(fspace);
        if ((fspace = H5Dget_space(vinfo->hdf_dataset)) < 0) return NC_EHDFERR;
        for (i = 0; i < ndims; i++)
            if (var->dim[i]->unlimited && newdims[i] > var->dim[i]->len)
                var->dim[i]->len = (size_t)newdims[i];
    }
    if (H5Sselect_hyperslab(fspace, H5S_SELECT_SET, start, stride, count, NULL) < 0 ||
        (mspace = H5Screate_simple(ndims, count, NULL)) < 0) { ret = NC_EHDFERR; goto done; }
    if (writing) {
        void *wdata = data;
        if (var->quantize_mode != NC_NOQUANTIZE) {
            nc_type file_xtype = var->type_info->hdr.id;
            nc_type src_type = (memtype == NC_NAT) ? file_xtype : memtype;
            size_t xtype_size;
            void *conv = NULL;
            int range_error;
            if ((ret = NEXTCDF4_type_size(file_xtype, &xtype_size))) goto done;
            if (!(conv = malloc(total * xtype_size))) { ret = NC_ENOMEM; goto done; }
            if ((ret = nc4_convert_type(data, conv, src_type, file_xtype, total,
                                        &range_error, var->fill_value, 0,
                                        var->quantize_mode, var->nsd))) {
                free(conv);
                goto done;
            }
            if (close_mtype) { H5Tclose(mtype); close_mtype = 0; }
            if ((mtype = memory_type(file_xtype, &close_mtype)) < 0) { free(conv); ret = NC_EBADTYPE; goto done; }
            wdata = conv;
            if (H5Dwrite(vinfo->hdf_dataset, mtype, mspace, fspace, H5P_DEFAULT, wdata) < 0)
                ret = NC_EHDFERR;
            else
                ret = NC_NOERR;
            free(conv);
        } else {
            ret = H5Dwrite(vinfo->hdf_dataset, mtype, mspace, fspace, H5P_DEFAULT, data) < 0 ? NC_EHDFERR : NC_NOERR;
        }
    } else
        ret = H5Dread(vinfo->hdf_dataset, mtype, mspace, fspace, H5P_DEFAULT, data) < 0 ? NC_EHDFERR : NC_NOERR;
done:
    if (mspace >= 0) H5Sclose(mspace);
    if (fspace >= 0) H5Sclose(fspace);
    if (mtype >= 0 && close_mtype) H5Tclose(mtype);
    return ret;
}

int NEXTCDF4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, NULL, value, memtype, 0); }
int NEXTCDF4_put_vara(int ncid, int varid, const size_t *start, const size_t *count,
                      const void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, NULL, (void *)value, memtype, 1); }
int NEXTCDF4_get_vars(int ncid, int varid, const size_t *start, const size_t *count,
                      const ptrdiff_t *stride, void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, stride, value, memtype, 0); }
int NEXTCDF4_put_vars(int ncid, int varid, const size_t *start, const size_t *count,
                      const ptrdiff_t *stride, const void *value, nc_type memtype)
{ return var_io(ncid, varid, start, count, stride, (void *)value, memtype, 1); }
int NEXTCDF4_get_varm(int ncid, int varid, const size_t *s, const size_t *c,
                      const ptrdiff_t *st, const ptrdiff_t *im, void *v, nc_type t)
{ (void)ncid;(void)varid;(void)s;(void)c;(void)st;(void)im;(void)v;(void)t; return NC_ENOTBUILT; }
int NEXTCDF4_put_varm(int ncid, int varid, const size_t *s, const size_t *c,
                      const ptrdiff_t *st, const ptrdiff_t *im, const void *v, nc_type t)
{ (void)ncid;(void)varid;(void)s;(void)c;(void)st;(void)im;(void)v;(void)t; return NC_ENOTBUILT; }

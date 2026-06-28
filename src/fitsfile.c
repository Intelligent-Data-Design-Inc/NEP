/**
 * @file
 * @internal The FITS file functions. These provide a read-only
 * interface to FITS astronomical data files via CFITSIO.
 *
 * Sprint 3: real CFITSIO integration. The open function calls
 * fits_open_file() and stores the CFITSIO file handle for later
 * use in metadata and data reading.
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

/* Include CFITSIO header if available */
#ifdef HAVE_FITS
#include <fitsio.h>
#endif

/**
 * @internal Map CFITSIO status codes to NetCDF error codes.
 *
 * @param status CFITSIO status code.
 *
 * @return NetCDF error code.
 * @author Edward Hartnett
 */
static int
map_fitsio_error(int status)
{
    if (status == 0)
        return NC_NOERR;
    
    /* Common CFITSIO error mappings */
    switch (status)
    {
    case 104: /* FILE_NOT_OPENED */
        return NC_EINVAL;
    case 107: /* END_OF_FILE */
        return NC_EIO;
    case 108: /* READ_ERROR */
        return NC_EIO;
    case 202: /* KEY_NO_EXIST */
        return NC_ENOTFOUND;
    default:
        return NC_EIO;
    }
}

extern int nc4_var_list_add(NC_GRP_INFO_T *grp, const char *name, int ndims,
                            NC_VAR_INFO_T **var);

/**
 * @internal Set the type of a netCDF-4 variable.
 *
 * @param xtype A netcdf type.
 * @param endianness The endianness of the data.
 * @param type_size The size in bytes of one element of this type.
 * @param type_name A name for the type.
 * @param typep Pointer to a pointer that gets the TYPE_INFO_T struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
nc4_set_var_type(nc_type xtype, int endianness, size_t type_size,
                 char *type_name, NC_TYPE_INFO_T **typep)
{
    NC_TYPE_INFO_T *type;

    assert(typep);

    if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
        return NC_ENOMEM;
    if (!(type->hdr.name = strdup(type_name)))
    {
        free(type);
        return NC_ENOMEM;
    }
    type->hdr.sort = NCTYP;

    if (xtype == NC_FLOAT)
        type->nc_type_class = NC_FLOAT;
    else if (xtype == NC_DOUBLE)
        type->nc_type_class = NC_DOUBLE;
    else if (xtype == NC_CHAR)
        type->nc_type_class = NC_STRING;
    else
        type->nc_type_class = NC_INT;

    type->endianness = endianness;
    type->size = type_size;
    type->hdr.id = (size_t)xtype;

    *typep = type;
    return NC_NOERR;
}

/**
 * @internal Create a new variable and insert it into relevant lists.
 *
 * @param grp The containing group.
 * @param name The variable name.
 * @param ndims The rank.
 * @param xtype The netCDF data type.
 * @param endianness Byte order.
 * @param type_size Size in bytes of one element.
 * @param type_name Human-readable type name.
 * @param fill_value Optional fill value (may be NULL).
 * @param contiguous Non-zero for contiguous storage.
 * @param chunksizes Optional chunk sizes (may be NULL).
 * @param format_var_info Format-specific per-variable metadata.
 * @param var Output: pointer to the new variable.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
nc4_var_list_add_full(NC_GRP_INFO_T *grp, const char *name, int ndims,
                      nc_type xtype, int endianness, size_t type_size,
                      char *type_name, void *fill_value, int contiguous,
                      size_t *chunksizes, void *format_var_info,
                      NC_VAR_INFO_T **var)
{
    int d, retval;

    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->format_var_info = format_var_info;
    (*var)->atts_read = 1;

    if ((retval = nc4_set_var_type(xtype, endianness, type_size, type_name,
                                   &(*var)->type_info)))
        return retval;

    (*var)->endianness = (*var)->type_info->endianness;
    (*var)->type_info->rc++;

    if (fill_value)
    {
        if (!((*var)->fill_value = malloc(type_size)))
            return NC_ENOMEM;
        memcpy((*var)->fill_value, fill_value, type_size);
    }

    (*var)->storage = contiguous ? NC_CONTIGUOUS : NC_CHUNKED;

    if (chunksizes)
    {
        if (!((*var)->chunksizes = malloc(ndims * sizeof(size_t))))
            return NC_ENOMEM;
        for (d = 0; d < ndims; d++)
            (*var)->chunksizes[d] = chunksizes[d];
    }

    return NC_NOERR;
}

/**
 * @internal Map FITS BITPIX value to a netCDF type and associated metadata.
 *
 * @param bitpix FITS BITPIX value.
 * @param xtypep Pointer that gets the nc_type. Ignored if NULL.
 * @param type_sizep Pointer that gets the type size in bytes. Ignored if NULL.
 * @param type_name Buffer (at least NC_MAX_NAME+1 bytes) that gets the type
 * name string. Ignored if NULL.
 * @param endiannessp Pointer that gets the endianness. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADTYPE Unknown BITPIX value.
 * @author Edward Hartnett
 */
#ifdef HAVE_FITS
static int
fits_bitpix_to_nc_type(int bitpix, nc_type *xtypep, size_t *type_sizep,
                       char *type_name, int *endiannessp)
{
    nc_type xtype;
    size_t type_size;
    const char *name;
    int endianness = NC_ENDIAN_BIG;

    switch (bitpix)
    {
    case 8:
        xtype = NC_UBYTE; type_size = 1; name = "ubyte";
        endianness = NC_ENDIAN_NATIVE;
        break;
    case 16:
        xtype = NC_SHORT; type_size = 2; name = "short";
        break;
    case 32:
        xtype = NC_INT; type_size = 4; name = "int";
        break;
    case 64:
        xtype = NC_INT64; type_size = 8; name = "int64";
        break;
    case -32:
        xtype = NC_FLOAT; type_size = 4; name = "float";
        break;
    case -64:
        xtype = NC_DOUBLE; type_size = 8; name = "double";
        break;
    default:
        return NC_EBADTYPE;
    }

    if (xtypep) *xtypep = xtype;
    if (type_sizep) *type_sizep = type_size;
    if (type_name) strncpy(type_name, name, NC_MAX_NAME);
    if (endiannessp) *endiannessp = endianness;

    return NC_NOERR;
}

/**
 * @internal Read all header keywords from the current FITS HDU and store
 * them as NC_CHAR global attributes on the given group.
 *
 * Structural keywords (SIMPLE, BITPIX, NAXIS, NAXISn, EXTEND, XTENSION,
 * PCOUNT, GCOUNT, END) are skipped. Multiple COMMENT and HISTORY cards are
 * each concatenated into a single attribute of the same name, separated by
 * newlines.
 *
 * @param fptr CFITSIO file pointer, positioned at the HDU to read.
 * @param grp Pointer to the netCDF-4 group that receives the attributes.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
static int
fits_read_primary_atts(fitsfile *fptr, NC_GRP_INFO_T *grp)
{
    int status = 0, nkeys, k, retval;
    char card[81];

    /* Structural keywords that map to netCDF structure, not attributes */
    static const char *skip_keys[] = {
        "SIMPLE", "BITPIX", "NAXIS", "EXTEND", "XTENSION",
        "PCOUNT", "GCOUNT", "END", NULL
    };

    fits_get_hdrspace(fptr, &nkeys, NULL, &status);
    if (status)
        return map_fitsio_error(status);

    for (k = 1; k <= nkeys; k++)
    {
        char keyword[9] = "";   /* 8-char keyword + null */
        char value[71] = "";    /* FLEN_VALUE */
        char comment[73] = "";  /* FLEN_COMMENT */
        int skip = 0;
        int i;

        status = 0;
        fits_read_record(fptr, k, card, &status);
        if (status)
            continue;

        /* Extract keyword (first 8 chars, trimmed) */
        strncpy(keyword, card, 8);
        keyword[8] = '\0';
        /* Right-trim spaces */
        for (i = 7; i >= 0 && keyword[i] == ' '; i--)
            keyword[i] = '\0';

        if (keyword[0] == '\0')
            continue;

        /* Skip NAXISn (NAXIS1..NAXIS9) */
        if (strncmp(keyword, "NAXIS", 5) == 0)
            continue;

        /* Skip known structural keywords */
        for (i = 0; skip_keys[i] != NULL; i++)
        {
            if (strcmp(keyword, skip_keys[i]) == 0)
            {
                skip = 1;
                break;
            }
        }
        if (skip)
            continue;

        /* Parse value and comment from card */
        {
            int ps = 0;
            fits_parse_value(card, value, comment, &ps);
            if (ps)
            {
                /* Blank/comment card: value is empty; use rest of card */
                strncpy(value, card + 8, 70);
                value[70] = '\0';
            }
        }

        /* Right-trim value */
        {
            int vlen = (int)strlen(value);
            while (vlen > 0 && (value[vlen-1] == ' ' || value[vlen-1] == '\''))
                value[--vlen] = '\0';
            /* Strip leading quote if present */
            if (value[0] == '\'')
                memmove(value, value + 1, strlen(value));
        }

        /* COMMENT and HISTORY: append to existing attribute if present */
        if (strcmp(keyword, "COMMENT") == 0 || strcmp(keyword, "HISTORY") == 0)
        {
            NC_ATT_INFO_T *existing = NULL;
            size_t att_idx;

            for (att_idx = 0; att_idx < ncindexsize(grp->att); att_idx++)
            {
                NC_ATT_INFO_T *a = (NC_ATT_INFO_T *)ncindexith(grp->att, att_idx);
                if (a && strcmp(a->hdr.name, keyword) == 0)
                {
                    existing = a;
                    break;
                }
            }

            if (existing)
            {
                /* Append newline + new value to existing data */
                size_t old_len = existing->len;
                size_t add_len = strlen(value);
                char *new_data = malloc(old_len + 1 + add_len + 1);
                if (!new_data)
                    return NC_ENOMEM;
                memcpy(new_data, existing->data, old_len);
                new_data[old_len] = '\n';
                memcpy(new_data + old_len + 1, value, add_len);
                new_data[old_len + 1 + add_len] = '\0';
                free(existing->data);
                existing->data = new_data;
                existing->len = old_len + 1 + add_len;
                continue;
            }
        }

        /* Add new attribute */
        {
            NC_ATT_INFO_T *att = NULL;
            size_t vlen = strlen(value);
            char *data = NULL;

            if ((retval = nc4_att_list_add(grp->att, keyword, &att)))
                return retval;

            att->nc_typeid = NC_CHAR;
            att->len = vlen;
            if (vlen > 0)
            {
                if (!(data = malloc(vlen + 1)))
                    return NC_ENOMEM;
                memcpy(data, value, vlen + 1);
            }
            att->data = data;
            att->dirty = NC_TRUE;
        }
    }

    return NC_NOERR;
}

/**
 * @internal Read the primary HDU of a FITS file and populate the
 * netCDF-4 in-memory model with its dimensions, variable, and attributes.
 *
 * The primary HDU image is mapped to the root group as:
 *  - FITS column-major axes reversed to netCDF row-major: dim_0 = NAXIS_N,
 *    ..., dim_{N-1} = NAXIS1
 *  - Variable named "image", type derived from BITPIX
 *  - All non-structural header keywords stored as NC_CHAR global attributes
 *
 * @param h5 Pointer to the netCDF-4 file info struct.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
static int
fits_read_primary_hdu(NC_FILE_INFO_T *h5)
{
    NC_FITS_FILE_INFO_T *fits_file;
    NC_GRP_INFO_T *grp;
    int status = 0, hdutype, bitpix, naxis, retval, d;
    long naxes[NC_MAX_VAR_DIMS];
    nc_type xtype;
    size_t type_size;
    char type_name[NC_MAX_NAME + 1];
    int endianness;
    int dimids[NC_MAX_VAR_DIMS];
    NC_FITS_VAR_INFO_T *var_info = NULL;
    NC_VAR_INFO_T *var = NULL;

    fits_file = (NC_FITS_FILE_INFO_T *)h5->format_file_info;
    grp = h5->root_grp;

    /* Position at primary HDU */
    fits_movabs_hdu(fits_file->fptr, 1, &hdutype, &status);
    if (status)
        return map_fitsio_error(status);

    /* Get image parameters */
    fits_get_img_param(fits_file->fptr, NC_MAX_VAR_DIMS, &bitpix, &naxis,
                       naxes, &status);
    if (status)
        return map_fitsio_error(status);

    /* Map BITPIX to netCDF type */
    if ((retval = fits_bitpix_to_nc_type(bitpix, &xtype, &type_size,
                                         type_name, &endianness)))
        return retval;

    if (naxis > 0)
    {
        /* Create dimensions in reversed order (FITS col-major → netCDF row-major).
           FITS naxes[0]=NAXIS1 (fastest), naxes[naxis-1]=NAXISn (slowest).
           netCDF dim_0 = naxes[naxis-1] (slowest = outermost). */
        for (d = 0; d < naxis; d++)
        {
            NC_DIM_INFO_T *dim;
            char dimname[NC_MAX_NAME + 1];
            size_t dimsize = (size_t)naxes[naxis - 1 - d];

            snprintf(dimname, NC_MAX_NAME, "dim_%d", d);
            if ((retval = nc4_dim_list_add(grp, dimname, dimsize, -1, &dim)))
                return retval;
            dimids[d] = dim->hdr.id;
        }

        /* Allocate per-variable FITS info */
        if (!(var_info = calloc(1, sizeof(NC_FITS_VAR_INFO_T))))
            return NC_ENOMEM;
        var_info->hdu_num = 1;
        var_info->col_num = 0;

        /* Add variable to root group */
        if ((retval = nc4_var_list_add_full(grp, "image", naxis, xtype,
                                            endianness, type_size, type_name,
                                            NULL, 1, NULL, var_info, &var)))
        {
            free(var_info);
            return retval;
        }

        /* Assign dimension IDs */
        for (d = 0; d < naxis; d++)
            var->dimids[d] = dimids[d];
    }

    /* Read all header keywords as global attributes */
    if ((retval = fits_read_primary_atts(fits_file->fptr, grp)))
        return retval;

    return NC_NOERR;
}
#endif /* HAVE_FITS */

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
#ifdef HAVE_FITS
    int fits_status = 0;
#endif

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

    /* Allocate FITS-specific file info with CFITSIO integration. */
    if (!(fits_file = calloc(1, sizeof(NC_FITS_FILE_INFO_T))))
        return NC_ENOMEM;
    
    if (!(fits_file->path = strdup(path)))
    {
        free(fits_file);
        return NC_ENOMEM;
    }

#ifdef HAVE_FITS
    /* Open the FITS file with CFITSIO */
    fits_status = 0;
    fits_open_file(&fits_file->fptr, path, READONLY, &fits_status);
    if (fits_status != 0)
    {
        free(fits_file->path);
        free(fits_file);
        return map_fitsio_error(fits_status);
    }

    /* Get total HDU count */
    fits_get_num_hdus(fits_file->fptr, &fits_file->num_hdus, &fits_status);
    if (fits_status != 0)
    {
        fits_close_file(fits_file->fptr, &fits_status);
        free(fits_file->path);
        free(fits_file);
        return map_fitsio_error(fits_status);
    }

    h5->format_file_info = fits_file;

    /* Read primary HDU metadata into the netCDF-4 in-memory model */
    if ((retval = fits_read_primary_hdu(h5)))
    {
        fits_close_file(fits_file->fptr, &fits_status);
        free(fits_file->path);
        free(fits_file);
        h5->format_file_info = NULL;
        return retval;
    }
#else
    fits_file->fptr = NULL;
    h5->format_file_info = fits_file;
#endif

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
#ifdef HAVE_FITS
    int fits_status = 0;
#endif

    assert(ignore || !ignore);

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get FITS-specific info. */
    fits_file = (NC_FITS_FILE_INFO_T *)h5->format_file_info;
    if (!fits_file)
        return NC_NOERR;

    /* Close CFITSIO file if it was opened. */
#ifdef HAVE_FITS
    if (fits_file->fptr)
    {
        fits_status = 0;
        fits_close_file(fits_file->fptr, &fits_status);
        /* Ignore close errors - we still want to free memory */
    }
#endif

    /* Free FITS-specific info. */
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
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_FITS_FILE_INFO_T *fits_file;
    int retval;
#ifdef HAVE_FITS
    int fits_status = 0;
#endif

    /* Get file info structure. */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    /* Get FITS-specific info. */
    fits_file = (NC_FITS_FILE_INFO_T *)h5->format_file_info;
    if (!fits_file)
        return NC_NOERR;

    /* Close CFITSIO file if it was opened. */
#ifdef HAVE_FITS
    if (fits_file->fptr)
    {
        fits_status = 0;
        fits_close_file(fits_file->fptr, &fits_status);
        /* Ignore close errors - we still want to free memory */
    }
#endif

    /* Free FITS-specific info. */
    free(fits_file->path);
    free(fits_file);
    h5->format_file_info = NULL;

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

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
#include <string.h>
#include <stdio.h>
#include "nep_nc4.h"
#include "grib2dispatch.h"
#include <grib2.h>

/** Maximum number of PDS/GDS/DRS template entries supported. */
#define GRIB2_MAX_TEMPLATE_LEN 200

/**
 * @internal Create a NetCDF-4 variable and insert it into the group's
 * variable list, wiring in an existing atomic type from the file's type
 * system. Follows the same pattern as geotifffile.c.
 *
 * @param h5 File info struct (needed to look up type).
 * @param grp Parent group.
 * @param name Variable name.
 * @param ndims Number of dimensions.
 * @param xtype NetCDF atomic type (e.g. NC_FLOAT).
 * @param format_var_info Format-specific var info (may be NULL).
 * @param var Output: pointer to the new NC_VAR_INFO_T.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 */
static int
grib2_var_list_add(NC_FILE_INFO_T *h5, NC_GRP_INFO_T *grp, const char *name,
                   int ndims, nc_type xtype, void *format_var_info,
                   NC_VAR_INFO_T **var)
{
    NC_TYPE_INFO_T *type_info;
    int retval;

    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created    = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->atts_read  = 1;
    (*var)->format_var_info = format_var_info;
    (*var)->storage = NC_CONTIGUOUS;

    /* Look up the atomic type object from the file's type system. */
    if ((retval = nc4_find_type(h5, xtype, &type_info)))
        return retval;
    (*var)->type_info = type_info;
    (*var)->type_info->rc++;

    return NC_NOERR;
}

/**
 * @internal Add a scalar NC_INT attribute to an attribute list.
 *
 * @param att_list The NCindex attribute list to add to.
 * @param name Attribute name.
 * @param value Integer value.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 */
static int
grib2_add_int_att(NCindex *att_list, const char *name, int value)
{
    NC_ATT_INFO_T *att;
    int *data;
    int retval;

    if ((retval = nc4_att_list_add(att_list, name, &att)))
        return retval;
    if (!(data = malloc(sizeof(int))))
        return NC_ENOMEM;
    *data = value;
    att->nc_typeid = NC_INT;
    att->len = 1;
    att->data = data;
    return NC_NOERR;
}

/**
 * @internal Add a NC_CHAR string attribute to an attribute list.
 *
 * @param att_list The NCindex attribute list to add to.
 * @param name Attribute name.
 * @param value String value (not NUL-terminated in att->data per NC_CHAR convention).
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 */
static int
grib2_add_str_att(NCindex *att_list, const char *name, const char *value)
{
    NC_ATT_INFO_T *att;
    char *data;
    int retval;
    size_t slen = strlen(value);

    if ((retval = nc4_att_list_add(att_list, name, &att)))
        return retval;
    if (!(data = malloc(slen ? slen : 1)))
        return NC_ENOMEM;
    memcpy(data, value, slen);
    att->nc_typeid = NC_CHAR;
    att->len = slen;
    att->data = data;
    return NC_NOERR;
}

/**
 * @internal Add a scalar NC_FLOAT attribute to an attribute list.
 *
 * @param att_list The NCindex attribute list to add to.
 * @param name Attribute name.
 * @param value Float value.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 */
static int
grib2_add_float_att(NCindex *att_list, const char *name, float value)
{
    NC_ATT_INFO_T *att;
    float *data;
    int retval;

    if ((retval = nc4_att_list_add(att_list, name, &att)))
        return retval;
    if (!(data = malloc(sizeof(float))))
        return NC_ENOMEM;
    *data = value;
    att->nc_typeid = NC_FLOAT;
    att->len = 1;
    att->data = data;
    return NC_NOERR;
}

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

    /* Build per-product inventory across all messages. */
    if (num_msg > 0)
    {
        int total_products = 0;
        int m;

        /* First pass: count total products. */
        for (m = 0; m < num_msg; m++)
        {
            unsigned char discipline, master_version, local_version;
            int num_fields, num_local;
            short center, subcenter;

            if (g2c_inq_msg(g2cid, m, &discipline, &num_fields, &num_local,
                            &center, &subcenter, &master_version, &local_version))
            {
                free(grib2_file);
                g2c_close(g2cid);
                return NC_EHDFERR;
            }
            total_products += num_fields;
        }

        /* Allocate product inventory array. */
        if (total_products > 0)
        {
            if (!(grib2_file->products = calloc(total_products,
                                                sizeof(NC_GRIB2_PROD_INFO_T))))
            {
                free(grib2_file);
                g2c_close(g2cid);
                return NC_ENOMEM;
            }
        }
        grib2_file->num_products = total_products;

        /* Second pass: populate inventory. */
        {
            int prod_idx = 0;

            for (m = 0; m < num_msg; m++)
            {
                unsigned char discipline, master_version, local_version;
                int num_fields, num_local, f;
                short center, subcenter;

                if (g2c_inq_msg(g2cid, m, &discipline, &num_fields, &num_local,
                                &center, &subcenter, &master_version,
                                &local_version))
                {
                    free(grib2_file->products);
                    free(grib2_file);
                    g2c_close(g2cid);
                    return NC_EHDFERR;
                }

                for (f = 0; f < num_fields; f++)
                {
                    NC_GRIB2_PROD_INFO_T *p = &grib2_file->products[prod_idx];
                    long long int pds_template[GRIB2_MAX_TEMPLATE_LEN];
                    long long int gds_template[GRIB2_MAX_TEMPLATE_LEN];
                    long long int drs_template[GRIB2_MAX_TEMPLATE_LEN];
                    int pds_len, gds_len, drs_len;
                    size_t nx = 0, ny = 0;
                    char dim_name[64];

                    p->msg_index  = m;
                    p->prod_index = f;
                    p->discipline = discipline;

                    /* Get PDS/GDS/DRS templates; category and param are in
                     * PDS template octets 0 and 1 (indices 0 and 1). */
                    if (g2c_inq_prod(g2cid, m, f, &pds_len, pds_template,
                                     &gds_len, gds_template,
                                     &drs_len, drs_template) == 0)
                    {
                        if (pds_len >= 2)
                        {
                            p->category    = (int)pds_template[0];
                            p->param_number = (int)pds_template[1];
                        }
                    }

                    /* Get grid dimensions; g2c_inq_dim_info returns size
                     * without allocating a coordinate value array. */
                    if (g2c_inq_dim_info(g2cid, m, f, 0, &nx, dim_name) == 0)
                        p->nx = nx;
                    if (g2c_inq_dim_info(g2cid, m, f, 1, &ny, dim_name) == 0)
                        p->ny = ny;

                    /* Store first product's grid size in file-level fields. */
                    if (prod_idx == 0)
                    {
                        grib2_file->num_x = p->nx;
                        grib2_file->num_y = p->ny;
                    }

                    /* Look up parameter abbreviation. */
                    p->abbrev[0] = '\0';
                    g2c_param_abbrev((int)discipline, p->category,
                                     p->param_number, p->abbrev);

                    prod_idx++;
                }
            }
        }
    }

    /* Build NetCDF-4 dimensions and variables from product inventory. */
    if (grib2_file->num_products > 0)
    {
        NC_DIM_INFO_T *dim_x, *dim_y;
        int i;

        /* Create shared x and y dimensions from the first product's grid. */
        if ((retval = nc4_dim_list_add(h5->root_grp, "x",
                                       grib2_file->num_x, -1, &dim_x)))
        {
            free(grib2_file->products);
            free(grib2_file);
            g2c_close(g2cid);
            return retval;
        }
        if ((retval = nc4_dim_list_add(h5->root_grp, "y",
                                       grib2_file->num_y, -1, &dim_y)))
        {
            free(grib2_file->products);
            free(grib2_file);
            g2c_close(g2cid);
            return retval;
        }

        /* Create one variable per product. */
        for (i = 0; i < grib2_file->num_products; i++)
        {
            NC_GRIB2_PROD_INFO_T *p = &grib2_file->products[i];
            NC_VAR_GRIB2_INFO_T *var_info;
            NC_VAR_INFO_T *var;
            /* NC_MAX_NAME + 1 for the base name; extra headroom for _NNN suffix. */
            char varname[NC_MAX_NAME + 1];
            char candidate[NC_MAX_NAME + 16];
            int suffix;

            /* Build variable name from abbreviation; fall back to var_{i}. */
            if (p->abbrev[0])
                snprintf(varname, sizeof(varname), "%s", p->abbrev);
            else
                snprintf(varname, sizeof(varname), "var_%d", i);

            /* Uniquify duplicate names by appending _2, _3, etc. */
            snprintf(candidate, sizeof(candidate), "%s", varname);
            {
                NC_VAR_INFO_T *dummy_var;
                for (suffix = 2;
                     nc4_find_var(h5->root_grp, candidate, &dummy_var) == NC_NOERR;
                     suffix++)
                    snprintf(candidate, sizeof(candidate), "%s_%d", varname, suffix);
            }
            snprintf(varname, sizeof(varname), "%.*s", NC_MAX_NAME, candidate);

            /* Allocate per-variable GRIB2 info. */
            if (!(var_info = calloc(1, sizeof(NC_VAR_GRIB2_INFO_T))))
            {
                free(grib2_file->products);
                free(grib2_file);
                g2c_close(g2cid);
                return NC_ENOMEM;
            }
            var_info->msg_index    = p->msg_index;
            var_info->prod_index   = p->prod_index;
            var_info->discipline   = (int)p->discipline;
            var_info->category     = p->category;
            var_info->param_number = p->param_number;

            /* Add 2D (y, x) NC_FLOAT variable. */
            if ((retval = grib2_var_list_add(h5, h5->root_grp, varname, 2,
                                             NC_FLOAT, var_info, &var)))
            {
                free(var_info);
                free(grib2_file->products);
                free(grib2_file);
                g2c_close(g2cid);
                return retval;
            }

            /* Wire dimension pointers (y first, then x). */
            var->dim[0]    = dim_y;
            var->dimids[0] = dim_y->hdr.id;
            var->dim[1]    = dim_x;
            var->dimids[1] = dim_x->hdr.id;

            /* Per-variable GRIB2 attributes. */
            if ((retval = grib2_add_int_att(var->att, "GRIB2_discipline",
                                            var_info->discipline)))
                return retval;
            if ((retval = grib2_add_int_att(var->att, "GRIB2_category",
                                            var_info->category)))
                return retval;
            if ((retval = grib2_add_int_att(var->att, "GRIB2_param_number",
                                            var_info->param_number)))
                return retval;
            if (p->abbrev[0])
                if ((retval = grib2_add_str_att(var->att, "long_name",
                                                p->abbrev)))
                    return retval;
            if ((retval = grib2_add_float_att(var->att, "_FillValue",
                                              9.999e20f)))
                return retval;
        }

        /* Global attributes on the root group. */
        if ((retval = grib2_add_str_att(h5->root_grp->att, "Conventions",
                                        "GRIB2")))
            return retval;
        if ((retval = grib2_add_int_att(h5->root_grp->att, "GRIB2_edition",
                                        2)))
            return retval;
    }

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
    free(grib2_file->products);
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
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EHDFERR Error from g2c or g2_getfld.
 * @return ::NC_EINVALCOORDS start/count exceeds dimension bounds.
 * @author Edward Hartnett
 */
int
NC_GRIB2_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                  void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NC_GRIB2_FILE_INFO_T *grib2_file;
    NC_VAR_GRIB2_INFO_T *var_info;
    unsigned char *cbuf = NULL;
    gribfield *gfld = NULL;
    float *full_buf = NULL;
    size_t nx, ny, nelem;
    size_t bytes_to_msg, bytes_in_msg;
    size_t msg_offset, msg_len;
    size_t row, col, out_idx;
    int retval;
    int i;

    /* Recover file, group, and variable metadata. */
    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;

    grib2_file = (NC_GRIB2_FILE_INFO_T *)h5->format_file_info;
    var_info    = (NC_VAR_GRIB2_INFO_T *)var->format_var_info;
    nx = grib2_file->num_x;
    ny = grib2_file->num_y;

    /* Validate start/count against dimension bounds. */
    if (start[0] + count[0] > ny || start[1] + count[1] > nx)
        return NC_EINVALCOORDS;

    /* Locate the raw GRIB2 message bytes for this variable's message. */
    if (g2c_seekmsg(grib2_file->g2cid, (size_t)var_info->msg_index,
                    &msg_offset, &msg_len))
        return NC_EHDFERR;

    /* Read the full raw message into a heap buffer. */
    if (g2c_get_msg(grib2_file->g2cid, msg_offset,
                    msg_offset + msg_len + 1,
                    &bytes_to_msg, &bytes_in_msg, &cbuf))
        return NC_EHDFERR;

    /* Expand the requested product to full [ny][nx] grid via g2_getfld.
     * prod_index is 1-based in the g2_getfld API. */
    if (g2_getfld(cbuf + bytes_to_msg,
                  var_info->prod_index + 1, 1, 1, &gfld))
    {
        free(cbuf);
        return NC_EHDFERR;
    }
    free(cbuf);
    cbuf = NULL;

    /* gfld->fld is full [ny*nx] with land/bitmap=0 points set to 0.0.
     * Build a float buffer with _FillValue substituted for masked points. */
    nelem = nx * ny;
    if (!(full_buf = malloc(nelem * sizeof(float))))
    {
        g2_free(gfld);
        return NC_ENOMEM;
    }
    for (i = 0; i < (int)nelem; i++)
    {
        if (gfld->ibmap == 0 && gfld->bmap && !gfld->bmap[i])
            full_buf[i] = 9.999e20f;
        else
            full_buf[i] = gfld->fld[i];
    }
    g2_free(gfld);

    /* Copy the requested [start[0]..start[0]+count[0]) x
     * [start[1]..start[1]+count[1]) subset from full_buf [ny][nx]
     * (row-major) into the caller's buffer. */
    out_idx = 0;
    nelem = count[0] * count[1];

    if (memtype == NC_FLOAT || memtype == NC_NAT)
    {
        float *out = (float *)value;
        for (row = start[0]; row < start[0] + count[0]; row++)
            for (col = start[1]; col < start[1] + count[1]; col++)
                out[out_idx++] = full_buf[row * nx + col];
    }
    else
    {
        float *tmp;
        int range_error = 0;

        if (!(tmp = malloc(nelem * sizeof(float))))
        {
            free(full_buf);
            return NC_ENOMEM;
        }
        for (row = start[0]; row < start[0] + count[0]; row++)
            for (col = start[1]; col < start[1] + count[1]; col++)
                tmp[out_idx++] = full_buf[row * nx + col];

        retval = nc4_convert_type(tmp, value, NC_FLOAT, memtype,
                                  nelem, &range_error, NULL, 0,
                                  NC_NOQUANTIZE, 0);
        free(tmp);
        if (retval)
        {
            free(full_buf);
            return retval;
        }
    }

    free(full_buf);
    return NC_NOERR;
}

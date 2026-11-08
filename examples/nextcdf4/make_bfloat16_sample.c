/**
 * @file make_bfloat16_sample.c
 * @brief Build a NEXTCDF-4 NC_BFLOAT16 copy of the GDAS sample file.
 *
 * Reads @c test/data/netCDF-4/gdas_sfc_sample.nc (an ordinary
 * NetCDF-4 file) and writes a NEXTCDF-4 file in which every NC_FLOAT
 * data variable is stored as NC_BFLOAT16. The conversion is done by
 * nc_get_var_bfloat16() on the source variables, which rounds to
 * nearest, ties to even; fill points (9.99e+20) are converted like
 * any other value. The NC_DOUBLE coordinate variables are copied
 * unchanged. The output is uncompressed and contiguous; run
 * @c make_bfloat16_sample.sh to apply the shuffle+zstd layout with
 * h5repack.
 *
 * Differences from the source: @c time is a fixed dimension of length
 * 1, and the bfloat16 variables carry no _FillValue or missing_value
 * attributes (NC_BFLOAT16 fill values are not supported, issue #377).
 * A @c history global attribute records the conversion.
 *
 * **Usage:**
 * @code
 *   make_bfloat16_sample <input.nc> <output.nc>
 * @endcode
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <nep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/** Value of the history global attribute added to the output. */
#define HISTORY "Data fields converted from gdas_sfc_sample.nc to NC_BFLOAT16 with nc_get_var_bfloat16 (round to nearest, ties to even) by make_bfloat16_sample"

/**
 * @brief Return the size in bytes of an atomic NetCDF type.
 *
 * @param xtype The nc_type.
 * @return Element size in bytes, or 0 for an unknown type.
 */
static size_t
type_size(nc_type xtype)
{
    switch (xtype)
    {
    case NC_CHAR:
    case NC_BYTE:
        return 1;
    case NC_SHORT:
        return 2;
    case NC_INT:
    case NC_FLOAT:
        return 4;
    case NC_DOUBLE:
        return 8;
    default:
        return 0;
    }
}

/**
 * @brief Copy one attribute between files, keeping type and length.
 *
 * @param ncid_in  Input file.
 * @param varid_in Variable the attribute is attached to (NC_GLOBAL ok).
 * @param name     Attribute name.
 * @param ncid_out Output file.
 * @param varid_out Output variable (NC_GLOBAL ok).
 * @return 0 on success, exits on NetCDF error.
 */
static int
copy_att(int ncid_in, int varid_in, const char *name, int ncid_out,
         int varid_out)
{
    nc_type xtype;
    size_t len, size;
    void *data;
    int ret;

    if ((ret = nc_inq_att(ncid_in, varid_in, name, &xtype, &len))) return ret;
    size = type_size(xtype);
    if (!size) return NC_EBADTYPE;
    if (!(data = malloc(len * size))) return NC_ENOMEM;
    if ((ret = nc_get_att(ncid_in, varid_in, name, data)))
    {
        free(data);
        return ret;
    }
    ret = nc_put_att(ncid_out, varid_out, name, xtype, len, data);
    free(data);
    return ret;
}

/**
 * @brief Copy the global attributes and add the history attribute.
 *
 * @param ncid_in  Input file.
 * @param ncid_out Output file.
 * @return 0 on success, exits on NetCDF error.
 */
static int
copy_global_atts(int ncid_in, int ncid_out)
{
    int natts, a, ret;
    char name[NC_MAX_NAME + 1];

    if ((ret = nc_inq_natts(ncid_in, &natts))) return ret;
    for (a = 0; a < natts; a++)
    {
        if ((ret = nc_inq_attname(ncid_in, NC_GLOBAL, a, name))) return ret;
        if ((ret = copy_att(ncid_in, NC_GLOBAL, name, ncid_out, NC_GLOBAL)))
            return ret;
    }
    return nc_put_att_text(ncid_out, NC_GLOBAL, "history",
                           strlen(HISTORY), HISTORY);
}

/**
 * @brief Define the output dimensions in fixed order.
 *
 * Defines @c grid_xt, @c grid_yt and a fixed-length @c time, with the
 * lengths taken from the same-named input dimensions.
 *
 * @param ncid_in  Input file.
 * @param ncid_out Output file.
 * @return 0 on success, exits on NetCDF error.
 */
static int
def_dims(int ncid_in, int ncid_out)
{
    static const char *names[3] = {"grid_xt", "grid_yt", "time"};
    int i, ret;

    for (i = 0; i < 3; i++)
    {
        int dimid;
        size_t len;

        if ((ret = nc_inq_dimid(ncid_in, names[i], &dimid))) return ret;
        if ((ret = nc_inq_dimlen(ncid_in, dimid, &len))) return ret;
        if ((ret = nc_def_dim(ncid_out, names[i], len, NULL))) return ret;
    }
    return 0;
}

/**
 * @brief Define all output variables and copy their attributes.
 *
 * For every input variable, in input order: NC_DOUBLE variables are
 * defined as NC_DOUBLE (the coordinates) and NC_FLOAT variables as
 * NC_BFLOAT16 with no fill value. Any other type is an error. Output
 * dims are mapped by name. Every variable attribute is copied except
 * _FillValue and missing_value.
 *
 * @param ncid_in  Input file.
 * @param ncid_out Output file.
 * @return 0 on success, 1 on an unexpected variable type, exits on
 *         NetCDF error.
 */
static int
def_vars(int ncid_in, int ncid_out)
{
    int nvars, v, ret;

    if ((ret = nc_inq_nvars(ncid_in, &nvars))) return ret;
    for (v = 0; v < nvars; v++)
    {
        nc_type xtype;
        nc_type out_type;
        char name[NC_MAX_NAME + 1];
        int ndims, dimids_in[NC_MAX_VAR_DIMS], dimids_out[NC_MAX_VAR_DIMS];
        int natts, a, d, varid_out;

        if ((ret = nc_inq_var(ncid_in, v, name, &xtype, &ndims, dimids_in,
                              &natts))) return ret;
        if (xtype == NC_DOUBLE)
            out_type = NC_DOUBLE;
        else if (xtype == NC_FLOAT)
            out_type = NC_BFLOAT16;
        else
        {
            fprintf(stderr, "%s: unexpected variable type %d\n", name,
                    (int)xtype);
            return 1;
        }
        for (d = 0; d < ndims; d++)
        {
            char dname[NC_MAX_NAME + 1];

            if ((ret = nc_inq_dimname(ncid_in, dimids_in[d], dname))) return ret;
            if ((ret = nc_inq_dimid(ncid_out, dname, &dimids_out[d]))) return ret;
        }
        if ((ret = nc_def_var(ncid_out, name, out_type, ndims, dimids_out,
                              &varid_out))) return ret;
        if (out_type == NC_BFLOAT16 &&
            (ret = nc_def_var_fill(ncid_out, varid_out, NC_NOFILL, NULL)))
            return ret;
        for (a = 0; a < natts; a++)
        {
            char aname[NC_MAX_NAME + 1];

            if ((ret = nc_inq_attname(ncid_in, v, a, aname))) return ret;
            if (!strcmp(aname, "_FillValue") || !strcmp(aname, "missing_value"))
                continue;
            if ((ret = copy_att(ncid_in, v, aname, ncid_out, varid_out)))
                return ret;
        }
    }
    return 0;
}

/**
 * @brief Copy the data of every variable.
 *
 * NC_DOUBLE variables are copied with nc_get_var_double()/
 * nc_put_var_double(). NC_BFLOAT16 variables are read from the source
 * NC_FLOAT variable with nc_get_var_bfloat16() into a uint16_t bit
 * buffer (NEP rounds to nearest, ties to even) and written with
 * nc_put_var_bfloat16(). An NC_ERANGE from the get is an error,
 * because the GDAS sample has no out-of-range values.
 *
 * @param ncid_in  Input file.
 * @param ncid_out Output file.
 * @return 0 on success, 1 on NC_ERANGE, exits on NetCDF error.
 */
static int
copy_data(int ncid_in, int ncid_out)
{
    int nvars, v, ret;

    if ((ret = nc_inq_nvars(ncid_in, &nvars))) return ret;
    for (v = 0; v < nvars; v++)
    {
        nc_type xtype;
        char name[NC_MAX_NAME + 1];
        int ndims, dimids[NC_MAX_VAR_DIMS], d, varid_out;
        size_t nvals = 1;

        if ((ret = nc_inq_var(ncid_in, v, name, &xtype, &ndims, dimids,
                              NULL))) return ret;
        for (d = 0; d < ndims; d++)
        {
            size_t len;

            if ((ret = nc_inq_dimlen(ncid_in, dimids[d], &len))) return ret;
            nvals *= len;
        }
        if ((ret = nc_inq_varid(ncid_out, name, &varid_out))) return ret;
        if (xtype == NC_DOUBLE)
        {
            double *data;

            if (!(data = malloc(nvals * sizeof(double))))
            {
                fprintf(stderr, "Memory allocation failed\n");
                return 1;
            }
            if ((ret = nc_get_var_double(ncid_in, v, data)))
            {
                free(data);
                return ret;
            }
            ret = nc_put_var_double(ncid_out, varid_out, data);
            free(data);
            if (ret) return ret;
        }
        else
        {
            uint16_t *bdata;

            if (!(bdata = malloc(nvals * sizeof(uint16_t))))
            {
                fprintf(stderr, "Memory allocation failed\n");
                return 1;
            }
            ret = nc_get_var_bfloat16(ncid_in, v,
                                      (nc_bfloat16_t *)(void *)bdata);
            if (ret == NC_ERANGE)
            {
                free(bdata);
                fprintf(stderr, "%s: values out of bfloat16 range\n", name);
                return 1;
            }
            if (ret)
            {
                free(bdata);
                return ret;
            }
            ret = nc_put_var_bfloat16(ncid_out, varid_out,
                                      (nc_bfloat16_t *)(void *)bdata);
            free(bdata);
            if (ret) return ret;
        }
    }
    return 0;
}

/**
 * @brief Main entry point.
 *
 * @param argc Argument count; must be 3.
 * @param argv argv[1] is the input netCDF-4 file, argv[2] the output
 *             NEXTCDF-4 file.
 * @return 0 on success, 1 on an unexpected type or NC_ERANGE, 2 on
 *         usage or NetCDF error.
 */
int
main(int argc, char **argv)
{
    int ncid_in, ncid_out;
    int ret;

    if (argc != 3)
    {
        fprintf(stderr, "usage: make_bfloat16_sample <input.nc> <output.nc>\n");
        return 2;
    }

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

    if ((ret = nc_open(argv[1], NC_NOWRITE, &ncid_in))) ERR(ret);
    if ((ret = nc_create(argv[2], NC_NEXTCDF4 | NC_CLOBBER, &ncid_out))) ERR(ret);

    if ((ret = copy_global_atts(ncid_in, ncid_out))) ERR(ret);
    if ((ret = def_dims(ncid_in, ncid_out))) ERR(ret);
    if ((ret = def_vars(ncid_in, ncid_out)))
    {
        if (ret != 1) ERR(ret);
        return 1;
    }
    if ((ret = nc_enddef(ncid_out))) ERR(ret);
    if ((ret = copy_data(ncid_in, ncid_out)))
    {
        if (ret != 1) ERR(ret);
        return 1;
    }

    if ((ret = nc_close(ncid_in))) ERR(ret);
    if ((ret = nc_close(ncid_out))) ERR(ret);

    printf("Wrote %s\n", argv[2]);
    return 0;
}

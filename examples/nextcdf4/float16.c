/**
 * @file float16.c
 * @brief Store a real meteorological field as NC_FLOAT16 in a
 *        NEXTCDF-4 file.
 *
 * IEEE 754 binary16 is a 16-bit floating-point format with 1 sign
 * bit, 5 exponent bits and 10 mantissa bits. It has about 3
 * significant decimal digits of precision, a maximum of 65504 and a
 * smallest normal value of about 6.1e-5, and takes half the storage
 * of float32. It suits data where a relative error of up to 2^-11
 * (about 0.05%) is acceptable and every value fits its narrow range.
 *
 * This program reads the @c tmp2m (2 m temperature) field and its
 * coordinate variables from the GDAS sample file
 * @c test/data/netCDF-4/gdas_sfc_sample.nc, writes it twice to
 * NEXTCDF-4 files - once as NC_FLOAT and once as NC_FLOAT16 - then
 * reads the float16 file back, checks the bits round-tripped exactly,
 * and reports file sizes and the maximum conversion error.
 *
 * **Usage:**
 * @code
 *   HDF5_PLUGIN_PATH=<dir with one zstd plugin> \
 *   NCRCENV_RC=<build>/.ncrc NETCDF_RC=<build> \
 *   ./float16 gdas_sfc_sample.nc
 * @endcode
 *
 * The input file is zstd-compressed, so @c HDF5_PLUGIN_PATH must point
 * to a directory holding the zstd HDF5 plugin. The NEXTCDF-4 backend
 * is loaded through the .ncrc UDF9 autoload mechanism, so @c NCRCENV_RC
 * and @c NETCDF_RC must name the NEP build directory that contains the
 * generated @c .ncrc.
 *
 * **Learning Objectives:**
 * - Create a NEXTCDF-4 file with nc_create() and NC_NEXTCDF4
 * - Define an NC_FLOAT16 variable
 * - Read and write float16 data with the nc_*_float16() functions
 * - Read NC_FLOAT16 data as float with nc_get_vara_float()
 * - Understand the precision and range trade-offs of binary16
 *
 * **Key Concepts:**
 * - **Raw bit patterns**: NC_FLOAT16 data is exchanged with the
 *   library as raw uint16_t bit patterns
 * - **Conversion by NEP**: the nc_*_float16() functions convert
 *   between float16 and float/double variables in the library;
 *   float_to_float16() stays as the reference conversion and is used
 *   here as a cross-check; the typed float/double calls use the same
 *   NEP round-to-nearest-even through the internal engine
 * - **Native mode only**: NC_FLOAT16 is only available in NEXTCDF-4
 *   native mode
 * - **No fill value**: float16 variables are defined without a fill
 *   value
 *
 * **Key API functions:**
 * - nc_create()      Create a NEXTCDF-4 file with NC_NEXTCDF4
 * - nc_def_var()     Define a variable with NC_FLOAT16
 * - nc_get_var_float16()   Read a float variable as float16 (NEP converts)
 * - nc_put_vara_float16()  Write float16 bits to an NC_FLOAT16 variable
 * - nc_get_vara_float16()  Read float16 bits from an NC_FLOAT16 variable
 * - nc_get_vara_float()  Read float16 data converted to float
 *
 * **Related Examples:**
 * - bfloat16.c     - The bfloat16 version of this tutorial
 * - float16_perf.c - Benchmark of float32 vs float16 size and speed
 * - compression.c  - NetCDF-4 compression tutorial
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <netcdf.h>
#include <nep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "float16_conv.h"

/** Length of the grid_yt dimension of the GDAS sample. */
#define NY 768
/** Length of the grid_xt dimension of the GDAS sample. */
#define NX 1536
/** Number of values in one field. */
#define NVALS ((size_t)NY * NX)

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/**
 * @brief Read a null-padded text attribute into a malloc'ed buffer.
 *
 * @param ncid     NetCDF file ID.
 * @param varid    Variable ID.
 * @param name     Attribute name.
 * @param text     Returned buffer (caller frees); NULL if absent.
 * @param len      Returned attribute length; 0 if absent.
 */
static void
get_att_text(int ncid, int varid, const char *name, char **text, size_t *len)
{
    size_t attlen = 0;
    int ret;

    *text = NULL;
    *len = 0;
    if ((ret = nc_inq_attlen(ncid, varid, name, &attlen)))
    {
        if (ret == NC_ENOTATT)
            return;
        ERR(ret);
    }
    *text = (char *)malloc(attlen + 1);
    if (!*text)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    if ((ret = nc_get_att_text(ncid, varid, name, *text))) ERR(ret);
    (*text)[attlen] = '\0';
    *len = attlen;
}

/**
 * @brief Write one field and the coordinate variables to a NEXTCDF-4 file.
 *
 * Creates @p path with nc_create(NC_NEXTCDF4 | NC_CLOBBER), defines
 * fixed dims @c time = 1, @c grid_yt = NY and @c grid_xt = NX, the
 * NC_DOUBLE coordinate variables @c grid_yt and @c grid_xt, and the
 * variable @c tmp2m(time, grid_yt, grid_xt) of type @p xtype with the
 * @c long_name and @c units attributes copied from the input. The
 * variable gets no fill value.
 *
 * @param path        Output file path.
 * @param xtype       Variable type (NC_FLOAT or NC_FLOAT16).
 * @param data        Data buffer (float or uint16_t, NVALS elements).
 * @param grid_yt     Y coordinates (NY doubles).
 * @param grid_xt     X coordinates (NX doubles).
 * @param long_name   long_name attribute text.
 * @param long_name_len Length of @p long_name.
 * @param units       units attribute text.
 * @param units_len   Length of @p units.
 * @return 0 on success, exits on NetCDF error.
 */
static int
write_file(const char *path, nc_type xtype, const void *data,
           const double *grid_yt, const double *grid_xt,
           const char *long_name, size_t long_name_len,
           const char *units, size_t units_len)
{
    int ncid, dimids[3], ydimid, xdimid, yvarid, xvarid, varid;
    size_t start[3] = {0, 0, 0};
    size_t count[3] = {1, NY, NX};
    int ret;

    if ((ret = nc_create(path, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "time", 1, &dimids[0]))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "grid_yt", NY, &ydimid))) ERR(ret);
    if ((ret = nc_def_dim(ncid, "grid_xt", NX, &xdimid))) ERR(ret);
    dimids[1] = ydimid;
    dimids[2] = xdimid;

    if ((ret = nc_def_var(ncid, "grid_yt", NC_DOUBLE, 1, &ydimid, &yvarid))) ERR(ret);
    if ((ret = nc_def_var(ncid, "grid_xt", NC_DOUBLE, 1, &xdimid, &xvarid))) ERR(ret);
    if ((ret = nc_def_var(ncid, "tmp2m", xtype, 3, dimids, &varid))) ERR(ret);
    if ((ret = nc_def_var_fill(ncid, varid, NC_NOFILL, NULL))) ERR(ret);
    if (long_name_len &&
        (ret = nc_put_att_text(ncid, varid, "long_name", long_name_len, long_name))) ERR(ret);
    if (units_len &&
        (ret = nc_put_att_text(ncid, varid, "units", units_len, units))) ERR(ret);
    if ((ret = nc_enddef(ncid))) ERR(ret);

    if ((ret = nc_put_var_double(ncid, yvarid, grid_yt))) ERR(ret);
    if ((ret = nc_put_var_double(ncid, xvarid, grid_xt))) ERR(ret);
    /* nc_put_vara_float16 stores the raw bits on an NC_FLOAT16
     * variable; nc_put_vara_float would work too - the internal
     * engine rounds to nearest, ties to even, the same as
     * float_to_float16() and the NEP functions. */
    if (xtype == NC_FLOAT16)
    {
        if ((ret = nc_put_vara_float16(ncid, varid, start, count,
                                       (nc_float16_t *)(void *)data))) ERR(ret);
    }
    else
    {
        if ((ret = nc_put_vara_float(ncid, varid, start, count, data))) ERR(ret);
    }
    if ((ret = nc_close(ncid))) ERR(ret);
    return 0;
}

/**
 * @brief Main entry point.
 *
 * Reads @c tmp2m and its coordinate variables from the GDAS sample,
 * writes @c float_tmp2m.nc (NC_FLOAT) and @c float16_tmp2m.nc
 * (NC_FLOAT16), verifies the float16 bits round-trip exactly, and
 * prints file sizes and conversion errors.
 *
 * @param argc Argument count; must be 2.
 * @param argv argv[1] is the path to gdas_sfc_sample.nc.
 * @return 0 on success, 1 on verification failure, 2 on usage or
 *         NetCDF error.
 */
int
main(int argc, char **argv)
{
    int ncid, varid, xvarid, yvarid;
    float *fdata = NULL;
    double *grid_yt = NULL, *grid_xt = NULL;
    uint16_t *hdata = NULL, *hdata_in = NULL;
    float *typed_in = NULL;
    char *long_name = NULL, *units = NULL;
    size_t long_name_len, units_len;
    size_t i;
    struct stat st;
    long long float_bytes, float16_bytes;
    nc_type xtype;
    double max_abs_err = 0.0, max_rel_err = 0.0;
    size_t n_typed_bad = 0, n_conv_bad = 0, n_gate_bad = 0;
    int fail = 0;

    if (argc != 2)
    {
        fprintf(stderr, "usage: float16 <gdas_sfc_sample.nc>\n");
        return 2;
    }

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

    fdata = (float *)malloc(NVALS * sizeof(float));
    grid_yt = (double *)malloc(NY * sizeof(double));
    grid_xt = (double *)malloc(NX * sizeof(double));
    hdata = (uint16_t *)malloc(NVALS * sizeof(uint16_t));
    hdata_in = (uint16_t *)malloc(NVALS * sizeof(uint16_t));
    typed_in = (float *)malloc(NVALS * sizeof(float));
    if (!fdata || !grid_yt || !grid_xt || !hdata || !hdata_in || !typed_in)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    /* Read the input with the built-in NetCDF-4 backend. */
    {
        int ret;

        if ((ret = nc_open(argv[1], NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, "grid_yt", &yvarid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, "grid_xt", &xvarid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, "tmp2m", &varid))) ERR(ret);
        if ((ret = nc_get_var_double(ncid, yvarid, grid_yt))) ERR(ret);
        if ((ret = nc_get_var_double(ncid, xvarid, grid_xt))) ERR(ret);
        if ((ret = nc_get_var_float(ncid, varid, fdata))) ERR(ret);
        /* Read the same field a second time as float16; NEP does the
         * conversion. */
        if ((ret = nc_get_var_float16(ncid, varid, (nc_float16_t *)(void *)hdata))) ERR(ret);
        get_att_text(ncid, varid, "long_name", &long_name, &long_name_len);
        get_att_text(ncid, varid, "units", &units, &units_len);
        if ((ret = nc_close(ncid))) ERR(ret);
    }

    /* Check NEP's conversion against the reference
     * float_to_float16(). */
    for (i = 0; i < NVALS; i++)
        if (hdata[i] != float_to_float16(fdata[i]))
            n_conv_bad++;

    write_file("float_tmp2m.nc", NC_FLOAT, fdata, grid_yt, grid_xt,
               long_name, long_name_len, units, units_len);
    write_file("float16_tmp2m.nc", NC_FLOAT16, hdata, grid_yt, grid_xt,
               long_name, long_name_len, units, units_len);

    /* Reopen the float16 file and check the bits round-tripped. */
    {
        int ret;

        if ((ret = nc_open("float16_tmp2m.nc", NC_NEXTCDF4 | NC_NOWRITE, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, "tmp2m", &varid))) ERR(ret);
        if ((ret = nc_inq_vartype(ncid, varid, &xtype))) ERR(ret);
        if (xtype != NC_FLOAT16)
        {
            fprintf(stderr, "tmp2m type is %d, expected NC_FLOAT16\n", (int)xtype);
            return 1;
        }
        /* nc_get_vara_float16 returns the raw stored bits.
         * nc_get_vara_float, used below as a cross-check, returns the
         * same values decoded exactly to float. */
        {
            size_t start[3] = {0, 0, 0};
            size_t count[3] = {1, NY, NX};
            if ((ret = nc_get_vara_float16(ncid, varid, start, count,
                                           (nc_float16_t *)(void *)hdata_in))) ERR(ret);
            if ((ret = nc_get_vara_float(ncid, varid, start, count, typed_in))) ERR(ret);
        }
        if ((ret = nc_close(ncid))) ERR(ret);
    }

    if (memcmp(hdata, hdata_in, NVALS * sizeof(uint16_t)))
    {
        fprintf(stderr, "float16 bits did not round-trip\n");
        fail = 1;
    }

    /* Cross-check the typed float read against the stored bits; the
     * bits are compared through memcpy, so NaN compares by bits. */
    for (i = 0; i < NVALS; i++)
    {
        uint32_t fbits, ebits;
        float expected = float16_to_float(hdata_in[i]);

        memcpy(&fbits, &typed_in[i], sizeof(fbits));
        memcpy(&ebits, &expected, sizeof(ebits));
        if (fbits != ebits)
            n_typed_bad++;
    }

    /* Conversion error over non-fill points only. */
    for (i = 0; i < NVALS; i++)
    {
        double err, rel;

        if (fdata[i] == F16_FILL_IN)
            continue;
        err = (double)float16_to_float(hdata_in[i]) - (double)fdata[i];
        if (err < 0.0)
            err = -err;
        if (err > max_abs_err)
            max_abs_err = err;
        if (fdata[i] != 0.0f)
        {
            rel = err / (double)(fdata[i] < 0.0f ? -fdata[i] : fdata[i]);
            if (rel > max_rel_err)
                max_rel_err = rel;
            /* The gate: a point must be within the RNE relative bound
             * or below the absolute floor (subnormal underflow). */
            if (rel > F16_REL_ERR_MAX && err > F16_ABS_ERR_MAX)
                n_gate_bad++;
        }
    }

    if (stat("float_tmp2m.nc", &st) != 0)
    {
        perror("stat");
        return 1;
    }
    float_bytes = (long long)st.st_size;
    if (stat("float16_tmp2m.nc", &st) != 0)
    {
        perror("stat");
        return 1;
    }
    float16_bytes = (long long)st.st_size;

    printf("tmp2m: %zu values\n", NVALS);
    printf("float32 file: %lld bytes\n", float_bytes);
    printf("float16 file: %lld bytes (%.1f%% of float32)\n",
           float16_bytes, 100.0 * (double)float16_bytes / (double)float_bytes);
    printf("max abs error: %g %s\n", max_abs_err, units ? units : "");
    printf("max rel error: %g\n", max_rel_err);
    printf("typed nc_get_vara_float: %zu mismatches\n", n_typed_bad);
    printf("nc_get_var_float16 vs float_to_float16: %zu mismatches\n", n_conv_bad);

    if (float16_bytes >= float_bytes)
    {
        fprintf(stderr, "float16 file is not smaller than the float32 file\n");
        fail = 1;
    }
    if (n_gate_bad)
    {
        fprintf(stderr, "%zu values exceed the error gate (rel > %g and abs > %g)\n",
                n_gate_bad, (double)F16_REL_ERR_MAX, (double)F16_ABS_ERR_MAX);
        fail = 1;
    }
    if (n_typed_bad)
    {
        fprintf(stderr, "typed nc_get_vara_float does not match the stored bits\n");
        fail = 1;
    }
    if (n_conv_bad)
    {
        fprintf(stderr, "NEP conversion does not match float_to_float16\n");
        fail = 1;
    }

    free(fdata);
    free(grid_yt);
    free(grid_xt);
    free(hdata);
    free(hdata_in);
    free(typed_in);
    free(long_name);
    free(units);

    return fail;
}

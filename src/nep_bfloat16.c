/**
 * @file
 * This is the code file for the NEP bfloat16 I/O functions.
 *
 * The nc_*_bfloat16 functions exchange bfloat16 values in memory with
 * NC_BFLOAT16, NC_FLOAT and NC_DOUBLE variables. They are written on
 * top of the public netCDF API, so they work with any file netCDF can
 * open. Buffers are always accessed as bytes through memcpy, never as
 * nc_bfloat16_t values, so the code is the same whether nc_bfloat16_t
 * is the compiler's __bf16 or uint16_t.
 *
 * @author Edward Hartnett
 * @date 2026-11-08
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "nep.h"
#include <stdlib.h>
#include <string.h>

/**
 * Convert a float to a bfloat16 bit pattern, rounding to nearest with
 * ties to even. NaN stays a quiet NaN, as in bfloat16_conv.h.
 *
 * @param f The float to convert.
 * @param range Set to 1 when a finite input becomes +-inf; otherwise
 * left unchanged.
 *
 * @return The bfloat16 bit pattern.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static uint16_t
f32_to_bf16(float f, int *range)
{
    uint32_t u, mag;
    uint16_t b;

    /* Work on the float's bits. bfloat16 is the top 16 bits of a
     * float, so converting means rounding away the low 16 bits. */
    memcpy(&u, &f, sizeof(u));
    mag = u & 0x7fffffffu;

    /* NaN: keep the sign and top payload bits, and set the quiet bit
     * so truncating the payload can't turn the NaN into infinity. */
    if (mag > 0x7f800000u)
        return (uint16_t)((u >> 16) | 0x0040u);

    /* Round to nearest, ties to even: add just under half of the
     * discarded range (0x7fff), plus 1 when the kept part is odd, so an
     * exact tie only carries into the kept bits if they are odd. A
     * carry out of the mantissa correctly bumps the exponent, and from
     * the largest finite value it gives infinity. */
    b = (uint16_t)((u + 0x7fffu + ((u >> 16) & 1u)) >> 16);

    /* A finite input that rounded to infinity is a range error;
     * an input that was already infinite is not. */
    if ((b & 0x7fffu) == 0x7f80u && mag != 0x7f800000u)
        *range = 1;
    return b;
}

/**
 * Convert a double to a bfloat16 bit pattern with a single rounding
 * (to nearest, ties to even). The double is first rounded to float
 * with round-to-odd (an inexact result gets an odd last bit), then
 * converted with f32_to_bf16(). float has 16 more significand bits
 * than bfloat16, so this equals rounding the double directly.
 *
 * @param d The double to convert.
 * @param range Set to 1 when a finite input becomes +-inf; otherwise
 * left unchanged.
 *
 * @return The bfloat16 bit pattern.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static uint16_t
f64_to_bf16(double d, int *range)
{
    /* Default C conversion: round to nearest float, ties to even. */
    float f = (float)d;
    uint32_t u;

    /* d is finite (d == d excludes NaN, d - d == 0 excludes inf) but
     * too big for float, so f is inf (f - f is NaN). Every finite value
     * past the float range is also past the bfloat16 range, so this is
     * a range error. */
    if (d == d && f - f != 0.0f && d - d == 0.0) {
        *range = 1;
        return d < 0 ? 0xff80u : 0x7f80u;
    }

    /* Two roundings (double to float, then float to bfloat16) can give
     * the wrong answer when the first one lands exactly on a bfloat16
     * tie. Avoid that with round-to-odd: if the float is inexact and
     * its last bit is 0, step it one ulp toward d. An odd last bit
     * means it can never look like an exact tie, and the step can't
     * cross a bfloat16 rounding boundary. Exact results and NaN are
     * left alone. */
    if (d == d && (double)f != d) {
        double ad = d < 0 ? -d : d, af = f < 0 ? -f : f;

        memcpy(&u, &f, sizeof(u));
        if (!(u & 1u))
            u = af < ad ? u + 1u : u - 1u;   /* toward d in magnitude */
        memcpy(&f, &u, sizeof(f));
    }

    /* The second, final rounding to bfloat16. */
    return f32_to_bf16(f, range);
}

/**
 * Convert a bfloat16 bit pattern to float. The conversion is exact.
 *
 * @param b The bfloat16 bit pattern.
 *
 * @return The float value.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static float
bf16_to_f32(uint16_t b)
{
    /* bfloat16 bits are the top half of the float with the same value;
     * the low 16 mantissa bits are zero. */
    uint32_t u = (uint32_t)b << 16;
    float f;

    memcpy(&f, &u, sizeof(f));
    return f;
}

/**
 * Common worker for the eight nc_*_bfloat16 functions. NC_BFLOAT16
 * variables are read or written as raw bits with nc_get_vars() or
 * nc_put_vars(). NC_FLOAT and NC_DOUBLE variables go through a
 * temporary float or double buffer and the typed nc_get_vars_* or
 * nc_put_vars_* call, with the conversion done here.
 *
 * @param ncid NetCDF file ID.
 * @param varid Variable ID.
 * @param startp Start index in each dimension.
 * @param countp Edge count in each dimension.
 * @param stridep Stride in each dimension, or NULL for a stride of 1.
 * @param buf The bfloat16 buffer (read into for a get, read from for a
 * put).
 * @param put Non-zero to write, zero to read.
 *
 * @return NC_NOERR No error.
 * @return NC_ERANGE A finite value overflowed to +-inf on a get; all
 * values are still converted.
 * @return NC_ECHAR The variable is NC_CHAR.
 * @return NC_EBADTYPE The variable is not NC_BFLOAT16, NC_FLOAT or
 * NC_DOUBLE.
 * @return NC_EINVAL NULL start, count or buffer where one is needed.
 * @return NC_ENOMEM Out of memory.
 * @return Other errors from the underlying netCDF calls.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
bf16_io(int ncid, int varid, const size_t *startp, const size_t *countp,
        const ptrdiff_t *stridep, void *buf, int put)
{
    nc_type xtype;
    int ndims, ret, i;
    size_t n = 1, esize, j;
    void *tmp;
    unsigned char *bp = (unsigned char *)buf;

    /* Find the variable's type and rank; this also checks ncid and
     * varid. */
    if ((ret = nc_inq_vartype(ncid, varid, &xtype))) return ret;
    if ((ret = nc_inq_varndims(ncid, varid, &ndims))) return ret;

    /* Reject unsupported types before touching the buffer or the
     * file. */
    if (xtype == NC_CHAR) return NC_ECHAR;
    if (xtype != NC_BFLOAT16 && xtype != NC_FLOAT && xtype != NC_DOUBLE)
        return NC_EBADTYPE;

    /* The memory and file types are both bfloat16, so the bits pass
     * straight through the untyped calls with no conversion. */
    if (xtype == NC_BFLOAT16)
        return put ? nc_put_vars(ncid, varid, startp, countp, stridep, buf)
                   : nc_get_vars(ncid, varid, startp, countp, stridep, buf);

    /* NC_FLOAT or NC_DOUBLE: count the elements so the temporary
     * buffer can be sized. A scalar (ndims 0) has one element. */
    if (ndims > 0 && (!startp || !countp)) return NC_EINVAL;
    for (i = 0; i < ndims; i++)
        n *= countp[i];
    if (n > 0 && !buf) return NC_EINVAL;

    /* Allocate at least one element, so a zero count still reaches
     * the netCDF call and gets its argument checks. */
    esize = (xtype == NC_FLOAT) ? sizeof(float) : sizeof(double);
    if (!(tmp = malloc((n ? n : 1) * esize))) return NC_ENOMEM;

    if (!put) {
        int range = 0;

        /* Read in the file's own type, then round each value to
         * bfloat16. Every value is converted even after a range error,
         * as netCDF's typed calls do. */
        if (xtype == NC_FLOAT) {
            float *tp = (float *)tmp;
            ret = nc_get_vars_float(ncid, varid, startp, countp, stridep, tp);
            if (!ret)
                for (j = 0; j < n; j++) {
                    uint16_t b = f32_to_bf16(tp[j], &range);
                    memcpy(bp + j * 2, &b, 2);
                }
        } else {
            double *tp = (double *)tmp;
            ret = nc_get_vars_double(ncid, varid, startp, countp, stridep, tp);
            if (!ret)
                for (j = 0; j < n; j++) {
                    uint16_t b = f64_to_bf16(tp[j], &range);
                    memcpy(bp + j * 2, &b, 2);
                }
        }
        free(tmp);
        if (ret) return ret;
        return range ? NC_ERANGE : NC_NOERR;
    }

    /* Write: widen each bfloat16 value to the file's type, which is
     * always exact, then write with the matching typed call. */
    if (xtype == NC_FLOAT) {
        float *tp = (float *)tmp;
        for (j = 0; j < n; j++) {
            uint16_t b;
            memcpy(&b, bp + j * 2, 2);
            tp[j] = bf16_to_f32(b);
        }
        ret = nc_put_vars_float(ncid, varid, startp, countp, stridep, tp);
    } else {
        double *tp = (double *)tmp;
        for (j = 0; j < n; j++) {
            uint16_t b;
            memcpy(&b, bp + j * 2, 2);
            tp[j] = (double)bf16_to_f32(b);
        }
        ret = nc_put_vars_double(ncid, varid, startp, countp, stridep, tp);
    }
    free(tmp);
    return ret;
}

/**
 * Read a strided hyperslab of a variable as bfloat16. See nep.h for
 * the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_vars_bfloat16(int ncid, int varid, const size_t *startp,
                     const size_t *countp, const ptrdiff_t *stridep,
                     nc_bfloat16_t *ip)
{
    return bf16_io(ncid, varid, startp, countp, stridep, ip, 0);
}

/**
 * Write a strided hyperslab of a variable from bfloat16 values. See
 * nep.h for the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_vars_bfloat16(int ncid, int varid, const size_t *startp,
                     const size_t *countp, const ptrdiff_t *stridep,
                     const nc_bfloat16_t *op)
{
    /* bf16_io only reads the buffer when put is set, so casting away
     * const is safe. */
    return bf16_io(ncid, varid, startp, countp, stridep, (void *)op, 1);
}

/**
 * Read a hyperslab of a variable as bfloat16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_vara_bfloat16(int ncid, int varid, const size_t *startp,
                     const size_t *countp, nc_bfloat16_t *ip)
{
    /* A vara call is a vars call with stride 1 (NULL). */
    return bf16_io(ncid, varid, startp, countp, NULL, ip, 0);
}

/**
 * Write a hyperslab of a variable from bfloat16 values. See nep.h for
 * the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_vara_bfloat16(int ncid, int varid, const size_t *startp,
                     const size_t *countp, const nc_bfloat16_t *op)
{
    /* A vara call is a vars call with stride 1 (NULL). */
    return bf16_io(ncid, varid, startp, countp, NULL, (void *)op, 1);
}

/**
 * Read one value from a variable as bfloat16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_var1_bfloat16(int ncid, int varid, const size_t *indexp,
                     nc_bfloat16_t *ip)
{
    size_t zeros[NC_MAX_VAR_DIMS] = {0};
    size_t ones[NC_MAX_VAR_DIMS];
    int ndims, i, ret;

    /* One element is a hyperslab starting at indexp with a count of 1
     * in every dimension. */
    if ((ret = nc_inq_varndims(ncid, varid, &ndims))) return ret;
    if (ndims > NC_MAX_VAR_DIMS) return NC_EINVAL;
    for (i = 0; i < ndims; i++)
        ones[i] = 1;
    return bf16_io(ncid, varid, indexp ? indexp : zeros, ones, NULL, ip, 0);
}

/**
 * Write one value to a variable as bfloat16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_var1_bfloat16(int ncid, int varid, const size_t *indexp,
                     const nc_bfloat16_t *op)
{
    size_t zeros[NC_MAX_VAR_DIMS] = {0};
    size_t ones[NC_MAX_VAR_DIMS];
    int ndims, i, ret;

    /* One element is a hyperslab starting at indexp with a count of 1
     * in every dimension. */
    if ((ret = nc_inq_varndims(ncid, varid, &ndims))) return ret;
    if (ndims > NC_MAX_VAR_DIMS) return NC_EINVAL;
    for (i = 0; i < ndims; i++)
        ones[i] = 1;
    return bf16_io(ncid, varid, indexp ? indexp : zeros, ones, NULL,
                   (void *)op, 1);
}

/**
 * Read a whole variable as bfloat16. See nep.h for the variable types
 * accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_var_bfloat16(int ncid, int varid, nc_bfloat16_t *ip)
{
    size_t start[NC_MAX_VAR_DIMS] = {0};
    size_t count[NC_MAX_VAR_DIMS];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, i, ret;

    /* The whole variable is a hyperslab from the origin with each
     * dimension's current length (for an unlimited dimension, the
     * number of records written so far). */
    if ((ret = nc_inq_varndims(ncid, varid, &ndims))) return ret;
    if (ndims > NC_MAX_VAR_DIMS) return NC_EINVAL;
    if ((ret = nc_inq_vardimid(ncid, varid, dimids))) return ret;
    for (i = 0; i < ndims; i++)
        if ((ret = nc_inq_dimlen(ncid, dimids[i], &count[i]))) return ret;
    return bf16_io(ncid, varid, start, count, NULL, ip, 0);
}

/**
 * Write a whole variable from bfloat16 values. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_var_bfloat16(int ncid, int varid, const nc_bfloat16_t *op)
{
    size_t start[NC_MAX_VAR_DIMS] = {0};
    size_t count[NC_MAX_VAR_DIMS];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, i, ret;

    /* The whole variable is a hyperslab from the origin with each
     * dimension's current length (for an unlimited dimension, the
     * number of records written so far). */
    if ((ret = nc_inq_varndims(ncid, varid, &ndims))) return ret;
    if (ndims > NC_MAX_VAR_DIMS) return NC_EINVAL;
    if ((ret = nc_inq_vardimid(ncid, varid, dimids))) return ret;
    for (i = 0; i < ndims; i++)
        if ((ret = nc_inq_dimlen(ncid, dimids[i], &count[i]))) return ret;
    return bf16_io(ncid, varid, start, count, NULL, (void *)op, 1);
}

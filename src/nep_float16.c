/**
 * @file
 * This is the code file for the NEP IEEE 754 float16 I/O functions.
 *
 * The nc_*_float16 functions exchange binary16 values in memory with
 * NC_FLOAT16, NC_FLOAT and NC_DOUBLE variables. They are written on
 * top of the public netCDF API, so they work with any file netCDF can
 * open. Buffers are always accessed as bytes through memcpy, never as
 * nc_float16_t values, so the code is the same whether nc_float16_t
 * is the compiler's _Float16 or uint16_t.
 *
 * @author Edward Hartnett
 * @date 2026-11-20
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "nep.h"
#include <stdlib.h>
#include <string.h>

/**
 * Convert a float to an IEEE 754 binary16 bit pattern, rounding to
 * nearest with ties to even. NaN stays a quiet NaN.
 *
 * @param f The float to convert.
 * @param range Set to 1 when a finite input becomes +-inf; otherwise
 * left unchanged.
 *
 * @return The float16 bit pattern.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static uint16_t
f32_to_f16(float f, int *range)
{
    uint32_t u, mag, sig, mant, rem;
    uint16_t sign;
    int e, shift;

    /* Work on the float's bits: sign, biased exponent, 23-bit
     * significand. binary16 keeps the sign, rebases the exponent
     * by -112 and rounds the significand to 10 bits. */
    memcpy(&u, &f, sizeof(u));
    mag = u & 0x7fffffffu;
    sign = (uint16_t)((u >> 16) & 0x8000u);

    /* NaN: keep the top payload bits and force the quiet bit, so
     * truncating the payload can't turn the NaN into infinity. */
    if (mag > 0x7f800000u)
        return (uint16_t)(sign | 0x7c00u | ((u >> 13) & 0x03ffu) | 0x0200u);

    /* Infinity stays infinity and is not a range error. */
    if (mag == 0x7f800000u)
        return (uint16_t)(sign | 0x7c00u);

    /* Finite magnitudes of 65520 or more are past the rounding
     * boundary and overflow to infinity. (The tie at 65520 rounds to
     * even, which is infinity; the odd candidate 0x7bff loses.) */
    if (mag >= 0x477ff000u) {   /* 65520.0f */
        *range = 1;
        return (uint16_t)(sign | 0x7c00u);
    }

    e = (int)(mag >> 23);

    /* Normal result: rebase the exponent and round the significand to
     * 10 bits, ties to even. A carry out of the mantissa ripples into
     * the exponent, which is correct (and at the top gives 0x7c00). */
    if (e >= 113) {
        uint32_t m = mag & 0x7fffffu;
        uint32_t rounded = (m + 0x0fffu + ((m >> 13) & 1u)) >> 13;

        return (uint16_t)(sign | (uint16_t)(((e - 112) << 10) + rounded));
    }

    /* Subnormal result (or zero): the significand drops below the
     * normal range. Very small inputs (shift of 32 or more, including
     * float subnormals) round to signed zero. */
    shift = 126 - e;
    if (shift >= 32)
        return sign;
    sig = 0x800000u | (mag & 0x7fffffu);
    mant = sig >> shift;
    rem = sig & ((1u << shift) - 1u);
    /* Round to nearest, ties to even. The result may carry to 1024,
     * which is exactly the smallest normal binary16. */
    if (rem > (1u << (shift - 1)) || (rem == (1u << (shift - 1)) && (mant & 1u)))
        mant++;
    return (uint16_t)(sign | (uint16_t)mant);
}

/**
 * Convert a double to an IEEE 754 binary16 bit pattern with a single
 * rounding (to nearest, ties to even). The double is first rounded to
 * float with round-to-odd (an inexact result gets an odd last bit),
 * then converted with f32_to_f16(). float has 13 more significand
 * bits than binary16, so this equals rounding the double directly.
 *
 * @param d The double to convert.
 * @param range Set to 1 when a finite input becomes +-inf; otherwise
 * left unchanged.
 *
 * @return The float16 bit pattern.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static uint16_t
f64_to_f16(double d, int *range)
{
    /* Default C conversion: round to nearest float, ties to even. */
    float f = (float)d;
    uint32_t u;

    /* d is finite (d == d excludes NaN, d - d == 0 excludes inf) but
     * too big for float, so f is inf (f - f is NaN). Every finite
     * value past the float range is also past the binary16 range, so
     * this is a range error. */
    if (d == d && f - f != 0.0f && d - d == 0.0) {
        *range = 1;
        return d < 0 ? 0xfc00u : 0x7c00u;
    }

    /* Two roundings (double to float, then float to binary16) can
     * give the wrong answer when the first one lands exactly on a
     * binary16 tie. Avoid that with round-to-odd: if the float is
     * inexact and its last bit is 0, step it one ulp toward d. An odd
     * last bit means it can never look like an exact tie, and the
     * step can't cross a binary16 rounding boundary. Exact results
     * and NaN are left alone. */
    if (d == d && (double)f != d) {
        double ad = d < 0 ? -d : d, af = f < 0 ? -f : f;

        memcpy(&u, &f, sizeof(u));
        if (!(u & 1u))
            u = af < ad ? u + 1u : u - 1u;   /* toward d in magnitude */
        memcpy(&f, &u, sizeof(f));
    }

    /* The second, final rounding to binary16. */
    return f32_to_f16(f, range);
}

/**
 * Convert an IEEE 754 binary16 bit pattern to float. The conversion
 * is exact.
 *
 * @param h The float16 bit pattern.
 *
 * @return The float value.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static float
f16_to_f32(uint16_t h)
{
    uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
    uint32_t e = (h >> 10) & 0x1fu;
    uint32_t m = h & 0x03ffu;
    uint32_t u;
    float f;

    if (e == 0) {
        if (!m) {
            u = sign;                    /* +-0 */
        } else {
            /* Subnormal: shift the leading bit into position 10 and
             * count the shifts off the exponent. */
            e = 1;
            while (!(m & 0x400u)) { m <<= 1; e--; }
            m &= 0x3ffu;
            u = sign | ((uint32_t)(e + 112) << 23) | (m << 13);
        }
    } else if (e == 31) {
        u = sign | 0x7f800000u | (m << 13);   /* inf or NaN */
    } else {
        u = sign | ((e + 112u) << 23) | (m << 13);
    }
    memcpy(&f, &u, sizeof(f));
    return f;
}

/**
 * Common worker for the eight nc_*_float16 functions. NC_FLOAT16
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
 * @param buf The float16 buffer (read into for a get, read from for a
 * put).
 * @param put Non-zero to write, zero to read.
 *
 * @return NC_NOERR No error.
 * @return NC_ERANGE A finite value overflowed to +-inf on a get; all
 * values are still converted.
 * @return NC_ECHAR The variable is NC_CHAR.
 * @return NC_EBADTYPE The variable is not NC_FLOAT16, NC_FLOAT or
 * NC_DOUBLE.
 * @return NC_EINVAL NULL start, count or buffer where one is needed.
 * @return NC_ENOMEM Out of memory.
 * @return Other errors from the underlying netCDF calls.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static int
f16_io(int ncid, int varid, const size_t *startp, const size_t *countp,
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
    if (xtype != NC_FLOAT16 && xtype != NC_FLOAT && xtype != NC_DOUBLE)
        return NC_EBADTYPE;

    /* The memory and file types are both float16, so the bits pass
     * straight through the untyped calls with no conversion. */
    if (xtype == NC_FLOAT16)
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
         * binary16. Every value is converted even after a range
         * error, as netCDF's typed calls do. */
        if (xtype == NC_FLOAT) {
            float *tp = (float *)tmp;
            ret = nc_get_vars_float(ncid, varid, startp, countp, stridep, tp);
            if (!ret)
                for (j = 0; j < n; j++) {
                    uint16_t h = f32_to_f16(tp[j], &range);
                    memcpy(bp + j * 2, &h, 2);
                }
        } else {
            double *tp = (double *)tmp;
            ret = nc_get_vars_double(ncid, varid, startp, countp, stridep, tp);
            if (!ret)
                for (j = 0; j < n; j++) {
                    uint16_t h = f64_to_f16(tp[j], &range);
                    memcpy(bp + j * 2, &h, 2);
                }
        }
        free(tmp);
        if (ret) return ret;
        return range ? NC_ERANGE : NC_NOERR;
    }

    /* Write: widen each binary16 value to the file's type, which is
     * always exact, then write with the matching typed call. */
    if (xtype == NC_FLOAT) {
        float *tp = (float *)tmp;
        for (j = 0; j < n; j++) {
            uint16_t h;
            memcpy(&h, bp + j * 2, 2);
            tp[j] = f16_to_f32(h);
        }
        ret = nc_put_vars_float(ncid, varid, startp, countp, stridep, tp);
    } else {
        double *tp = (double *)tmp;
        for (j = 0; j < n; j++) {
            uint16_t h;
            memcpy(&h, bp + j * 2, 2);
            tp[j] = (double)f16_to_f32(h);
        }
        ret = nc_put_vars_double(ncid, varid, startp, countp, stridep, tp);
    }
    free(tmp);
    return ret;
}

/**
 * Read a strided hyperslab of a variable as float16. See nep.h for
 * the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_vars_float16(int ncid, int varid, const size_t *startp,
                    const size_t *countp, const ptrdiff_t *stridep,
                    nc_float16_t *ip)
{
    return f16_io(ncid, varid, startp, countp, stridep, ip, 0);
}

/**
 * Write a strided hyperslab of a variable from float16 values. See
 * nep.h for the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_vars_float16(int ncid, int varid, const size_t *startp,
                    const size_t *countp, const ptrdiff_t *stridep,
                    const nc_float16_t *op)
{
    /* f16_io only reads the buffer when put is set, so casting away
     * const is safe. */
    return f16_io(ncid, varid, startp, countp, stridep, (void *)op, 1);
}

/**
 * Read a hyperslab of a variable as float16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_vara_float16(int ncid, int varid, const size_t *startp,
                    const size_t *countp, nc_float16_t *ip)
{
    /* A vara call is a vars call with stride 1 (NULL). */
    return f16_io(ncid, varid, startp, countp, NULL, ip, 0);
}

/**
 * Write a hyperslab of a variable from float16 values. See nep.h for
 * the variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_vara_float16(int ncid, int varid, const size_t *startp,
                    const size_t *countp, const nc_float16_t *op)
{
    /* A vara call is a vars call with stride 1 (NULL). */
    return f16_io(ncid, varid, startp, countp, NULL, (void *)op, 1);
}

/**
 * Read one value from a variable as float16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_var1_float16(int ncid, int varid, const size_t *indexp,
                    nc_float16_t *ip)
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
    return f16_io(ncid, varid, indexp ? indexp : zeros, ones, NULL, ip, 0);
}

/**
 * Write one value to a variable as float16. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_var1_float16(int ncid, int varid, const size_t *indexp,
                    const nc_float16_t *op)
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
    return f16_io(ncid, varid, indexp ? indexp : zeros, ones, NULL,
                  (void *)op, 1);
}

/**
 * Read a whole variable as float16. See nep.h for the variable types
 * accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_get_var_float16(int ncid, int varid, nc_float16_t *ip)
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
    return f16_io(ncid, varid, start, count, NULL, ip, 0);
}

/**
 * Write a whole variable from float16 values. See nep.h for the
 * variable types accepted and the conversion rules.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nc_put_var_float16(int ncid, int varid, const nc_float16_t *op)
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
    return f16_io(ncid, varid, start, count, NULL, (void *)op, 1);
}

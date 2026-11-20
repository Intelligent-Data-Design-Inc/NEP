/**
 * @file float16_conv.h
 * @brief Conversions between IEEE 754 float32 and binary16 bit patterns.
 *
 * NEXTCDF-4 stores NC_FLOAT16 variables as raw 16-bit IEEE 754
 * binary16 bit patterns; conversion to and from float32 is done in
 * user code. This header provides the two conversions used by the
 * NEXTCDF-4 float16 examples.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#ifndef FLOAT16_CONV_H
#define FLOAT16_CONV_H

#include <stdint.h>
#include <string.h>

/** GDAS input fill value; points equal to it are left out of error metrics. */
#define F16_FILL_IN 9.99e+20f

/** Maximum relative error for binary16 round-to-nearest (2^-11). */
#define F16_REL_ERR_MAX 0.00048828125

/** Absolute-error floor: half the smallest binary16 subnormal (2^-25).
 * A point also passes when its absolute error is under this floor, so
 * subnormal underflow does not fail the gate. */
#define F16_ABS_ERR_MAX 2.9802322387695312e-08

/**
 * @brief Convert a float32 to an IEEE 754 binary16 bit pattern, round
 * to nearest, ties to even.
 *
 * NaN stays a quiet NaN. Finite magnitudes of 65520 or more overflow
 * to infinity; magnitudes in (65504, 65520) round to the largest
 * finite value 0x7bff. Results below 2^-14 become subnormals or
 * signed zero.
 *
 * @param f The float32 value to convert.
 * @return The binary16 bit pattern in the low 16 bits of a uint16_t.
 */
static inline uint16_t
float_to_float16(float f)
{
    uint32_t u, mag, sig, mant, rem;
    uint16_t sign;
    int e, shift;

    memcpy(&u, &f, sizeof(u));
    mag = u & 0x7fffffffu;
    sign = (uint16_t)((u >> 16) & 0x8000u);

    /* NaN: keep the top payload bits and force the quiet bit. */
    if (mag > 0x7f800000u)
        return (uint16_t)(sign | 0x7c00u | ((u >> 13) & 0x03ffu) | 0x0200u);

    /* Infinity stays infinity. */
    if (mag == 0x7f800000u)
        return (uint16_t)(sign | 0x7c00u);

    /* Finite magnitudes of 65520 or more overflow to infinity (the
     * tie at 65520 rounds to even, which is infinity). */
    if (mag >= 0x477ff000u)     /* 65520.0f */
        return (uint16_t)(sign | 0x7c00u);

    e = (int)(mag >> 23);

    /* Normal result: rebase the exponent and round the significand to
     * 10 bits, ties to even. A carry out of the mantissa ripples into
     * the exponent, which is correct. */
    if (e >= 113) {
        uint32_t m = mag & 0x7fffffu;
        uint32_t rounded = (m + 0x0fffu + ((m >> 13) & 1u)) >> 13;

        return (uint16_t)(sign | (uint16_t)(((e - 112) << 10) + rounded));
    }

    /* Subnormal result or signed zero. */
    shift = 126 - e;
    if (shift >= 32)
        return sign;
    sig = 0x800000u | (mag & 0x7fffffu);
    mant = sig >> shift;
    rem = sig & ((1u << shift) - 1u);
    if (rem > (1u << (shift - 1)) || (rem == (1u << (shift - 1)) && (mant & 1u)))
        mant++;
    return (uint16_t)(sign | (uint16_t)mant);
}

/**
 * @brief Convert an IEEE 754 binary16 bit pattern to float32 (exact).
 *
 * @param h The binary16 bit pattern.
 * @return The exact float32 value.
 */
static inline float
float16_to_float(uint16_t h)
{
    uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
    uint32_t e = (h >> 10) & 0x1fu;
    uint32_t m = h & 0x03ffu;
    uint32_t u;
    float f;

    if (e == 0) {
        if (!m) {
            u = sign;
        } else {
            e = 1;
            while (!(m & 0x400u)) { m <<= 1; e--; }
            m &= 0x3ffu;
            u = sign | ((uint32_t)(e + 112) << 23) | (m << 13);
        }
    } else if (e == 31) {
        u = sign | 0x7f800000u | (m << 13);
    } else {
        u = sign | ((e + 112u) << 23) | (m << 13);
    }
    memcpy(&f, &u, sizeof(f));
    return f;
}

#endif /* FLOAT16_CONV_H */

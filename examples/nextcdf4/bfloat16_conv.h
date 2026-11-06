/**
 * @file bfloat16_conv.h
 * @brief Conversions between IEEE 754 float32 and bfloat16 bit patterns.
 *
 * NEXTCDF-4 stores NC_BFLOAT16 variables as raw 16-bit bfloat16 bit
 * patterns; conversion to and from float32 is done in user code. This
 * header provides the two conversions used by the NEXTCDF-4 bfloat16
 * examples.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#ifndef BFLOAT16_CONV_H
#define BFLOAT16_CONV_H

#include <stdint.h>
#include <string.h>

/** GDAS input fill value; points equal to it are left out of error metrics. */
#define BF16_FILL_IN 9.99e+20f

/** Maximum relative error for bfloat16 round-to-nearest (2^-8). */
#define BF16_REL_ERR_MAX 0.00390625

/**
 * @brief Convert a float32 to a bfloat16 bit pattern, round to nearest,
 * ties to even.
 *
 * NaN stays a quiet NaN. Values that round past the bfloat16 maximum
 * overflow to infinity, which is correct.
 *
 * @param f The float32 value to convert.
 * @return The bfloat16 bit pattern in the low 16 bits of a uint16_t.
 */
static inline uint16_t
float_to_bfloat16(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof(u));
    if ((u & 0x7fffffffu) > 0x7f800000u)
        return (uint16_t)((u >> 16) | 0x0040u);
    return (uint16_t)((u + 0x7fffu + ((u >> 16) & 1u)) >> 16);
}

/**
 * @brief Convert a bfloat16 bit pattern to float32 (exact).
 *
 * @param b The bfloat16 bit pattern.
 * @return The exact float32 value.
 */
static inline float
bfloat16_to_float(uint16_t b)
{
    uint32_t u = (uint32_t)b << 16;
    float f;

    memcpy(&f, &u, sizeof(f));
    return f;
}

#endif /* BFLOAT16_CONV_H */

/**
 * @file nxt4convert.c
 * @brief Internal numeric conversion and quantization for NEXTCDF-4.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include "config.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nxt4internal.h"

/**
 * @brief Test whether a type is a standard numeric atomic type.
 *
 * "Standard numeric" means the classic netCDF numeric types
 * (`NC_BYTE` through `NC_DOUBLE`, including the unsigned set) — the
 * ones nx4_convert_type() handles itself. `NC_CHAR`, `NC_STRING`,
 * small floats, bitfields, references and user-defined types are
 * excluded.
 *
 * @param t NetCDF type.
 * @return Nonzero for a standard numeric type.
 */
static int
numeric(nc_type t)
{
    return t == NC_BYTE || (t >= NC_SHORT && t <= NC_DOUBLE) ||
        (t >= NC_UBYTE && t <= NC_UINT64);
}

/**
 * @brief Encode a `float` as bfloat16 with round-to-nearest-even.
 *
 * bfloat16 is the top 16 bits of an IEEE-754 float, so encoding is a
 * right shift — but the dropped bits must round correctly. Adding
 * `0x7fff + lsb` before shifting implements ties-to-even. NaN inputs
 * are preserved as NaN; finite values that overflow to the bfloat16
 * infinity exponent raise @p range.
 *
 * @param f Source value.
 * @param range Receives a range flag on finite overflow.
 * @return Encoded bits.
 */
static uint16_t
encode_bfloat(float f, int *range)
{
    uint32_t u;
    uint16_t b;
    memcpy(&u, &f, sizeof(u));
    if ((u & 0x7fffffffU) > 0x7f800000U)
        return (uint16_t)((u >> 16) | 0x0040U);
    b = (uint16_t)((u + 0x7fffU + ((u >> 16) & 1U)) >> 16);
    if ((b & 0x7fffU) == 0x7f80U && isfinite(f))
        *range = 1;
    return b;
}

/**
 * @brief Encode a `double` as bfloat16 with a single final rounding.
 *
 * A naive `double→float→bfloat16` sequence rounds twice and can land
 * on the wrong representable value (double rounding). To avoid it,
 * when the `float` result is inexact the neighboring float on the
 * correct side of @p d is chosen *before* the final bfloat16 rounding
 * — so only one rounding decides the outcome. Values beyond
 * `FLT_MAX` become bfloat16 infinities with @p range set.
 *
 * @param d Source value.
 * @param range Receives a range flag on finite overflow.
 * @return Encoded bits.
 */
static uint16_t
encode_bfloat_double(double d, int *range)
{
    float f;
    uint32_t u;
    if (isfinite(d) && fabs(d) > FLT_MAX) {
        *range = 1;
        return signbit(d) ? 0xff80U : 0x7f80U;
    }
    f = (float)d;
    if (isfinite(d) && (double)f != d) {
        memcpy(&u, &f, sizeof(u));
        if (!(u & 1U))
            u = fabs((double)f) < fabs(d) ? u + 1U : u - 1U;
        memcpy(&f, &u, sizeof(f));
    }
    return encode_bfloat(f, range);
}

/**
 * @brief Decode an IEEE 754 binary16 bit pattern as `float`.
 *
 * The conversion is exact: subnormals are normalized, infinities and
 * NaN payloads are preserved into the float NaN.
 *
 * @param h Encoded bits.
 * @return Decoded value.
 */
static float
decode_half(uint16_t h)
{
    uint32_t sign = (uint32_t)(h & 0x8000U) << 16;
    uint32_t e = (h >> 10) & 0x1fU;
    uint32_t m = h & 0x03ffU;
    uint32_t u;
    float f;
    if (e == 0) {
        if (!m) {
            u = sign;
        } else {
            e = 1;
            while (!(m & 0x400U)) { m <<= 1; e--; }
            m &= 0x3ffU;
            u = sign | ((uint32_t)(e + 112) << 23) | (m << 13);
        }
    } else if (e == 31) {
        u = sign | 0x7f800000U | (m << 13);
    } else {
        u = sign | ((e + 112U) << 23) | (m << 13);
    }
    memcpy(&f, &u, sizeof(f));
    return f;
}

/**
 * @brief Encode a `float` as IEEE 754 binary16 with
 * round-to-nearest-even.
 *
 * binary16 keeps the sign, rebases the exponent by -112 and rounds
 * the significand to 10 bits. Finite magnitudes of 65520 or more
 * overflow to infinity with @p range set (the tie at 65520 rounds to
 * even, which is infinity); magnitudes in (65504, 65520) round to the
 * largest finite 0x7bff. Results below 2^-14 become subnormals or
 * signed zero; that underflow is not a range error. NaN stays a quiet
 * NaN. Mirrors f32_to_f16() in `src/nep_float16.c`, which is the
 * reference implementation.
 *
 * @param f Source value.
 * @param range Receives a range flag on finite overflow.
 * @return Encoded bits.
 */
static uint16_t
encode_half(float f, int *range)
{
    uint32_t u, mag, sig, mant, rem;
    uint16_t sign;
    int e, shift;
    memcpy(&u, &f, sizeof(u));
    mag = u & 0x7fffffffU;
    sign = (uint16_t)((u >> 16) & 0x8000U);
    if (mag > 0x7f800000U)
        return (uint16_t)(sign | 0x7c00U | ((u >> 13) & 0x03ffU) | 0x0200U);
    if (mag == 0x7f800000U)
        return (uint16_t)(sign | 0x7c00U);
    if (mag >= 0x477ff000U) {   /* 65520.0f */
        *range = 1;
        return (uint16_t)(sign | 0x7c00U);
    }
    e = (int)(mag >> 23);
    if (e >= 113) {
        uint32_t m = mag & 0x7fffffU;
        uint32_t rounded = (m + 0x0fffU + ((m >> 13) & 1U)) >> 13;
        return (uint16_t)(sign | (uint16_t)(((e - 112) << 10) + rounded));
    }
    shift = 126 - e;
    if (shift >= 32)
        return sign;
    sig = 0x800000U | (mag & 0x7fffffU);
    mant = sig >> shift;
    rem = sig & ((1U << shift) - 1U);
    if (rem > (1U << (shift - 1)) || (rem == (1U << (shift - 1)) && (mant & 1U)))
        mant++;
    return (uint16_t)(sign | (uint16_t)mant);
}

/**
 * @brief Encode a `double` as IEEE 754 binary16 with a single final
 * rounding.
 *
 * Same round-to-odd technique as encode_bfloat_double(): when the
 * `float` result is inexact the neighboring float on the correct side
 * of @p d is chosen before the final binary16 rounding. float's
 * 24-bit significand has 13 spare bits over binary16's 11, so this
 * equals rounding the double directly. Values beyond `FLT_MAX` become
 * binary16 infinities with @p range set.
 *
 * @param d Source value.
 * @param range Receives a range flag on finite overflow.
 * @return Encoded bits.
 */
static uint16_t
encode_half_double(double d, int *range)
{
    float f;
    uint32_t u;
    if (isfinite(d) && fabs(d) > FLT_MAX) {
        *range = 1;
        return signbit(d) ? 0xfc00U : 0x7c00U;
    }
    f = (float)d;
    if (isfinite(d) && (double)f != d) {
        memcpy(&u, &f, sizeof(u));
        if (!(u & 1U))
            u = fabs((double)f) < fabs(d) ? u + 1U : u - 1U;
        memcpy(&f, &u, sizeof(f));
    }
    return encode_half(f, range);
}

/**
 * @brief Apply netCDF quantization in place after conversion.
 *
 * Quantization throws away low-order mantissa bits of float/double
 * data to make compression more effective. The three modes:
 * `NC_QUANTIZE_BITGROOM` alternately sets/zeros the tail bits,
 * `NC_QUANTIZE_GRANULARBR` keeps a per-value decimal precision, and
 * `NC_QUANTIZE_BITROUND` keeps a fixed bit count and rounds. Fill
 * values, zeros and non-finite values are left untouched.
 *
 * All bit manipulation goes through `memcpy` to stay legal under
 * strict aliasing — the buffer was written as a real float/double,
 * and reading it as an integer through a cast would be undefined.
 *
 * @param data Numeric destination buffer.
 * @param t Destination type (`NC_FLOAT` or `NC_DOUBLE`).
 * @param n Element count.
 * @param fill Optional fill value.
 * @param mode Validated quantization mode.
 * @param nsd Validated precision.
 */
static void
quantize(void *data, nc_type t, size_t n, const void *fill, int mode, int nsd)
{
    const double bits_per_digit = log(10.0) / log(2.0);
    const double digits_per_bit = log(2.0) / log(10.0);
    int mantissa = t == NC_FLOAT ? 23 : 52;
    size_t size = t == NC_FLOAT ? sizeof(float) : sizeof(double), i;
    double missing = t == NC_FLOAT ? (fill ? *(const float *)fill : NC_FILL_FLOAT)
        : (fill ? *(const double *)fill : NC_FILL_DOUBLE);
    for (i = 0; i < n; i++) {
        unsigned char *p = (unsigned char *)data + i * size;
        double value;
        uint64_t bits = 0, mask;
        int keep = mode == NC_QUANTIZE_BITROUND ? nsd : (int)ceil(nsd * bits_per_digit) + 1;
        int zero;
        if (t == NC_FLOAT) {
            float v;
            uint32_t u;
            memcpy(&v, p, sizeof(v));
            memcpy(&u, p, sizeof(u));
            value = v;
            bits = u;
        } else {
            memcpy(&value, p, sizeof(value));
            memcpy(&bits, p, sizeof(bits));
        }
        if (value == missing || value == 0 || !isfinite(value))
            continue;
        if (mode == NC_QUANTIZE_GRANULARBR) {
            int exponent, digits, power;
            double fraction = fabs(frexp(value, &exponent));
            double log_fraction = log10(fraction);
            digits = (int)floor(exponent * digits_per_bit + log_fraction) + 1;
            power = (int)floor(bits_per_digit * (digits - nsd));
            keep = abs((int)floor(exponent - bits_per_digit * log_fraction) - power) - 1;
        }
        zero = mantissa - keep;
        if (zero <= 0)
            continue;
        if (zero > mantissa)
            zero = mantissa;
        mask = UINT64_MAX << zero;
        if (mode == NC_QUANTIZE_BITGROOM)
            bits = i & 1 ? bits | ~mask : bits & mask;
        else
            bits = (bits + (UINT64_C(1) << (zero - 1))) & mask;
        if (t == NC_FLOAT) {
            uint32_t u = (uint32_t)bits;
            memcpy(p, &u, sizeof(u));
        } else
            memcpy(p, &bits, sizeof(bits));
    }
}

/**
 * @brief Check integer range without passing 64-bit boundaries
 * through floating point.
 *
 * Comparing a `long long` to 2^63 via floating point is imprecise
 * near the boundary (exactly the values that matter for `NC_UINT64`
 * and `NC_INT64`), so integer-to-integer conversions are range-checked
 * entirely in integer arithmetic: unsigned sources go through @p u,
 * signed through @p s.
 *
 * @param source Source type.
 * @param target Integer destination type.
 * @param u Source converted to unsigned long long, used for unsigned
 * sources.
 * @param s Source converted to long long, used for signed sources.
 * @return Nonzero when outside the destination range.
 */
static int
integer_range(nc_type source, nc_type target, unsigned long long u, long long s)
{
    long long minimum;
    unsigned long long maximum;
    switch (target) {
    case NC_BYTE: minimum = SCHAR_MIN; maximum = SCHAR_MAX; break;
    case NC_UBYTE: minimum = 0; maximum = UCHAR_MAX; break;
    case NC_SHORT: minimum = SHRT_MIN; maximum = SHRT_MAX; break;
    case NC_USHORT: minimum = 0; maximum = USHRT_MAX; break;
    case NC_INT: minimum = INT_MIN; maximum = INT_MAX; break;
    case NC_UINT: minimum = 0; maximum = UINT_MAX; break;
    case NC_INT64: minimum = LLONG_MIN; maximum = LLONG_MAX; break;
    default: minimum = 0; maximum = ULLONG_MAX; break;
    }
    if (source == NC_UBYTE || source == NC_USHORT || source == NC_UINT || source == NC_UINT64)
        return u > maximum;
    return s < minimum || (s >= 0 && (unsigned long long)s > maximum);
}

/**
 * @brief Macro: expand a loop converting one source C type to every
 * standard destination type.
 *
 * Used inside nx4_convert_type()'s `switch (src_type)`; generates the
 * per-destination loops, including `DEST_INT` cases with range
 * checking and `NC_FLOAT`/`NC_DOUBLE` cases with overflow detection.
 *
 * @param T Source C type.
 * @param REAL Whether the source is floating point.
 */
#define CONVERT_SOURCE(T, REAL) do { \
    const T *input = src; \
    const int real_source = REAL; \
    size_t j; \
    switch (dest_type) { \
    DEST_INT(NC_BYTE, signed char, -0x1p7L, 0x1p7L, NC_FILL_BYTE) \
    DEST_INT(NC_UBYTE, unsigned char, 0, 0x1p8L, NC_FILL_UBYTE) \
    DEST_INT(NC_SHORT, short, -0x1p15L, 0x1p15L, NC_FILL_SHORT) \
    DEST_INT(NC_USHORT, unsigned short, 0, 0x1p16L, NC_FILL_USHORT) \
    DEST_INT(NC_INT, int, -0x1p31L, 0x1p31L, NC_FILL_INT) \
    DEST_INT(NC_UINT, unsigned int, 0, 0x1p32L, NC_FILL_UINT) \
    DEST_INT(NC_INT64, long long, -0x1p63L, 0x1p63L, NC_FILL_INT64) \
    DEST_INT(NC_UINT64, unsigned long long, 0, 0x1p64L, NC_FILL_UINT64) \
    case NC_FLOAT: \
        for (j = 0; j < len; j++) { \
            long double v = (long double)input[j]; \
            if (isfinite(v) && fabsl(v) > FLT_MAX) { \
                *range_error = 1; \
                ((float *)dest)[j] = signbit(v) ? -INFINITY : INFINITY; \
            } else ((float *)dest)[j] = (float)input[j]; \
        } \
        break; \
    case NC_DOUBLE: \
        for (j = 0; j < len; j++) ((double *)dest)[j] = (double)input[j]; \
        break; \
    } \
} while (0)

/**
 * @brief Macro: one `case` of CONVERT_SOURCE writing an integer
 * destination with guarded conversion.
 *
 * Real sources are truncated and checked against `LOW`/`HIGH` in
 * `long double`; integer sources are checked by integer_range().
 * Out-of-range values set `*range_error` and store the fill value —
 * except the classic `NC_UBYTE`→`NC_BYTE` overflow, which netcdf-c
 * historically tolerates under `_NCProperties` strict-nc3 semantics.
 *
 * @param ID NetCDF destination type.
 * @param D Destination C type.
 * @param LOW Inclusive lower bound.
 * @param HIGH Exclusive upper bound.
 * @param FILL Default fill value.
 */
#define DEST_INT(ID, D, LOW, HIGH, FILL) \
    case ID: \
        for (j = 0; j < len; j++) { \
            long double v = (long double)input[j]; \
            int outside; \
            if (real_source) v = truncl(v); \
            outside = real_source ? (!isfinite(v) || v < (LOW) || v >= (HIGH)) \
                : integer_range(src_type, ID, (unsigned long long)input[j], (long long)input[j]); \
            if (outside && !(strict_nc3 && src_type == NC_UBYTE && ID == NC_BYTE)) \
                *range_error = 1; \
            ((D *)dest)[j] = outside && real_source \
                ? (fill_value ? *(const D *)fill_value : (D)(FILL)) : (D)input[j]; \
        } \
        break;

/**
 * @brief NEP's internal numeric conversion engine for NEXTCDF-4.
 *
 * Converts between standard numeric atomic types, and between
 * `NC_BFLOAT16`/`NC_FLOAT16` storage and `NC_FLOAT`/`NC_DOUBLE`
 * buffers. Modeled on netcdf-c's `libsrc4/nc4var.c` conversion but
 * owned by NEP so behavior (in particular `NC_ERANGE` reporting,
 * issue #381) is consistent across all I/O paths: variable I/O,
 * attributes and quantized writes.
 *
 * Three strategies: standard numerics convert elementwise in C with
 * exact range checks; `NC_FLOAT16` pairs use decode_half() and
 * encode_half()/encode_half_double(); `NC_BFLOAT16` pairs use
 * encode_bfloat() with round-to-nearest-even. Both 16-bit types
 * decode exactly and narrow with round-to-nearest-even, single
 * rounding from double — HDF5 performs no application numeric
 * conversion. Quantization is applied to the converted output last.
 * Callers may probe with NULL buffers to validate a conversion before
 * allocating.
 *
 * @param src Source buffer (may be NULL for a validation-only probe).
 * @param dest Destination buffer (may be NULL for a probe).
 * @param src_type Resolved source type.
 * @param dest_type Resolved destination type.
 * @param len Element count.
 * @param range_error Required output range flag.
 * @param fill_value Optional destination fill.
 * @param strict_nc3 Classic byte-range convention.
 * @param quantize_mode Quantization mode.
 * @param nsd Quantization precision.
 * @return NC_NOERR or a type, argument, allocation or HDF5 error;
 * overflow is reported via @p range_error, not the return value.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nx4_convert_type(const void *src, void *dest, nc_type src_type,
                 nc_type dest_type, size_t len, int *range_error,
                 const void *fill_value, int strict_nc3, int quantize_mode, int nsd)
{
    size_t src_size, dest_size, i;
    int standard = numeric(src_type) && numeric(dest_type);
    int bf = (src_type == NC_BFLOAT16 && (dest_type == NC_FLOAT || dest_type == NC_DOUBLE)) ||
        (dest_type == NC_BFLOAT16 && (src_type == NC_FLOAT || src_type == NC_DOUBLE));
    int half = (src_type == NC_FLOAT16 && (dest_type == NC_FLOAT || dest_type == NC_DOUBLE)) ||
        (dest_type == NC_FLOAT16 && (src_type == NC_FLOAT || src_type == NC_DOUBLE));
    /* Validate argument and type combination first, then dispatch on
     * the conversion kind: identical types memcpy, the 16-bit floats
     * use the RNE encoders/decoders, everything else
     * uses the generated per-source loops. Quantization runs last. */
    if (!range_error) return NC_EINVAL;
    *range_error = 0;
    if ((src_type == NC_CHAR) != (dest_type == NC_CHAR)) return NC_ECHAR;
    if (!standard && !bf && !half &&
        !(src_type == dest_type && (src_type == NC_CHAR || src_type == NC_BFLOAT16 || src_type == NC_FLOAT16)))
        return NC_EBADTYPE;
    if (quantize_mode != NC_NOQUANTIZE &&
        ((dest_type != NC_FLOAT && dest_type != NC_DOUBLE) ||
         quantize_mode < NC_QUANTIZE_BITGROOM || quantize_mode > NC_QUANTIZE_BITROUND ||
         nsd < 1 || nsd > (quantize_mode == NC_QUANTIZE_BITROUND ?
                           (dest_type == NC_FLOAT ? 23 : 52) : (dest_type == NC_FLOAT ? 6 : 15))))
        return NC_EINVAL;
    if (NEXTCDF4_type_size(src_type, &src_size) || NEXTCDF4_type_size(dest_type, &dest_size))
        return NC_EBADTYPE;
    if (len > SIZE_MAX / src_size || len > SIZE_MAX / dest_size) return NC_EINVAL;
    if (!len) return NC_NOERR;
    if (!src || !dest) return NC_EINVAL;
    if (src_type == dest_type) {
        memcpy(dest, src, len * src_size);
    } else if (half || bf) {
        for (i = 0; i < len; i++) {
            uint16_t bits;
            if (src_type == NC_FLOAT16 || src_type == NC_BFLOAT16) {
                uint32_t u;
                float f;
                memcpy(&bits, (const unsigned char *)src + i * 2, 2);
                if (src_type == NC_FLOAT16) {
                    f = decode_half(bits);
                } else {
                    u = (uint32_t)bits << 16;
                    memcpy(&f, &u, sizeof(f));
                }
                if (dest_type == NC_FLOAT) ((float *)dest)[i] = f;
                else ((double *)dest)[i] = f;
            } else {
                if (dest_type == NC_FLOAT16)
                    bits = src_type == NC_FLOAT ? encode_half(((const float *)src)[i], range_error)
                        : encode_half_double(((const double *)src)[i], range_error);
                else
                    bits = src_type == NC_FLOAT ? encode_bfloat(((const float *)src)[i], range_error)
                        : encode_bfloat_double(((const double *)src)[i], range_error);
                memcpy((unsigned char *)dest + i * 2, &bits, 2);
            }
        }
    } else {
        switch (src_type) {
        case NC_BYTE: CONVERT_SOURCE(signed char, 0); break;
        case NC_UBYTE: CONVERT_SOURCE(unsigned char, 0); break;
        case NC_SHORT: CONVERT_SOURCE(short, 0); break;
        case NC_USHORT: CONVERT_SOURCE(unsigned short, 0); break;
        case NC_INT: CONVERT_SOURCE(int, 0); break;
        case NC_UINT: CONVERT_SOURCE(unsigned int, 0); break;
        case NC_INT64: CONVERT_SOURCE(long long, 0); break;
        case NC_UINT64: CONVERT_SOURCE(unsigned long long, 0); break;
        case NC_FLOAT: CONVERT_SOURCE(float, 1); break;
        case NC_DOUBLE: CONVERT_SOURCE(double, 1); break;
        }
    }
    if (quantize_mode != NC_NOQUANTIZE)
        quantize(dest, dest_type, len, fill_value, quantize_mode, nsd);
    return NC_NOERR;
}

/**
 * @file nxt4convert.c
 * @brief Experimental numeric conversion and quantization for NEXTCDF-4.
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

/** Observe conversion exceptions without changing HDF5's result.
 * @param exception Conversion exception.
 * @param src_id Source datatype.
 * @param dst_id Destination datatype.
 * @param src_buf Source value.
 * @param dst_buf Destination value.
 * @param user_data Per-operation boolean range flag.
 * @return H5T_CONV_UNHANDLED to retain HDF5 conversion.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
static H5T_conv_ret_t
nx4_conversion_exception(H5T_conv_except_t exception, hid_t src_id,
                         hid_t dst_id, void *src_buf, void *dst_buf,
                         void *user_data)
{
    int range = exception == H5T_CONV_EXCEPT_RANGE_HI ||
        exception == H5T_CONV_EXCEPT_RANGE_LOW;
    H5T_class_t target = H5Tget_class(dst_id);
    (void)dst_buf;
    if (target == H5T_INTEGER &&
        (exception == H5T_CONV_EXCEPT_NAN || exception == H5T_CONV_EXCEPT_PINF ||
         exception == H5T_CONV_EXCEPT_NINF))
        range = 1;
    if (range && H5Tget_class(src_id) == H5T_FLOAT) {
        size_t size = H5Tget_size(src_id), i;
        unsigned char bytes[sizeof(double)];
        long double value;
        if (size == sizeof(float) || size == sizeof(double)) {
            int swap = H5Tget_order(src_id) != H5Tget_order(H5T_NATIVE_DOUBLE);
            for (i = 0; i < size; i++)
                bytes[i] = ((const unsigned char *)src_buf)[swap ? size - i - 1 : i];
            if (size == sizeof(float)) {
                float f;
                memcpy(&f, bytes, size);
                value = f;
            } else {
                double d;
                memcpy(&d, bytes, size);
                value = d;
            }
            if (target == H5T_INTEGER) {
                size_t bits = H5Tget_precision(dst_id);
                int sign = H5Tget_sign(dst_id) == H5T_SGN_2;
                long double limit = ldexpl(1.0L, (int)bits - sign);
                value = truncl(value);
                range = !isfinite(value) || value < (sign ? -limit : 0) || value >= limit;
            } else
                range = isfinite(value) && fabsl(value) >= 1;
        }
    }
    if (range) *(int *)user_data = 1;
    return H5T_CONV_UNHANDLED;
}

/** Create an owned transfer property list observing conversion range errors.
 * @param dxplp Receives the property list, or -1 on failure.
 * @param range_error Per-operation flag, reset to zero.
 * @return NC_NOERR or NC_EHDFERR.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
int
nx4_conversion_dxpl(hid_t *dxplp, int *range_error)
{
    *range_error = 0;
    *dxplp = H5Pcreate(H5P_DATASET_XFER);
    if (*dxplp < 0) return NC_EHDFERR;
    if (H5Pset_type_conv_cb(*dxplp, nx4_conversion_exception, range_error) < 0) {
        H5Pclose(*dxplp);
        *dxplp = -1;
        return NC_EHDFERR;
    }
    return NC_NOERR;
}

/** Test whether a type is a standard numeric atomic type.
 * @param t NetCDF type.
 * @return Nonzero for a standard numeric type.
 */
static int
numeric(nc_type t)
{
    return t == NC_BYTE || (t >= NC_SHORT && t <= NC_DOUBLE) ||
        (t >= NC_UBYTE && t <= NC_UINT64);
}

/** Encode float as bfloat16 with ties to even.
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

/** Encode double as bfloat16 with one final rounding.
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

/** Apply NetCDF quantization after conversion, using alias-safe bit access.
 * @param data Numeric destination buffer.
 * @param t Destination type.
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

/** Check integer range without passing 64-bit boundaries through floating point.
 * @param source Source type.
 * @param target Integer destination type.
 * @param u Source converted to unsigned long long, used for unsigned sources.
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

/** Convert one source array to each standard destination type.
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

/** Write integer destinations with guarded real-to-integer conversion.
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

/** Convert supported numeric buffers; see the private header for parameters.
 * @param src Source buffer.
 * @param dest Destination buffer.
 * @param src_type Resolved source type.
 * @param dest_type Resolved destination type.
 * @param len Element count.
 * @param range_error Required output range flag.
 * @param fill_value Optional destination fill.
 * @param strict_nc3 Classic byte-range convention.
 * @param quantize_mode Quantization mode.
 * @param nsd Quantization precision.
 * @return NC_NOERR or a type, argument, allocation or HDF5 error.
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
    } else if (half) {
        int close_src = 0, close_dest = 0, ret = NC_NOERR;
        hid_t dxpl = -1;
        hid_t s = NEXTCDF4_memory_type(src_type, &close_src);
        hid_t d = NEXTCDF4_memory_type(dest_type, &close_dest);
        void *tmp = malloc(len * (src_size > dest_size ? src_size : dest_size));
        if (!tmp) ret = NC_ENOMEM;
        else if (s < 0 || d < 0) ret = NC_EBADTYPE;
        else if (!(ret = nx4_conversion_dxpl(&dxpl, range_error))) {
            memcpy(tmp, src, len * src_size);
            if (H5Tconvert(s, d, len, tmp, NULL, dxpl) < 0) ret = NC_EHDFERR;
            else memcpy(dest, tmp, len * dest_size);
        }
        if (dxpl >= 0) H5Pclose(dxpl);
        free(tmp);
        if (close_src && s >= 0) H5Tclose(s);
        if (close_dest && d >= 0) H5Tclose(d);
        if (ret) return ret;
    } else if (bf) {
        for (i = 0; i < len; i++) {
            uint16_t bits;
            if (src_type == NC_BFLOAT16) {
                uint32_t u;
                float f;
                memcpy(&bits, (const unsigned char *)src + i * 2, 2);
                u = (uint32_t)bits << 16;
                memcpy(&f, &u, sizeof(f));
                if (dest_type == NC_FLOAT) ((float *)dest)[i] = f;
                else ((double *)dest)[i] = f;
            } else {
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

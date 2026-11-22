/* Test the NEXTCDF-4 internal conversion engine.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "nxt4internal.h"
#include "nep_meta.h"

#define CHECK(call) do { int e = (call); if (e) { fprintf(stderr, "%s:%d %s: %s\n", __FILE__, __LINE__, #call, nc_strerror(e)); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define FILE_NAME "tst_nextcdf4_internal_convert.h5"

typedef union {
    unsigned long long align;
    double d[16];
    unsigned char bytes[128];
} buffer;

static int
matrix(void)
{
    const nc_type types[] = {NC_BYTE, NC_UBYTE, NC_SHORT, NC_USHORT, NC_INT,
                             NC_UINT, NC_INT64, NC_UINT64, NC_FLOAT, NC_DOUBLE};
    const double values[] = {0, 1, 7, 31};
    buffer source, target;
    size_t i, j;
    int range;
    for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        CHECK(nx4_convert_type(values, &source, NC_DOUBLE, types[i], 4,
                               &range, NULL, 0, 0, 0, 1));
        for (j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
            double back[4];
            memset(&target, 0x5a, sizeof(target));
            CHECK(nx4_convert_type(&source, &target, types[i], types[j], 4,
                                   &range, NULL, 0, 0, 0, 1));
            REQUIRE(!range);
            CHECK(nx4_convert_type(&target, back, types[j], NC_DOUBLE, 4,
                                   &range, NULL, 0, 0, 0, 1));
            REQUIRE(!memcmp(values, back, sizeof(values)));
        }
    }
    {
        unsigned long long u[] = {ULLONG_MAX, 0, 1ULL << 63}, out[3];
        long long s[] = {LLONG_MIN, LLONG_MAX}, sout[2];
        double bad[] = {NAN, INFINITY, -INFINITY, 0x1p63, -0x1p63, 3.75};
        long long fill = 17, integers[6];
        CHECK(nx4_convert_type(u, out, NC_UINT64, NC_UINT64, 3, &range, NULL, 0, 0, 0, 1));
        REQUIRE(!range && !memcmp(u, out, sizeof(u)));
        CHECK(nx4_convert_type(s, sout, NC_INT64, NC_INT64, 2, &range, NULL, 0, 0, 0, 1));
        REQUIRE(!range && !memcmp(s, sout, sizeof(s)));
        CHECK(nx4_convert_type(bad, integers, NC_DOUBLE, NC_INT64, 6, &range, &fill, 0, 0, 0, 1));
        REQUIRE(range && integers[0] == fill && integers[1] == fill && integers[2] == fill &&
                integers[3] == fill && integers[4] == LLONG_MIN && integers[5] == 3);
    }
    {
        double input[] = {1.00390625, nextafter(1.00390625, INFINITY),
                          nextafter(1.00390625, -INFINITY), -0.0, 1e300};
        uint16_t bits[5];
        CHECK(nx4_convert_type(input, bits, NC_DOUBLE, NC_BFLOAT16, 5, &range, NULL, 0, 0, 0, 1));
        REQUIRE(range && bits[0] == 0x3f80 && bits[1] == 0x3f81 && bits[2] == 0x3f80 &&
                bits[3] == 0x8000 && bits[4] == 0x7f80);
    }
    {
        double input[] = {0, -0.0, 1.234567, -789.1234, NC_FILL_DOUBLE, 123456.75};
        int mode, type;
        for (type = NC_FLOAT; type <= NC_DOUBLE; type++)
            for (mode = NC_QUANTIZE_BITGROOM; mode <= NC_QUANTIZE_BITROUND; mode++) {
                buffer expected, actual;
                size_t size;
                CHECK(NEXTCDF4_type_size(type, &size));
                CHECK(nc4_convert_type(input, &expected, NC_DOUBLE, type, 6, &range, NULL, 0, mode, 3));
                CHECK(nx4_convert_type(input, &actual, NC_DOUBLE, type, 6, &range, NULL, 0, mode, 3, 1));
                REQUIRE(!memcmp(&expected, &actual, 6 * size));
            }
    }
    memset(&target, 0x5a, sizeof(target));
    source = target;
    REQUIRE(nx4_convert_type(&source, &target, NC_INT, NC_CHAR, 1, &range, NULL, 0, 0, 0, 1) == NC_ECHAR);
    REQUIRE(!memcmp(&source, &target, sizeof(source)));
    REQUIRE(nx4_convert_type(NULL, NULL, NC_INT, NC_SHORT, SIZE_MAX, &range, NULL, 0, 0, 0, 1) == NC_EINVAL);
    CHECK(nx4_convert_type(NULL, NULL, NC_DOUBLE, NC_INT, 0, &range, NULL, 0, 0, 0, 1));
    return 0;
}

static int
public_io(int mode)
{
    int ncid, dim, v, scalar, b, ids[2], e;
    size_t start = 0, count = 4, at = 1;
    ptrdiff_t stride = 2;
    double input[] = {1, 2, 3, 4}, output[4], value = 449;
    short raw[4];
    uint16_t bits;
    char *strings[] = {"one", "two"}, *loaded[2];
    const char *filename = FILE_NAME;
    CHECK(nc_create(filename, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid));
    CHECK(nc_def_dim(ncid, "x", 4, &dim));
    CHECK(nc_def_var(ncid, "v", NC_SHORT, 1, &dim, &v));
    CHECK(nc_def_var(ncid, "scalar", NC_FLOAT, 0, NULL, &scalar));
    CHECK(nc_put_att_double(ncid, NC_GLOBAL, "attribute", NC_SHORT, 4, input));
    if (!mode) {
        CHECK(nc_put_att_string(ncid, v, "strings", 2, (const char **)strings));
        CHECK(nc_get_att_string(ncid, v, "strings", loaded));
        REQUIRE(!strcmp(loaded[0], "one") && !strcmp(loaded[1], "two"));
        CHECK(nc_free_string(2, loaded));
    }
    REQUIRE(nc_put_att_text(ncid, NC_GLOBAL, "attribute", 1, "x") == NC_NOERR);
    CHECK(nc_put_att_double(ncid, NC_GLOBAL, "attribute", NC_SHORT, 4, input));
    e = nc_put_att_double(ncid, NC_GLOBAL, "attribute", NC_CHAR, 1, input);
    REQUIRE(e == NC_ECHAR);
    if (!mode) {
        CHECK(nc_def_var(ncid, "b", NC_BFLOAT16, 0, NULL, &b));
        CHECK(nc_put_att_double(ncid, v, "bf", NC_BFLOAT16, 1, &value));
    }
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_var_double(ncid, v, input));
    CHECK(nc_get_var(ncid, v, raw));
    REQUIRE(raw[0] == 1 && raw[3] == 4);
    CHECK(nc_put_var1_double(ncid, v, &at, &input[3]));
    CHECK(nc_get_var1_double(ncid, v, &at, output));
    REQUIRE(output[0] == 4);
    count = 2;
    CHECK(nc_get_vars_double(ncid, v, &start, &count, &stride, output));
    REQUIRE(output[0] == 1 && output[1] == 3);
    CHECK(nc_put_vars_double(ncid, v, &start, &count, &stride, input));
    CHECK(nc_put_var_double(ncid, scalar, &input[2]));
    CHECK(nc_get_var_double(ncid, scalar, output));
    REQUIRE(output[0] == 3);
    if (!mode) {
        CHECK(nc_put_var_double(ncid, b, &value));
        CHECK(nc_get_var(ncid, b, &bits));
        REQUIRE(bits == 0x43e0);
    }
    CHECK(nc_close(ncid));
    CHECK(nc_open(filename, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_get_att_double(ncid, NC_GLOBAL, "attribute", output));
    REQUIRE(!memcmp(input, output, sizeof(input)));
    CHECK(nc_inq_varid(ncid, "v", &ids[0]));
    if (!mode) {
        CHECK(nc_get_att_double(ncid, ids[0], "bf", output));
        REQUIRE(output[0] == 448);
    }
    CHECK(nc_close(ncid));
    unlink(filename);
    return 0;
}

static int
raw_types(void)
{
    const nc_type types[] = {NC_COMPLEX, NC_DOUBLECOMPLEX, NC_BITFIELD8,
        NC_BITFIELD16, NC_BITFIELD32, NC_BITFIELD64, NC_STD_REF};
    int ncid, vars[7];
    size_t i, size;
    buffer source, target;
    char name[16];
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    for (i = 0; i < 7; i++) {
        snprintf(name, sizeof(name), "v%u", (unsigned)i);
        CHECK(nc_def_var(ncid, name, types[i], 0, NULL, &vars[i]));
    }
    CHECK(nc_enddef(ncid));
    for (i = 0; i < 7; i++) {
        memset(&source, i < 6 ? 0x5a : 0, sizeof(source));
        memset(&target, 0, sizeof(target));
        CHECK(NEXTCDF4_type_size(types[i], &size));
        CHECK(nc_put_var(ncid, vars[i], &source));
        CHECK(nc_get_var(ncid, vars[i], &target));
        REQUIRE(!memcmp(&source, &target, size));
    }
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

static int
edge_io(void)
{
    int ncid, dims[2], v, scalar, quantized, range;
    size_t start[2] = {0, 0}, count[2] = {0, 4}, length;
    ptrdiff_t stride[2] = {1, -1};
    double input[] = {1.234567, -7.891234, 0, 42.125}, saved[4], back[4];
    float expected[4], actual[4];
    int mode;
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "record", NC_UNLIMITED, &dims[0]));
    CHECK(nc_def_dim(ncid, "x", 4, &dims[1]));
    CHECK(nc_def_var(ncid, "records", NC_SHORT, 2, dims, &v));
    CHECK(nc_def_var(ncid, "scalar", NC_FLOAT, 0, NULL, &scalar));
    CHECK(nc_def_var(ncid, "quantized", NC_FLOAT, 1, &dims[1], &quantized));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_vara_double(ncid, v, start, count, input));
    CHECK(nc_inq_dimlen(ncid, dims[0], &length));
    REQUIRE(length == 0);
    count[0] = 1;
    REQUIRE(nc_put_vars_double(ncid, v, start, count, stride, input) == NC_ESTRIDE);
    CHECK(nc_inq_dimlen(ncid, dims[0], &length));
    REQUIRE(length == 0);
    REQUIRE(nc_put_vara_text(ncid, v, start, count, "abcd") == NC_ECHAR);
    CHECK(nc_inq_dimlen(ncid, dims[0], &length));
    REQUIRE(length == 0);
    CHECK(nc_put_vara_double(ncid, v, start, count, input));
    CHECK(nc_get_vara_double(ncid, v, start, count, back));
    REQUIRE(back[0] == 1 && back[1] == -7 && back[3] == 42);
    for (mode = NC_QUANTIZE_BITGROOM; mode <= NC_QUANTIZE_BITROUND; mode++) {
        CHECK(nc_redef(ncid));
        CHECK(nc_def_var_quantize(ncid, scalar, mode, 3));
        CHECK(nc_def_var_quantize(ncid, quantized, mode, 3));
        CHECK(nc_enddef(ncid));
        memcpy(saved, input, sizeof(input));
        CHECK(nc_put_var_double(ncid, scalar, input));
        CHECK(nc_put_var_double(ncid, quantized, input));
        REQUIRE(!memcmp(saved, input, sizeof(input)));
        CHECK(nc_get_var_float(ncid, quantized, actual));
        CHECK(nc4_convert_type(input, expected, NC_DOUBLE, NC_FLOAT, 4, &range, NULL, 0, mode, 3));
        REQUIRE(!memcmp(expected, actual, sizeof(actual)));
        CHECK(nc_get_var_float(ncid, scalar, actual));
        REQUIRE(actual[0] == expected[0]);
    }
    CHECK(nc_close(ncid));
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "records", &v));
    REQUIRE(nc_put_var_double(ncid, v, input) == NC_EPERM);
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

int
main(void)
{
    int failed;
    nep_set_log_level(-1);
    failed = matrix();
    failed |= public_io(0);
    failed |= public_io(NC_NETCDF4_MODEL);
    failed |= public_io(NC_CLASSIC_MODEL);
    failed |= edge_io();
    failed |= raw_types();
    return failed;
}

/* Test completed conversion range warnings in NEXTCDF-4.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nep.h"
#include "nep_meta.h"

#define FILE_NAME "tst_nextcdf4_range.h5"
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); failed = 1; goto done; } } while (0)
#define STATUS(call, want) do { int status = (call); if (status != (want)) { fprintf(stderr, "%s:%d: %s: got %s, expected %s\n", __FILE__, __LINE__, #call, nc_strerror(status), nc_strerror(want)); failed = 1; goto done; } } while (0)
#define CHECK(call) STATUS(call, NC_NOERR)

static int
variables(int mode, int compressed)
{
    int ncid = -1, dim, v, s, record, rd, failed = 0;
    double input[] = {300000, -2, 3.75, 7}, back[4];
    short shorts[4];
    unsigned int uints[4];
    size_t start = 0, count = 4, one = 1, length, chunk = 4;
    ptrdiff_t stride = 2;
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid));
    CHECK(nc_def_dim(ncid, "x", 4, &dim));
    CHECK(nc_def_dim(ncid, "record", NC_UNLIMITED, &rd));
    CHECK(nc_def_var(ncid, "v", NC_DOUBLE, 1, &dim, &v));
    CHECK(nc_def_var(ncid, "scalar", NC_SHORT, 0, NULL, &s));
    CHECK(nc_def_var(ncid, "record", NC_SHORT, 1, &rd, &record));
    if (compressed) {
        CHECK(nc_def_var_chunking(ncid, v, NC_CHUNKED, &chunk));
        CHECK(nc_def_var_deflate(ncid, v, 1, 1, 1));
    }
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_var_double(ncid, v, input));
    STATUS(nc_get_var_short(ncid, v, shorts), NC_ERANGE);
    REQUIRE(shorts[1] == -2 && shorts[2] == 3 && shorts[3] == 7);
    REQUIRE(shorts[0] == NC_FILL_SHORT);
    STATUS(nc_get_var_uint(ncid, v, uints), NC_ERANGE);
    REQUIRE(uints[0] == 300000 && uints[2] == 3 && uints[3] == 7);
    STATUS(nc_get_var1_short(ncid, v, &start, shorts), NC_ERANGE);
    count = 2;
    STATUS(nc_get_vars_short(ncid, v, &start, &count, &stride, shorts), NC_ERANGE);
    REQUIRE(shorts[1] == 3);
    STATUS(nc_put_var_double(ncid, s, input), NC_ERANGE);
    CHECK(nc_get_var_short(ncid, s, shorts));
    REQUIRE(shorts[0] == NC_FILL_SHORT);
    STATUS(nc_put_vara_double(ncid, record, &start, &count, input), NC_ERANGE);
    CHECK(nc_inq_dimlen(ncid, rd, &length));
    REQUIRE(length == 2);
    CHECK(nc_get_var_double(ncid, record, back));
    REQUIRE(back[1] == -2);
    STATUS(nc_put_vars_double(ncid, record, &start, &count, &stride, input), NC_ERANGE);
    CHECK(nc_inq_dimlen(ncid, rd, &length));
    REQUIRE(length == 3);
    STATUS(nc_put_var1_double(ncid, record, &one, input), NC_ERANGE);
    count = 0;
    CHECK(nc_put_vara_double(ncid, record, &start, &count, input));
    CHECK(nc_get_vara_short(ncid, v, &start, &count, shorts));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "v", &v));
    count = 4;
    STATUS(nc_get_vara_short(ncid, v, &start, &count, shorts), NC_ERANGE);
    REQUIRE(shorts[3] == 7);
    CHECK(nc_inq_varid(ncid, "scalar", &s));
    STATUS(nc_put_var_double(ncid, s, input), NC_EPERM);
 done:
    if (ncid >= 0) nc_close(ncid);
    unlink(FILE_NAME);
    return failed;
}

static int
attributes(void)
{
    int ncid = -1, group, dim, v, failed = 0, contexts[3], i;
    double input[] = {300000, -2, 7}, back[3];
    short output[3];
    float f;
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_grp(ncid, "g", &group));
    CHECK(nc_def_dim(ncid, "x", 3, &dim));
    CHECK(nc_def_var(ncid, "v", NC_FLOAT, 1, &dim, &v));
    contexts[0] = ncid; contexts[1] = group; contexts[2] = ncid;
    for (i = 0; i < 3; i++) {
        int varid = i == 2 ? v : NC_GLOBAL;
        CHECK(nc_put_att_double(contexts[i], varid, "source", NC_DOUBLE, 3, input));
        STATUS(nc_get_att_short(contexts[i], varid, "source", output), NC_ERANGE);
        REQUIRE(output[1] == -2 && output[2] == 7);
        CHECK(nc_put_att_short(contexts[i], varid, "replace", NC_SHORT, 1, output + 2));
        STATUS(nc_put_att_double(contexts[i], varid, "replace", NC_SHORT, 3, input), NC_ERANGE);
        CHECK(nc_get_att_double(contexts[i], varid, "replace", back));
        REQUIRE(back[1] == -2 && back[2] == 7);
        STATUS(nc_put_att_double(contexts[i], varid, "replace", NC_CHAR, 3, input), NC_ECHAR);
        CHECK(nc_get_att_double(contexts[i], varid, "replace", back));
        REQUIRE(back[2] == 7);
        CHECK(nc_put_att_double(contexts[i], varid, "empty", NC_SHORT, 0, NULL));
    }
    input[0] = 65505;
    STATUS(nc_put_att_double(ncid, NC_GLOBAL, "half", NC_FLOAT16, 1, input), NC_ERANGE);
    CHECK(nc_get_att_float(ncid, NC_GLOBAL, "half", &f));
    REQUIRE(isinf(f));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_get_att_double(ncid, NC_GLOBAL, "replace", back));
    REQUIRE(back[1] == -2 && back[2] == 7);
    CHECK(nc_get_att_float(ncid, NC_GLOBAL, "half", &f));
    REQUIRE(isinf(f));
 done:
    if (ncid >= 0) nc_close(ncid);
    unlink(FILE_NAME);
    return failed;
}

static int
boundaries(void)
{
    int ncid = -1, dim, v, h, b, f, integers, failed = 0;
    double values[] = {NAN, INFINITY, -INFINITY, 0x1p63, -0x1p63, 7};
    double edge[] = {32767.75, -32768.75, -0.75, 3.75, 0, 7};
    double tiny[] = {0x1p-1000, -0x1p-1000, NAN, INFINITY, -0.0, 7};
    double half[] = {65505, -65505, 0, INFINITY, NAN, 7};
    float bf[] = {FLT_MAX, -FLT_MAX, 0, INFINITY, NAN, 7}, floats[6];
    unsigned short raw[6];
    short shorts[6];
    unsigned int uints[6];
    long long longs[6];
    unsigned long long ulongs[6], uvalues[] = {ULLONG_MAX, 0, 1ULL << 63, 7, 8, 9};
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "x", 6, &dim));
    CHECK(nc_def_var(ncid, "v", NC_DOUBLE, 1, &dim, &v));
    CHECK(nc_def_var(ncid, "h", NC_FLOAT16, 1, &dim, &h));
    CHECK(nc_def_var(ncid, "b", NC_BFLOAT16, 1, &dim, &b));
    CHECK(nc_def_var(ncid, "f", NC_FLOAT, 1, &dim, &f));
    CHECK(nc_def_var(ncid, "u", NC_UINT64, 1, &dim, &integers));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_var_double(ncid, v, values));
    STATUS(nc_get_var_longlong(ncid, v, longs), NC_ERANGE);
    REQUIRE(longs[4] == LLONG_MIN && longs[5] == 7);
    STATUS(nc_get_var_ulonglong(ncid, v, ulongs), NC_ERANGE);
    REQUIRE(ulongs[5] == 7);
    CHECK(nc_put_var_ulonglong(ncid, integers, uvalues));
    STATUS(nc_get_var_longlong(ncid, integers, longs), NC_ERANGE);
    REQUIRE(longs[1] == 0 && longs[3] == 7 && longs[5] == 9);
    CHECK(nc_get_var_ulonglong(ncid, integers, ulongs));
    REQUIRE(!memcmp(ulongs, uvalues, sizeof(ulongs)));
    CHECK(nc_put_var_double(ncid, v, edge));
    CHECK(nc_get_var_short(ncid, v, shorts));
    REQUIRE(shorts[0] == 32767 && shorts[1] == -32768 && shorts[2] == 0 && shorts[5] == 7);
    CHECK(nc_put_var_double(ncid, f, tiny));
    CHECK(nc_get_var_float(ncid, f, floats));
    REQUIRE(floats[0] == 0 && floats[1] == 0 && isnan(floats[2]) && isinf(floats[3]) && signbit(floats[4]));
    STATUS(nc_put_var_double(ncid, h, half), NC_ERANGE);
    CHECK(nc_get_var(ncid, h, raw));
    REQUIRE(raw[0] == 0x7c00 && raw[1] == 0xfc00 && raw[5] == 0x4700);
    STATUS(nc_put_var_float(ncid, b, bf), NC_ERANGE);
    CHECK(nc_get_var(ncid, b, raw));
    REQUIRE(raw[0] == 0x7f80);
    REQUIRE(raw[5] == 0x40e0);
    edge[0] = -0.75; edge[1] = 0; edge[2] = 3.75; edge[3] = 7; edge[4] = 0; edge[5] = 7;
    CHECK(nc_put_var_double(ncid, v, edge));
    CHECK(nc_get_var_uint(ncid, v, uints));
    REQUIRE(uints[0] == 0 && uints[2] == 3 && uints[5] == 7);
    values[0] = DBL_MAX; values[1] = -DBL_MAX;
    CHECK(nc_put_var_double(ncid, v, values));
    STATUS(nc_get_var_float(ncid, v, floats), NC_ERANGE);
    REQUIRE(isinf(floats[0]) && isinf(floats[1]) && floats[5] == 7);
    STATUS(nc_put_var_double(ncid, f, values), NC_ERANGE);
    CHECK(nc_get_var_float(ncid, f, floats));
    REQUIRE(isinf(floats[0]) && isinf(floats[1]) && floats[5] == 7);
 done:
    if (ncid >= 0) nc_close(ncid);
    unlink(FILE_NAME);
    return failed;
}

static int
quantization(void)
{
    int ncid = -1, dim, vars[3], scalars[3], mode, failed = 0;
    double input[] = {DBL_MAX, -DBL_MAX, 1.234567, 7}, saved[4];
    float output[4];
    char name[32];
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "x", 4, &dim));
    for (mode = 0; mode < 3; mode++) {
        snprintf(name, sizeof(name), "v%d", mode);
        CHECK(nc_def_var(ncid, name, NC_FLOAT, 1, &dim, &vars[mode]));
        snprintf(name, sizeof(name), "s%d", mode);
        CHECK(nc_def_var(ncid, name, NC_FLOAT, 0, NULL, &scalars[mode]));
        CHECK(nc_def_var_quantize(ncid, vars[mode], mode + 1, 3));
        CHECK(nc_def_var_quantize(ncid, scalars[mode], mode + 1, 3));
    }
    CHECK(nc_enddef(ncid));
    memcpy(saved, input, sizeof(input));
    for (mode = 0; mode < 3; mode++) {
        STATUS(nc_put_var_double(ncid, vars[mode], input), NC_ERANGE);
        STATUS(nc_put_var_double(ncid, scalars[mode], input), NC_ERANGE);
        REQUIRE(!memcmp(saved, input, sizeof(input)));
        CHECK(nc_get_var_float(ncid, vars[mode], output));
        REQUIRE(isinf(output[0]) && isinf(output[1]) && output[3] > 6.9f);
    }
 done:
    if (ncid >= 0) nc_close(ncid);
    unlink(FILE_NAME);
    return failed;
}

int
main(void)
{
    int failed = 0, mode, compressed;
    const int modes[] = {0, NC_NETCDF4_MODEL, NC_CLASSIC_MODEL};
    nep_set_log_level(-1);
    for (mode = 0; mode < 3; mode++)
        for (compressed = 0; compressed < (mode == 2 ? 1 : 2); compressed++)
            failed |= variables(modes[mode], compressed);
    failed |= attributes();
    failed |= boundaries();
    failed |= quantization();
    return failed;
}

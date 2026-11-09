/* Test typed (converting) I/O on NEXTCDF-4 small floating-point
 * variables: NC_FLOAT16 and NC_BFLOAT16 convert to and from float and
 * double through HDF5; every other typed call on a small-float variable
 * is rejected.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_conv.h5"
#define NPAT 65536

static uint16_t pat[NPAT];
static uint16_t raw_f[NPAT], raw_d[NPAT];
static float f[NPAT], f2[NPAT];
static double d[NPAT];

static uint32_t
bf16_to_f32_bits(uint16_t b)
{
    return (uint32_t)b << 16;
}

static double
f16_decode(uint16_t h)
{
    int s = (h & 0x8000) != 0;
    int e = (h >> 10) & 0x1f;
    int m = h & 0x03ff;
    double v;

    if (e == 0)
        v = ldexp(m, -24);
    else if (e == 31)
        v = m ? NAN : INFINITY;
    else
        v = ldexp(1024 + m, e - 25);
    return s ? -v : v;
}

static int
f16_is_nan(uint16_t h)
{
    return (h & 0x7c00) == 0x7c00 && (h & 0x03ff);
}

static int
bf16_is_nan(uint16_t b)
{
    return (b & 0x7f80) == 0x7f80 && (b & 0x007f);
}

static float
f32_from_bits(uint32_t u)
{
    float v;

    memcpy(&v, &u, sizeof(v));
    return v;
}

static uint32_t
f32_bits(float v)
{
    uint32_t u;

    memcpy(&u, &v, sizeof(u));
    return u;
}

/* Read all 65536 patterns of xtype as float and double, check the
 * widening, then write the float and double values back and check the
 * raw bits round-trip for every non-NaN pattern. */
static int
test_widening(nc_type xtype, const char *name)
{
    int ncid, dimid, varid, var_f, var_d;
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))) {
        fprintf(stderr, "%s: nc_create failed %d\n", name, ret);
        return 1;
    }
    if ((ret = nc_def_dim(ncid, "p", NPAT, &dimid)) ||
        (ret = nc_def_var(ncid, "bits", xtype, 1, &dimid, &varid)) ||
        (ret = nc_def_var(ncid, "back_f", xtype, 1, &dimid, &var_f)) ||
        (ret = nc_def_var(ncid, "back_d", xtype, 1, &dimid, &var_d)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "%s: define failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++)
        pat[i] = (uint16_t)i;
    if ((ret = nc_put_var(ncid, varid, pat))) {
        fprintf(stderr, "%s: nc_put_var failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    memset(f, 0, sizeof(f));
    memset(d, 0, sizeof(d));
    if ((ret = nc_get_var_float(ncid, varid, f)) ||
        (ret = nc_get_var_double(ncid, varid, d))) {
        fprintf(stderr, "%s: typed read failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++) {
        int is_nan = (xtype == NC_FLOAT16) ? f16_is_nan(pat[i]) : bf16_is_nan(pat[i]);
        if (is_nan) {
            if (!isnan(f[i]) || !isnan(d[i])) {
                fprintf(stderr, "%s: pattern 0x%04x NaN not read as NaN\n", name, pat[i]);
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (xtype == NC_BFLOAT16) {
            if (f32_bits(f[i]) != bf16_to_f32_bits(pat[i])) {
                fprintf(stderr, "%s: pattern 0x%04x float bits 0x%08x\n",
                        name, pat[i], f32_bits(f[i]));
                nc_close(ncid);
                return 1;
            }
        } else {
            if ((double)f[i] != f16_decode(pat[i])) {
                fprintf(stderr, "%s: pattern 0x%04x float %.17g expected %.17g\n",
                        name, pat[i], (double)f[i], f16_decode(pat[i]));
                nc_close(ncid);
                return 1;
            }
        }
        if ((double)f[i] != d[i] || !!signbit(f[i]) != !!signbit(d[i])) {
            fprintf(stderr, "%s: pattern 0x%04x float/double disagree\n", name, pat[i]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_put_var_float(ncid, var_f, f)) ||
        (ret = nc_put_var_double(ncid, var_d, d))) {
        fprintf(stderr, "%s: typed write failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    memset(raw_f, 0, sizeof(raw_f));
    memset(raw_d, 0, sizeof(raw_d));
    if ((ret = nc_get_var(ncid, var_f, raw_f)) ||
        (ret = nc_get_var(ncid, var_d, raw_d))) {
        fprintf(stderr, "%s: raw read-back failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++) {
        int is_nan = (xtype == NC_FLOAT16) ? f16_is_nan(pat[i]) : bf16_is_nan(pat[i]);
        int nan_f = (xtype == NC_FLOAT16) ? f16_is_nan(raw_f[i]) : bf16_is_nan(raw_f[i]);
        int nan_d = (xtype == NC_FLOAT16) ? f16_is_nan(raw_d[i]) : bf16_is_nan(raw_d[i]);
        if (is_nan) {
            if (!nan_f || !nan_d) {
                fprintf(stderr, "%s: NaN pattern 0x%04x wrote 0x%04x/0x%04x\n",
                        name, pat[i], raw_f[i], raw_d[i]);
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (raw_f[i] != pat[i] || raw_d[i] != pat[i]) {
            fprintf(stderr, "%s: pattern 0x%04x round-trip gave 0x%04x/0x%04x\n",
                    name, pat[i], raw_f[i], raw_d[i]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "%s: nc_close failed %d\n", name, ret);
        return 1;
    }

    if ((ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) {
        fprintf(stderr, "%s: reopen failed %d\n", name, ret);
        return 1;
    }
    if ((ret = nc_inq_varid(ncid, "bits", &varid))) {
        fprintf(stderr, "%s: nc_inq_varid failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    memset(f2, 0, sizeof(f2));
    if ((ret = nc_get_var_float(ncid, varid, f2))) {
        fprintf(stderr, "%s: reopen typed read failed %d\n", name, ret);
        nc_close(ncid);
        return 1;
    }
    if (memcmp(f, f2, sizeof(f))) {
        fprintf(stderr, "%s: reopen float data differs\n", name);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "%s: nc_close 2 failed %d\n", name, ret);
        return 1;
    }
    return 0;
}

/* These are HDF5 2.1.1's rounding results (float16 ties to even;
 * bfloat16 ties away from zero and saturates float input), recorded in
 * docs/plan/v4.3.0-sprint1B-fix-float-conversion.md. A change here means
 * HDF5 changed its conversion. */
static int
test_pinned_writes(void)
{
    int ncid, dimid, hvar, bvar;
    size_t start[1] = {0};
    size_t count[1] = {13};
    size_t count3[1] = {3};
    float hin[13] = {1.5f, -2.f, 0.1f, 2049.f, 2051.f, 65504.f, 65505.f,
                     3e5f, -FLT_MAX, INFINITY, -0.0f, 1e-30f, NAN};
    float bin[13] = {1.5f, -2.f, 0.1f, 449.f, 0.f, 0.f, 1e-30f,
                     FLT_MAX, -FLT_MAX, INFINITY, -INFINITY, -0.0f, NAN};
    double din[3] = {1.5, 0.1, 1e300};
    uint16_t hraw[13] = {0}, braw[13] = {0};
    static const uint16_t hexp[13] = {0x3e00, 0xc000, 0x2e66, 0x6800, 0x6802,
                                      0x7bff, 0x7c00, 0x7c00, 0xfc00, 0x7c00,
                                      0x8000, 0x0000, 0};
    static const uint16_t bexp[13] = {0x3fc0, 0xc000, 0x3dcd, 0x43e1, 0x3f81,
                                      0x0001, 0x0da2, 0x7f7f, 0xff7f, 0x7f80,
                                      0xff80, 0x8000, 0};
    static const uint16_t bexp3[3] = {0x3fc0, 0x3dcd, 0x7f80};
    uint32_t u;
    int i, ret;

    u = 0x3f808000;
    memcpy(&bin[4], &u, sizeof(float));
    u = 0x00008000;
    memcpy(&bin[5], &u, sizeof(float));

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "n", 13, &dimid)) ||
        (ret = nc_def_var(ncid, "h", NC_FLOAT16, 1, &dimid, &hvar)) ||
        (ret = nc_def_var(ncid, "b", NC_BFLOAT16, 1, &dimid, &bvar)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "pinned: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_put_vara_float(ncid, hvar, start, count, hin)) ||
        (ret = nc_put_vara_float(ncid, bvar, start, count, bin))) {
        fprintf(stderr, "pinned: nc_put_vara_float failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vara(ncid, hvar, start, count, hraw)) ||
        (ret = nc_get_vara(ncid, bvar, start, count, braw))) {
        fprintf(stderr, "pinned: raw read failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 13; i++) {
        if (i == 12) {
            if (!f16_is_nan(hraw[i]) || !bf16_is_nan(braw[i])) {
                fprintf(stderr, "pinned: index %d NaN expected, got 0x%04x/0x%04x\n",
                        i, hraw[i], braw[i]);
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (hraw[i] != hexp[i] || braw[i] != bexp[i]) {
            fprintf(stderr, "pinned: index %d got 0x%04x/0x%04x expected 0x%04x/0x%04x\n",
                    i, hraw[i], braw[i], hexp[i], bexp[i]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_put_vara_double(ncid, bvar, start, count3, din))) {
        fprintf(stderr, "pinned: nc_put_vara_double failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(braw, 0, sizeof(braw));
    if ((ret = nc_get_vara(ncid, bvar, start, count3, braw))) {
        fprintf(stderr, "pinned: raw re-read failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 3; i++) {
        if (braw[i] != bexp3[i]) {
            fprintf(stderr, "pinned: double write index %d got 0x%04x expected 0x%04x\n",
                    i, braw[i], bexp3[i]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "pinned: nc_close failed %d\n", ret);
        return 1;
    }
    return 0;
}

/* The exact reproduction from issue #376. */
static int
test_issue_376(void)
{
    int ncid, dimid, varid;
    size_t start[1] = {0};
    size_t count[1] = {4};
    size_t chunks[1] = {4};
    size_t count2[1] = {2};
    ptrdiff_t stride2[1] = {2};
    size_t idx[1] = {2};
    uint16_t wdata[4] = {0x4380, 0x4381, 0x7f00, 0xc000};
    float fvals[4] = {1.f, 2.f, 3.f, 4.f};
    float fexp[4] = {256.f, 258.f, 0x1p127f, -2.f};
    float f1 = 0.f, fs[2] = {0.f, 0.f};
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "x", 4, &dimid)) ||
        (ret = nc_def_var(ncid, "b", NC_BFLOAT16, 1, &dimid, &varid)) ||
        (ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks)) ||
        (ret = nc_def_var_deflate(ncid, varid, 1, 1, 1)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "376: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_put_vara(ncid, varid, start, count, wdata))) {
        fprintf(stderr, "376: nc_put_vara failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "376: nc_close failed %d\n", ret);
        return 1;
    }
    if ((ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))) {
        fprintf(stderr, "376: reopen failed %d\n", ret);
        return 1;
    }
    if ((ret = nc_inq_varid(ncid, "b", &varid))) {
        fprintf(stderr, "376: nc_inq_varid failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vara_float(ncid, varid, start, count, fvals))) {
        fprintf(stderr, "376: nc_get_vara_float failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (fvals[i] != fexp[i]) {
            fprintf(stderr, "376: index %d got %g expected %g\n", i, fvals[i], fexp[i]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_get_var1_float(ncid, varid, idx, &f1)) || f1 != 0x1p127f) {
        fprintf(stderr, "376: nc_get_var1_float failed %d value %g\n", ret, f1);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vars_float(ncid, varid, start, count2, stride2, fs)) ||
        fs[0] != 256.f || fs[1] != 0x1p127f) {
        fprintf(stderr, "376: nc_get_vars_float failed %d values %g %g\n", ret, fs[0], fs[1]);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "376: nc_close 2 failed %d\n", ret);
        return 1;
    }
    return 0;
}

static int
check_5a(const unsigned char *buf, size_t n, const char *what)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (buf[i] != 0x5a) {
            fprintf(stderr, "rejected: %s touched buffer at %zu\n", what, i);
            return 1;
        }
    return 0;
}

static int
test_rejected(void)
{
    static const struct {
        const char *name;
        nc_type xtype;
        int esize;
    } cases[7] = {
        {"float16", NC_FLOAT16, 2},
        {"bfloat16", NC_BFLOAT16, 2},
        {"float8_e4m3", NC_FLOAT8_E4M3, 1},
        {"float8_e5m2", NC_FLOAT8_E5M2, 1},
        {"float6_e2m3", NC_FLOAT6_E2M3, 1},
        {"float6_e3m2", NC_FLOAT6_E3M2, 1},
        {"float4_e2m1", NC_FLOAT4_E2M1, 1},
    };
    int ncid, dimid, varid[7];
    size_t start[1] = {0};
    size_t count[1] = {4};
    uint16_t w16[4] = {0x3c00, 0x4000, 0x4200, 0x4400};
    uint8_t w8[4] = {0x01, 0x02, 0x03, 0x04};
    unsigned char buf[64];
    char text[8] = "abcd";
    signed char sc[4] = {1, 2, 3, 4};
    short sh[4] = {1, 2, 3, 4};
    int iv[4] = {1, 2, 3, 4};
    long long ll[4] = {1, 2, 3, 4};
    float fv[4] = {1.f, 2.f, 3.f, 4.f};
    double dv[4] = {1., 2., 3., 4.};
    uint16_t r16[4] = {0};
    uint8_t r8[4] = {0};
    int i, ret, fail = 0;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "x", 4, &dimid))) {
        fprintf(stderr, "rejected: create/define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 7; i++) {
        if ((ret = nc_def_var(ncid, cases[i].name, cases[i].xtype, 1, &dimid, &varid[i]))) {
            fprintf(stderr, "rejected: nc_def_var %s failed %d\n", cases[i].name, ret);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_enddef(ncid))) {
        fprintf(stderr, "rejected: nc_enddef failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 7; i++) {
        void *w = cases[i].esize == 2 ? (void *)w16 : (void *)w8;
        if ((ret = nc_put_vara(ncid, varid[i], start, count, w))) {
            fprintf(stderr, "rejected: raw write %s failed %d\n", cases[i].name, ret);
            nc_close(ncid);
            return 1;
        }
    }

    for (i = 0; i < 7; i++) {
        int small_fp = cases[i].esize == 1;

        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_text(ncid, varid[i], start, count, (char *)buf) != NC_ECHAR) {
            fprintf(stderr, "rejected: %s nc_get_vara_text not NC_ECHAR\n", cases[i].name);
            fail = 1;
        }
        if (check_5a(buf, sizeof(buf), "nc_get_vara_text")) fail = 1;
        if (nc_put_vara_text(ncid, varid[i], start, count, text) != NC_ECHAR) {
            fprintf(stderr, "rejected: %s nc_put_vara_text not NC_ECHAR\n", cases[i].name);
            fail = 1;
        }

        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_schar(ncid, varid[i], start, count, (signed char *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_schar")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_uchar(ncid, varid[i], start, count, buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_uchar")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_short(ncid, varid[i], start, count, (short *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_short")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_ushort(ncid, varid[i], start, count, (unsigned short *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_ushort")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_int(ncid, varid[i], start, count, (int *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_int")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_uint(ncid, varid[i], start, count, (unsigned int *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_uint")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_longlong(ncid, varid[i], start, count, (long long *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_longlong")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_ulonglong(ncid, varid[i], start, count, (unsigned long long *)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_ulonglong")) fail = 1;
        memset(buf, 0x5a, sizeof(buf));
        if (nc_get_vara_string(ncid, varid[i], start, count, (char **)buf) != NC_EBADTYPE ||
            check_5a(buf, sizeof(buf), "nc_get_vara_string")) fail = 1;

        if (nc_put_vara_schar(ncid, varid[i], start, count, sc) != NC_EBADTYPE ||
            nc_put_vara_short(ncid, varid[i], start, count, sh) != NC_EBADTYPE ||
            nc_put_vara_int(ncid, varid[i], start, count, iv) != NC_EBADTYPE ||
            nc_put_vara_ulonglong(ncid, varid[i], start, count, (unsigned long long *)ll) != NC_EBADTYPE) {
            fprintf(stderr, "rejected: %s typed put not NC_EBADTYPE\n", cases[i].name);
            fail = 1;
        }

        if (small_fp) {
            memset(buf, 0x5a, sizeof(buf));
            if (nc_get_vara_float(ncid, varid[i], start, count, (float *)buf) != NC_EBADTYPE ||
                check_5a(buf, sizeof(buf), "nc_get_vara_float")) fail = 1;
            memset(buf, 0x5a, sizeof(buf));
            if (nc_get_vara_double(ncid, varid[i], start, count, (double *)buf) != NC_EBADTYPE ||
                check_5a(buf, sizeof(buf), "nc_get_vara_double")) fail = 1;
            if (nc_put_vara_float(ncid, varid[i], start, count, fv) != NC_EBADTYPE ||
                nc_put_vara_double(ncid, varid[i], start, count, dv) != NC_EBADTYPE) {
                fprintf(stderr, "rejected: %s float/double put not NC_EBADTYPE\n", cases[i].name);
                fail = 1;
            }
        }
    }

    /* The rejected puts must have written nothing. */
    for (i = 0; i < 7; i++) {
        if (cases[i].esize == 2) {
            memset(r16, 0, sizeof(r16));
            if ((ret = nc_get_vara(ncid, varid[i], start, count, r16)) ||
                memcmp(r16, w16, sizeof(w16))) {
                fprintf(stderr, "rejected: %s data changed after rejected puts\n", cases[i].name);
                fail = 1;
            }
        } else {
            memset(r8, 0, sizeof(r8));
            if ((ret = nc_get_vara(ncid, varid[i], start, count, r8)) ||
                memcmp(r8, w8, sizeof(w8))) {
                fprintf(stderr, "rejected: %s data changed after rejected puts\n", cases[i].name);
                fail = 1;
            }
        }
        /* Untyped calls still round-trip raw patterns. */
        if (cases[i].esize == 2) {
            uint16_t t16[4] = {0xdef0, 0x9abc, 0x5678, 0x1234};
            memset(r16, 0, sizeof(r16));
            if ((ret = nc_put_vara(ncid, varid[i], start, count, t16)) ||
                (ret = nc_get_vara(ncid, varid[i], start, count, r16)) ||
                memcmp(r16, t16, sizeof(t16))) {
                fprintf(stderr, "rejected: %s untyped round-trip failed\n", cases[i].name);
                fail = 1;
            }
        } else {
            uint8_t t8[4] = {0xa0, 0xa1, 0xa2, 0xa3};
            memset(r8, 0, sizeof(r8));
            if ((ret = nc_put_vara(ncid, varid[i], start, count, t8)) ||
                (ret = nc_get_vara(ncid, varid[i], start, count, r8)) ||
                memcmp(r8, t8, sizeof(t8))) {
                fprintf(stderr, "rejected: %s untyped round-trip failed\n", cases[i].name);
                fail = 1;
            }
        }
    }

    /* In define mode a typed call returns NC_EINDEFINE, not the memtype error. */
    if ((ret = nc_redef(ncid))) {
        fprintf(stderr, "rejected: nc_redef failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vara_float(ncid, varid[1], start, count, fv)) != NC_EINDEFINE) {
        fprintf(stderr, "rejected: define-mode get returned %d, not NC_EINDEFINE\n", ret);
        fail = 1;
    }
    if ((ret = nc_enddef(ncid)) || (ret = nc_close(ncid))) {
        fprintf(stderr, "rejected: enddef/close failed %d\n", ret);
        return 1;
    }
    return fail;
}

int
main(void)
{
    int failed = 0;

    nep_set_log_level(-1);
    failed |= test_widening(NC_FLOAT16, "NC_FLOAT16");
    failed |= test_widening(NC_BFLOAT16, "NC_BFLOAT16");
    failed |= test_pinned_writes();
    failed |= test_issue_376();
    failed |= test_rejected();
    unlink(FILE_NAME);
    return failed;
}

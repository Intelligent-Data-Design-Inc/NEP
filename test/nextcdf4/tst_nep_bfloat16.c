/* Test the nc_*_bfloat16 functions in libnep: NC_BFLOAT16 variables
 * exchange raw bits; NC_FLOAT and NC_DOUBLE variables are converted by
 * NEP (reads round to nearest, ties to even, with NC_ERANGE on
 * overflow; writes are exact); other variable types are rejected.
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

#define FILE_NAME "tst_nep_bfloat16.h5"
#define FILE_NAME4 "tst_nep_bfloat16.nc"
#define NPAT 65536

static uint16_t pat[NPAT], back[NPAT];
static float fbuf[NPAT];
static double dbuf[NPAT];

static float
bf16_to_float(uint16_t b)
{
    uint32_t u = (uint32_t)b << 16;
    float f;

    memcpy(&f, &u, sizeof(f));
    return f;
}

static int
bf16_is_nan(uint16_t b)
{
    return (b & 0x7f80) == 0x7f80 && (b & 0x007f);
}

static uint32_t
f32_bits(float v)
{
    uint32_t u;

    memcpy(&u, &v, sizeof(u));
    return u;
}

/* The 12-value float read-back case shared by test 1 and test 6. */
static int
check_ok_var(int ncid, int varid, const char *tag)
{
    static const float in[12] = {1.5f, -2.f, 0.1f, 449.f, 0.f, 0.f, 0.f,
                                 1e-30f, INFINITY, -INFINITY, -0.0f, NAN};
    static const uint16_t exp[12] = {0x3fc0, 0xc000, 0x3dcd, 0x43e0, 0x3f80,
                                     0x3f82, 0x0000, 0x0da2, 0x7f80, 0xff80,
                                     0x8000, 0};
    float v[12];
    uint16_t got[12] = {0};
    size_t start[1] = {0}, count[1] = {12};
    uint32_t u;
    int i, ret;

    memcpy(v, in, sizeof(v));
    u = 0x3f808000;
    memcpy(&v[4], &u, sizeof(float));
    u = 0x3f818000;
    memcpy(&v[5], &u, sizeof(float));
    u = 0x00008000;
    memcpy(&v[6], &u, sizeof(float));

    if ((ret = nc_put_vara_float(ncid, varid, start, count, v))) {
        fprintf(stderr, "%s: write ok failed %d\n", tag, ret);
        return 1;
    }
    if ((ret = nc_get_vara_bfloat16(ncid, varid, start, count,
                                    (nc_bfloat16_t *)(void *)got))) {
        fprintf(stderr, "%s: nc_get_vara_bfloat16 failed %d\n", tag, ret);
        return 1;
    }
    for (i = 0; i < 12; i++) {
        if (i == 11) {
            if (!bf16_is_nan(got[i])) {
                fprintf(stderr, "%s: index %d NaN expected, got 0x%04x\n",
                        tag, i, got[i]);
                return 1;
            }
            continue;
        }
        if (got[i] != exp[i]) {
            fprintf(stderr, "%s: index %d got 0x%04x expected 0x%04x\n",
                    tag, i, got[i], exp[i]);
            return 1;
        }
    }
    return 0;
}

static int
test_get_float(void)
{
    int ncid, dimid, dimb, dim2, okv, bigv, allv;
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "n", 12, &dimid)) ||
        (ret = nc_def_dim(ncid, "m", 2, &dimb)) ||
        (ret = nc_def_dim(ncid, "p", NPAT, &dim2)) ||
        (ret = nc_def_var(ncid, "ok", NC_FLOAT, 1, &dimid, &okv)) ||
        (ret = nc_def_var(ncid, "big", NC_FLOAT, 1, &dimb, &bigv)) ||
        (ret = nc_def_var(ncid, "all", NC_FLOAT, 1, &dim2, &allv)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "get_float: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    {
        size_t start[1] = {0}, count[1] = {2};
        float big[2] = {FLT_MAX, 1.f};
        uint16_t got[2] = {0};

        if ((ret = nc_put_vara_float(ncid, bigv, start, count, big))) {
            fprintf(stderr, "get_float: write big failed %d\n", ret);
            nc_close(ncid);
            return 1;
        }
        if ((ret = nc_get_vara_bfloat16(ncid, bigv, start, count,
                                        (nc_bfloat16_t *)(void *)got)) != NC_ERANGE) {
            fprintf(stderr, "get_float: big returned %d, not NC_ERANGE\n", ret);
            nc_close(ncid);
            return 1;
        }
        if (got[0] != 0x7f80 || got[1] != 0x3f80) {
            fprintf(stderr, "get_float: big gave 0x%04x 0x%04x\n", got[0], got[1]);
            nc_close(ncid);
            return 1;
        }
    }
    if (check_ok_var(ncid, okv, "get_float")) {
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++)
        fbuf[i] = bf16_to_float((uint16_t)i);
    if ((ret = nc_put_var_float(ncid, allv, fbuf))) {
        fprintf(stderr, "get_float: write all failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(back, 0, sizeof(back));
    if ((ret = nc_get_var_bfloat16(ncid, allv, (nc_bfloat16_t *)(void *)back))) {
        fprintf(stderr, "get_float: nc_get_var_bfloat16 all failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++) {
        uint16_t p = (uint16_t)i;
        if (bf16_is_nan(p)) {
            if (!bf16_is_nan(back[i])) {
                fprintf(stderr, "get_float: NaN pattern 0x%04x read 0x%04x\n", p, back[i]);
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (back[i] != p) {
            fprintf(stderr, "get_float: pattern 0x%04x read 0x%04x\n", p, back[i]);
            nc_close(ncid);
            return 1;
        }
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_get_double(void)
{
    int ncid, dim8, dim3, dv, dbv;
    double din[8] = {1.5, 0.1, 1.0 + 0x1p-8 + 0x1p-30, 1.0 + 0x1p-8 - 0x1p-30,
                     0x1p-133, INFINITY, -0.0, NAN};
    static const uint16_t dexp[8] = {0x3fc0, 0x3dcd, 0x3f81, 0x3f80, 0x0001,
                                     0x7f80, 0x8000, 0};
    double dbig[3] = {1e300, -1e300, 2.0};
    static const uint16_t dbexp[3] = {0x7f80, 0xff80, 0x4000};
    uint16_t got[8] = {0};
    size_t start[1] = {0}, count8[1] = {8}, count3[1] = {3};
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "n", 8, &dim8)) ||
        (ret = nc_def_dim(ncid, "m", 3, &dim3)) ||
        (ret = nc_def_var(ncid, "d", NC_DOUBLE, 1, &dim8, &dv)) ||
        (ret = nc_def_var(ncid, "dbig", NC_DOUBLE, 1, &dim3, &dbv)) ||
        (ret = nc_enddef(ncid)) ||
        (ret = nc_put_vara_double(ncid, dv, start, count8, din)) ||
        (ret = nc_put_vara_double(ncid, dbv, start, count3, dbig))) {
        fprintf(stderr, "get_double: setup failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vara_bfloat16(ncid, dv, start, count8,
                                    (nc_bfloat16_t *)(void *)got))) {
        fprintf(stderr, "get_double: read failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 8; i++) {
        if (i == 7) {
            if (!bf16_is_nan(got[i])) {
                fprintf(stderr, "get_double: index 7 not NaN\n");
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (got[i] != dexp[i]) {
            fprintf(stderr, "get_double: index %d got 0x%04x expected 0x%04x\n",
                    i, got[i], dexp[i]);
            nc_close(ncid);
            return 1;
        }
    }
    memset(got, 0, sizeof(got));
    if ((ret = nc_get_vara_bfloat16(ncid, dbv, start, count3,
                                    (nc_bfloat16_t *)(void *)got)) != NC_ERANGE) {
        fprintf(stderr, "get_double: dbig returned %d, not NC_ERANGE\n", ret);
        nc_close(ncid);
        return 1;
    }
    if (memcmp(got, dbexp, sizeof(dbexp))) {
        fprintf(stderr, "get_double: dbig gave 0x%04x 0x%04x 0x%04x\n",
                got[0], got[1], got[2]);
        nc_close(ncid);
        return 1;
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_put(void)
{
    int ncid, dimid, pfv, pdv;
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "p", NPAT, &dimid)) ||
        (ret = nc_def_var(ncid, "pf", NC_FLOAT, 1, &dimid, &pfv)) ||
        (ret = nc_def_var(ncid, "pd", NC_DOUBLE, 1, &dimid, &pdv)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "put: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++)
        pat[i] = (uint16_t)i;
    if ((ret = nc_put_var_bfloat16(ncid, pfv, (nc_bfloat16_t *)(void *)pat)) ||
        (ret = nc_put_var_bfloat16(ncid, pdv, (nc_bfloat16_t *)(void *)pat))) {
        fprintf(stderr, "put: nc_put_var_bfloat16 failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(fbuf, 0, sizeof(fbuf));
    memset(dbuf, 0, sizeof(dbuf));
    if ((ret = nc_get_var_float(ncid, pfv, fbuf)) ||
        (ret = nc_get_var_double(ncid, pdv, dbuf))) {
        fprintf(stderr, "put: read-back failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++) {
        uint16_t p = (uint16_t)i;
        if (f32_bits(fbuf[i]) != (uint32_t)p << 16) {
            fprintf(stderr, "put: pf pattern 0x%04x stored bits 0x%08x\n",
                    p, f32_bits(fbuf[i]));
            nc_close(ncid);
            return 1;
        }
        if (bf16_is_nan(p)) {
            if (!isnan(dbuf[i])) {
                fprintf(stderr, "put: pd NaN pattern 0x%04x read %g\n", p, dbuf[i]);
                nc_close(ncid);
                return 1;
            }
            continue;
        }
        if (dbuf[i] != (double)bf16_to_float(p)) {
            fprintf(stderr, "put: pd pattern 0x%04x read %.17g\n", p, dbuf[i]);
            nc_close(ncid);
            return 1;
        }
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_raw(void)
{
    int ncid, dimid, varid;
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "p", NPAT, &dimid)) ||
        (ret = nc_def_var(ncid, "b", NC_BFLOAT16, 1, &dimid, &varid)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "raw: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < NPAT; i++)
        pat[i] = (uint16_t)i;
    if ((ret = nc_put_var_bfloat16(ncid, varid, (nc_bfloat16_t *)(void *)pat))) {
        fprintf(stderr, "raw: nc_put_var_bfloat16 failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(back, 0, sizeof(back));
    if ((ret = nc_get_var_bfloat16(ncid, varid, (nc_bfloat16_t *)(void *)back))) {
        fprintf(stderr, "raw: nc_get_var_bfloat16 failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if (memcmp(back, pat, sizeof(pat))) {
        fprintf(stderr, "raw: same-handle round-trip differs\n");
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_close(ncid))) {
        fprintf(stderr, "raw: nc_close failed %d\n", ret);
        return 1;
    }
    if ((ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid)) ||
        (ret = nc_inq_varid(ncid, "b", &varid))) {
        fprintf(stderr, "raw: reopen failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(back, 0, sizeof(back));
    if ((ret = nc_get_var_bfloat16(ncid, varid, (nc_bfloat16_t *)(void *)back))) {
        fprintf(stderr, "raw: reopen read failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if (memcmp(back, pat, sizeof(pat))) {
        fprintf(stderr, "raw: reopen round-trip differs\n");
        nc_close(ncid);
        return 1;
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_forms(void)
{
    int ncid, dimids[2], varid;
    float g[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    static const float gexp[12] = {8, 1, 2, 1, 4, 5, 6, 7, 2, 1, 2, 1};
    uint16_t b1 = 0, bs[4] = {0, 0, 0, 0};
    uint16_t w8 = 0x4100, w2[4] = {0x4000, 0x4000, 0x4000, 0x4000};
    uint16_t w1[4] = {0x3f80, 0x3f80, 0x3f80, 0x3f80};
    size_t idx[2] = {1, 2};
    size_t s0[2] = {0, 0}, c2[2] = {2, 2};
    ptrdiff_t st2[2] = {2, 2};
    size_t s20[2] = {2, 0}, c14[2] = {1, 4};
    size_t s01[2] = {0, 1};
    int i, ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "y", 3, &dimids[0])) ||
        (ret = nc_def_dim(ncid, "x", 4, &dimids[1])) ||
        (ret = nc_def_var(ncid, "g", NC_FLOAT, 2, dimids, &varid)) ||
        (ret = nc_enddef(ncid)) ||
        (ret = nc_put_var_float(ncid, varid, g))) {
        fprintf(stderr, "forms: setup failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_var1_bfloat16(ncid, varid, idx,
                                    (nc_bfloat16_t *)(void *)&b1)) || b1 != 0x40c0) {
        fprintf(stderr, "forms: var1 failed %d bits 0x%04x\n", ret, b1);
        nc_close(ncid);
        return 1;
    }
    if ((ret = nc_get_vars_bfloat16(ncid, varid, s0, c2, st2,
                                    (nc_bfloat16_t *)(void *)bs))) {
        fprintf(stderr, "forms: vars read failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    {
        static const uint16_t bexp[4] = {0x0000, 0x4000, 0x4100, 0x4120};
        if (memcmp(bs, bexp, sizeof(bexp))) {
            fprintf(stderr, "forms: vars gave 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    bs[0], bs[1], bs[2], bs[3]);
            nc_close(ncid);
            return 1;
        }
    }
    if ((ret = nc_put_var1_bfloat16(ncid, varid, s0,
                                    (nc_bfloat16_t *)(void *)&w8)) ||
        (ret = nc_put_vara_bfloat16(ncid, varid, s20, c14,
                                    (nc_bfloat16_t *)(void *)w2)) ||
        (ret = nc_put_vars_bfloat16(ncid, varid, s01, c2, st2,
                                    (nc_bfloat16_t *)(void *)w1))) {
        fprintf(stderr, "forms: put failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    memset(g, 0, sizeof(g));
    if ((ret = nc_get_var_float(ncid, varid, g))) {
        fprintf(stderr, "forms: read-back failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    for (i = 0; i < 12; i++) {
        if (g[i] != gexp[i]) {
            fprintf(stderr, "forms: index %d got %g expected %g\n", i, g[i], gexp[i]);
            nc_close(ncid);
            return 1;
        }
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_netcdf4(void)
{
    int ncid, dimid, varid, ret;

    unlink(FILE_NAME4);
    if ((ret = nc_create(FILE_NAME4, NC_NETCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "n", 12, &dimid)) ||
        (ret = nc_def_var(ncid, "ok", NC_FLOAT, 1, &dimid, &varid)) ||
        (ret = nc_enddef(ncid))) {
        fprintf(stderr, "netcdf4: define failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }
    if (check_ok_var(ncid, varid, "netcdf4")) {
        nc_close(ncid);
        return 1;
    }
    return nc_close(ncid) != NC_NOERR;
}

static int
test_errors(void)
{
    int ncid, dimid, iv, sv, hv, cv, fv;
    int ivals[4] = {1, 2, 3, 4};
    short svals[4] = {5, 6, 7, 8};
    uint16_t hvals[4] = {0x3c00, 0x4000, 0x4200, 0x4400};
    char cvals[4] = "abc";
    unsigned char buf[64];
    int rin[4] = {0};
    short rin_s[4] = {0};
    uint16_t rin_h[4] = {0};
    char rin_c[4] = {0};
    uint16_t bvals[4] = {0x3f80, 0x4000, 0x4040, 0x4080};
    size_t start[1] = {0}, count[1] = {4};
    int ret, fail = 0;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ||
        (ret = nc_def_dim(ncid, "x", 4, &dimid)) ||
        (ret = nc_def_var(ncid, "i", NC_INT, 1, &dimid, &iv)) ||
        (ret = nc_def_var(ncid, "s", NC_SHORT, 1, &dimid, &sv)) ||
        (ret = nc_def_var(ncid, "h", NC_FLOAT16, 1, &dimid, &hv)) ||
        (ret = nc_def_var(ncid, "c", NC_CHAR, 1, &dimid, &cv)) ||
        (ret = nc_def_var(ncid, "f", NC_FLOAT, 1, &dimid, &fv)) ||
        (ret = nc_enddef(ncid)) ||
        (ret = nc_put_vara_int(ncid, iv, start, count, ivals)) ||
        (ret = nc_put_vara_short(ncid, sv, start, count, svals)) ||
        (ret = nc_put_vara(ncid, hv, start, count, hvals)) ||
        (ret = nc_put_vara_text(ncid, cv, start, count, cvals))) {
        fprintf(stderr, "errors: setup failed %d\n", ret);
        nc_close(ncid);
        return 1;
    }

    memset(buf, 0x5a, sizeof(buf));
    if (nc_get_vara_bfloat16(ncid, iv, start, count, (nc_bfloat16_t *)(void *)buf) != NC_EBADTYPE ||
        nc_get_vara_bfloat16(ncid, sv, start, count, (nc_bfloat16_t *)(void *)buf) != NC_EBADTYPE ||
        nc_get_vara_bfloat16(ncid, hv, start, count, (nc_bfloat16_t *)(void *)buf) != NC_EBADTYPE) {
        fprintf(stderr, "errors: get not NC_EBADTYPE\n");
        fail = 1;
    }
    if (nc_get_vara_bfloat16(ncid, cv, start, count, (nc_bfloat16_t *)(void *)buf) != NC_ECHAR) {
        fprintf(stderr, "errors: char get not NC_ECHAR\n");
        fail = 1;
    }
    {
        size_t i;
        for (i = 0; i < sizeof(buf); i++)
            if (buf[i] != 0x5a) {
                fprintf(stderr, "errors: rejected get touched buffer at %zu\n", i);
                fail = 1;
                break;
            }
    }
    if (nc_put_vara_bfloat16(ncid, iv, start, count, (nc_bfloat16_t *)(void *)bvals) != NC_EBADTYPE ||
        nc_put_vara_bfloat16(ncid, sv, start, count, (nc_bfloat16_t *)(void *)bvals) != NC_EBADTYPE ||
        nc_put_vara_bfloat16(ncid, hv, start, count, (nc_bfloat16_t *)(void *)bvals) != NC_EBADTYPE ||
        nc_put_vara_bfloat16(ncid, cv, start, count, (nc_bfloat16_t *)(void *)bvals) != NC_ECHAR) {
        fprintf(stderr, "errors: put not NC_EBADTYPE/NC_ECHAR\n");
        fail = 1;
    }
    if (nc_get_vara_int(ncid, iv, start, count, rin) || memcmp(rin, ivals, sizeof(ivals)) ||
        nc_get_vara_short(ncid, sv, start, count, rin_s) || memcmp(rin_s, svals, sizeof(svals)) ||
        nc_get_vara(ncid, hv, start, count, rin_h) || memcmp(rin_h, hvals, sizeof(hvals)) ||
        nc_get_vara_text(ncid, cv, start, count, rin_c) || memcmp(rin_c, cvals, 3)) {
        fprintf(stderr, "errors: variable data changed after rejected puts\n");
        fail = 1;
    }
    if ((ret = nc_get_vara_bfloat16(ncid, 99, start, count, (nc_bfloat16_t *)(void *)buf)) != NC_ENOTVAR) {
        fprintf(stderr, "errors: bad varid gave %d, not NC_ENOTVAR\n", ret);
        fail = 1;
    }
    if ((ret = nc_get_vara_bfloat16(ncid, fv, start, NULL, (nc_bfloat16_t *)(void *)buf)) != NC_EINVAL) {
        fprintf(stderr, "errors: NULL countp gave %d, not NC_EINVAL\n", ret);
        fail = 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        fail = 1;
    return fail;
}

int
main(void)
{
    int failed = 0;

    nep_set_log_level(-1);
    failed |= test_get_float();
    failed |= test_get_double();
    failed |= test_put();
    failed |= test_raw();
    failed |= test_forms();
    failed |= test_netcdf4();
    failed |= test_errors();
    unlink(FILE_NAME);
    unlink(FILE_NAME4);
    return failed;
}

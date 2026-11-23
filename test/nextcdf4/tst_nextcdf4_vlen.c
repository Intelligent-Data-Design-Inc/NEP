/* Tests variable-length types (vlens) for NEXTCDF-4: vlens of atomic
 * and user-defined bases, nested vlens, compounds containing vlens,
 * element helpers, attribute ownership, fills, unlimited append,
 * reopen and upstream NetCDF-4 interchange, in native and
 * NC_NETCDF4_MODEL modes.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NATIVE "tst_nextcdf4_vlen_native.h5"
#define FILE_MODEL "tst_nextcdf4_vlen_model.h5"
#define FILE_UP_IN "tst_nextcdf4_vlen_upin.h5"
#define FILE_UP_OUT "tst_nextcdf4_vlen_upout.h5"
#define FILE_ATT "tst_nextcdf4_vlen_att.h5"
#define FILE_REDEF "tst_nextcdf4_vlen_redef.h5"
#define FILE_ERR "tst_nextcdf4_vlen_err.h5"

#define NV 4
#define CMP_NAME "point"
#define REC_NAME "record"

/* Compound used as a vlen base: a fixed-size {x, y} point. */
struct point {
    int x;
    float y;
};

/* Compound containing a vlen member and an array-of-vlen field. */
struct rec {
    int id;
    nc_vlen_t vals;    /* vlen of int */
    nc_vlen_t past[2]; /* array field: two vlens of int */
};

static const int mileage0[3] = {300, 400, 500};
static const int mileage2[1] = {42};
static const double depths1[2] = {1.5, -2.5};
static const struct point track0[2] = {{1, 0.5f}, {2, 1.5f}};
static const struct point track3[1] = {{-9, 99.0f}};
static const int nest_a[2] = {7, 8};
static const int nest_b[3] = {9, 10, 11};

/* Deep-compare two nc_vlen_t of int. */
static int
vlen_int_eq(const nc_vlen_t *a, const nc_vlen_t *b)
{
    if (a->len != b->len)
        return 0;
    return !a->len || !memcmp(a->p, b->p, a->len * sizeof(int));
}

/* Deep-compare two nc_vlen_t of struct point. */
static int
vlen_point_eq(const nc_vlen_t *a, const nc_vlen_t *b)
{
    if (a->len != b->len)
        return 0;
    return !a->len || !memcmp(a->p, b->p, a->len * sizeof(struct point));
}

/* Define the compound and the vlen types used in the round trip.
 * The vlen of compound is defined before its base commits; the
 * nested vlen and the compound-containing-vlen exercise the full
 * non-string type graph. */
static int
def_test_types(int ncid, nc_type *cmpid, nc_type *vint, nc_type *vdbl,
               nc_type *vcmp, nc_type *vnest, nc_type *recid)
{
    if (nc_def_compound(ncid, sizeof(struct point), CMP_NAME, cmpid))
        return 1;
    if (nc_insert_compound(ncid, *cmpid, "x",
                           offsetof(struct point, x), NC_INT))
        return 1;
    if (nc_insert_compound(ncid, *cmpid, "y",
                           offsetof(struct point, y), NC_FLOAT))
        return 1;
    if (nc_def_vlen(ncid, "track", *cmpid, vcmp))
        return 2;
    if (nc_def_vlen(ncid, "nested", *vcmp, vnest))
        return 3;
    if (nc_def_vlen(ncid, "mileage", NC_INT, vint))
        return 4;
    if (nc_def_vlen(ncid, "depths", NC_DOUBLE, vdbl))
        return 5;
    if (nc_def_compound(ncid, sizeof(struct rec), REC_NAME, recid))
        return 6;
    if (nc_insert_compound(ncid, *recid, "id",
                           offsetof(struct rec, id), NC_INT))
        return 7;
    if (nc_insert_compound(ncid, *recid, "vals",
                           offsetof(struct rec, vals), *vint))
        return 8;
    {
        int dims[1] = {2};
        if (nc_insert_array_compound(ncid, *recid, "past",
                                     offsetof(struct rec, past), *vint,
                                     1, dims))
            return 8;
    }
    return 0;
}

/* Fill `out` (NV nc_vlen_t of int) with the expected variable data. */
static void
fill_mileage(nc_vlen_t *out)
{
    out[0].len = 3;
    out[0].p = (void *)mileage0;
    out[1].len = 0;
    out[1].p = NULL;
    out[2].len = 1;
    out[2].p = (void *)mileage2;
    out[3].len = 2;
    out[3].p = (void *)mileage0;
}

/* Compare a read buffer against the expected mileage values. */
static int
check_mileage(const nc_vlen_t *got)
{
    nc_vlen_t exp[NV];
    int i;

    fill_mileage(exp);
    for (i = 0; i < NV; i++)
        if (!vlen_int_eq(&exp[i], &got[i]))
            return 1;
    return 0;
}

/* Create a file, write all vlen variables/attributes, close. */
static int
write_roundtrip(const char *fname, int cmode)
{
    int ncid, dimid, udimid, varid, dvarid, cvarid, nvarid, rvarid;
    int ovarid;
    nc_type cmpid, vint, vdbl, vcmp, vnest, recid;
    nc_vlen_t in[NV];
    nc_vlen_t din[2], cin[NV], nin[NV], rin[1];
    nc_vlen_t inner_a[2], inner_b[1];
    struct rec rin_data;
    size_t start[1], count[1];
    ptrdiff_t stride[1];
    nc_vlen_t attv[2];
    int d;

    if (nc_create(fname, NC_NEXTCDF4 | cmode, &ncid))
        return 1;
    if (nc_def_dim(ncid, "n", NV, &dimid))
        return 2;
    if (nc_def_dim(ncid, "t", NC_UNLIMITED, &udimid))
        return 2;
    if (def_test_types(ncid, &cmpid, &vint, &vdbl, &vcmp, &vnest, &recid))
        return 3;
    if (nc_def_var(ncid, "miles", vint, 1, &dimid, &varid))
        return 4;
    if (nc_def_var(ncid, "depth", vdbl, 1, &dimid, &dvarid))
        return 4;
    if (nc_def_var(ncid, "tracks", vcmp, 1, &dimid, &cvarid))
        return 4;
    if (nc_def_var(ncid, "nest_var", vnest, 1, &dimid, &nvarid))
        return 4;
    if (nc_def_var(ncid, "records", recid, 1, &dimid, &rvarid))
        return 4;
    if (nc_def_var(ncid, "series", vint, 1, &udimid, &ovarid))
        return 4;

    /* Global and variable vlen attributes. */
    {
        int av0[2] = {1, 2};
        int av1[1] = {3};
        attv[0].len = 2;
        attv[0].p = av0;
        attv[1].len = 1;
        attv[1].p = av1;
        if (nc_put_att(ncid, NC_GLOBAL, "history", vint, 2, attv))
            return 5;
        if (nc_put_att(ncid, varid, "expected", vint, 2, attv))
            return 5;
        d = 7;
        if (nc_put_att_int(ncid, dvarid, "scale", NC_INT, 1, &d))
            return 5;
    }
    if (nc_enddef(ncid))
        return 6;

    /* Write the int vlen variable. */
    fill_mileage(in);
    start[0] = 0;
    count[0] = NV;
    if (nc_put_vara(ncid, varid, start, count, in))
        return 7;

    /* Strided write on the double vlen variable. */
    din[0].len = 2;
    din[0].p = (void *)depths1;
    din[1].len = 0;
    din[1].p = NULL;
    start[0] = 0;
    count[0] = 2;
    stride[0] = 2;
    if (nc_put_vars(ncid, dvarid, start, count, stride, din))
        return 8;
    /* Scalar slots through var1. */
    din[0].len = 1;
    din[0].p = (void *)depths1;
    start[0] = 1;
    if (nc_put_var1(ncid, dvarid, start, din))
        return 8;
    start[0] = 3;
    if (nc_put_var1(ncid, dvarid, start, din))
        return 8;

    /* Vlen-of-compound variable. */
    cin[0].len = 2;
    cin[0].p = (void *)track0;
    cin[1].len = 0;
    cin[1].p = NULL;
    cin[2].len = 1;
    cin[2].p = (void *)track0;
    cin[3].len = 1;
    cin[3].p = (void *)track3;
    start[0] = 0;
    count[0] = NV;
    if (nc_put_vara(ncid, cvarid, start, count, cin))
        return 9;

    /* Nested vlen: two inner vlens of struct point. */
    inner_a[0].len = 2;
    inner_a[0].p = (void *)track0;
    inner_a[1].len = 0;
    inner_a[1].p = NULL;
    inner_b[0].len = 1;
    inner_b[0].p = (void *)track3;
    nin[0].len = 2;
    nin[0].p = inner_a;
    nin[1].len = 0;
    nin[1].p = NULL;
    nin[2].len = 1;
    nin[2].p = inner_b;
    nin[3].len = 0;
    nin[3].p = NULL;
    if (nc_put_vara(ncid, nvarid, start, count, nin))
        return 10;

    /* Compound containing a vlen member and an array-of-vlen
     * field. */
    {
        int rv[2] = {11, 12};
        int p0[1] = {13}, p1[2] = {14, 15};
        rin_data.id = 55;
        rin_data.vals.len = 2;
        rin_data.vals.p = rv;
        rin_data.past[0].len = 1;
        rin_data.past[0].p = p0;
        rin_data.past[1].len = 2;
        rin_data.past[1].p = p1;
        rin[0] = rin_data.vals;
        /* Write the full rec through a compound buffer. */
        start[0] = 0;
        count[0] = 1;
        if (nc_put_vara(ncid, rvarid, start, count, &rin_data))
            return 11;
        rin_data.id = 56;
        rin_data.vals.len = 0;
        rin_data.vals.p = NULL;
        rin_data.past[0].len = 0;
        rin_data.past[0].p = NULL;
        rin_data.past[1].len = 1;
        rin_data.past[1].p = p0;
        start[0] = 1;
        count[0] = 1;
        if (nc_put_vara(ncid, rvarid, start, count, &rin_data))
            return 11;
    }

    /* Two records on the unlimited variable via separate puts. */
    fill_mileage(in);
    start[0] = 0;
    count[0] = 1;
    if (nc_put_vara(ncid, ovarid, start, count, in))
        return 12;
    start[0] = 1;
    count[0] = 1;
    if (nc_put_vara(ncid, ovarid, start, count, in + 2))
        return 12;

    if (nc_close(ncid))
        return 13;
    return 0;
}

/* Verify every vlen datum in an open file (used for both fresh
 * reopens and upstream files). `with_extras` selects the native-only
 * compound/nested variables that upstream interchange does not
 * exercise. */
static int
verify_vlen_file(int ncid, int with_extras)
{
    int varid, dimid;
    nc_type cmpid, vint, vdbl, vcmp, vnest, recid;
    nc_vlen_t got[NV], g2[2], g1;
    size_t start[1], count[1], len;
    int i;

    if (nc_inq_dimid(ncid, "n", &dimid))
        return 20;
    if (nc_inq_dimlen(ncid, dimid, &len) || len != NV)
        return 20;
    if (nc_inq_typeid(ncid, "point", &cmpid))
        return 21;
    if (nc_inq_typeid(ncid, "mileage", &vint))
        return 21;
    if (nc_inq_typeid(ncid, "depths", &vdbl))
        return 21;
    if (nc_inq_typeid(ncid, "track", &vcmp))
        return 21;
    if (nc_inq_typeid(ncid, "nested", &vnest))
        return 21;
    if (nc_inq_typeid(ncid, "record", &recid))
        return 21;
    {
        char nm[NC_MAX_NAME + 1];
        size_t sz, nf;
        nc_type base;
        int cls;
        if (nc_inq_user_type(ncid, vint, nm, &sz, &base, &nf, &cls))
            return 22;
        if (cls != NC_VLEN || base != NC_INT || sz != sizeof(nc_vlen_t))
            return 22;
        if (nc_inq_user_type(ncid, vcmp, nm, &sz, &base, &nf, &cls))
            return 23;
        if (cls != NC_VLEN || base != cmpid)
            return 23;
        if (nc_inq_user_type(ncid, vnest, nm, &sz, &base, &nf, &cls))
            return 24;
        if (cls != NC_VLEN || base != vcmp)
            return 24;
    }

    /* Int vlen variable: whole-var read then nc_reclaim_data. */
    if (nc_inq_varid(ncid, "miles", &varid))
        return 25;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 25;
    if (check_mileage(got))
        return 26;
    if (nc_reclaim_data(ncid, vint, got, NV))
        return 26;
    for (i = 0; i < NV; i++)
        got[i].p = NULL;

    /* nc_get_vara subset and a strided read. */
    start[0] = 2;
    count[0] = 2;
    memset(got, 0, sizeof(got));
    if (nc_get_vara(ncid, varid, start, count, got))
        return 27;
    if (!vlen_int_eq(&got[0], &(nc_vlen_t){1, (void *)mileage2}) ||
        got[0].len != 1 || ((int *)got[0].p)[0] != 42 ||
        got[1].len != 2 || ((int *)got[1].p)[0] != 300)
        return 27;
    if (nc_reclaim_data(ncid, vint, got, 2))
        return 27;

    /* Legacy flat free: nc_free_vlen on a non-nested payload. */
    start[0] = 0;
    memset(&g1, 0, sizeof(g1));
    if (nc_get_var1(ncid, varid, start, &g1))
        return 28;
    if (g1.len != 3 || ((int *)g1.p)[2] != 500)
        return 28;
    if (nc_free_vlen(&g1))
        return 28;

    /* Double vlen written strided + scalar. */
    if (nc_inq_varid(ncid, "depth", &varid))
        return 29;
    memset(g2, 0, sizeof(g2));
    if (nc_get_var(ncid, varid, got))
        return 29;
    if (got[0].len != 2 || got[1].len != 1 || got[2].len != 0 ||
        got[3].len != 1)
        return 29;
    {
        double *d0 = (double *)got[0].p;
        double *d1 = (double *)got[1].p;
        double *d3 = (double *)got[3].p;
        if (d0[0] != 1.5 || d0[1] != -2.5 || d1[0] != 1.5 ||
            d3[0] != 1.5)
            return 29;
    }
    if (nc_reclaim_data(ncid, vdbl, got, NV))
        return 30;

    /* Attributes. */
    {
        nc_vlen_t ag[2];
        memset(ag, 0, sizeof(ag));
        if (nc_get_att(ncid, NC_GLOBAL, "history", ag))
            return 31;
        if (ag[0].len != 2 || ((int *)ag[0].p)[1] != 2 ||
            ag[1].len != 1 || ((int *)ag[1].p)[0] != 3)
            return 31;
        if (nc_reclaim_data(ncid, vint, ag, 2))
            return 31;
        if (nc_inq_varid(ncid, "miles", &varid))
            return 32;
        memset(ag, 0, sizeof(ag));
        if (nc_get_att(ncid, varid, "expected", ag))
            return 32;
        if (ag[0].len != 2 || ag[1].len != 1)
            return 32;
        if (nc_reclaim_data(ncid, vint, ag, 2))
            return 32;
    }

    /* Unlimited variable: two records. */
    {
        size_t ulen;
        nc_vlen_t ug[2];
        if (nc_inq_varid(ncid, "series", &varid))
            return 33;
        if (nc_inq_dimid(ncid, "t", &dimid) ||
            nc_inq_dimlen(ncid, dimid, &ulen) || ulen != 2)
            return 33;
        memset(ug, 0, sizeof(ug));
        if (nc_get_var(ncid, varid, ug))
            return 33;
        if (ug[0].len != 3 || ug[1].len != 1 ||
            ((int *)ug[0].p)[2] != 500 || ((int *)ug[1].p)[0] != 42)
            return 34;
        if (nc_reclaim_data(ncid, vint, ug, 2))
            return 34;
    }

    if (!with_extras)
        return 0;

    /* Vlen of compound. */
    if (nc_inq_varid(ncid, "tracks", &varid))
        return 35;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 35;
    if (!vlen_point_eq(&got[0], &(nc_vlen_t){2, (void *)track0}) ||
        got[1].len != 0 ||
        !vlen_point_eq(&got[2], &(nc_vlen_t){1, (void *)track0}) ||
        !vlen_point_eq(&got[3], &(nc_vlen_t){1, (void *)track3}))
        return 36;
    if (nc_reclaim_data(ncid, vcmp, got, NV))
        return 36;

    /* Nested vlen. */
    if (nc_inq_varid(ncid, "nest_var", &varid))
        return 37;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 37;
    {
        nc_vlen_t *ia = (nc_vlen_t *)got[0].p;
        nc_vlen_t *ib = (nc_vlen_t *)got[2].p;
        if (got[0].len != 2 || got[1].len != 0 || got[2].len != 1 ||
            got[3].len != 0)
            return 38;
        if (!ia || !ib)
            return 38;
        if (!vlen_point_eq(&ia[0], &(nc_vlen_t){2, (void *)track0}) ||
            ia[1].len != 0 ||
            !vlen_point_eq(&ib[0], &(nc_vlen_t){1, (void *)track3}))
            return 38;
    }
    if (nc_reclaim_data(ncid, vnest, got, NV))
        return 38;

    /* Compound containing a vlen member. */
    if (nc_inq_varid(ncid, "records", &varid))
        return 39;
    {
        struct rec rg[2];
        int *v0;
        memset(rg, 0, sizeof(rg));
        start[0] = 0;
        count[0] = 2;
        if (nc_get_vara(ncid, varid, start, count, rg))
            return 39;
        if (rg[0].id != 55 || rg[0].vals.len != 2 || rg[1].id != 56 ||
            rg[1].vals.len != 0)
            return 40;
        v0 = (int *)rg[0].vals.p;
        if (!v0 || v0[0] != 11 || v0[1] != 12)
            return 40;
        /* Array-of-vlen field. */
        if (rg[0].past[0].len != 1 ||
            ((int *)rg[0].past[0].p)[0] != 13 ||
            rg[0].past[1].len != 2 ||
            ((int *)rg[0].past[1].p)[1] != 15)
            return 40;
        if (rg[1].past[0].len != 0 || rg[1].past[1].len != 1 ||
            ((int *)rg[1].past[1].p)[0] != 13)
            return 40;
        if (nc_reclaim_data(ncid, recid, rg, 2))
            return 40;
    }
    (void)g2;
    (void)count;
    return 0;
}

/* One full create/write/reopen/verify cycle in mode `cmode`. */
static int
run_roundtrip(const char *fname, int cmode)
{
    int ncid;
    int ret;

    if ((ret = write_roundtrip(fname, cmode)))
        return ret;
    if (nc_open(fname, NC_NEXTCDF4, &ncid))
        return 50;
    if ((ret = verify_vlen_file(ncid, 1)))
        return ret;
    if (nc_close(ncid))
        return 51;
    return 0;
}

/* Element helpers: borrowed descriptors, real base sizes, error
 * handling. */
static int
run_helpers(void)
{
    int ncid;
    nc_type vint, vbyte, vdbl, vcmp, cmpid, dummy1, dummy2, dummy3;
    nc_vlen_t el;
    size_t len;
    unsigned char bbuf[4];
    double dbuf[4];
    static const unsigned char bvals[4] = {1, 2, 3, 4};
    static const double dvals[2] = {3.25, -0.5};
    int rc;

    if (nc_create(FILE_ERR, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_def_vlen(ncid, "vi", NC_INT, &vint))
        return 2;
    if (nc_def_vlen(ncid, "vb", NC_BYTE, &vbyte))
        return 2;
    if (nc_def_vlen(ncid, "vd", NC_DOUBLE, &vdbl))
        return 2;

    /* put_vlen_element borrows the payload pointer. */
    if (nc_put_vlen_element(ncid, (int)vint, &el, 3, mileage0))
        return 3;
    if (el.len != 3 || el.p != (void *)mileage0)
        return 3;
    if (nc_put_vlen_element(ncid, (int)vint, &el, 0, NULL))
        return 3;
    if (el.len != 0)
        return 3;
    if (nc_put_vlen_element(ncid, (int)vint, &el, 2, NULL) != NC_EINVAL)
        return 4;
    if (nc_put_vlen_element(ncid, (int)vint, NULL, 1, mileage0) !=
        NC_EINVAL)
        return 4;
    if (nc_put_vlen_element(ncid, (int)NC_INT, &el, 1, mileage0) !=
        NC_EBADTYPE)
        return 4;

    /* get_vlen_element copies len * real base size — a byte base
     * catches upstream's hard-coded 4-byte bug. */
    if (nc_put_vlen_element(ncid, (int)vbyte, &el, 4, bvals))
        return 5;
    memset(bbuf, 0, sizeof(bbuf));
    if (nc_get_vlen_element(ncid, (int)vbyte, &el, &len, bbuf))
        return 5;
    if (len != 4 || memcmp(bbuf, bvals, 4))
        return 5;

    if (nc_put_vlen_element(ncid, (int)vdbl, &el, 2, dvals))
        return 6;
    memset(dbuf, 0, sizeof(dbuf));
    if (nc_get_vlen_element(ncid, (int)vdbl, &el, &len, dbuf))
        return 6;
    if (len != 2 || dbuf[0] != 3.25 || dbuf[1] != -0.5)
        return 6;

    /* Zero-length descriptor: NULL data permitted. */
    if (nc_put_vlen_element(ncid, (int)vint, &el, 0, NULL))
        return 7;
    if (nc_get_vlen_element(ncid, (int)vint, &el, &len, NULL))
        return 7;
    if (len != 0)
        return 7;

    /* Error paths. */
    if (nc_get_vlen_element(ncid, (int)vint, &el, NULL, bbuf) !=
        NC_EINVAL)
        return 8;
    el.len = 2;
    el.p = NULL;
    if (nc_get_vlen_element(ncid, (int)vint, &el, &len, bbuf) !=
        NC_EINVAL)
        return 8;
    el.p = (void *)mileage0;
    if (nc_get_vlen_element(ncid, (int)vint, &el, &len, NULL) !=
        NC_EINVAL)
        return 8;

    /* A compound typeid is not a vlen. */
    if (nc_def_compound(ncid, sizeof(struct point), CMP_NAME, &cmpid))
        return 9;
    if (nc_insert_compound(ncid, cmpid, "x",
                           offsetof(struct point, x), NC_INT))
        return 9;
    if (nc_put_vlen_element(ncid, (int)cmpid, &el, 1, mileage0) !=
        NC_EBADTYPE)
        return 9;
    (void)vcmp; (void)dummy1; (void)dummy2; (void)dummy3;
    if (nc_close(ncid))
        return 10;
    (void)rc;
    return 0;
}

/* Type-graph rejections and mode checks. */
static int
run_errors(void)
{
    int ncid, dimid;
    nc_type cmpid, vint, bad;
    int varid;

    /* String bases work; reference bases remain unsupported. */
    if (nc_create(FILE_ERR, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_def_vlen(ncid, "vs", NC_STRING, &bad))
        return 2;
    if (nc_def_vlen(ncid, "vr", NC_STD_REF, &bad) != NC_EBADTYPE)
        return 2;
    /* Unknown type id. */
    if (nc_def_vlen(ncid, "vx", (nc_type)9999, &bad) != NC_EBADTYPE)
        return 2;

    /* Cycle: compound C holding vlen-of-C is rejected at insert. */
    if (nc_def_compound(ncid, sizeof(struct rec), "c", &cmpid))
        return 3;
    if (nc_def_vlen(ncid, "vc", cmpid, &vint))
        return 3;
    if (nc_insert_compound(ncid, cmpid, "v",
                           offsetof(struct rec, vals), vint) !=
        NC_EBADTYPE)
        return 3;
    if (nc_insert_compound(ncid, cmpid, "id",
                           offsetof(struct rec, id), NC_INT))
        return 3;
    /* String fields use pointer-sized storage. */
    if (nc_def_compound(ncid, sizeof(char *), "cs", &bad) ||
        nc_insert_compound(ncid, bad, "s", 0, NC_STRING))
        return 4;
    /* ...and a reference field. */
    if (nc_insert_compound(ncid, cmpid, "r", 0, NC_STD_REF) !=
        NC_EBADTYPE)
        return 4;
    /* A vlen member in the compound is legal though. */
    if (nc_def_vlen(ncid, "vi", NC_INT, &vint))
        return 5;
    if (nc_insert_compound(ncid, cmpid, "vals",
                           offsetof(struct rec, vals), vint))
        return 5;
    if (nc_close(ncid))
        return 6;

    /* Classic model rejects user-defined types entirely. */
    if (nc_create(FILE_ERR, NC_NEXTCDF4 | NC_CLASSIC_MODEL, &ncid))
        return 7;
    if (nc_def_vlen(ncid, "vi", NC_INT, &vint) != NC_ENOTNC4)
        return 7;
    if (nc_def_dim(ncid, "n", 2, &dimid))
        return 8;
    if (nc_def_var(ncid, "v", NC_INT, 1, &dimid, &varid))
        return 8;
    if (nc_close(ncid))
        return 9;
    return 0;
}

/* Attribute ownership: put frees caller storage; independent gets;
 * replace, rename, copy, delete, reclaim cycles. */
static int
run_attrs(void)
{
    int ncid, ncid2;
    nc_type vint;
    nc_vlen_t attv[2], g[2], g2[2];
    int rep = 0, i;
    int *dyn;
    char name[NC_MAX_NAME + 1];
    size_t alen;
    nc_type atype;

    for (rep = 0; rep < 3; rep++) {
        if (nc_create(FILE_ATT, NC_NEXTCDF4, &ncid))
            return 1;
        if (nc_def_vlen(ncid, "vi", NC_INT, &vint))
            return 2;

        /* Payload owned by us, freed immediately after put. */
        dyn = malloc(3 * sizeof(int));
        if (!dyn)
            return 3;
        dyn[0] = 5; dyn[1] = 6; dyn[2] = 7;
        attv[0].len = 3;
        attv[0].p = dyn;
        attv[1].len = 1;
        attv[1].p = dyn;
        if (nc_put_att(ncid, NC_GLOBAL, "h", vint, 2, attv))
            return 4;
        /* Mutate and free the source — the attribute must be
         * unaffected. */
        dyn[0] = -99;
        free(dyn);

        memset(g, 0, sizeof(g));
        memset(g2, 0, sizeof(g2));
        if (nc_get_att(ncid, NC_GLOBAL, "h", g))
            return 5;
        if (nc_get_att(ncid, NC_GLOBAL, "h", g2))
            return 5;
        if (g[0].len != 3 || g2[0].len != 3 ||
            ((int *)g[0].p)[0] != 5 || ((int *)g2[0].p)[2] != 7)
            return 6;
        /* The two results are independent deep copies. */
        if (g[0].p == g2[0].p)
            return 6;
        ((int *)g[0].p)[0] = -1;
        if (((int *)g2[0].p)[0] != 5)
            return 6;
        /* A third read still sees the stored value. */
        {
            nc_vlen_t g3[2];
            memset(g3, 0, sizeof(g3));
            if (nc_get_att(ncid, NC_GLOBAL, "h", g3))
                return 7;
            if (g3[0].len != 3 || ((int *)g3[0].p)[0] != 5)
                return 7;
            if (nc_reclaim_data(ncid, vint, g3, 2))
                return 7;
        }

        /* Replace keeps the index position. */
        {
            int nv[1] = {77};
            attv[0].len = 1;
            attv[0].p = nv;
            attv[1].len = 0;
            attv[1].p = NULL;
            if (nc_put_att(ncid, NC_GLOBAL, "h", vint, 2, attv))
                return 8;
        }
        if (nc_reclaim_data(ncid, vint, g, 2))
            return 8;
        if (nc_reclaim_data(ncid, vint, g2, 2))
            return 8;
        memset(g, 0, sizeof(g));
        if (nc_get_att(ncid, NC_GLOBAL, "h", g))
            return 9;
        if (g[0].len != 1 || ((int *)g[0].p)[0] != 77 || g[1].len != 0)
            return 9;
        if (nc_reclaim_data(ncid, vint, g, 2))
            return 9;

        /* Rename, inquire, copy to another file, delete. */
        if (nc_rename_att(ncid, NC_GLOBAL, "h", "h2"))
            return 10;
        if (nc_inq_att(ncid, NC_GLOBAL, "h2", &atype, &alen))
            return 10;
        if (atype != vint || alen != 2)
            return 10;
        if (nc_get_att(ncid, NC_GLOBAL, "h", g) != NC_ENOTATT)
            return 10;

        if (nc_create(FILE_ATT ".copy", NC_NEXTCDF4, &ncid2))
            return 11;
        if (nc_def_vlen(ncid2, "vi", NC_INT, &vint))
            return 11;
        if (nc_copy_att(ncid, NC_GLOBAL, "h2", ncid2, NC_GLOBAL))
            return 11;
        memset(g2, 0, sizeof(g2));
        if (nc_get_att(ncid2, NC_GLOBAL, "h2", g2))
            return 11;
        if (g2[0].len != 1 || ((int *)g2[0].p)[0] != 77)
            return 11;
        if (nc_reclaim_data(ncid2, vint, g2, 2))
            return 11;
        if (nc_close(ncid2))
            return 11;

        if (nc_del_att(ncid, NC_GLOBAL, "h2"))
            return 12;
        if (nc_inq_attname(ncid, NC_GLOBAL, 0, name) != NC_ENOTATT)
            return 12;
        if (nc_close(ncid))
            return 13;
    }
    (void)i;
    return 0;
}

/* Enddef, redef, then add more vlen metadata; verify on reopen. */
static int
run_redef(void)
{
    int ncid, dimid, varid;
    nc_type vint;
    nc_vlen_t in[1], got[1];
    size_t start[1] = {0}, count[1] = {1};
    int payload[2] = {1, 2};

    if (nc_create(FILE_REDEF, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_def_dim(ncid, "n", 1, &dimid))
        return 2;
    if (nc_def_vlen(ncid, "vi", NC_INT, &vint))
        return 2;
    if (nc_def_var(ncid, "v1", vint, 1, &dimid, &varid))
        return 2;
    if (nc_enddef(ncid))
        return 3;
    if (nc_redef(ncid))
        return 3;
    if (nc_def_var(ncid, "v2", vint, 1, &dimid, &varid))
        return 4;
    if (nc_put_att(ncid, NC_GLOBAL, "g", vint, 0, NULL))
        return 4;
    if (nc_enddef(ncid))
        return 5;

    in[0].len = 2;
    in[0].p = payload;
    if (nc_inq_varid(ncid, "v1", &varid) ||
        nc_put_vara(ncid, varid, start, count, in))
        return 6;
    if (nc_inq_varid(ncid, "v2", &varid) ||
        nc_put_vara(ncid, varid, start, count, in))
        return 6;
    if (nc_close(ncid))
        return 7;

    if (nc_open(FILE_REDEF, NC_NEXTCDF4, &ncid))
        return 8;
    if (nc_inq_varid(ncid, "v1", &varid))
        return 9;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 9;
    if (got[0].len != 2 || ((int *)got[0].p)[1] != 2)
        return 9;
    if (nc_reclaim_data(ncid, vint, got, 1))
        return 9;
    if (nc_inq_varid(ncid, "v2", &varid))
        return 10;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 10;
    if (nc_reclaim_data(ncid, vint, got, 1))
        return 10;
    if (nc_close(ncid))
        return 11;
    return 0;
}

/* Upstream -> NEXTCDF-4: an upstream netCDF-4 file with a vlen var
 * and attribute must open and read correctly. */
static int
run_upstream_in(void)
{
    int ncid, dimid, varid;
    nc_type vint;
    nc_vlen_t in[2], got[2];
    int a0[2] = {21, 22};

    if (nc_create(FILE_UP_IN, NC_NETCDF4, &ncid))
        return 1;
    if (nc_def_dim(ncid, "n", 2, &dimid))
        return 2;
    if (nc_def_vlen(ncid, "up_vlen", NC_INT, &vint))
        return 3;
    if (nc_def_var(ncid, "v", vint, 1, &dimid, &varid))
        return 4;
    in[0].len = 3;
    in[0].p = (void *)mileage0;
    in[1].len = 2;
    in[1].p = a0;
    if (nc_put_att(ncid, NC_GLOBAL, "g", vint, 2, in))
        return 5;
    if (nc_put_var(ncid, varid, in))
        return 5;
    if (nc_close(ncid))
        return 6;

    if (nc_open(FILE_UP_IN, NC_NEXTCDF4, &ncid))
        return 7;
    if (nc_inq_typeid(ncid, "up_vlen", &vint))
        return 8;
    if (nc_inq_varid(ncid, "v", &varid))
        return 8;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 9;
    if (got[0].len != 3 || ((int *)got[0].p)[2] != 500 ||
        got[1].len != 2 || ((int *)got[1].p)[1] != 22)
        return 9;
    if (nc_reclaim_data(ncid, vint, got, 2))
        return 10;
    memset(got, 0, sizeof(got));
    if (nc_get_att(ncid, NC_GLOBAL, "g", got))
        return 11;
    if (got[0].len != 3 || got[1].len != 2)
        return 11;
    if (nc_reclaim_data(ncid, vint, got, 2))
        return 11;
    if (nc_close(ncid))
        return 12;
    return 0;
}

/* NEXTCDF-4 (model mode) -> upstream: a NEXTCDF-4 file must read
 * back through plain NC_NETCDF4. */
static int
run_upstream_out(void)
{
    int ncid, dimid, varid;
    nc_type vint;
    nc_vlen_t in[2], got[2];
    int a0[2] = {31, 32};

    if (nc_create(FILE_UP_OUT, NC_NEXTCDF4 | NC_NETCDF4_MODEL, &ncid))
        return 1;
    if (nc_def_dim(ncid, "n", 2, &dimid))
        return 2;
    if (nc_def_vlen(ncid, "nx_vlen", NC_INT, &vint))
        return 3;
    if (nc_def_var(ncid, "v", vint, 1, &dimid, &varid))
        return 4;
    in[0].len = 3;
    in[0].p = (void *)mileage0;
    in[1].len = 2;
    in[1].p = a0;
    if (nc_put_var(ncid, varid, in))
        return 5;
    if (nc_put_att(ncid, NC_GLOBAL, "g", vint, 2, in))
        return 5;
    if (nc_close(ncid))
        return 6;

    if (nc_open(FILE_UP_OUT, NC_NETCDF4, &ncid))
        return 7;
    if (nc_inq_varid(ncid, "v", &varid))
        return 8;
    memset(got, 0, sizeof(got));
    if (nc_get_var(ncid, varid, got))
        return 9;
    if (got[0].len != 3 || ((int *)got[0].p)[0] != 300 ||
        got[1].len != 2 || ((int *)got[1].p)[0] != 31)
        return 9;
    if (nc_reclaim_data(ncid, vint, got, 2))
        return 10;
    memset(got, 0, sizeof(got));
    if (nc_get_att(ncid, NC_GLOBAL, "g", got))
        return 11;
    if (got[1].len != 2 || ((int *)got[1].p)[1] != 32)
        return 11;
    if (nc_reclaim_data(ncid, vint, got, 2))
        return 11;
    if (nc_close(ncid))
        return 12;
    return 0;
}

int
main(void)
{
    int fails = 0;
    struct {
        const char *name;
        int (*fn)(void);
    } tests[] = {
        {"roundtrip native", NULL}, {"roundtrip model", NULL},
        {"element helpers", run_helpers},
        {"errors", run_errors},
        {"attributes", run_attrs},
        {"redef", run_redef},
        {"upstream read", run_upstream_in},
        {"upstream write", run_upstream_out},
    };
    size_t i;

    if (!NC_NEXTCDF4_initialize()) {
        fprintf(stderr, "initialize failed\n");
        return 1;
    }

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        int r;
        if (i == 0)
            r = run_roundtrip(FILE_NATIVE, 0);
        else if (i == 1)
            r = run_roundtrip(FILE_MODEL, NC_NETCDF4_MODEL);
        else
            r = tests[i].fn();
        printf("%-18s %s (rc=%d)\n", tests[i].name,
               r ? "FAIL" : "ok", r);
        fails += (r != 0);
    }
    printf("%s\n", fails ? "FAILURES" : "all tests passed");
    return fails ? 1 : 0;
}

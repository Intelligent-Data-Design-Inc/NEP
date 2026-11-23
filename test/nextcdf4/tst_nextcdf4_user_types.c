/* Tests fixed-size user-defined types (compound, enum, opaque) for
 * NEXTCDF-4: definition, I/O, attributes, inquiry, reopen and
 * upstream NetCDF-4 interchange, in both native and
 * NC_NETCDF4_MODEL modes.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NATIVE "tst_nextcdf4_user_types_native.h5"
#define FILE_MODEL "tst_nextcdf4_user_types_model.h5"
#define FILE_UP_IN "tst_nextcdf4_user_types_upin.h5"
#define FILE_UP_OUT "tst_nextcdf4_user_types_upout.h5"

#define ND 4
#define ND2 8
#define CMP_NAME "sample"
#define ENUM_NAME "cloud_class"
#define OPAQUE_NAME "blob"

/* Compound element used throughout the test: three scalar atomic
 * fields with a deliberate padding byte. */
struct sample {
    int id;
    float temp;
    unsigned char flag;
    unsigned char pad;
};

#define CMP_SIZE ((int)sizeof(struct sample))

static const struct sample data_in[ND2] = {
    {1, 10.5f, 7, 0}, {2, -3.25f, 0, 0}, {3, 0.0f, 255, 0}, {4, 99.75f, 1, 0},
    {5, -1.0f, 4, 0}, {6, 7.75f, 2, 0}, {7, 42.0f, 8, 0}, {8, 0.5f, 16, 0}
};
static const int enum_vals[ND] = {0, 2, 1, 3};
static const unsigned char opaque_in[ND][8] = {
    {0,1,2,3,4,5,6,7}, {7,6,5,4,3,2,1,0},
    {0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22}, {9,9,9,9,9,9,9,9}
};

/* Define the three test types on the group ncid refers to. The enum
 * includes a member with the sign bit set to check signed/unsigned
 * handling. */
static int
def_types(int ncid, nc_type *cmpid, nc_type *enumid, nc_type *opaqueid)
{
    int v;
    unsigned char u8;

    if (nc_def_compound(ncid, sizeof(struct sample), CMP_NAME, cmpid))
        return 1;
    if (nc_insert_compound(ncid, *cmpid, "id",
                           offsetof(struct sample, id), NC_INT))
        return 1;
    if (nc_insert_compound(ncid, *cmpid, "temp",
                           offsetof(struct sample, temp), NC_FLOAT))
        return 1;
    if (nc_insert_compound(ncid, *cmpid, "flag",
                           offsetof(struct sample, flag), NC_UBYTE))
        return 1;

    if (nc_def_enum(ncid, NC_INT, ENUM_NAME, enumid))
        return 1;
    v = 0; if (nc_insert_enum(ncid, *enumid, "clear", &v)) return 1;
    v = 1; if (nc_insert_enum(ncid, *enumid, "cumulus", &v)) return 1;
    v = -2; if (nc_insert_enum(ncid, *enumid, "missing", &v)) return 1;
    v = 3; if (nc_insert_enum(ncid, *enumid, "cirrus", &v)) return 1;

    if (nc_def_opaque(ncid, 8, OPAQUE_NAME, opaqueid))
        return 1;
    (void)u8;
    return 0;
}

/* Check all type-level inquiries on the types def_types() made. */
static int
check_type_inq(int ncid, nc_type cmpid, nc_type enumid, nc_type opaqueid)
{
    char name[NC_MAX_NAME + 1];
    size_t size, nfields;
    nc_type base;
    int classp, ndims;
    size_t offset;
    nc_type ftype;
    int ival;
    char ident[NC_MAX_NAME + 1];
    int ntypes, typeids[8];

    if (nc_inq_user_type(ncid, cmpid, name, &size, &base, &nfields,
                         &classp))
        return 10;
    if (strcmp(name, CMP_NAME) || size != sizeof(struct sample) ||
        classp != NC_COMPOUND || nfields != 3)
        return 11;
    if (nc_inq_compound_field(ncid, cmpid, 1, name, &offset, &ftype,
                              &ndims, NULL))
        return 12;
    if (strcmp(name, "temp") || offset != offsetof(struct sample, temp) ||
        ftype != NC_FLOAT || ndims != 0)
        return 13;
    if (nc_inq_user_type(ncid, enumid, name, &size, &base, &nfields,
                         &classp))
        return 14;
    if (classp != NC_ENUM || base != NC_INT || nfields != 4 || size != 4)
        return 15;
    ival = -2;
    if (nc_inq_enum_member(ncid, enumid, 2, name, &ival))
        return 16;
    if (strcmp(name, "missing") || ival != -2)
        return 17;
    if (nc_inq_enum_ident(ncid, enumid, -2, ident))
        return 18;
    if (strcmp(ident, "missing"))
        return 19;
    if (nc_inq_user_type(ncid, opaqueid, name, &size, &base, &nfields,
                         &classp))
        return 20;
    if (classp != NC_OPAQUE || size != 8)
        return 21;
    if (nc_inq_typeids(ncid, &ntypes, typeids))
        return 22;
    if (ntypes != 3)
        return 23;
    if (nc_inq_typeid(ncid, CMP_NAME, &ftype) || ftype != cmpid)
        return 24;
    return 0;
}

/* One full create/write/reopen/verify cycle in mode `cmode` on file
 * `fname`. */
static int
run_roundtrip(const char *fname, int cmode)
{
    int ncid, dimid, dimid2, varid, evarid, ovarid, g2;
    nc_type cmpid, enumid, opaqueid;
    struct sample out[ND2];
    int eout[ND];
    unsigned char oout[ND][8];
    size_t start[2] = {0, 0}, count[2] = {2, 2};
    ptrdiff_t stride[2] = {1, 1};
    nc_type xtype;
    int ret;

    unlink(fname);
    if ((ret = nc_create(fname, NC_NEXTCDF4 | NC_CLOBBER | cmode, &ncid)))
        { fprintf(stderr, "create %s: %s\n", fname, nc_strerror(ret)); return 1; }
    if (def_types(ncid, &cmpid, &enumid, &opaqueid))
        return 2;
    if (check_type_inq(ncid, cmpid, enumid, opaqueid))
        { fprintf(stderr, "check_type_inq mode 0x%x\n", cmode); return 3; }

    if (nc_def_dim(ncid, "n", ND, &dimid))
        return 4;
    if (nc_def_dim(ncid, "m", 2, &dimid2))
        return 4;
    {
        int dims2[2] = {dimid, dimid2};
        if (nc_def_var(ncid, "samples", cmpid, 2, dims2, &varid))
            return 5;
    }
    if (nc_def_var(ncid, "classes", enumid, 1, &dimid, &evarid))
        return 6;
    if (nc_def_var(ncid, "blobs", opaqueid, 1, &dimid, &ovarid))
        return 7;

    /* Attributes before enddef: a user-typed global attribute queues
     * until the type is committed; atomic atts on a pending
     * user-typed var queue until its dataset exists. */
    {
        struct sample gatt = {99, 1.5f, 3, 0};
        if (nc_put_att(ncid, NC_GLOBAL, "reference", cmpid, 1, &gatt))
            return 8;
        if (nc_put_att_int(ncid, varid, "units_code", NC_INT, 1, &gatt.id))
            return 9;
        if (nc_put_att(ncid, evarid, "note", enumid, 2, enum_vals))
            return 10;
    }

    if (nc_enddef(ncid))
        return 11;

    /* Untyped I/O of every user-typed variable. */
    if (nc_put_var(ncid, varid, data_in))
        return 12;
    if (nc_put_var(ncid, evarid, enum_vals))
        return 13;
    if (nc_put_var(ncid, ovarid, opaque_in))
        return 14;

    memset(out, 0, sizeof(out));
    if (nc_get_var(ncid, varid, out))
        return 15;
    if (memcmp(out, data_in, sizeof(out)))
        return 16;
    memset(eout, 0, sizeof(eout));
    if (nc_get_var(ncid, evarid, eout))
        return 17;
    if (memcmp(eout, enum_vals, sizeof(eout)))
        return 18;
    memset(oout, 0, sizeof(oout));
    if (nc_get_var(ncid, ovarid, oout))
        return 19;
    if (memcmp(oout, opaque_in, sizeof(oout)))
        return 20;

    /* Strided hyperslab. */
    memset(out, 0, sizeof(out));
    if (nc_get_vars(ncid, varid, start, count, stride, out))
        return 21;
    if (memcmp(out, data_in, 4 * sizeof(struct sample)))
        return 22;

    /* Typed atomic buffers are rejected on user-typed vars. */
    {
        int ibuf[ND];
        if (nc_get_var_int(ncid, varid, ibuf) != NC_EBADTYPE)
            return 23;
        if (nc_put_var_int(ncid, varid, ibuf) != NC_EBADTYPE)
            return 24;
    }

    /* Variable and global attribute reads. */
    {
        int ucode = 0;
        struct sample gatt;
        int en[2];
        if (nc_get_att_int(ncid, varid, "units_code", &ucode))
            return 25;
        if (ucode != 99)
            return 26;
        if (nc_get_att(ncid, NC_GLOBAL, "reference", &gatt))
            return 27;
        if (gatt.id != 99 || gatt.temp != 1.5f || gatt.flag != 3)
            return 28;
        if (nc_get_att(ncid, evarid, "note", en))
            return 29;
        if (en[0] != enum_vals[0] || en[1] != enum_vals[1])
            return 30;
    }

    if (nc_close(ncid))
        return 31;

    /* Reopen and re-verify. */
    if ((ret = nc_open(fname, NC_NEXTCDF4, &ncid)))
        { fprintf(stderr, "open %s: %s\n", fname, nc_strerror(ret)); return 32; }
    {
        int ntypes, tids[8];
        nc_type c2, e2, o2;
        if (nc_inq_typeids(ncid, &ntypes, tids) || ntypes != 3)
            return 33;
        if (nc_inq_typeid(ncid, CMP_NAME, &c2) ||
            nc_inq_typeid(ncid, ENUM_NAME, &e2) ||
            nc_inq_typeid(ncid, OPAQUE_NAME, &o2))
            return 34;
        if (check_type_inq(ncid, c2, e2, o2))
            return 35;
        memset(out, 0, sizeof(out));
        if (nc_inq_varid(ncid, "samples", &varid))
            return 36;
        if (nc_inq_var(ncid, varid, NULL, &xtype, NULL, NULL, NULL) ||
            xtype != c2)
            return 37;
        if (nc_get_var(ncid, varid, out) ||
            memcmp(out, data_in, sizeof(out)))
            return 38;
        if (nc_inq_varid(ncid, "blobs", &ovarid) ||
            nc_get_var(ncid, ovarid, oout) ||
            memcmp(oout, opaque_in, sizeof(oout)))
            return 39;
        {
            struct sample gatt;
            if (nc_get_att(ncid, NC_GLOBAL, "reference", &gatt) ||
                gatt.id != 99)
                return 40;
        }
    }
    if (nc_close(ncid))
        return 41;
    (void)g2;
    return 0;
}

/* A compound defined in a parent group is usable in a child group;
 * the same name in a sibling group is a distinct type. */
static int
run_group_scope(void)
{
    int ncid, dimid, varid, sub;
    nc_type cmpid, sub_cmp;
    char name[NC_MAX_NAME + 1];
    size_t size;
    int ret;

    unlink(FILE_NATIVE);
    if ((ret = nc_create(FILE_NATIVE, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        return 1;
    if (def_types(ncid, &cmpid, &sub_cmp, &varid))
        return 2;
    if (nc_def_dim(ncid, "n", ND, &dimid))
        return 3;
    if (nc_def_grp(ncid, "child", &sub))
        return 4;

    /* Child sees the parent's type. */
    if (nc_def_var(sub, "sub_samples", cmpid, 1, &dimid, &varid))
        return 5;
    /* A sibling-scope duplicate name is allowed and distinct. */
    if (nc_def_compound(sub, 4, CMP_NAME, &sub_cmp))
        return 6;
    if (nc_insert_compound(sub, sub_cmp, "x", 0, NC_INT))
        return 7;
    if (sub_cmp == cmpid)
        return 8;
    if (nc_def_var(sub, "sub2", sub_cmp, 1, &dimid, &varid))
        return 9;
    /* A locally defined enum attribute on the subgroup. */
    {
        nc_type eid;
        int v = 5;
        if (nc_def_enum(sub, NC_UBYTE, "level", &eid))
            return 10;
        if (nc_insert_enum(sub, eid, "high", &v))
            return 11;
        {
            unsigned char lv = 5;
            if (nc_put_att(sub, NC_GLOBAL, "level_att", eid, 1, &lv))
                return 12;
        }
    }
    if (nc_enddef(ncid))
        return 13;
    {
        struct sample s2[ND];
        if (nc_inq_varid(sub, "sub_samples", &varid))
            return 14;
        if (nc_put_var(sub, varid, data_in))
            return 15;
        memset(s2, 0, sizeof(s2));
        if (nc_get_var(sub, varid, s2) || memcmp(s2, data_in, sizeof(s2)))
            return 16;
    }
    if (nc_close(ncid))
        return 17;

    /* Reopen: the child's own type and attribute survive, scoped to
     * the child group. */
    if ((ret = nc_open(FILE_NATIVE, NC_NEXTCDF4, &ncid)))
        return 18;
    if (nc_inq_ncid(ncid, "child", &sub))
        return 19;
    {
        int ntypes, tids[8];
        if (nc_inq_typeids(sub, &ntypes, tids) || ntypes != 2)
            return 20;
        if (nc_inq_typeid(sub, CMP_NAME, &sub_cmp))
            return 21;
        if (nc_inq_user_type(sub, sub_cmp, name, &size, NULL, NULL,
                             NULL))
            return 22;
        if (size != 4 || strcmp(name, CMP_NAME))
            return 23;
        {
            unsigned char lv = 0;
            if (nc_get_att(sub, NC_GLOBAL, "level_att", &lv) || lv != 5)
                return 24;
        }
        {
            struct sample s2[ND];
            if (nc_inq_varid(sub, "sub_samples", &varid))
                return 25;
            if (nc_get_var(sub, varid, s2) ||
                memcmp(s2, data_in, sizeof(s2)))
                return 26;
        }
    }
    if (nc_close(ncid))
        return 27;
    return 0;
}

/* Negative/error-path coverage in native mode. */
static int
run_errors(void)
{
    int ncid, dimid, varid;
    nc_type cmpid, enumid, opaqueid, bad;
    int ret, v = 0;
    int ibuf[4] = {0};

    unlink(FILE_NATIVE);
    if ((ret = nc_create(FILE_NATIVE, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        return 1;
    if (def_types(ncid, &cmpid, &enumid, &opaqueid))
        return 2;
    if (nc_def_dim(ncid, "n", ND, &dimid))
        return 3;

    /* Invalid type ids and out-of-scope types. */
    if (nc_def_var(ncid, "bad", (nc_type)9999, 1, &dimid, &varid) !=
        NC_EBADTYPE)
        return 4;
    if (nc_put_att(ncid, NC_GLOBAL, "bad", (nc_type)9999, 1, ibuf) !=
        NC_EBADTYPE)
        return 5;

    /* Field bounds and duplicate field name. */
    if (nc_insert_compound(ncid, cmpid, "toofar",
                           sizeof(struct sample), NC_INT) != NC_EINVAL)
        return 6;
    if (nc_insert_compound(ncid, cmpid, "id", 0, NC_INT) !=
        NC_ENAMEINUSE)
        return 7;

    /* Typed atomic attribute value rejected for user datatype. */
    if (nc_put_att_int(ncid, NC_GLOBAL, "badatt", cmpid, 1, ibuf) !=
        NC_EBADTYPE)
        return 8;

    if (nc_enddef(ncid))
        return 9;

    /* Committed types reject further member inserts. */
    if (nc_redef(ncid)) {
        /* After enddef redef is legal; inserts on the committed type
         * must still fail. */
        if (nc_insert_compound(ncid, cmpid, "late", 8, NC_INT) !=
            NC_ENOTINDEFINE)
            return 10;
        if (nc_insert_enum(ncid, enumid, "late", &v) != NC_ENOTINDEFINE)
            return 11;
        if (nc_enddef(ncid))
            return 12;
    }

    /* Read-only open rejects definitions. */
    if (nc_close(ncid))
        return 13;
    if ((ret = nc_open(FILE_NATIVE, NC_NEXTCDF4, &ncid)))
        return 14;
    if (nc_inq_typeid(ncid, CMP_NAME, &bad))
        return 15;
    if (nc_def_var(ncid, "late", bad, 0, NULL, &varid) != NC_EPERM)
        return 16;
    if (nc_put_att(ncid, NC_GLOBAL, "late", bad, 1, ibuf) != NC_EPERM)
        return 17;
    if (nc_close(ncid))
        return 18;
    return 0;
}

/* Classic-model files reject user types outright. */
static int
run_classic(void)
{
    int ncid;
    nc_type tid;
    int ret;

    unlink(FILE_NATIVE);
    if ((ret = nc_create(FILE_NATIVE,
                         NC_NEXTCDF4 | NC_CLOBBER | NC_CLASSIC_MODEL,
                         &ncid)))
        return 1;
    if (nc_def_compound(ncid, 4, "cmp", &tid) != NC_ENOTNC4)
        return 2;
    if (nc_def_enum(ncid, NC_INT, "e", &tid) != NC_ENOTNC4)
        return 3;
    if (nc_def_opaque(ncid, 4, "o", &tid) != NC_ENOTNC4)
        return 4;
    if (nc_close(ncid))
        return 5;
    return 0;
}

/* Upstream netcdf-c (NC_NETCDF4) writes a file with the three types;
 * NEXTCDF-4 reads it. */
static int
run_upstream_in(void)
{
    int ncid, dimid, varid;
    nc_type cmpid, enumid, opaqueid;
    int ret;

    unlink(FILE_UP_IN);
    if ((ret = nc_create(FILE_UP_IN, NC_NETCDF4 | NC_CLOBBER, &ncid))) {
        fprintf(stderr, "upstream create: %s\n", nc_strerror(ret));
        return 1;
    }
    if (def_types(ncid, &cmpid, &enumid, &opaqueid))
        return 2;
    if (nc_def_dim(ncid, "n", ND, &dimid))
        return 3;
    if (nc_def_var(ncid, "samples", cmpid, 1, &dimid, &varid))
        return 4;
    if (nc_def_var(ncid, "classes", enumid, 1, &dimid, &varid))
        return 5;
    if (nc_def_var(ncid, "blobs", opaqueid, 1, &dimid, &varid))
        return 6;
    {
        struct sample gatt = {7, 2.5f, 9, 0};
        if (nc_put_att(ncid, NC_GLOBAL, "reference", cmpid, 1, &gatt))
            return 7;
    }
    if (nc_enddef(ncid))
        return 8;
    if (nc_inq_varid(ncid, "samples", &varid) ||
        nc_put_var(ncid, varid, data_in))
        return 9;
    if (nc_inq_varid(ncid, "classes", &varid) ||
        nc_put_var(ncid, varid, enum_vals))
        return 10;
    if (nc_inq_varid(ncid, "blobs", &varid) ||
        nc_put_var(ncid, varid, opaque_in))
        return 11;
    if (nc_close(ncid))
        return 12;

    if ((ret = nc_open(FILE_UP_IN, NC_NEXTCDF4, &ncid))) {
        fprintf(stderr, "nextcdf4 open upstream file: %s\n",
                nc_strerror(ret));
        return 13;
    }
    {
        nc_type c2, e2, o2;
        struct sample out[ND];
        int eout[ND];
        unsigned char oout[ND][8];
        struct sample gatt;

        if (nc_inq_typeid(ncid, CMP_NAME, &c2) ||
            nc_inq_typeid(ncid, ENUM_NAME, &e2) ||
            nc_inq_typeid(ncid, OPAQUE_NAME, &o2))
            return 14;
        if (check_type_inq(ncid, c2, e2, o2))
            return 15;
        if (nc_inq_varid(ncid, "samples", &varid) ||
            nc_get_var(ncid, varid, out) ||
            memcmp(out, data_in, sizeof(out)))
            return 16;
        if (nc_inq_varid(ncid, "classes", &varid) ||
            nc_get_var(ncid, varid, eout) ||
            memcmp(eout, enum_vals, sizeof(eout)))
            return 17;
        if (nc_inq_varid(ncid, "blobs", &varid) ||
            nc_get_var(ncid, varid, oout) ||
            memcmp(oout, opaque_in, sizeof(oout)))
            return 18;
        if (nc_get_att(ncid, NC_GLOBAL, "reference", &gatt) ||
            gatt.id != 7 || gatt.temp != 2.5f || gatt.flag != 9)
            return 19;
    }
    if (nc_close(ncid))
        return 20;
    return 0;
}

/* NEXTCDF-4 writes a NC_NETCDF4_MODEL file; upstream netcdf-c reads
 * it back. */
static int
run_upstream_out(void)
{
    int ncid, varid;
    int ret;

    if (run_roundtrip(FILE_UP_OUT, NC_NETCDF4_MODEL))
        return 1;

    if ((ret = nc_open(FILE_UP_OUT, NC_NETCDF4, &ncid))) {
        fprintf(stderr, "upstream open nextcdf4 file: %s\n",
                nc_strerror(ret));
        return 2;
    }
    {
        nc_type c2, e2, o2;
        struct sample out[ND2];
        int eout[ND];
        unsigned char oout[ND][8];

        if (nc_inq_typeid(ncid, CMP_NAME, &c2) ||
            nc_inq_typeid(ncid, ENUM_NAME, &e2) ||
            nc_inq_typeid(ncid, OPAQUE_NAME, &o2)) {
            fprintf(stderr, "upstream typeid lookup failed\n");
            return 3;
        }
        if (nc_inq_varid(ncid, "samples", &varid) ||
            nc_get_var(ncid, varid, out) ||
            memcmp(out, data_in, sizeof(out))) {
            fprintf(stderr, "upstream compound read failed\n");
            return 4;
        }
        if (nc_inq_varid(ncid, "classes", &varid) ||
            nc_get_var(ncid, varid, eout) ||
            memcmp(eout, enum_vals, sizeof(eout)))
            return 5;
        if (nc_inq_varid(ncid, "blobs", &varid) ||
            nc_get_var(ncid, varid, oout) ||
            memcmp(oout, opaque_in, sizeof(oout)))
            return 6;
        {
            struct sample gatt;
            if (nc_get_att(ncid, NC_GLOBAL, "reference", &gatt) ||
                gatt.id != 99)
                return 7;
        }
    }
    if (nc_close(ncid))
        return 8;
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
        {"group scope", run_group_scope},
        {"errors", run_errors},
        {"classic reject", run_classic},
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

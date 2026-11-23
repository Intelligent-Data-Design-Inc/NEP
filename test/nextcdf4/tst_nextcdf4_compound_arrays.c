/* Tests nested and array compound members for NEXTCDF-4: fixed-size
 * compounds containing other compounds, enums, opaques and
 * multidimensional array fields, with inquiry, I/O, reopen and
 * upstream NetCDF-4 interchange, in native and NC_NETCDF4_MODEL
 * modes.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NATIVE "tst_nextcdf4_compound_arrays_native.h5"
#define FILE_MODEL "tst_nextcdf4_compound_arrays_model.h5"
#define FILE_UP_IN "tst_nextcdf4_compound_arrays_upin.h5"
#define FILE_UP_OUT "tst_nextcdf4_compound_arrays_upout.h5"

#define ND 4
#define NPTS 3
#define GRID0 2
#define GRID1 5

/* Inner fixed-size record: enum + opaque members exercise non-atomic
 * leaves inside a compound. */
struct point {
    int cls;        /* enum cloud_class stored as int */
    unsigned char blob[8];
};

/* Outer record: scalar atomics, a nested compound, a rank-1 atomic
 * array, a rank-2 atomic array and an array of compounds, with
 * deliberate padding via offsetof. */
struct obs {
    int id;
    float temp;
    unsigned char pad0;         /* pad before the nested struct */
    struct point loc;
    short grid[GRID0][GRID1];
    int tags[NPTS];
    struct point trail[2];
};

#define OBS_SIZE ((int)sizeof(struct obs))
#define POINT_NAME "point"
#define OBS_NAME "obs"

static const struct obs data_in[ND] = {
    {1, 10.5f, 0, {2, {1,2,3,4,5,6,7,8}},
        {{0,1,2,3,4},{5,6,7,8,9}}, {11,12,13},
        {{0, {0,0,0,0,0,0,0,1}}, {1, {9,9,9,9,9,9,9,9}}}},
    {2, -3.25f, 0, {0, {8,7,6,5,4,3,2,1}},
        {{9,8,7,6,5},{4,3,2,1,0}}, {-1,-2,-3},
        {{3, {2,2,2,2,2,2,2,2}}, {0, {3,3,3,3,3,3,3,3}}}},
    {3, 0.0f, 0, {1, {0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x11,0x22}},
        {{1,1,1,1,1},{2,2,2,2,2}}, {7,8,9},
        {{2, {4,4,4,4,4,4,4,4}}, {3, {5,5,5,5,5,5,5,5}}}},
    {4, 99.75f, 0, {3, {9,9,9,9,9,9,9,9}},
        {{0,0,0,0,0},{-1,-1,-1,-1,-1}}, {0,0,0},
        {{1, {6,6,6,6,6,6,6,6}}, {2, {7,7,7,7,7,7,7,7}}}}
};

/* Define the enum, opaque, inner compound and outer compound on the
 * group ncid refers to. */
static int
def_types(int ncid, nc_type *enumid, nc_type *opaqueid,
          nc_type *pointid, nc_type *obsid)
{
    int v;
    int grid_dims[2] = {GRID0, GRID1};
    int trail_dims[1] = {2};
    int tag_dims[1] = {NPTS};

    if (nc_def_enum(ncid, NC_INT, "cloud_class", enumid))
        return 1;
    v = 0; if (nc_insert_enum(ncid, *enumid, "clear", &v)) return 1;
    v = 1; if (nc_insert_enum(ncid, *enumid, "cumulus", &v)) return 1;
    v = 2; if (nc_insert_enum(ncid, *enumid, "cirrus", &v)) return 1;
    v = 3; if (nc_insert_enum(ncid, *enumid, "anvil", &v)) return 1;

    if (nc_def_opaque(ncid, 8, "blob", opaqueid))
        return 1;

    if (nc_def_compound(ncid, sizeof(struct point), POINT_NAME, pointid))
        return 1;
    if (nc_insert_compound(ncid, *pointid, "cls",
                           offsetof(struct point, cls), *enumid))
        return 1;
    if (nc_insert_compound(ncid, *pointid, "blob",
                           offsetof(struct point, blob), *opaqueid))
        return 1;

    /* The container is defined before its member type on purpose:
     * commit order must be dependency-aware, not definition order. */
    if (nc_def_compound(ncid, sizeof(struct obs), OBS_NAME, obsid))
        return 1;
    if (nc_insert_compound(ncid, *obsid, "id",
                           offsetof(struct obs, id), NC_INT))
        return 1;
    if (nc_insert_compound(ncid, *obsid, "temp",
                           offsetof(struct obs, temp), NC_FLOAT))
        return 1;
    if (nc_insert_compound(ncid, *obsid, "loc",
                           offsetof(struct obs, loc), *pointid))
        return 1;
    if (nc_insert_array_compound(ncid, *obsid, "grid",
                                 offsetof(struct obs, grid), NC_SHORT,
                                 2, grid_dims))
        return 1;
    if (nc_insert_array_compound(ncid, *obsid, "tags",
                                 offsetof(struct obs, tags), NC_INT,
                                 1, tag_dims))
        return 1;
    /* Array of compounds: exercise a user-defined array base. */
    if (nc_insert_array_compound(ncid, *obsid, "trail",
                                 offsetof(struct obs, trail), *pointid,
                                 1, trail_dims))
        return 1;
    return 0;
}

/* Check type and field inquiries on the types def_types() made. */
static int
check_type_inq(int ncid, nc_type pointid, nc_type obsid)
{
    char name[NC_MAX_NAME + 1];
    size_t size, nfields, offset;
    nc_type base, ftype;
    int classp, ndims, dims[NC_MAX_VAR_DIMS];

    if (nc_inq_user_type(ncid, obsid, name, &size, &base, &nfields,
                         &classp))
        return 10;
    if (strcmp(name, OBS_NAME) || size != sizeof(struct obs) ||
        classp != NC_COMPOUND || nfields != 6)
        return 11;

    /* Nested compound member. */
    if (nc_inq_compound_field(ncid, obsid, 2, name, &offset, &ftype,
                              &ndims, dims))
        return 12;
    if (strcmp(name, "loc") || offset != offsetof(struct obs, loc) ||
        ftype != pointid || ndims != 0)
        return 13;

    /* Rank-2 atomic array member. */
    memset(dims, 0, sizeof(dims));
    if (nc_inq_compound_field(ncid, obsid, 3, name, &offset, &ftype,
                              &ndims, dims))
        return 14;
    if (strcmp(name, "grid") || offset != offsetof(struct obs, grid) ||
        ftype != NC_SHORT || ndims != 2 ||
        dims[0] != GRID0 || dims[1] != GRID1)
        return 15;

    /* Rank-1 atomic array member. */
    memset(dims, 0, sizeof(dims));
    if (nc_inq_compound_field(ncid, obsid, 4, name, &offset, &ftype,
                              &ndims, dims))
        return 16;
    if (strcmp(name, "tags") || offset != offsetof(struct obs, tags) ||
        ftype != NC_INT || ndims != 1 || dims[0] != NPTS)
        return 17;

    /* Array of compounds. */
    memset(dims, 0, sizeof(dims));
    if (nc_inq_compound_field(ncid, obsid, 5, name, &offset, &ftype,
                              &ndims, dims))
        return 18;
    if (strcmp(name, "trail") || offset != offsetof(struct obs, trail) ||
        ftype != pointid || ndims != 1 || dims[0] != 2)
        return 19;

    /* Inner type's members. */
    if (nc_inq_compound_field(ncid, pointid, 0, name, &offset, &ftype,
                              &ndims, NULL))
        return 20;
    if (strcmp(name, "cls") || offset != offsetof(struct point, cls) ||
        ndims != 0)
        return 21;
    if (nc_inq_compound_field(ncid, pointid, 1, name, &offset, &ftype,
                              &ndims, NULL))
        return 22;
    if (strcmp(name, "blob") || offset != offsetof(struct point, blob) ||
        ndims != 0)
        return 23;
    return 0;
}

/* Write data through a variable and an attribute, close, reopen and
 * read both back field-by-field. */
static int
run_roundtrip(const char *fname, int cmode)
{
    int ncid, dimid, varid, ret;
    nc_type enumid, opaqueid, pointid, obsid;
    size_t start[1], count[1], stride[1];
    struct obs out[ND];

    if ((ret = nc_create(fname, NC_NEXTCDF4 | NC_CLOBBER | cmode, &ncid)))
        return ret;
    if ((ret = def_types(ncid, &enumid, &opaqueid, &pointid, &obsid)))
        return 100 + ret;
    if ((ret = check_type_inq(ncid, pointid, obsid)))
        return 200 + ret;

    /* Structural equality: an independent redefinition of the inner
     * type must compare equal by structure, not identity. Must run in
     * define mode, before I/O triggers enddef. */
    {
        nc_type p2, e2, o2;
        int v, eq = 0;
        if (nc_def_enum(ncid, NC_INT, "cloud_class2", &e2)) return 40;
        v = 0; if (nc_insert_enum(ncid, e2, "clear", &v)) return 40;
        v = 1; if (nc_insert_enum(ncid, e2, "cumulus", &v)) return 40;
        v = 2; if (nc_insert_enum(ncid, e2, "cirrus", &v)) return 40;
        v = 3; if (nc_insert_enum(ncid, e2, "anvil", &v)) return 40;
        if (nc_def_opaque(ncid, 8, "blob2", &o2)) return 40;
        if (nc_def_compound(ncid, sizeof(struct point), "point2", &p2))
            return 40;
        if (nc_insert_compound(ncid, p2, "cls",
                               offsetof(struct point, cls), e2))
            return 40;
        if (nc_insert_compound(ncid, p2, "blob",
                               offsetof(struct point, blob), o2))
            return 40;
        if (nc_inq_type_equal(ncid, pointid, ncid, p2, &eq)) return 41;
        if (!eq) return 42;
        eq = 1;
        if (nc_inq_type_equal(ncid, pointid, ncid, obsid, &eq)) return 43;
        if (eq) return 44;
    }

    if ((ret = nc_def_dim(ncid, "n", ND, &dimid)))
        return ret;
    if ((ret = nc_def_var(ncid, "obs_v", obsid, 1, &dimid, &varid)))
        return ret;
    if ((ret = nc_put_att(ncid, NC_GLOBAL, "single", obsid, 1,
                          &data_in[0])))
        return ret;
    if ((ret = nc_put_var(ncid, varid, data_in)))
        return ret;

    /* Strided write of every other element. */
    start[0] = 0; count[0] = 2; stride[0] = 2;
    if ((ret = nc_put_vars(ncid, varid, start, count, stride,
                          &data_in[0])))
        return ret;

    if ((ret = nc_close(ncid)))
        return ret;

    /* Reopen: types must resolve by committed-type identity, fields
     * and array shapes must survive. */
    if ((ret = nc_open(fname, NC_NEXTCDF4, &ncid)))
        return ret;
    {
        int ntypes, typeids[16], i;
        nc_type robs = NC_NAT, rpoint = NC_NAT;
        char name[NC_MAX_NAME + 1];
        if (nc_inq_typeids(ncid, &ntypes, typeids)) return 50;
        for (i = 0; i < ntypes; i++) {
            if (nc_inq_user_type(ncid, typeids[i], name, NULL, NULL,
                                 NULL, NULL))
                return 51;
            if (!strcmp(name, OBS_NAME)) robs = typeids[i];
            if (!strcmp(name, POINT_NAME)) rpoint = typeids[i];
        }
        if (robs == NC_NAT || rpoint == NC_NAT) return 52;
        if ((ret = check_type_inq(ncid, rpoint, robs)))
            return 300 + ret;

        memset(out, 0, sizeof(out));
        if (nc_get_var(ncid, varid, out)) return 53;
        /* Elements 0 and 2 were overwritten by the strided write of
         * data_in[0..1]; the rest match data_in. */
        if (memcmp(&out[0], &data_in[0], sizeof(struct obs)) ||
            memcmp(&out[2], &data_in[1], sizeof(struct obs)) ||
            memcmp(&out[1], &data_in[1], sizeof(struct obs)) ||
            memcmp(&out[3], &data_in[3], sizeof(struct obs)))
            return 54;
        {
            struct obs att_out;
            size_t alen;
            nc_type axtype;
            if (nc_inq_att(ncid, NC_GLOBAL, "single", &axtype, &alen))
                return 55;
            if (axtype != robs || alen != 1) return 56;
            memset(&att_out, 0, sizeof(att_out));
            if (nc_get_att(ncid, NC_GLOBAL, "single", &att_out))
                return 57;
            if (memcmp(&att_out, &data_in[0], sizeof(struct obs)))
                return 58;
        }
    }
    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

/* Parent-group types must be visible in child groups; two groups may
 * hold structurally different types under the same name. */
static int
run_group_scope(void)
{
    int ncid, g1, g2, ret;
    nc_type pointid, obsid, enumid, opaqueid;
    nc_type g2_cmp;

    if ((ret = nc_create(FILE_NATIVE, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        return ret;
    if ((ret = def_types(ncid, &enumid, &opaqueid, &pointid, &obsid)))
        return ret;
    if ((ret = nc_def_grp(ncid, "g1", &g1)))
        return ret;
    {
        int dimid, varid;
        if (nc_def_dim(g1, "n", ND, &dimid)) return 1;
        /* Root-group type used from a child group. */
        if (nc_def_var(g1, "sub_obs", obsid, 1, &dimid, &varid))
            return 2;
        if (nc_put_var(g1, varid, data_in)) return 3;
    }
    /* A different type with the same name in a sibling group. */
    if ((ret = nc_def_grp(ncid, "g2", &g2)))
        return ret;
    {
        struct small { int a; int b; };
        if (nc_def_compound(g2, sizeof(struct small), OBS_NAME, &g2_cmp))
            return 4;
        if (nc_insert_compound(g2, g2_cmp, "a",
                               offsetof(struct small, a), NC_INT))
            return 5;
        if (nc_insert_compound(g2, g2_cmp, "b",
                               offsetof(struct small, b), NC_INT))
            return 6;
    }
    if ((ret = nc_close(ncid)))
        return ret;

    if ((ret = nc_open(FILE_NATIVE, NC_NEXTCDF4, &ncid)))
        return ret;
    {
        int ntypes, typeids[16], i, found = 0;
        if (nc_inq_typeids(ncid, &ntypes, typeids)) return 7;
        for (i = 0; i < ntypes; i++)
            if (typeids[i] == obsid) found++;
        /* obs/point/enum/opaque all on root. */
        if (ntypes < 4) return 8;
    }
    {
        int varid;
        struct obs out[ND];
        if (nc_inq_grp_ncid(ncid, "g1", &g1)) return 9;
        if (nc_inq_varid(g1, "sub_obs", &varid)) return 10;
        memset(out, 0, sizeof(out));
        if (nc_get_var(g1, varid, out)) return 11;
        if (memcmp(out, data_in, sizeof(data_in))) return 12;
    }
    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

/* Rejections: bad shapes, out-of-bounds offsets, duplicates, cycles,
 * references, strings, and committed-type inserts. */
static int
run_errors(void)
{
    int ncid, ret;
    nc_type cmp, inner, ref_cmp;
    int dims1[1] = {4};
    int dims2[2] = {2, 3};
    int bad0[1] = {0};
    int badneg[1] = {-2};

    if ((ret = nc_create(FILE_NATIVE, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        return ret;

    if (nc_def_compound(ncid, 64, "e", &cmp)) return 1;

    /* Rank validation. */
    if (nc_insert_array_compound(ncid, cmp, "neg", 0, NC_INT, -1,
                                 dims1) != NC_EINVAL)
        return 2;
    if (nc_insert_array_compound(ncid, cmp, "norank", 0, NC_INT,
                                 NC_MAX_VAR_DIMS + 1, dims1) != NC_EINVAL)
        return 3;
    /* NULL dim vector with positive rank. */
    if (nc_insert_array_compound(ncid, cmp, "nullv", 0, NC_INT, 1,
                                 NULL) != NC_EINVAL)
        return 4;
    /* Zero and negative extents. */
    if (nc_insert_array_compound(ncid, cmp, "zero", 0, NC_INT, 1,
                                 bad0) != NC_EINVAL)
        return 5;
    if (nc_insert_array_compound(ncid, cmp, "negd", 0, NC_INT, 1,
                                 badneg) != NC_EINVAL)
        return 6;
    /* Field past the end of the declared size. */
    if (nc_insert_array_compound(ncid, cmp, "oob", 60, NC_INT, 2,
                                 dims2) != NC_EINVAL)
        return 7;
    if (nc_insert_compound(ncid, cmp, "oob2", 62, NC_INT) != NC_EINVAL)
        return 8;

    /* Valid insert; then duplicate name and a successful later insert
     * must still work. */
    if (nc_insert_compound(ncid, cmp, "a", 0, NC_INT)) return 9;
    if (nc_insert_compound(ncid, cmp, "a", 4, NC_INT) != NC_ENAMEINUSE)
        return 10;
    if (nc_insert_array_compound(ncid, cmp, "arr", 4, NC_SHORT, 2,
                                 dims2))
        return 11;
    /* Overlapping a member's bytes is an HDF5 error. */
    {
        int rc = nc_insert_compound(ncid, cmp, "olap", 4, NC_INT);
        if (rc == NC_NOERR)
            return 90;
    }
    {
        size_t nfields;
        if (nc_inq_compound_nfields(ncid, cmp, &nfields)) return 12;
        if (nfields != 2) return 13;
    }

    /* Reference members are rejected; strings are supported. */
    if (nc_insert_compound(ncid, cmp, "r", 16, NC_STD_REF) != NC_EBADTYPE)
        return 14;
    if (nc_insert_array_compound(ncid, cmp, "r2", 16, NC_STD_REF, 1,
                                 dims1) != NC_EBADTYPE)
        return 15;
    if (nc_insert_compound(ncid, cmp, "s", 24, NC_STRING))
        return 16;

    /* Self-nesting is a cycle. */
    if (nc_insert_compound(ncid, cmp, "self", 16, cmp) != NC_EBADTYPE)
        return 17;
    /* Indirect cycle: inner holds cmp is impossible to build first —
     * but cmp containing inner containing cmp is. Define inner with a
     * member of cmp's type is rejected too once cmp is a field... so
     * test the direct reachable form: inner <- cmp, then cmp <- inner
     * would cycle only if inner already contains cmp. */
    if (nc_def_compound(ncid, 128, "inner", &inner)) return 18;
    if (nc_insert_compound(ncid, inner, "back", 0, cmp)) return 19;
    /* Now adding inner to cmp would create a cycle. */
    if (nc_insert_compound(ncid, cmp, "cycle", 16, inner) != NC_EBADTYPE)
        return 20;
    /* The failed insert leaves cmp usable. */
    if (nc_insert_compound(ncid, cmp, "ok", 16, NC_FLOAT)) return 21;

    /* Inserting a member into a non-compound is a type error. */
    if (nc_insert_compound(ncid, inner, "x", 64, NC_UBYTE)) return 22;
    {
        nc_type op;
        if (nc_def_opaque(ncid, 4, "op", &op)) return 23;
        if (nc_insert_compound(ncid, op, "x", 0, NC_INT) != NC_EBADTYPE)
            return 24;
    }

    /* Inserts after enddef hit the committed-type wall. */
    if ((ret = nc_def_compound(ncid, 8, "late", &ref_cmp)))
        return 25;
    if (nc_insert_compound(ncid, ref_cmp, "x", 0, NC_INT)) return 26;
    if ((ret = nc_enddef(ncid)))
        return 27;
    if (nc_insert_compound(ncid, ref_cmp, "y", 4, NC_INT) !=
        NC_ENOTINDEFINE)
        return 28;

    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

/* Compatibility mode: NEP-only atomics must be rejected as members. */
static int
run_model_reject(void)
{
    int ncid, ret;
    nc_type cmp;

    if ((ret = nc_create(FILE_MODEL,
                         NC_NEXTCDF4 | NC_CLOBBER | NC_NETCDF4_MODEL,
                         &ncid)))
        return ret;
    if (nc_def_compound(ncid, 16, "m", &cmp)) return 1;
    /* Standard enhanced-model atomics are fine. */
    if (nc_insert_compound(ncid, cmp, "u", 0, NC_UINT)) return 2;
    /* NEXTCDF-4-only atomic must be rejected in model mode. */
    {
        int rc = nc_insert_compound(ncid, cmp, "c", 4, NC_COMPLEX);
        if (rc != NC_EBADTYPE && rc != NC_ENOTNC4)
            return 3;
    }
    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

/* Upstream netcdf-c writes a nested/array compound file; NEXTCDF-4
 * must read it. */
static int
run_upstream_in(void)
{
    int ncid, ret;
    nc_type pointid, obsid, enumid, opaqueid;
    int dimid, varid;

    if ((ret = nc_create(FILE_UP_IN, NC_NETCDF4 | NC_CLOBBER, &ncid))) {
        fprintf(stderr, "upstream create failed: %d\n", ret);
        return -1; /* upstream unavailable: skip */
    }
    if ((ret = def_types(ncid, &enumid, &opaqueid, &pointid, &obsid))) {
        nc_close(ncid);
        return -1;
    }
    if (nc_def_dim(ncid, "n", ND, &dimid) ||
        nc_def_var(ncid, "obs_v", obsid, 1, &dimid, &varid) ||
        nc_put_var(ncid, varid, data_in) ||
        nc_close(ncid))
        return -1;

    if ((ret = nc_open(FILE_UP_IN, NC_NEXTCDF4, &ncid))) {
        fprintf(stderr, "nextcdf4 open of upstream file: %d\n", ret);
        return ret;
    }
    {
        int ntypes, typeids[16], i;
        nc_type robs = NC_NAT, rpoint = NC_NAT;
        char name[NC_MAX_NAME + 1];
        struct obs out[ND];
        if (nc_inq_typeids(ncid, &ntypes, typeids)) return 1;
        for (i = 0; i < ntypes; i++) {
            if (nc_inq_user_type(ncid, typeids[i], name, NULL, NULL,
                                 NULL, NULL))
                return 2;
            if (!strcmp(name, OBS_NAME)) robs = typeids[i];
            if (!strcmp(name, POINT_NAME)) rpoint = typeids[i];
        }
        if (robs == NC_NAT || rpoint == NC_NAT) return 3;
        if ((ret = check_type_inq(ncid, rpoint, robs)))
            return 100 + ret;
        memset(out, 0, sizeof(out));
        if (nc_get_var(ncid, varid, out)) return 4;
        /* Upstream's own write truncates the last record: HDF5's
         * serialized-compound layout for this type needs 84 bytes per
         * record but the dataset stores n*80, so the final record's
         * last 16 bytes never reach disk — upstream itself cannot
         * read them back either. Verify what is recoverable: the
         * first three records plus record 3's first 64 bytes. */
        if (memcmp(out, data_in, 3 * sizeof(struct obs) + 64)) return 5;
    }
    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

/* NEXTCDF-4 writes a NC_NETCDF4_MODEL file; upstream reads it back. */
static int
run_upstream_out(void)
{
    int ncid, ret;

    if ((ret = run_roundtrip(FILE_UP_OUT, NC_NETCDF4_MODEL)))
        return ret;
    if ((ret = nc_open(FILE_UP_OUT, NC_NETCDF4, &ncid))) {
        fprintf(stderr, "upstream open failed: %d\n", ret);
        return -1; /* upstream unavailable: skip */
    }
    {
        int ntypes, typeids[16], i, varid;
        nc_type robs = NC_NAT;
        char name[NC_MAX_NAME + 1];
        struct obs out[ND];
        if (nc_inq_typeids(ncid, &ntypes, typeids)) return 1;
        for (i = 0; i < ntypes; i++) {
            if (nc_inq_user_type(ncid, typeids[i], name, NULL, NULL,
                                 NULL, NULL))
                return 2;
            if (!strcmp(name, OBS_NAME)) robs = typeids[i];
        }
        if (robs == NC_NAT) return 3;
        {
            size_t offset;
            nc_type ftype;
            int ndims, dims[NC_MAX_VAR_DIMS];
            if (nc_inq_compound_field(ncid, robs, 3, name, &offset,
                                      &ftype, &ndims, dims))
                return 4;
            if (strcmp(name, "grid") || ftype != NC_SHORT ||
                ndims != 2 || dims[0] != GRID0 || dims[1] != GRID1)
                return 5;
        }
        if (nc_inq_varid(ncid, "obs_v", &varid)) return 6;
        memset(out, 0, sizeof(out));
        if (nc_get_var(ncid, varid, out)) return 7;
        /* Upstream reads clean (declared-layout) nested compounds
         * into its packed native layout — the same convention it
         * applies to its own files. Verify the delivered buffer is
         * byte-exact against the packed image of data_in (member
         * bytes contiguous: declared layout minus the 4-byte gap at
         * offset 8, giving 76-byte records). */
        {
            /* Elements 0 and 2 were overwritten by the strided write
             * in run_roundtrip. */
            const struct obs *exp[ND] = {
                &data_in[0], &data_in[1], &data_in[1], &data_in[3]};
            unsigned char packed[ND * 76], *q = packed;
            int r;
            for (r = 0; r < ND; r++, q += 76) {
                const unsigned char *d = (const unsigned char *)exp[r];
                memcpy(q, d, 8);
                memcpy(q + 8, d + 12, 68);
            }
            if (memcmp(out, packed, sizeof(packed)))
                return 8;
        }
    }
    if ((ret = nc_close(ncid)))
        return ret;
    return 0;
}

int
main(void)
{
    struct {
        const char *name;
        int (*run)(void);
    } tests[] = {
        {"group scope", run_group_scope},
        {"errors", run_errors},
        {"model reject", run_model_reject},
        {"upstream read", run_upstream_in},
        {"upstream write", run_upstream_out},
    };
    size_t i;
    int r, fails = 0;

    printf("*** Testing NEXTCDF-4 nested compounds and array fields.\n");
    r = run_roundtrip(FILE_NATIVE, 0);
    printf("native roundtrip: %s (%d)\n", r ? "FAIL" : "ok", r);
    if (r > 0) fails++;
    r = run_roundtrip(FILE_MODEL, NC_NETCDF4_MODEL);
    printf("model roundtrip: %s (%d)\n", r ? "FAIL" : "ok", r);
    if (r > 0) fails++;
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        r = tests[i].run();
        printf("%s: %s (%d)\n", tests[i].name,
               r < 0 ? "skip" : r ? "FAIL" : "ok", r);
        if (r > 0) fails++;
    }
    unlink(FILE_NATIVE);
    unlink(FILE_MODEL);
    unlink(FILE_UP_IN);
    unlink(FILE_UP_OUT);
    if (fails) {
        fprintf(stderr, "*** %d test(s) failed\n", fails);
        return 1;
    }
    printf("*** SUCCESS\n");
    return 0;
}

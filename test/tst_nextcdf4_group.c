/**
 * @file tst_nextcdf4_group.c
 * @brief Tests NEXTCDF-4 Sprint 5 groups, user types, and strings.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hdf5.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"
#include "nxt4internal.h"

/** NEXTCDF-4 file used for group and type tests. */
#define FILE_NAME "tst_nextcdf4_group.h5"

/* Helper macro for error checking. */
#define CHECK(expr) do { \
    int _ret = (expr); \
    if (_ret != NC_NOERR) { \
        fprintf(stderr, "%s:%d: %s failed: %s\n", __FILE__, __LINE__, #expr, nc_strerror(_ret)); \
        return 1; \
    } \
} while (0)

/** @return Zero when group lifecycle and nested groups round-trip. */
static int
test_groups(void)
{
    int ncid, grpid, gid2;
    int numgrps;
    int grpids[4];
    char name[NC_MAX_NAME + 1];
    char full[NC_MAX_NAME * 4 + 1];
    int parentid;
    int rootid;
    size_t len;

    unlink(FILE_NAME);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_grp(ncid, "g1", &grpid));
    CHECK(nc_def_grp(grpid, "g2", &gid2));

    /* Root group inquiry. */
    CHECK(nc_inq_grpname(ncid, name));
    if (strcmp(name, "/")) {
        fprintf(stderr, "root group name expected '/', got '%s'\n", name);
        return 1;
    }
    CHECK(nc_inq_grpname_full(ncid, &len, full));
    if (strcmp(full, "/")) {
        fprintf(stderr, "root full name expected '/', got '%s'\n", full);
        return 1;
    }
    /* Look up groups by full name. */
    CHECK(nc_inq_ncid(ncid, "/g1", &grpid));
    CHECK(nc_inq_ncid(ncid, "/g1/g2", &gid2));
    CHECK(nc_inq_grp_full_ncid(ncid, "/g1/g2", &gid2));

    /* Group count and listing. */
    CHECK(nc_inq_grps(ncid, &numgrps, grpids));
    if (numgrps != 1) {
        fprintf(stderr, "expected 1 child of root, got %d\n", numgrps);
        return 1;
    }
    CHECK(nc_inq_grps(grpid, &numgrps, grpids));
    if (numgrps != 1) {
        fprintf(stderr, "expected 1 child of g1, got %d\n", numgrps);
        return 1;
    }

    /* Name and parent. */
    CHECK(nc_inq_grpname(grpid, name));
    if (strcmp(name, "g1")) {
        fprintf(stderr, "g1 name mismatch: '%s'\n", name);
        return 1;
    }
    CHECK(nc_inq_grpname_full(grpid, &len, full));
    if (strcmp(full, "/g1")) {
        fprintf(stderr, "g1 full name mismatch: '%s'\n", full);
        return 1;
    }
    CHECK(nc_inq_grp_parent(gid2, &parentid));
    if (parentid != grpid) {
        fprintf(stderr, "g2 parent mismatch\n");
        return 1;
    }

    CHECK(nc_close(ncid));

    /* Reopen and verify. */
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_ncid(ncid, "/g1", &grpid));
    CHECK(nc_inq_ncid(ncid, "/g1/g2", &gid2));
    CHECK(nc_inq_grp_parent(gid2, &parentid));
    if (parentid != grpid)
        return 1;
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when compound and enum types round-trip. */
static int
test_types(void)
{
    int ncid;
    int typeid;
    int xtype;
    char name[NC_MAX_NAME + 1];
    size_t size;
    size_t offset;
    size_t nfields;
    nc_type fieldtype;
    int fieldndims;
    int dimsizes[NC_MAX_VAR_DIMS];
    int fieldindex;
    int enum_value;
    char enum_name[NC_MAX_NAME + 1];
    int base_type;

    unlink(FILE_NAME);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));

    /* Opaque type. */
    CHECK(nc_def_opaque(ncid, 8, "eight", &typeid));
    CHECK(nc_inq_user_type(ncid, typeid, name, &size, &base_type, &nfields, &xtype));
    if (strcmp(name, "eight") || size != 8 || xtype != NC_OPAQUE) {
        fprintf(stderr, "opaque type mismatch\n");
        return 1;
    }

    /* Compound type. */
    CHECK(nc_def_compound(ncid, sizeof(int) + sizeof(double), "cmpd", &typeid));
    CHECK(nc_insert_compound(ncid, typeid, "i", 0, NC_INT));
    CHECK(nc_insert_compound(ncid, typeid, "d", sizeof(int), NC_DOUBLE));
    CHECK(nc_inq_compound_field(ncid, typeid, 0, name, &offset, &fieldtype,
                                &fieldndims, dimsizes));
    if (strcmp(name, "i") || offset != 0 || fieldtype != NC_INT) {
        fprintf(stderr, "compound field 0 mismatch\n");
        return 1;
    }
    CHECK(nc_inq_compound_fieldindex(ncid, typeid, "d", &fieldindex));
    if (fieldindex != 1) {
        fprintf(stderr, "compound field index mismatch\n");
        return 1;
    }

    /* Enum type. */
    CHECK(nc_def_enum(ncid, NC_INT, "colors", &typeid));
    enum_value = 1;
    CHECK(nc_insert_enum(ncid, typeid, "red", &enum_value));
    enum_value = 2;
    CHECK(nc_insert_enum(ncid, typeid, "blue", &enum_value));
    CHECK(nc_inq_enum_member(ncid, typeid, 0, enum_name, &enum_value));
    if (strcmp(enum_name, "red") || enum_value != 1) {
        fprintf(stderr, "enum member mismatch\n");
        return 1;
    }
    CHECK(nc_inq_enum_ident(ncid, typeid, 2, enum_name));
    if (strcmp(enum_name, "blue")) {
        fprintf(stderr, "enum ident mismatch\n");
        return 1;
    }

    CHECK(nc_close(ncid));

    /* Reopen and verify typeids. */
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    {
        int ntypes, typeids[4];
        CHECK(nc_inq_typeids(ncid, &ntypes, typeids));
        if (ntypes != 3) {
            fprintf(stderr, "expected 3 types, got %d\n", ntypes);
            return 1;
        }
    }
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when NC_CHAR and NC_STRING variables round-trip. */
static int
test_strings(void)
{
    int ncid;
    int dimid;
    int var_char, var_str;
    size_t len;
    char in_text[4] = {'a', 'b', 'c', 'd'};
    char out_text[4] = {0};
    char *in_str[2] = {"hello", "world"};
    char *out_str[2] = {NULL, NULL};

    unlink(FILE_NAME);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "len", 4, &dimid));
    CHECK(nc_def_var(ncid, "chars", NC_CHAR, 1, &dimid, &var_char));
    CHECK(nc_def_var(ncid, "strings", NC_STRING, 1, &dimid, &var_str));
    CHECK(nc_enddef(ncid));

    {
        size_t start[1] = {0};
        size_t count[1] = {4};
        CHECK(nc_put_vara_text(ncid, var_char, start, count, in_text));
        CHECK(nc_get_vara_text(ncid, var_char, start, count, out_text));
    }
    if (memcmp(in_text, out_text, 4)) {
        fprintf(stderr, "NC_CHAR round-trip mismatch\n");
        return 1;
    }

    {
        size_t start[1] = {0};
        size_t count[1] = {2};
        CHECK(nc_put_vara_string(ncid, var_str, start, count, (const char **)in_str));
        CHECK(nc_get_vara_string(ncid, var_str, start, count, out_str));
    }
    if (!out_str[0] || !out_str[1] ||
        strcmp(in_str[0], out_str[0]) || strcmp(in_str[1], out_str[1])) {
        fprintf(stderr, "NC_STRING round-trip mismatch\n");
        return 1;
    }
    nc_free_string(2, out_str);

    /* Attribute I/O. */
    CHECK(nc_redef(ncid));
    CHECK(nc_put_att_string(ncid, NC_GLOBAL, "att_str", 2, (const char **)in_str));
    CHECK(nc_inq_attlen(ncid, NC_GLOBAL, "att_str", &len));
    if (len != 2) {
        fprintf(stderr, "att_str len mismatch: %zu\n", len);
        return 1;
    }
    memset(out_str, 0, sizeof(out_str));
    CHECK(nc_get_att_string(ncid, NC_GLOBAL, "att_str", out_str));
    if (strcmp(in_str[0], out_str[0]) || strcmp(in_str[1], out_str[1])) {
        fprintf(stderr, "NC_STRING attribute mismatch\n");
        return 1;
    }
    nc_free_string(2, out_str);

    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when classic/NETCDF4_MODEL restrictions work. */
static int
test_compat(void)
{
    int ncid;
    int grpid, typeid;

    unlink(FILE_NAME);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    /* Classic model should reject groups and user types. */
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | NC_CLASSIC_MODEL, &ncid)
        != NC_NOERR)
        return 1;
    if (nc_def_grp(ncid, "bad", &grpid) != NC_ENOTNC4) {
        fprintf(stderr, "classic def_grp did not reject\n");
        return 1;
    }
    if (nc_def_compound(ncid, 4, "bad", &typeid) != NC_ENOTNC4) {
        fprintf(stderr, "classic def_compound did not reject\n");
        return 1;
    }
    if (nc_def_var(ncid, "bad", NC_STRING, 0, NULL, &typeid) != NC_EBADTYPE) {
        fprintf(stderr, "classic NC_STRING did not reject\n");
        return 1;
    }
    nc_close(ncid);
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when all Sprint 5 checks pass. */
int
main(void)
{
    int failed = 0;

    failed |= test_groups();
    failed |= test_types();
    failed |= test_strings();
    failed |= test_compat();

    if (failed) {
        fprintf(stderr, "FAILED\n");
        return 1;
    }
    printf("OK: NEXTCDF-4 Sprint 5 groups and types\n");
    return 0;
}

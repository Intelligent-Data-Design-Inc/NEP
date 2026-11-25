/* Parallel variable access, empty selections, extension and filters.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include "config.h"
#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>
#include <netcdf_filter.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nxt4internal.h"

static int rank;
#define CHECK(call) EXPECT(call, NC_NOERR)
#define EXPECT(call, want) do { int status_ = (call); if (status_ != (want)) { \
    fprintf(stderr, "rank %d line %d: %s: %s (expected %s)\n", rank, __LINE__, \
            #call, nc_strerror(status_), nc_strerror(want)); \
    MPI_Abort(MPI_COMM_WORLD, 1); } } while (0)
#define ASSERT(test) do { if (!(test)) { \
    fprintf(stderr, "rank %d line %d: %s\n", rank, __LINE__, #test); \
    MPI_Abort(MPI_COMM_WORLD, 1); } } while (0)

static void
barrier(void)
{
    ASSERT(MPI_Barrier(MPI_COMM_WORLD) == MPI_SUCCESS);
}

static int
create(const char *path)
{
    int ncid;
    if (!rank) unlink(path);
    barrier();
    CHECK(nc_create_par(path, NC_NEXTCDF4 | NC_NETCDF4,
                        MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    return ncid;
}

static void
access_modes(void)
{
    int ncid = create("access.nc"), dim, a, b, scalar, group, gvar;
    int value = rank + 10, got[16], scalar_value = 91;
    size_t start = (size_t)rank, count = 1;
    ptrdiff_t stride = 2;
    CHECK(nc_def_dim(ncid, "d", 16, &dim));
    CHECK(nc_def_var(ncid, "a", NC_INT, 1, &dim, &a));
    CHECK(nc_def_var(ncid, "b", NC_INT, 1, &dim, &b));
    CHECK(nc_def_var(ncid, "scalar", NC_INT, 0, NULL, &scalar));
    CHECK(nc_def_grp(ncid, "group", &group));
    CHECK(nc_def_var(group, "v", NC_INT, 1, &dim, &gvar));
    EXPECT(nc_var_par_access(ncid, a, 77), NC_EINVAL);
    EXPECT(nc_var_par_access(ncid, -1, NC_COLLECTIVE), NC_ENOTVAR);
    EXPECT(nc_var_par_access(-123, a, NC_COLLECTIVE), NC_EBADID);
    CHECK(nc_var_par_access(ncid, a, NC_COLLECTIVE));
    CHECK(nc_var_par_access(ncid, a, NC_INDEPENDENT));
    CHECK(nc_var_par_access(ncid, a, NC_COLLECTIVE));
    CHECK(nc_var_par_access(group, gvar, NC_COLLECTIVE));
    CHECK(nc_var_par_access(ncid, scalar, NC_COLLECTIVE));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_vara_int(ncid, a, &start, &count, &value));
    CHECK(nc_put_vara_int(group, gvar, &start, &count, &value));
    if (!rank) CHECK(nc_put_vara_int(ncid, b, &start, &count, &value));
    barrier();
    CHECK(nc_put_var_int(ncid, scalar, &scalar_value));
    CHECK(nc_get_var_int(ncid, scalar, &value));
    ASSERT(value == scalar_value);
    count = rank == 3 ? 0 : 1;
    start = (size_t)rank * 2 + 4;
    value = rank + 20;
    CHECK(nc_put_vars_int(ncid, a, &start, &count, &stride, count ? &value : NULL));
    got[0] = -99; got[1] = -99; got[2] = -99;
    CHECK(nc_get_vars_int(ncid, a, &start, &count, &stride, count ? &got[1] : NULL));
    ASSERT(got[0] == -99 && got[2] == -99 && (!count || got[1] == value));
    {
        int values[2] = {rank + 30, rank + 40}, readback[4] = {-99, -99, -99, -99};
        start = (size_t)rank; count = 2;
        stride = 4;
        CHECK(nc_put_vars_int(group, gvar, &start, &count, &stride, values));
        CHECK(nc_get_vars_int(group, gvar, &start, &count, &stride, &readback[1]));
        ASSERT(readback[0] == -99 && readback[3] == -99 &&
               readback[1] == values[0] && readback[2] == values[1]);
    }
    count = 0;
    CHECK(nc_put_vara_int(ncid, a, &start, &count, NULL));
    CHECK(nc_get_vara_int(ncid, a, &start, &count, NULL));
    CHECK(nc_close(ncid));
    CHECK(nc_open_par("access.nc", NC_NEXTCDF4, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    CHECK(nc_var_par_access(ncid, a, NC_COLLECTIVE));
    start = 0; count = 16;
    CHECK(nc_get_vara_int(ncid, a, &start, &count, got));
    for (int i = 0; i < 16; i++) {
        int expected = i < 4 ? i + 10 : NC_FILL_INT;
        if (i == 4 || i == 6 || i == 8) expected = 20 + (i - 4) / 2;
        ASSERT(got[i] == expected);
    }
    CHECK(nc_close(ncid));
    barrier();
    if (!rank) {
        CHECK(nc_open("access.nc", NC_NEXTCDF4, &ncid));
        EXPECT(nc_var_par_access(ncid, a, NC_COLLECTIVE), NC_ENOPAR);
        CHECK(nc_get_var_int(ncid, a, got));
        ASSERT(got[0] == 10 && got[8] == 22 && got[15] == NC_FILL_INT);
        CHECK(nc_close(ncid));
        unlink("access.nc");
    }
    barrier();
}

static void
append(void)
{
    int ncid = create("append.nc"), dims[2], v, shortvar, coord;
    int value = rank + 1, fill = -17, got[40 * 4];
    size_t start[2], count[2], len;
    CHECK(nc_def_dim(ncid, "time", NC_UNLIMITED, &dims[0]));
    CHECK(nc_def_dim(ncid, "x", NC_UNLIMITED, &dims[1]));
    CHECK(nc_def_var(ncid, "time", NC_INT, 1, dims, &coord));
    CHECK(nc_def_var(ncid, "v", NC_INT, 2, dims, &v));
    CHECK(nc_def_var_fill(ncid, v, 0, &fill));
    CHECK(nc_def_var(ncid, "short", NC_INT, 2, dims, &shortvar));
    CHECK(nc_enddef(ncid));
    start[0] = (size_t)rank; start[1] = 0; count[0] = count[1] = 1;
    EXPECT(nc_put_vara_int(ncid, v, start, count, &value), NC_ECANTEXTEND);
    CHECK(nc_inq_dimlen(ncid, dims[0], &len)); ASSERT(len == 0);
    CHECK(nc_var_par_access(ncid, v, NC_COLLECTIVE));
    CHECK(nc_var_par_access(ncid, shortvar, NC_COLLECTIVE));
    CHECK(nc_var_par_access(ncid, coord, NC_COLLECTIVE));
    for (int round = 0; round < 2; round++) {
        start[0] = (size_t)(round * 10 + rank * 2);
        start[1] = (size_t)rank;
        count[0] = rank == 3 ? 0 : 1; count[1] = 1;
        CHECK(nc_put_vara_int(ncid, v, start, count, count[0] ? &value : NULL));
        CHECK(nc_inq_dimlen(ncid, dims[0], &len)); ASSERT(len == (size_t)(round * 10 + 5));
        CHECK(nc_inq_dimlen(ncid, dims[1], &len)); ASSERT(len == 3);
    }
    start[0] = 100; start[1] = 100; count[0] = 1; count[1] = 0;
    CHECK(nc_put_vara_int(ncid, v, start, count, NULL));
    CHECK(nc_inq_dimlen(ncid, dims[0], &len)); ASSERT(len == 15);
    start[0] = 0; start[1] = 0; count[0] = rank ? 0 : 1; count[1] = 1;
    CHECK(nc_put_vara_int(ncid, shortvar, start, count, rank ? NULL : &value));
    start[0] = (size_t)rank * 4; count[0] = 1; count[1] = 1;
    CHECK(nc_get_vara_int(ncid, shortvar, start, count, got));
    ASSERT(got[0] == (rank ? NC_FILL_INT : 1));
    start[0] = (size_t)rank; count[0] = 1;
    CHECK(nc_put_vara_int(ncid, coord, start, count, &value));
    CHECK(nc_inq_dimlen(ncid, dims[0], &len)); ASSERT(len == 15);
    CHECK(nc_close(ncid));
    barrier();
    if (!rank) {
        CHECK(nc_open("append.nc", NC_NEXTCDF4, &ncid));
        CHECK(nc_inq_dimlen(ncid, dims[0], &len)); ASSERT(len == 15);
        CHECK(nc_inq_dimlen(ncid, dims[1], &len)); ASSERT(len == 3);
        start[0] = start[1] = 0; count[0] = 15; count[1] = 3;
        CHECK(nc_get_vara_int(ncid, v, start, count, got));
        for (int t = 0; t < 15; t++)
            for (int x = 0; x < 3; x++) {
                int expected = (t == x * 2 || t == 10 + x * 2) ? x + 1 : fill;
                ASSERT(got[t * 3 + x] == expected);
            }
        CHECK(nc_close(ncid)); unlink("append.nc");
    }
    barrier();
}

static void
filters(void)
{
    int ncid = create("filters.nc"), dim, vars[5], value = rank + 1, got;
    size_t start = (size_t)rank, count = rank == 3 ? 0 : 1, nfilters;
    unsigned int level = 1;
    CHECK(nc_def_dim(ncid, "d", 4, &dim));
    for (int i = 0; i < 5; i++) {
        char name[16];
        snprintf(name, sizeof(name), "v%d", i);
        CHECK(nc_def_var(ncid, name, NC_INT, 1, &dim, &vars[i]));
    }
    CHECK(nc_def_var_deflate(ncid, vars[0], 0, 1, 1));
    CHECK(nc_def_var_deflate(ncid, vars[1], 1, 0, 0));
    CHECK(nc_def_var_fletcher32(ncid, vars[2], NC_FLETCHER32));
    CHECK(nc_def_var_deflate(ncid, vars[3], 1, 1, 1));
    CHECK(nc_def_var_filter(ncid, vars[4], H5Z_FILTER_DEFLATE, 1, &level));
    for (int i = 0; i < 5; i++)
        EXPECT(nc_var_par_access(ncid, vars[i], NC_INDEPENDENT), NC_EINVAL);
    CHECK(nc_enddef(ncid));
    for (int i = 0; i < 5; i++)
        CHECK(nc_put_vara_int(ncid, vars[i], &start, &count, count ? &value : NULL));
    CHECK(nc_close(ncid));
    CHECK(nc_open_par("filters.nc", NC_NEXTCDF4, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    for (int i = 0; i < 5; i++) {
        EXPECT(nc_get_vara_int(ncid, vars[i], &start, &count, &got), NC_EINVAL);
        CHECK(nc_var_par_access(ncid, vars[i], NC_COLLECTIVE));
        CHECK(nc_inq_var_filter_ids(ncid, vars[i], &nfilters, NULL));
        ASSERT(nfilters == (size_t)(i == 3 ? 2 : 1));
        count = 1;
        CHECK(nc_get_vara_int(ncid, vars[i], &start, &count, &got));
        ASSERT(got == (rank == 3 ? NC_FILL_INT : value));
    }
    CHECK(nc_close(ncid));
    barrier();
    if (!rank) unlink("filters.nc");
    barrier();
}

static void
preference(int ncid, int varid, int expected)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    CHECK(nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var));
    ASSERT(((NEXTCDF4_VAR_INFO_T *)var->format_var_info)->parallel_access == expected);
}

static void
types(void)
{
    const nc_type atomic[] = {NC_BYTE, NC_CHAR, NC_SHORT, NC_INT, NC_FLOAT,
        NC_DOUBLE, NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64,
        NC_FLOAT16, NC_BFLOAT16, NC_FLOAT8_E4M3, NC_FLOAT8_E5M2,
        NC_FLOAT6_E2M3, NC_FLOAT6_E3M2, NC_FLOAT4_E2M1,
        NC_COMPLEX, NC_DOUBLECOMPLEX, NC_BITFIELD8, NC_BITFIELD16,
        NC_BITFIELD32, NC_BITFIELD64};
    union value {
        signed char b; char c; short s; int i; float f; double d;
        unsigned char ub; unsigned short us; unsigned int ui;
        long long ll; unsigned long long ull;
        struct { float r, i; } cf;
        struct { double r, i; } cd;
        unsigned char bytes[64];
    } out, in;
    struct inner { int x; };
    struct outer { struct inner a; int b[2]; } compound_out = {{rank + 1}, {7, 9}}, compound_in;
    nc_type user[4], inner;
    int ncid = create("types.nc"), dim, vars[28], one = 1;
    int shape = 2;
    size_t start = (size_t)rank, count = 1, size;
    NC_GRP_INFO_T *grp;
    CHECK(nc4_find_nc4_grp(ncid, &grp));
    CHECK(nc_def_dim(ncid, "d", 4, &dim));
    CHECK(nc_def_enum(ncid, NC_INT, "enum", &user[0]));
    CHECK(nc_insert_enum(ncid, user[0], "one", &one));
    CHECK(nc_def_opaque(ncid, 3, "opaque", &user[1]));
    CHECK(nc_def_compound(ncid, sizeof(struct inner), "inner", &inner));
    CHECK(nc_insert_compound(ncid, inner, "x", offsetof(struct inner, x), NC_INT));
    CHECK(nc_def_compound(ncid, sizeof(struct outer), "outer", &user[2]));
    CHECK(nc_insert_compound(ncid, user[2], "a", offsetof(struct outer, a), inner));
    CHECK(nc_insert_array_compound(ncid, user[2], "b", offsetof(struct outer, b), NC_INT, 1, &shape));
    user[3] = inner;
    for (int i = 0; i < 28; i++) {
        char name[16];
        snprintf(name, sizeof(name), "v%d", i);
        CHECK(nc_def_var(ncid, name, i < 24 ? atomic[i] : user[i - 24], 1, &dim, &vars[i]));
        preference(ncid, vars[i], NC_INDEPENDENT);
        CHECK(nc_var_par_access(ncid, vars[i], NC_COLLECTIVE));
    }
    CHECK(nc_enddef(ncid));
    for (int i = 0; i < 28; i++) {
        nc_type type = i < 24 ? atomic[i] : user[i - 24];
        memset(&out, 0, sizeof(out));
        switch (type) {
        case NC_BYTE: out.b = -7; break;
        case NC_CHAR: out.c = 'A'; break;
        case NC_SHORT: out.s = -300; break;
        case NC_INT: out.i = -70000; break;
        case NC_FLOAT: out.f = 1.5f; break;
        case NC_DOUBLE: out.d = 1.25; break;
        case NC_UBYTE: out.ub = 200; break;
        case NC_USHORT: out.us = 50000; break;
        case NC_UINT: out.ui = 4000000000U; break;
        case NC_INT64: out.ll = -10000000000LL; break;
        case NC_UINT64: out.ull = 10000000000ULL; break;
        case NC_FLOAT16: out.us = 0x3c00; break;
        case NC_BFLOAT16: out.us = 0x3f80; break;
        case NC_FLOAT8_E4M3: out.ub = 0x38; break;
        case NC_FLOAT8_E5M2: out.ub = 0x3c; break;
        case NC_FLOAT6_E2M3: out.ub = 0x08; break;
        case NC_FLOAT6_E3M2: out.ub = 0x0c; break;
        case NC_FLOAT4_E2M1: out.ub = 0x02; break;
        case NC_COMPLEX: out.cf.r = 1; out.cf.i = 2; break;
        case NC_DOUBLECOMPLEX: out.cd.r = 1; out.cd.i = 2; break;
        case NC_BITFIELD8: out.ub = 0xa5; break;
        case NC_BITFIELD16: out.us = 0xa55a; break;
        case NC_BITFIELD32: out.ui = 0xa55aa55a; break;
        case NC_BITFIELD64: out.ull = 0xa55aa55aa55aa55aULL; break;
        default:
            if (i == 24) out.i = 1;
            else if (i == 25) memcpy(out.bytes, "abc", 3);
            else if (i == 26) memcpy(out.bytes, &compound_out, sizeof(compound_out));
            else out.i = rank + 3;
        }
        CHECK(nx4_type_size(grp, type, &size));
        ASSERT(size <= sizeof(out));
        CHECK(nc_put_vara(ncid, vars[i], &start, &count, &out));
        memset(&in, 0, sizeof(in));
        CHECK(nc_get_vara(ncid, vars[i], &start, &count, &in));
        ASSERT(!memcmp(&out, &in, size));
        if (i == 26) {
            memcpy(&compound_in, in.bytes, sizeof(compound_in));
            ASSERT(compound_in.a.x == rank + 1 && compound_in.b[1] == 9);
        }
        if (type == NC_FLOAT16 || type == NC_BFLOAT16) {
            float value = 1.5f, got;
            double overflow = rank ? 1.0 : 1.e300;
            CHECK(nc_put_vara_float(ncid, vars[i], &start, &count, &value));
            CHECK(nc_get_vara_float(ncid, vars[i], &start, &count, &got));
            ASSERT(got == value);
            EXPECT(nc_put_vara_double(ncid, vars[i], &start, &count, &overflow), rank ? NC_NOERR : NC_ERANGE);
            CHECK(nc_get_vara_float(ncid, vars[i], &start, &count, &got));
            ASSERT(rank ? got == 1.f : got > 1.e30f);
        }
    }
    CHECK(nc_close(ncid));
    CHECK(nc_open_par("types.nc", NC_NEXTCDF4, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    for (int i = 0; i < 28; i++) preference(ncid, vars[i], NC_INDEPENDENT);
    CHECK(nc_close(ncid));
    barrier();
    if (!rank) unlink("types.nc");
    barrier();
}

static void
excluded_types(void)
{
    int ncid, dim, vars[4];
    nc_type vlen, compound;
    const char *names[] = {"string", "vlen", "compound", "reference"};
    size_t start = 0, count = 1, len;
    unsigned char buffer[128] = {0};
    struct dynamic { char *s; };
    if (!rank) {
        CHECK(nc_create("excluded.nc", NC_NEXTCDF4, &ncid));
        CHECK(nc_def_dim(ncid, "d", NC_UNLIMITED, &dim));
        CHECK(nc_def_vlen(ncid, "vl", NC_INT, &vlen));
        CHECK(nc_def_compound(ncid, sizeof(struct dynamic), "dynamic", &compound));
        CHECK(nc_insert_compound(ncid, compound, "s", offsetof(struct dynamic, s), NC_STRING));
        CHECK(nc_def_var(ncid, names[0], NC_STRING, 1, &dim, &vars[0]));
        CHECK(nc_def_var(ncid, names[1], vlen, 1, &dim, &vars[1]));
        CHECK(nc_def_var(ncid, names[2], compound, 1, &dim, &vars[2]));
        CHECK(nc_def_var(ncid, names[3], NC_STD_REF, 1, &dim, &vars[3]));
        CHECK(nc_close(ncid));
    }
    barrier();
    CHECK(nc_open_par("excluded.nc", NC_NEXTCDF4 | NC_WRITE, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    for (int i = 0; i < 4; i++) {
        CHECK(nc_inq_varid(ncid, names[i], &vars[i]));
        CHECK(nc_var_par_access(ncid, vars[i], NC_COLLECTIVE));
        EXPECT(nc_put_vara(ncid, vars[i], &start, &count, buffer), NC_ENOTBUILT);
        EXPECT(nc_get_vara(ncid, vars[i], &start, &count, buffer), NC_ENOTBUILT);
    }
    CHECK(nc_inq_dimid(ncid, "d", &dim));
    CHECK(nc_inq_dimlen(ncid, dim, &len)); ASSERT(len == 0);
    CHECK(nc_redef(ncid));
    EXPECT(nc_put_vara(ncid, vars[0], &start, &count, buffer), NC_ENOTBUILT);
    {
        NEXTCDF4_FILE_INFO_T *file;
        CHECK(NEXTCDF4_get_file(ncid, NULL, &file));
        ASSERT(file->define_mode);
    }
    CHECK(nc_enddef(ncid));
    CHECK(nc_close(ncid));
    barrier();
    if (!rank) unlink("excluded.nc");
    barrier();
}

int
main(int argc, char **argv)
{
    int size;
    ASSERT(MPI_Init(&argc, &argv) == MPI_SUCCESS);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    ASSERT(size == 4 && NC_NEXTCDF4_initialize());
    access_modes();
    append();
    filters();
    types();
    excluded_types();
    if (!rank) puts("OK: NEXTCDF-4 collective/independent variable access");
    CHECK(NC_NEXTCDF4_finalize());
    ASSERT(MPI_Finalize() == MPI_SUCCESS);
    return 0;
}

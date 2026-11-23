/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
#include <netcdf.h>
#include <hdf5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define CHECK(call) do { int e = (call); if (e) { fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(e)); goto fail; } } while (0)
#define REQUIRE(expr) do { if (!(expr)) { fprintf(stderr, "%d: %s\n", __LINE__, #expr); goto fail; } } while (0)

struct record { char *names[2]; nc_vlen_t words; };

static int
strings(int mode)
{
    int ncid = -1, dim, var, scalar, grp, nofill, rc;
    char *input[] = {"alpha", "", "caf\xc3\xa9", NULL};
    char *out[8] = {0}, *fill = "missing", *got = NULL;
    size_t start[] = {0}, count[] = {4};
    ptrdiff_t stride[] = {2};
    int numeric = 0;
    CHECK(nc_create("strings.nc", NC_NEXTCDF4 | mode, &ncid));
    CHECK(nc_def_dim(ncid, "n", NC_UNLIMITED, &dim));
    CHECK(nc_def_var(ncid, "s", NC_STRING, 1, &dim, &var));
    CHECK(nc_def_var(ncid, "scalar", NC_STRING, 0, NULL, &scalar));
    REQUIRE(nc_def_var_fill(ncid, scalar, NC_NOFILL, NULL) == NC_EINVAL);
    CHECK(nc_def_var_fill(ncid, var, NC_FILL, &fill));
    CHECK(nc_def_grp(ncid, "g", &grp));
    CHECK(nc_put_att_string(grp, NC_GLOBAL, "a", 3, (const char **)input));
    CHECK(nc_put_att_string(ncid, var, "a", 4, (const char **)input));
    CHECK(nc_put_att_string(ncid, NC_GLOBAL, "empty", 0, NULL));
    CHECK(nc_put_att_text(ncid, NC_GLOBAL, "replace", 3, "old"));
    CHECK(nc_put_att_string(ncid, NC_GLOBAL, "replace", 1, (const char **)&fill));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_vars_string(ncid, var, start, count, stride, (const char **)input));
    CHECK(nc_put_var_string(ncid, scalar, (const char **)&fill));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open("strings.nc", NC_NEXTCDF4 | NC_WRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "s", &var));
    CHECK(nc_inq_varid(ncid, "scalar", &scalar));
    CHECK(nc_inq_ncid(ncid, "g", &grp));
    CHECK(nc_get_att_string(ncid, var, "a", out));
    REQUIRE(!strcmp(out[0], "alpha") && !strcmp(out[2], input[2]) && !strcmp(out[3], ""));
    CHECK(nc_free_string(4, out)); memset(out, 0, sizeof(out));
    CHECK(nc_get_att_string(grp, NC_GLOBAL, "a", out));
    CHECK(nc_free_string(3, out)); memset(out, 0, sizeof(out));
    CHECK(nc_inq_var_fill(ncid, var, &nofill, &got));
    REQUIRE(!nofill && got && !strcmp(got, fill));
    CHECK(nc_free_string(1, &got)); got = NULL;
    CHECK(nc_get_var(ncid, var, out));
    REQUIRE(!strcmp(out[0], "alpha") && !strcmp(out[1], fill) && !strcmp(out[6], ""));
    CHECK(nc_free_string(7, out)); memset(out, 0, sizeof(out));
    CHECK(nc_get_var_string(ncid, scalar, out));
    REQUIRE(!strcmp(out[0], fill));
    CHECK(nc_free_string(1, out)); memset(out, 0, sizeof(out));
    REQUIRE(nc_get_var_int(ncid, scalar, &numeric) == NC_EBADTYPE);
    REQUIRE(nc_get_var_text(ncid, scalar, (char *)&numeric) == NC_ECHAR);
    CHECK(nc_copy_att(grp, NC_GLOBAL, "a", ncid, NC_GLOBAL));
    CHECK(nc_rename_att(ncid, NC_GLOBAL, "a", "copied"));
    CHECK(nc_enddef(ncid));
    CHECK(nc_close(ncid)); ncid = -1;
    if (mode) {
        CHECK(nc_open("strings.nc", NC_NOWRITE, &ncid));
        CHECK(nc_get_att_string(ncid, NC_GLOBAL, "copied", out));
        CHECK(nc_free_string(3, out)); memset(out, 0, sizeof(out));
        CHECK(nc_close(ncid)); ncid = -1;
    }
    return 0;
fail:
    if (got) nc_free_string(1, &got);
    nc_free_string(8, out);
    if (ncid >= 0) nc_abort(ncid);
    rc = 1;
    return rc;
}

static int
nested(int mode, int upstream)
{
    int ncid = -1, dim, var, fillvar, shape[] = {2};
    nc_type words, record, list;
    char *names[] = {"one", "two"};
    struct record input = {{"left", "right"}, {2, names}};
    struct record result = {0};
    nc_vlen_t source = {1, &input}, output = {0};
    int have_result = 0, have_output = 0;
    CHECK(nc_create("nested.nc", upstream ? NC_NETCDF4 : NC_NEXTCDF4 | mode, &ncid));
    CHECK(nc_def_vlen(ncid, "words", NC_STRING, &words));
    CHECK(nc_def_compound(ncid, sizeof(input), "record", &record));
    CHECK(nc_insert_array_compound(ncid, record, "names", offsetof(struct record, names), NC_STRING, 1, shape));
    CHECK(nc_insert_compound(ncid, record, "words", offsetof(struct record, words), words));
    CHECK(nc_def_vlen(ncid, "list", record, &list));
    CHECK(nc_def_dim(ncid, "n", 1, &dim));
    CHECK(nc_def_var(ncid, "records", record, 1, &dim, &var));
    CHECK(nc_def_var(ncid, "fill_records", record, 1, &dim, &fillvar));
    CHECK(nc_put_att(ncid, fillvar, "_FillValue", record, 1, &input));
    CHECK(nc_put_att(ncid, NC_GLOBAL, "list", list, 1, &source));
    CHECK(nc_put_var(ncid, var, &input));
    names[0] = "source changed";
    input.names[0] = "source changed";
    CHECK(nc_close(ncid)); ncid = -1;
    names[0] = "one";
    input.names[0] = "left";
    CHECK(nc_open("nested.nc", NC_NEXTCDF4 | NC_WRITE, &ncid));
    CHECK(nc_inq_typeid(ncid, "list", &list));
    CHECK(nc_inq_varid(ncid, "records", &var));
    CHECK(nc_inq_vartype(ncid, var, &record));
    CHECK(nc_get_var(ncid, var, &result)); have_result = 1;
    REQUIRE(!strcmp(result.names[1], "right") && result.words.len == 2 && !strcmp(((char **)result.words.p)[0], "one"));
    CHECK(nc_get_att(ncid, NC_GLOBAL, "list", &output)); have_output = 1;
    REQUIRE(output.len == 1 && !strcmp(((struct record *)output.p)->names[0], "left"));
    CHECK(nc_reclaim_data(ncid, record, &result, 1)); have_result = 0; memset(&result, 0, sizeof(result));
    CHECK(nc_reclaim_data(ncid, list, &output, 1)); have_output = 0; memset(&output, 0, sizeof(output));
    CHECK(nc_inq_varid(ncid, "fill_records", &fillvar));
    CHECK(nc_get_var(ncid, fillvar, &result)); have_result = 1;
    REQUIRE(result.names[0] && !strcmp(result.names[0], "left") &&
            result.words.len == 2 && !strcmp(((char **)result.words.p)[0], "one"));
    CHECK(nc_reclaim_data(ncid, record, &result, 1)); have_result = 0; memset(&result, 0, sizeof(result));
    CHECK(nc_inq_var_fill(ncid, fillvar, NULL, &result)); have_result = 1;
    REQUIRE(result.names[0] && !strcmp(result.names[0], "left"));
    CHECK(nc_reclaim_data(ncid, record, &result, 1)); have_result = 0; memset(&result, 0, sizeof(result));
    input.names[0] = "updated";
    CHECK(nc_put_var(ncid, var, &input));
    CHECK(nc_put_att(ncid, NC_GLOBAL, "list", list, 1, &source));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open("nested.nc", upstream || mode ? NC_NOWRITE : NC_NEXTCDF4, &ncid));
    CHECK(nc_inq_varid(ncid, "records", &var));
    CHECK(nc_inq_vartype(ncid, var, &record));
    CHECK(nc_get_var(ncid, var, &result)); have_result = 1;
    REQUIRE(!strcmp(result.names[0], "updated"));
    CHECK(nc_reclaim_data(ncid, record, &result, 1)); have_result = 0;
    CHECK(nc_close(ncid));
    return 0;
fail:
    if (ncid >= 0) {
        if (have_result) nc_reclaim_data(ncid, record, &result, 1);
        if (have_output) nc_reclaim_data(ncid, list, &output, 1);
        nc_abort(ncid);
    }
    return 1;
}

static int
clipped(void)
{
    int ncid = -1, dims[2], small, large;
    char *value[] = {"a", "b"}, *out[12] = {0};
    size_t start[] = {0, 0}, count[] = {1, 2}, end[] = {2, 3};
    ptrdiff_t stride[] = {1, 2};
    CHECK(nc_create("clipped.nc", NC_NEXTCDF4, &ncid));
    CHECK(nc_def_dim(ncid, "x", NC_UNLIMITED, &dims[0]));
    CHECK(nc_def_dim(ncid, "y", NC_UNLIMITED, &dims[1]));
    CHECK(nc_def_var(ncid, "small", NC_STRING, 2, dims, &small));
    CHECK(nc_def_var(ncid, "large", NC_STRING, 2, dims, &large));
    CHECK(nc_put_vara_string(ncid, small, start, count, (const char **)value));
    CHECK(nc_put_var1_string(ncid, large, end, (const char **)value));
    count[0] = 3; count[1] = 4;
    CHECK(nc_get_vara_string(ncid, small, start, count, out));
    for (int k = 0; k < 12; k++) REQUIRE(out[k] && !strcmp(out[k], k == 0 ? "a" : k == 1 ? "b" : ""));
    CHECK(nc_free_string(12, out)); memset(out, 0, sizeof(out));
    count[1] = 2;
    CHECK(nc_get_vars_string(ncid, small, start, count, stride, out));
    for (int k = 0; k < 6; k++) REQUIRE(out[k] && !strcmp(out[k], k == 0 ? "a" : ""));
    CHECK(nc_free_string(6, out)); memset(out, 0, sizeof(out));
    CHECK(nc_close(ncid));
    return 0;
fail:
    nc_free_string(12, out);
    if (ncid >= 0) nc_abort(ncid);
    return 1;
}

static int
legacy(void)
{
    hid_t file = -1, type = -1, scalar = -1, dataset = -1, att = -1, dcpl = -1;
    int ncid = -1, var;
    char *value = "legacy", *out = NULL;
    file = H5Fcreate("legacy.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    type = H5Tcopy(H5T_C_S1);
    scalar = H5Screate(H5S_SCALAR);
    dcpl = H5Pcreate(H5P_DATASET_CREATE);
    REQUIRE(file >= 0 && type >= 0 && scalar >= 0 && dcpl >= 0);
    REQUIRE(H5Tset_size(type, H5T_VARIABLE) >= 0);
    REQUIRE(H5Pset_fill_value(dcpl, type, &value) >= 0);
    dataset = H5Dcreate2(file, "legacy", type, scalar, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    REQUIRE(dataset >= 0 && H5Dwrite(dataset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value) >= 0);
    att = H5Acreate2(file, "a", type, scalar, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(att >= 0 && H5Awrite(att, type, &value) >= 0);
    H5Aclose(att); att = -1;
    H5Dclose(dataset); dataset = -1;
    H5Fclose(file); file = -1;
    CHECK(nc_open("legacy.h5", NC_NEXTCDF4 | NC_WRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "legacy", &var));
    CHECK(nc_get_var_string(ncid, var, &out));
    REQUIRE(out && !strcmp(out, value));
    CHECK(nc_free_string(1, &out)); out = NULL;
    CHECK(nc_get_att_string(ncid, NC_GLOBAL, "a", &out));
    REQUIRE(out && !strcmp(out, value));
    CHECK(nc_free_string(1, &out)); out = NULL;
    CHECK(nc_inq_var_fill(ncid, var, NULL, &out));
    REQUIRE(out && !strcmp(out, value));
    CHECK(nc_free_string(1, &out)); out = NULL;
    value = "updated";
    CHECK(nc_put_var_string(ncid, var, (const char **)&value));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open("legacy.h5", NC_NEXTCDF4, &ncid));
    CHECK(nc_get_var_string(ncid, var, &out));
    REQUIRE(out && !strcmp(out, value));
    CHECK(nc_free_string(1, &out)); out = NULL;
    CHECK(nc_close(ncid)); ncid = -1;
    H5Pclose(dcpl); dcpl = -1;
    H5Tclose(type); type = H5Tcopy(H5T_C_S1);
    REQUIRE(type >= 0 && H5Tset_size(type, 8) >= 0);
    file = H5Fcreate("unsupported.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(file >= 0);
    dataset = H5Dcreate2(file, "wide", type, scalar, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(dataset >= 0);
    H5Dclose(dataset); dataset = -1;
    H5Fclose(file); file = -1;
    for (int k = 0; k < 8; k++) {
        REQUIRE(nc_open("unsupported.h5", NC_NEXTCDF4, &ncid) == NC_EBADTYPE);
        ncid = -1;
    }
    H5Sclose(scalar);
    H5Tclose(type);
    return 0;
fail:
    if (out) nc_free_string(1, &out);
    if (ncid >= 0) nc_abort(ncid);
    if (att >= 0) H5Aclose(att);
    if (dataset >= 0) H5Dclose(dataset);
    if (dcpl >= 0) H5Pclose(dcpl);
    if (scalar >= 0) H5Sclose(scalar);
    if (type >= 0) H5Tclose(type);
    if (file >= 0) H5Fclose(file);
    return 1;
}

static int
metadata(int model)
{
    int ncid = -1, grp, dim, var, old, nofill;
    size_t start = 0, count = 0, len;
    ptrdiff_t stride = 0;
    char mutable[] = "owned", *input[] = {mutable}, *out[2] = {0};
    char name[NC_MAX_NAME + 1];
    CHECK(nc_create("metadata.nc", NC_NEXTCDF4 | model, &ncid));
    CHECK(nc_set_fill(ncid, NC_NOFILL, &old));
    CHECK(nc_def_grp(ncid, "g", &grp));
    CHECK(nc_def_dim(grp, "n", NC_UNLIMITED, &dim));
    CHECK(nc_def_var(grp, "v", NC_STRING, 1, &dim, &var));
    CHECK(nc_inq_var_fill(grp, var, &nofill, &out[0]));
    REQUIRE(!nofill && out[0] && !out[0][0]);
    CHECK(nc_free_string(1, out)); memset(out, 0, sizeof(out));
    CHECK(nc_put_att_string(grp, NC_GLOBAL, "first", 1, (const char **)input));
    mutable[0] = 'X';
    CHECK(nc_copy_att(grp, NC_GLOBAL, "first", grp, var));
    CHECK(nc_rename_att(grp, var, "first", "renamed"));
    CHECK(nc_put_att_text(grp, var, "replace", 3, "old"));
    CHECK(nc_put_att_string(grp, var, "replace", 1, (const char **)input));
    CHECK(nc_put_att_text(grp, var, "replace", 3, "new"));
    CHECK(nc_del_att(grp, var, "replace"));
    CHECK(nc_inq_attname(grp, var, 0, name)); REQUIRE(!strcmp(name, "renamed"));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_vara_string(grp, var, &start, &count, NULL));
    CHECK(nc_inq_dimlen(grp, dim, &len)); REQUIRE(!len);
    count = 1;
    REQUIRE(nc_put_vars_string(grp, var, &start, &count, &stride, (const char **)input) == NC_ESTRIDE);
    REQUIRE(nc_put_vara_int(grp, var, &start, &count, &old) == NC_EBADTYPE);
    REQUIRE(nc_put_vara_text(grp, var, &start, &count, "x") == NC_ECHAR);
    CHECK(nc_put_var1_string(grp, var, &start, (const char **)input));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_open("metadata.nc", NC_NEXTCDF4, &ncid));
    CHECK(nc_inq_ncid(ncid, "g", &grp));
    CHECK(nc_inq_varid(grp, "v", &var));
    CHECK(nc_get_att_string(grp, NC_GLOBAL, "first", out));
    CHECK(nc_get_att_string(grp, var, "renamed", out + 1));
    REQUIRE(out[0] != out[1] && !strcmp(out[0], "owned") && !strcmp(out[1], "owned"));
    CHECK(nc_free_string(2, out)); memset(out, 0, sizeof(out));
    CHECK(nc_get_var1_string(grp, var, &start, out));
    REQUIRE(out[0] && !strcmp(out[0], "Xwned"));
    CHECK(nc_free_string(1, out)); memset(out, 0, sizeof(out));
    CHECK(nc_close(ncid)); ncid = -1;
    CHECK(nc_create("classic.nc", NC_NEXTCDF4 | NC_CLASSIC_MODEL, &ncid));
    REQUIRE(nc_def_var(ncid, "s", NC_STRING, 0, NULL, &var) == NC_EBADTYPE);
    REQUIRE(nc_put_att_string(ncid, NC_GLOBAL, "s", 1, (const char **)input) == NC_ESTRICTNC3);
    CHECK(nc_close(ncid));
    return 0;
fail:
    nc_free_string(2, out);
    if (ncid >= 0) nc_abort(ncid);
    return 1;
}

int
main(void)
{
    if (!NC_NEXTCDF4_initialize()) return 1;
    for (int mode = 0; mode < 2; mode++) {
        if (strings(mode ? NC_NETCDF4_MODEL : 0) || metadata(mode ? NC_NETCDF4_MODEL : 0)) return 1;
        if (nested(mode ? NC_NETCDF4_MODEL : 0, 0)) return 1;
    }
    if (nested(0, 1) || clipped() || legacy()) return 1;
    return 0;
}

/**
 * @file tst_nextcdf4_ref.c
 * @brief Tests for NC_STD_REF reference types and the nc_ref_* API
 * in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-11-21
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hdf5.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NAME  "tst_nextcdf4_ref.h5"
#define OTHER_NAME "tst_nextcdf4_ref_other.h5"

/* Helper macro for error checking. */
#define CHECK(expr) do { \
    int _ret = (expr); \
    if (_ret != NC_NOERR) { \
        fprintf(stderr, "%s:%d: %s failed: %s\n", __FILE__, __LINE__, #expr, nc_strerror(_ret)); \
        return 1; \
    } \
} while (0)

/* Require a specific NetCDF error code. */
#define EXPECT(expr, want) do { \
    int _ret = (expr); \
    if (_ret != (want)) { \
        fprintf(stderr, "%s:%d: %s returned %d, wanted %d\n", __FILE__, __LINE__, #expr, _ret, (want)); \
        return 1; \
    } \
} while (0)

/* Create the file with one dim, one data var, and one NC_STD_REF var. */
static int
make_file(int *ncidp, int *dimidp, int *varidp, int *refidp)
{
    unlink(FILE_NAME);
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, ncidp));
    CHECK(nc_def_dim(*ncidp, "x", 4, dimidp));
    CHECK(nc_def_var(*ncidp, "data", NC_INT, 1, dimidp, varidp));
    CHECK(nc_def_var(*ncidp, "refs", NC_STD_REF, 1, dimidp, refidp));
    CHECK(nc_enddef(*ncidp));
    return 0;
}

/* Token write/read round-trip through an NC_STD_REF variable. */
static int
test_std_ref_io(void)
{
    int ncid, dimid, varid, refid;
    size_t start[1] = {0}, count[1] = {1};
    nc_std_ref_t ref, ref_in;
    nc_type xtype_in;
    int obj_type, gid, id;

    memset(&ref_in, 0, sizeof(ref_in));
    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref));
    CHECK(nc_put_vara(ncid, refid, start, count, &ref));
    CHECK(nc_ref_free(&ref));
    CHECK(nc_close(ncid));

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    CHECK(nc_inq_var(ncid, refid, NULL, &xtype_in, NULL, NULL, NULL));
    if (xtype_in != NC_STD_REF) {
        fprintf(stderr, "xtype %d != NC_STD_REF\n", xtype_in);
        nc_close(ncid);
        return 1;
    }
    if (sizeof(nc_std_ref_t) != NC_STD_REF_SIZE) {
        fprintf(stderr, "token size %d != %d\n", (int)sizeof(nc_std_ref_t),
                NC_STD_REF_SIZE);
        nc_close(ncid);
        return 1;
    }
    CHECK(nc_get_vara(ncid, refid, start, count, &ref_in));
    /* The stored token's bytes differ from a live token; check it
     * by dereferencing back to the same variable. */
    CHECK(nc_deref_object(ncid, &ref_in, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_VAR || id != varid)
        return 1;
    CHECK(nc_ref_free(&ref_in));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/* Object references for a variable, a group, and a user type. */
static int
test_object_ref(void)
{
    int ncid, dimid, varid, refid, grpid, typeid;
    size_t start[1] = {0}, count[1] = {1};
    nc_std_ref_t ref_var, ref_grp, ref_type;
    int obj_type, gid, id, rk;

    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_redef(ncid));
    CHECK(nc_def_grp(ncid, "g1", &grpid));
    CHECK(nc_def_opaque(ncid, 8, "opaque8", &typeid));
    CHECK(nc_enddef(ncid));

    CHECK(nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref_var));
    CHECK(nc_ref_object(ncid, NC_REF_OBJ_GRP, grpid, &ref_grp));
    CHECK(nc_ref_object(ncid, NC_REF_OBJ_TYPE, typeid, &ref_type));

    CHECK(nc_ref_get_type(&ref_var, &rk));
    if (rk != NC_STD_REF_OBJECT)
        return 1;

    /* Variable: containing group is root, id is the varid. */
    CHECK(nc_deref_object(ncid, &ref_var, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_VAR || id != varid || gid != ncid)
        return 1;

    /* Group: grpid and id are both the group ncid. */
    CHECK(nc_deref_object(ncid, &ref_grp, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_GRP || id != grpid || gid != grpid)
        return 1;

    /* Type: id is the typeid, group is root. */
    CHECK(nc_deref_object(ncid, &ref_type, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_TYPE || id != typeid || gid != ncid)
        return 1;

    /* Wrong-kind token is rejected. */
    {
        size_t s[1] = {0}, c[1] = {2};
        nc_std_ref_t region;

        CHECK(nc_ref_region(ncid, varid, s, c, NULL, &region));
        EXPECT(nc_deref_object(ncid, &region, NULL, NULL, NULL), NC_EINVAL);
        CHECK(nc_ref_free(&region));
    }

    /* Tokens survive a close/reopen. */
    CHECK(nc_put_vara(ncid, refid, start, count, &ref_var));
    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_close(ncid));
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    CHECK(nc_get_vara(ncid, refid, start, count, &ref_var));
    CHECK(nc_deref_object(ncid, &ref_var, &obj_type, &gid, &id));
    if (id != varid)
        return 1;
    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/* Region references recover variable and strided selection. */
static int
test_region_ref(void)
{
    int ncid, dimid, varid, refid;
    nc_std_ref_t ref;
    size_t start[1] = {1}, count[1] = {2};
    ptrdiff_t stride[1] = {2};
    int gid, vid, ndims, rk;
    size_t s_in[1], c_in[1];
    ptrdiff_t st_in[1];
    hid_t file, dset, space;
    hsize_t hs1[1] = {0}, hc1[1] = {1}, hs2[1] = {3}, hc2[1] = {3};
    H5R_ref_t irreg;

    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_close(ncid));

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "data", &varid));
    CHECK(nc_ref_region(ncid, varid, start, count, stride, &ref));
    CHECK(nc_ref_get_type(&ref, &rk));
    if (rk != NC_STD_REF_REGION)
        return 1;
    CHECK(nc_deref_region(ncid, &ref, &gid, &vid, &ndims, s_in, c_in,
                          st_in));
    if (vid != varid || gid != ncid || ndims != 1 ||
        s_in[0] != 1 || c_in[0] != 2 || st_in[0] != 2)
        return 1;
    CHECK(nc_ref_free(&ref));
    CHECK(nc_close(ncid));

    /* An irregular (union of unequal slabs) region is rejected. The
     * open must match NEXTCDF-4's H5F_CLOSE_STRONG because the irreg
     * token keeps the file alive past H5Fclose. */
    {
        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        if (fapl < 0 || H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0)
            return 1;
        file = H5Fopen(FILE_NAME, H5F_ACC_RDWR, fapl);
        H5Pclose(fapl);
        if (file < 0)
            return 1;
    }
    if ((dset = H5Dopen2(file, "data", H5P_DEFAULT)) < 0 ||
        (space = H5Dget_space(dset)) < 0)
        return 1;
    if (H5Sselect_hyperslab(space, H5S_SELECT_SET, hs1, NULL, hc1, NULL) < 0 ||
        H5Sselect_hyperslab(space, H5S_SELECT_OR, hs2, NULL, hc2, NULL) < 0 ||
        H5Rcreate_region(file, "data", space, H5P_DEFAULT, &irreg) < 0)
        return 1;
    H5Sclose(space);
    H5Dclose(dset);
    H5Fclose(file);

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    EXPECT(nc_deref_region(ncid, (nc_std_ref_t *)&irreg, NULL, NULL,
                           NULL, NULL, NULL, NULL), NC_EINVAL);
    H5Rdestroy(&irreg);
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/* Attribute references on a variable, the root group, and a
 * committed type. Global attributes live on the root group, so the
 * NC_REF_OBJ_GRP case targets the root group ncid. */
static int
test_attr_ref(void)
{
    int ncid, dimid, varid, refid, typeid;
    int ival = 42;
    nc_std_ref_t ref_var, ref_grp, ref_type;
    int obj_type, gid, id, rk;
    char aname[NC_MAX_NAME + 1];
    hid_t file, tid, att, space;

    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_redef(ncid));
    CHECK(nc_def_opaque(ncid, 8, "opaque8", &typeid));
    CHECK(nc_enddef(ncid));
    /* Attributes are written in define mode; the var's dataset must
     * exist, so enddef first, then redef. */
    CHECK(nc_redef(ncid));
    CHECK(nc_put_att(ncid, varid, "units", NC_INT, 1, &ival));
    CHECK(nc_put_att(ncid, NC_GLOBAL, "title", NC_INT, 1, &ival));
    CHECK(nc_enddef(ncid));
    CHECK(nc_close(ncid));

    /* The NetCDF API cannot put attributes on types; add one on the
     * committed datatype directly through HDF5. */
    {
        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        if (fapl < 0 || H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0)
            return 1;
        file = H5Fopen(FILE_NAME, H5F_ACC_RDWR, fapl);
        H5Pclose(fapl);
        if (file < 0)
            return 1;
    }
    if ((tid = H5Topen2(file, "opaque8", H5P_DEFAULT)) < 0 ||
        (space = H5Screate(H5S_SCALAR)) < 0 ||
        (att = H5Acreate2(tid, "dtype_note", H5T_NATIVE_INT, space,
                          H5P_DEFAULT, H5P_DEFAULT)) < 0 ||
        H5Awrite(att, H5T_NATIVE_INT, &ival) < 0)
        return 1;
    H5Aclose(att);
    H5Sclose(space);
    H5Tclose(tid);
    H5Fclose(file);

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "data", &varid));
    CHECK(nc_inq_typeid(ncid, "opaque8", &typeid));

    CHECK(nc_ref_attr(ncid, NC_REF_OBJ_VAR, varid, "units", &ref_var));
    CHECK(nc_ref_attr(ncid, NC_REF_OBJ_GRP, ncid, "title", &ref_grp));
    CHECK(nc_ref_attr(ncid, NC_REF_OBJ_TYPE, typeid, "dtype_note",
                      &ref_type));
    CHECK(nc_ref_get_type(&ref_var, &rk));
    if (rk != NC_STD_REF_ATTR)
        return 1;

    CHECK(nc_deref_attr(ncid, &ref_var, &obj_type, &gid, &id, aname));
    if (obj_type != NC_REF_OBJ_VAR || id != varid ||
        strcmp(aname, "units"))
        return 1;
    CHECK(nc_deref_attr(ncid, &ref_grp, &obj_type, &gid, &id, aname));
    if (obj_type != NC_REF_OBJ_GRP || id != ncid ||
        strcmp(aname, "title"))
        return 1;
    CHECK(nc_deref_attr(ncid, &ref_type, &obj_type, &gid, &id, aname));
    if (obj_type != NC_REF_OBJ_TYPE || id != typeid ||
        strcmp(aname, "dtype_note"))
        return 1;

    /* Missing attribute is rejected. */
    EXPECT(nc_ref_attr(ncid, NC_REF_OBJ_VAR, varid, "no_such_att",
                       &ref_var), NC_EINVAL);
    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/* NC_STD_REF attributes store and return tokens. */
static int
test_std_ref_att(void)
{
    int ncid, dimid, varid, refid;
    nc_std_ref_t ref, ref_in;
    nc_type xtype_in;
    int obj_type, gid, id, rk;

    memset(&ref_in, 0, sizeof(ref_in));
    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref));
    CHECK(nc_redef(ncid));
    CHECK(nc_put_att(ncid, varid, "ref_att", NC_STD_REF, 1, &ref));
    CHECK(nc_enddef(ncid));
    CHECK(nc_ref_free(&ref));
    CHECK(nc_close(ncid));

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "data", &varid));
    CHECK(nc_inq_att(ncid, varid, "ref_att", &xtype_in, NULL));
    if (xtype_in != NC_STD_REF)
        return 1;
    CHECK(nc_get_att(ncid, varid, "ref_att", &ref_in));
    CHECK(nc_ref_get_type(&ref_in, &rk));
    if (rk != NC_STD_REF_OBJECT)
        return 1;
    CHECK(nc_deref_object(ncid, &ref_in, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_VAR || id != varid)
        return 1;
    CHECK(nc_ref_free(&ref_in));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

/* References can point into another file; absolute target filename
 * required. Full cross-file dereference coverage lives in
 * tst_nextcdf4_ref_external. */
static int
test_external_ref(void)
{
    int ncid, dimid, varid, refid;
    size_t start[1] = {0}, count[1] = {1};
    int ival = 7;
    nc_std_ref_t ref_obj, ref_reg, ref_att;
    size_t rs[1] = {0}, rc[1] = {2};
    char fname[1024], oname[1024], abs_other[2048], cwd[1024];
    int rk;
    hid_t obj;

    if (!getcwd(cwd, sizeof(cwd)))
        return 1;
    snprintf(abs_other, sizeof(abs_other), "%s/%s", cwd, OTHER_NAME);

    /* Relative target filenames are rejected. */
    unlink(OTHER_NAME);
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    EXPECT(nc_ref_object_ext(ncid, OTHER_NAME, "/ext_data", &ref_obj),
           NC_EINVAL);
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);

    /* Second file holding the external targets. The variable
     * attribute can only be written once the dataset exists. */
    CHECK(nc_create(OTHER_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "y", 4, &dimid));
    CHECK(nc_def_var(ncid, "ext_data", NC_INT, 1, &dimid, &varid));
    CHECK(nc_enddef(ncid));
    CHECK(nc_redef(ncid));
    CHECK(nc_put_att(ncid, varid, "ext_att", NC_INT, 1, &ival));
    CHECK(nc_enddef(ncid));
    CHECK(nc_close(ncid));

    if (make_file(&ncid, &dimid, &varid, &refid))
        return 1;
    CHECK(nc_ref_object_ext(ncid, abs_other, "/ext_data", &ref_obj));
    CHECK(nc_ref_region_ext(ncid, abs_other, "/ext_data", rs, rc,
                            NULL, &ref_reg));
    CHECK(nc_ref_attr_ext(ncid, abs_other, "/ext_data", "ext_att",
                          &ref_att));

    /* Token kinds and stored names. */
    CHECK(nc_ref_get_type(&ref_obj, &rk));
    if (rk != NC_STD_REF_OBJECT)
        return 1;
    CHECK(nc_ref_get_file_name(&ref_obj, fname, sizeof(fname)));
    if (strcmp(fname, abs_other))
        return 1;
    CHECK(nc_ref_get_obj_name(&ref_obj, oname, sizeof(oname)));
    if (strcmp(oname, "/ext_data"))
        return 1;
    CHECK(nc_ref_get_type(&ref_reg, &rk));
    if (rk != NC_STD_REF_REGION)
        return 1;
    CHECK(nc_ref_get_type(&ref_att, &rk));
    if (rk != NC_STD_REF_ATTR)
        return 1;

    /* The containing file's ncid is not the token's target. */
    EXPECT(nc_deref_object(ncid, &ref_obj, NULL, NULL, NULL),
           NC_EINVAL);
    EXPECT(nc_deref_region(ncid, &ref_reg, NULL, NULL, NULL, NULL,
                         NULL, NULL), NC_EINVAL);
    EXPECT(nc_deref_attr(ncid, &ref_att, NULL, NULL, NULL, NULL),
           NC_EINVAL);
    CHECK(nc_ref_free(&ref_reg));
    CHECK(nc_ref_free(&ref_att));

    /* Store a token, reopen, and resolve the names again. */
    CHECK(nc_put_vara(ncid, refid, start, count, &ref_obj));
    CHECK(nc_ref_free(&ref_obj));
    CHECK(nc_close(ncid));
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    memset(&ref_obj, 0, sizeof(ref_obj));
    CHECK(nc_get_vara(ncid, refid, start, count, &ref_obj));
    CHECK(nc_ref_get_file_name(&ref_obj, fname, sizeof(fname)));
    if (strcmp(fname, abs_other))
        return 1;
    EXPECT(nc_deref_object(ncid, &ref_obj, NULL, NULL, NULL),
           NC_EINVAL);

    /* Raw HDF5 can still follow the cross-file reference. */
    obj = H5Ropen_object((H5R_ref_t *)&ref_obj, H5P_DEFAULT, H5P_DEFAULT);
    if (obj < 0) {
        fprintf(stderr, "H5Ropen_object on external ref failed\n");
        nc_close(ncid);
        return 1;
    }
    {
        char nbuf[1024];
        if (H5Iget_name(obj, nbuf, sizeof(nbuf)) <= 0 ||
            strcmp(nbuf, "/ext_data")) {
            H5Oclose(obj);
            nc_close(ncid);
            return 1;
        }
    }
    H5Oclose(obj);
    CHECK(nc_ref_free(&ref_obj));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    unlink(OTHER_NAME);
    return 0;
}

/* Compatibility modes reject NC_STD_REF and the create API. */
static int
test_rejected_in_model(int mode)
{
    int ncid, dimid, varid, refid;
    nc_std_ref_t ref;
    size_t z[1] = {0}, c[1] = {1};

    unlink(FILE_NAME);
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid));
    CHECK(nc_def_dim(ncid, "x", 4, &dimid));
    CHECK(nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid));
    if (nc_def_var(ncid, "refs", NC_STD_REF, 1, &dimid, &refid) == NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    EXPECT(nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref), NC_ENOTNC4);
    EXPECT(nc_ref_region(ncid, varid, z, c, NULL, &ref), NC_ENOTNC4);
    EXPECT(nc_ref_attr(ncid, NC_REF_OBJ_VAR, varid, "a", &ref),
           NC_ENOTNC4);
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    return 0;
}

int
main(void)
{
    int failed = 0;

    if (!NC_NEXTCDF4_initialize())
        return 1;
    if (test_std_ref_io()) {
        fprintf(stderr, "test_std_ref_io failed\n");
        failed = 1;
    }
    if (test_object_ref()) {
        fprintf(stderr, "test_object_ref failed\n");
        failed = 1;
    }
    if (test_region_ref()) {
        fprintf(stderr, "test_region_ref failed\n");
        failed = 1;
    }
    if (test_attr_ref()) {
        fprintf(stderr, "test_attr_ref failed\n");
        failed = 1;
    }
    if (test_std_ref_att()) {
        fprintf(stderr, "test_std_ref_att failed\n");
        failed = 1;
    }
    if (test_external_ref()) {
        fprintf(stderr, "test_external_ref failed\n");
        failed = 1;
    }
    if (test_rejected_in_model(NC_NETCDF4_MODEL)) {
        fprintf(stderr, "NC_NETCDF4_MODEL rejection failed\n");
        failed = 1;
    }
    if (test_rejected_in_model(NC_CLASSIC_MODEL)) {
        fprintf(stderr, "NC_CLASSIC_MODEL rejection failed\n");
        failed = 1;
    }
    return failed;
}

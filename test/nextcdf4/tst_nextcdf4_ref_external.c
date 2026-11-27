/*
 * tst_nextcdf4_ref_external.c - tests for dereferencing external
 * NC_STD_REF tokens through nc_ref_open_target() and the nc_deref_*
 * functions.
 *
 * Author: Edward Hartnett
 * Date: 2026-11-27
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <hdf5.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NAME   "tst_nextcdf4_ref_ext.h5"
#define TARGET_NAME "tst_nextcdf4_ref_ext_target.h5"

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

/* Absolute path of the target file in the test working directory. */
static int
target_path(char *buf, size_t size)
{
    char cwd[1024];

    if (!getcwd(cwd, sizeof(cwd)))
        return 1;
    if (snprintf(buf, size, "%s/%s", cwd, TARGET_NAME) >= (int)size)
        return 1;
    return 0;
}

/* Create the target file: nested groups g1/g2 with variable tdata,
 * a var attribute, a group attribute, and a committed type carrying
 * an HDF5-only attribute. */
static int
make_target(void)
{
    int ncid, dimid, varid, g1, g2;
    int ival = 7, gval = 5, data[4] = {10, 20, 30, 40};
    hid_t file, tid, att, space;
    int tval = 9;

    unlink(TARGET_NAME);
    CHECK(nc_create(TARGET_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
    CHECK(nc_def_dim(ncid, "y", 4, &dimid));
    CHECK(nc_def_grp(ncid, "g1", &g1));
    CHECK(nc_def_grp(g1, "g2", &g2));
    CHECK(nc_def_var(g2, "tdata", NC_INT, 1, &dimid, &varid));
    CHECK(nc_def_opaque(ncid, 8, "opaque8", NULL));
    CHECK(nc_enddef(ncid));
    CHECK(nc_put_var_int(g2, varid, data));
    CHECK(nc_redef(ncid));
    CHECK(nc_put_att(g2, varid, "units", NC_INT, 1, &ival));
    CHECK(nc_put_att(g1, NC_GLOBAL, "title", NC_INT, 1, &gval));
    CHECK(nc_enddef(ncid));
    CHECK(nc_close(ncid));

    /* The NetCDF API cannot put attributes on types; add one on the
     * committed datatype directly through HDF5. */
    {
        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        if (fapl < 0 || H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0)
            return 1;
        file = H5Fopen(TARGET_NAME, H5F_ACC_RDWR, fapl);
        H5Pclose(fapl);
        if (file < 0)
            return 1;
    }
    if ((tid = H5Topen2(file, "opaque8", H5P_DEFAULT)) < 0 ||
        (space = H5Screate(H5S_SCALAR)) < 0 ||
        (att = H5Acreate2(tid, "dtype_note", H5T_NATIVE_INT, space,
                          H5P_DEFAULT, H5P_DEFAULT)) < 0 ||
        H5Awrite(att, H5T_NATIVE_INT, &tval) < 0)
        return 1;
    H5Aclose(att);
    H5Sclose(space);
    H5Tclose(tid);
    H5Fclose(file);
    return 0;
}

/* Create the containing file with one NC_STD_REF variable. */
static int
make_containing(int *ncidp, int *refidp)
{
    int dimid;

    unlink(FILE_NAME);
    CHECK(nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, ncidp));
    CHECK(nc_def_dim(*ncidp, "x", 4, &dimid));
    CHECK(nc_def_var(*ncidp, "refs", NC_STD_REF, 1, &dimid, refidp));
    CHECK(nc_enddef(*ncidp));
    return 0;
}

/* Object references to a nested variable, a group, and a committed
 * type resolve through the opened target session; renaming the
 * variable between token creation and dereference still resolves. */
static int
test_object_ext(void)
{
    int ncid, refid, tnc, tnc2, varid, g1, g2;
    size_t start[1] = {0}, count[1] = {3};
    nc_std_ref_t ref_var, ref_grp, ref_type;
    char abs_t[2048], cwd[1024], name[NC_MAX_NAME + 1];
    int obj_type, gid, id, data[4];
    int t1;

    if (target_path(abs_t, sizeof(abs_t)) || !getcwd(cwd, sizeof(cwd)))
        return 1;
    if (make_target() || make_containing(&ncid, &refid))
        return 1;
    CHECK(nc_ref_object_ext(ncid, abs_t, "/g1/g2/tdata", &ref_var));
    CHECK(nc_ref_object_ext(ncid, abs_t, "/g1", &ref_grp));
    CHECK(nc_ref_object_ext(ncid, abs_t, "/opaque8", &ref_type));
    /* Persist all three tokens, then release them: a live token pins
     * the file it was created in, which would block the writable
     * reopen below. */
    {
        nc_std_ref_t toks[3];

        toks[0] = ref_var;
        toks[1] = ref_grp;
        toks[2] = ref_type;
        CHECK(nc_put_vara(ncid, refid, start, count, toks));
    }
    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(ncid));

    /* Rename the variable in the target; the token must still
     * resolve because identity, not the stored path, is followed. */
    CHECK(nc_open(TARGET_NAME, NC_NEXTCDF4 | NC_WRITE, &tnc));
    CHECK(nc_inq_grp_ncid(tnc, "g1", &g1));
    CHECK(nc_inq_grp_ncid(g1, "g2", &g2));
    CHECK(nc_inq_varid(g2, "tdata", &varid));
    CHECK(nc_rename_var(g2, varid, "renamed_data"));
    CHECK(nc_close(tnc));

    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    {
        nc_std_ref_t toks[3];

        memset(toks, 0, sizeof(toks));
        CHECK(nc_get_vara(ncid, refid, start, count, toks));
        ref_var = toks[0];
        ref_grp = toks[1];
        ref_type = toks[2];
    }

    /* The containing ncid is not the token's target. */
    EXPECT(nc_deref_object(ncid, &ref_var, NULL, NULL, NULL), NC_EINVAL);

    /* Absolute stored names resolve independent of the working
     * directory. */
    if (chdir("/"))
        return 1;
    CHECK(nc_ref_open_target(ncid, &ref_var, &tnc));
    /* A second call yields an independent, separately owned session. */
    CHECK(nc_ref_open_target(ncid, &ref_var, &tnc2));
    if (tnc == tnc2)
        return 1;
    CHECK(nc_deref_object(tnc2, &ref_var, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_VAR)
        return 1;
    CHECK(nc_close(tnc2));

    CHECK(nc_deref_object(tnc, &ref_var, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_VAR)
        return 1;
    CHECK(nc_inq_grp_ncid(tnc, "g1", &g1));
    CHECK(nc_inq_grp_ncid(g1, "g2", &g2));
    if (gid != g2)
        return 1;
    CHECK(nc_inq_varname(gid, id, name));
    if (strcmp(name, "renamed_data"))
        return 1;
    CHECK(nc_get_var_int(gid, id, data));
    if (data[0] != 10 || data[3] != 40)
        return 1;

    /* Group and committed-type object references. */
    CHECK(nc_deref_object(tnc, &ref_grp, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_GRP || id != g1)
        return 1;
    CHECK(nc_deref_object(tnc, &ref_type, &obj_type, &gid, &id));
    if (obj_type != NC_REF_OBJ_TYPE || gid != tnc)
        return 1;
    CHECK(nc_inq_typeid(tnc, "opaque8", &t1));
    if (id != t1)
        return 1;

    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(tnc));
    if (chdir(cwd))
        return 1;
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    unlink(TARGET_NAME);
    return 0;
}

/* Region references recover the target variable and strided
 * selection on the target session. */
static int
test_region_ext(void)
{
    int ncid, refid, tnc, g1, g2;
    nc_std_ref_t ref;
    size_t rs[1] = {1}, rc[1] = {2};
    ptrdiff_t rst[1] = {2};
    int gid, vid, ndims;
    size_t s_in[1], c_in[1];
    ptrdiff_t st_in[1];
    int vals[2];
    char abs_t[2048];

    if (target_path(abs_t, sizeof(abs_t)))
        return 1;
    size_t start[1] = {0}, count[1] = {1};

    if (make_target() || make_containing(&ncid, &refid))
        return 1;
    CHECK(nc_ref_region_ext(ncid, abs_t, "/g1/g2/tdata", rs, rc, rst,
                            &ref));
    /* A live external token pins the target file open; persist the
     * token, release it, and read it back before opening the target
     * session, which is the intended usage flow. */
    CHECK(nc_put_vara(ncid, refid, start, count, &ref));
    CHECK(nc_ref_free(&ref));
    CHECK(nc_close(ncid));
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    memset(&ref, 0, sizeof(ref));
    CHECK(nc_get_vara(ncid, refid, start, count, &ref));
    CHECK(nc_ref_open_target(ncid, &ref, &tnc));
    CHECK(nc_deref_region(tnc, &ref, &gid, &vid, &ndims, s_in, c_in,
                          st_in));
    CHECK(nc_inq_grp_ncid(tnc, "g1", &g1));
    CHECK(nc_inq_grp_ncid(g1, "g2", &g2));
    if (gid != g2 || ndims != 1 || s_in[0] != 1 || c_in[0] != 2 ||
        st_in[0] != 2)
        return 1;
    /* The recovered selection reads the expected elements. */
    CHECK(nc_get_vars_int(gid, vid, s_in, c_in, st_in, vals));
    if (vals[0] != 20 || vals[1] != 40)
        return 1;
    CHECK(nc_ref_free(&ref));
    CHECK(nc_close(tnc));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    unlink(TARGET_NAME);
    return 0;
}

/* Attribute references resolve owner + name on the target session;
 * variable and group values read through NetCDF, the committed-type
 * attribute value through raw HDF5. */
static int
test_attr_ext(void)
{
    int ncid, refid, tnc, g1, typeid;
    nc_std_ref_t ref_var, ref_grp, ref_type;
    int obj_type, gid, id, v;
    char aname[NC_MAX_NAME + 1], abs_t[2048];
    hid_t att;

    if (target_path(abs_t, sizeof(abs_t)))
        return 1;
    size_t start[1] = {0}, count[1] = {3};

    if (make_target() || make_containing(&ncid, &refid))
        return 1;
    CHECK(nc_ref_attr_ext(ncid, abs_t, "/g1/g2/tdata", "units",
                          &ref_var));
    CHECK(nc_ref_attr_ext(ncid, abs_t, "/g1", "title", &ref_grp));
    CHECK(nc_ref_attr_ext(ncid, abs_t, "/opaque8", "dtype_note",
                          &ref_type));
    /* Live external tokens pin the target file open; persist,
     * release, and read back before opening the target session. */
    {
        nc_std_ref_t toks[3];

        toks[0] = ref_var;
        toks[1] = ref_grp;
        toks[2] = ref_type;
        CHECK(nc_put_vara(ncid, refid, start, count, toks));
    }
    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(ncid));
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    {
        nc_std_ref_t toks[3];

        memset(toks, 0, sizeof(toks));
        CHECK(nc_get_vara(ncid, refid, start, count, toks));
        ref_var = toks[0];
        ref_grp = toks[1];
        ref_type = toks[2];
    }
    CHECK(nc_ref_open_target(ncid, &ref_var, &tnc));

    CHECK(nc_deref_attr(tnc, &ref_var, &obj_type, &gid, &id, aname));
    if (obj_type != NC_REF_OBJ_VAR || strcmp(aname, "units"))
        return 1;
    CHECK(nc_get_att_int(gid, id, "units", &v));
    if (v != 7)
        return 1;

    CHECK(nc_deref_attr(tnc, &ref_grp, &obj_type, &gid, &id, aname));
    CHECK(nc_inq_grp_ncid(tnc, "g1", &g1));
    if (obj_type != NC_REF_OBJ_GRP || id != g1 ||
        strcmp(aname, "title"))
        return 1;
    CHECK(nc_get_att_int(g1, NC_GLOBAL, "title", &v));
    if (v != 5)
        return 1;

    CHECK(nc_deref_attr(tnc, &ref_type, &obj_type, &gid, &id, aname));
    CHECK(nc_inq_typeid(tnc, "opaque8", &typeid));
    if (obj_type != NC_REF_OBJ_TYPE || id != typeid ||
        strcmp(aname, "dtype_note"))
        return 1;
    /* Committed-type attribute values read through raw HDF5. */
    if ((att = H5Ropen_attr((H5R_ref_t *)&ref_type, H5P_DEFAULT,
                            H5P_DEFAULT)) < 0)
        return 1;
    if (H5Aread(att, H5T_NATIVE_INT, &v) < 0)
        return 1;
    H5Aclose(att);
    if (v != 9)
        return 1;

    CHECK(nc_ref_free(&ref_var));
    CHECK(nc_ref_free(&ref_grp));
    CHECK(nc_ref_free(&ref_type));
    CHECK(nc_close(tnc));
    CHECK(nc_close(ncid));
    unlink(FILE_NAME);
    unlink(TARGET_NAME);
    return 0;
}

/* Failure paths: relative names, in-file tokens, missing and
 * read-only targets, and wrong token kinds. */
static int
test_failures(void)
{
    int ncid, refid, tnc, varid, dimid;
    nc_std_ref_t ref, ext_ref;
    char abs_t[2048];

    if (target_path(abs_t, sizeof(abs_t)))
        return 1;
    if (make_target() || make_containing(&ncid, &refid))
        return 1;
    CHECK(nc_inq_varid(ncid, "refs", &varid));
    CHECK(nc_inq_dimid(ncid, "x", &dimid));

    /* Relative filenames are rejected at creation. */
    EXPECT(nc_ref_object_ext(ncid, TARGET_NAME, "/g1/g2/tdata", &ref),
           NC_EINVAL);
    EXPECT(nc_ref_region_ext(ncid, TARGET_NAME, "/g1/g2/tdata",
                             (size_t[]){0}, (size_t[]){1}, NULL,
                             &ref), NC_EINVAL);
    EXPECT(nc_ref_attr_ext(ncid, TARGET_NAME, "/g1/g2/tdata", "units",
                           &ref), NC_EINVAL);

    /* An in-file token has no external target to open. */
    CHECK(nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref));
    EXPECT(nc_ref_open_target(ncid, &ref, &tnc), NC_EINVAL);
    if (tnc != -1)
        return 1;
    CHECK(nc_ref_free(&ref));

    /* NULL and kind checks on the opener. */
    EXPECT(nc_ref_open_target(ncid, NULL, &tnc), NC_EINVAL);
    EXPECT(nc_ref_open_target(ncid, &ref, NULL), NC_EINVAL);

    /* A valid external token; then break the target. Persist and
     * release the token so it does not pin the target open. */
    CHECK(nc_ref_object_ext(ncid, abs_t, "/g1/g2/tdata", &ext_ref));
    {
        size_t start[1] = {0}, count[1] = {1};

        CHECK(nc_put_vara(ncid, refid, start, count, &ext_ref));
    }
    CHECK(nc_ref_free(&ext_ref));
    CHECK(nc_close(ncid));

    /* Read-only target cannot satisfy the writable-open contract. */
    if (chmod(TARGET_NAME, 0444))
        return 1;
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    CHECK(nc_inq_varid(ncid, "refs", &refid));
    {
        size_t start[1] = {0}, count[1] = {1};

        memset(&ext_ref, 0, sizeof(ext_ref));
        CHECK(nc_get_vara(ncid, refid, start, count, &ext_ref));
    }
    if (nc_ref_open_target(ncid, &ext_ref, &tnc) == NC_NOERR) {
        fprintf(stderr, "read-only target opened read-write\n");
        nc_close(tnc);
        return 1;
    }
    if (chmod(TARGET_NAME, 0644))
        return 1;

    /* Missing target fails deterministically. */
    CHECK(nc_close(ncid));
    unlink(TARGET_NAME);
    CHECK(nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
    if (nc_ref_open_target(ncid, &ext_ref, &tnc) == NC_NOERR) {
        fprintf(stderr, "missing target opened\n");
        nc_close(tnc);
        return 1;
    }
    CHECK(nc_ref_free(&ext_ref));
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
    if (test_object_ext()) {
        fprintf(stderr, "test_object_ext failed\n");
        failed = 1;
    }
    if (test_region_ext()) {
        fprintf(stderr, "test_region_ext failed\n");
        failed = 1;
    }
    if (test_attr_ext()) {
        fprintf(stderr, "test_attr_ext failed\n");
        failed = 1;
    }
    if (test_failures()) {
        fprintf(stderr, "test_failures failed\n");
        failed = 1;
    }
    return failed;
}

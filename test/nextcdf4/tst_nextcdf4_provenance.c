/* Provenance attributes: stored _NCProperties/_nc3_strict and virtual
 * _IsNetcdf4/_SuperblockVersion.
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include <hdf5.h>
#include "nxt4internal.h"

#define CHECK(call) do { int r = (call); if (r != NC_NOERR) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(r)); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "%d: %s\n", __LINE__, #test); return 1; } } while (0)

/* Read the stored _NCProperties bytes straight from HDF5. Returns a
 * malloc'd string (NUL-terminated; the stored size is strlen with
 * NULLTERM padding) or NULL when the attribute is absent. */
static char *
h5_ncprops(const char *path)
{
    hid_t fid, attr, type;
    char *value = NULL;
    size_t size;

    if ((fid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        return NULL;
    if (H5Aexists(fid, "_NCProperties") > 0 &&
        (attr = H5Aopen(fid, "_NCProperties", H5P_DEFAULT)) >= 0 &&
        (type = H5Aget_type(attr)) >= 0) {
        size = H5Tget_size(type);
        if ((value = calloc(size + 1, 1)))
            H5Aread(attr, type, value);
        H5Tclose(type);
        H5Aclose(attr);
    }
    H5Fclose(fid);
    return value;
}

/* Expected _SuperblockVersion for the file: the FCPL super field. */
static int
h5_superblock(const char *path, int *superp)
{
    hid_t fid, fcpl;
    unsigned super;

    if ((fid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        return 1;
    if ((fcpl = H5Fget_create_plist(fid)) < 0 ||
        H5Pget_version(fcpl, &super, NULL, NULL, NULL) < 0)
        return 1;
    H5Pclose(fcpl);
    H5Fclose(fid);
    *superp = (int)super;
    return 0;
}

static int
check_file(const char *path, int mode)
{
    int ncid, iv, super = -1;
    short sv;
    long long llv;
    double dv;
    nc_type xtype;
    size_t len;
    int attnum;
    char buf[256];
    char *props;

    /* Raw HDF5 inspection: the stored surface must match upstream. */
    REQUIRE((props = h5_ncprops(path)) != NULL);
    REQUIRE(!strncmp(props, "version=2,netcdf=", 17));
    REQUIRE(strstr(props, ",hdf5=") != NULL);
    REQUIRE(strstr(props, ",nextcdf4=") != NULL);
    free(props);
    {
        hid_t fid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT);
        REQUIRE(fid >= 0);
        REQUIRE(!!(mode & NC_CLASSIC_MODEL) ==
                (H5Aexists(fid, "_nc3_strict") > 0));
        /* The virtual attributes are never stored. */
        REQUIRE(H5Aexists(fid, "_IsNetcdf4") <= 0);
        REQUIRE(H5Aexists(fid, "_SuperblockVersion") <= 0);
        H5Fclose(fid);
    }
    REQUIRE(!h5_superblock(path, &super));

    /* API view: name lookups answer, enumeration hides them. */
    CHECK(nc_open(path, NC_NEXTCDF4, &ncid));
    CHECK(nc_inq_att(ncid, NC_GLOBAL, "_IsNetcdf4", &xtype, &len));
    REQUIRE(xtype == NC_INT && len == 1);
    CHECK(nc_get_att_int(ncid, NC_GLOBAL, "_IsNetcdf4", &iv));
    REQUIRE(iv == 1);
    CHECK(nc_inq_att(ncid, NC_GLOBAL, "_SuperblockVersion", &xtype, &len));
    CHECK(nc_get_att_int(ncid, NC_GLOBAL, "_SuperblockVersion", &iv));
    REQUIRE(iv == super);
    CHECK(nc_get_att_short(ncid, NC_GLOBAL, "_IsNetcdf4", &sv));
    REQUIRE(sv == 1);
    CHECK(nc_get_att_longlong(ncid, NC_GLOBAL, "_IsNetcdf4", &llv));
    REQUIRE(llv == 1);
    REQUIRE(nc_get_att_double(ncid, NC_GLOBAL, "_IsNetcdf4", &dv) == NC_ERANGE);
    REQUIRE(nc_inq_attid(ncid, NC_GLOBAL, "_IsNetcdf4", &attnum) == NC_EATTMETA);
    REQUIRE(nc_inq_attid(ncid, NC_GLOBAL, "_SuperblockVersion", &attnum) == NC_EATTMETA);
    REQUIRE(nc_inq_attid(ncid, NC_GLOBAL, "_NCProperties", &attnum) == NC_EATTMETA);
    CHECK(nc_inq_att(ncid, NC_GLOBAL, "_NCProperties", &xtype, &len));
    REQUIRE(xtype == NC_CHAR && len > 0);
    memset(buf, 0, sizeof(buf));
    CHECK(nc_get_att_text(ncid, NC_GLOBAL, "_NCProperties", buf));
    REQUIRE(!strncmp(buf, "version=2,netcdf=", 17));
    REQUIRE(strstr(buf, ",nextcdf4=") != NULL);
    CHECK(nc_close(ncid));
    return 0;
}

static int
run_mode(int mode)
{
    int ncid, dim, var, natts = -1, i;
    char name[NC_MAX_NAME + 1];
    const char *path =
        (mode & NC_CLASSIC_MODEL) ? "prov_classic.nc" :
        (mode & NC_NETCDF4_MODEL) ? "prov_model.nc" : "prov_native.nc";

    CHECK(nc_create(path, NC_NEXTCDF4 | mode, &ncid));
    /* Virtual attributes answer during the session too. */
    {
        int iv;
        CHECK(nc_get_att_int(ncid, NC_GLOBAL, "_IsNetcdf4", &iv));
        REQUIRE(iv == 1);
        /* _NCProperties is known as soon as the file exists. */
        char buf[256];
        CHECK(nc_get_att_text(ncid, NC_GLOBAL, "_NCProperties", buf));
        REQUIRE(strstr(buf, ",nextcdf4=") != NULL);
    }
    CHECK(nc_def_dim(ncid, "x", 4, &dim));
    CHECK(nc_def_var(ncid, "v", NC_INT, 1, &dim, &var));
    /* Reserved names stay rejected. */
    {
        int one = 1;
        REQUIRE(nc_put_att_int(ncid, NC_GLOBAL, "_IsNetcdf4", NC_INT, 1, &one) == NC_EBADNAME);
        REQUIRE(nc_put_att_int(ncid, NC_GLOBAL, "_nc3_strict", NC_INT, 1, &one) == NC_EBADNAME);
        CHECK(nc_put_att_int(ncid, NC_GLOBAL, "real", NC_INT, 1, &one));
        REQUIRE(nc_rename_att(ncid, NC_GLOBAL, "real", "_NCProperties") == NC_EBADNAME);
        REQUIRE(nc_del_att(ncid, NC_GLOBAL, "_NCProperties") == NC_ENOTATT);
    }
    /* Close without an explicit enddef: provenance is still stamped. */
    CHECK(nc_close(ncid));

    /* The provenance names never appear in attribute enumeration. */
    CHECK(nc_open(path, NC_NEXTCDF4, &ncid));
    CHECK(nc_inq_natts(ncid, &natts));
    for (i = 0; i < natts; i++) {
        CHECK(nc_inq_attname(ncid, NC_GLOBAL, i, name));
        REQUIRE(strncmp(name, "_Is", 3) && strcmp(name, "_NCProperties") &&
                strcmp(name, "_SuperblockVersion") && strcmp(name, "_nc3_strict"));
    }
    CHECK(nc_close(ncid));
    return check_file(path, mode);
}

/* An upstream-created file keeps its own stamp byte-for-byte after a
 * NEXTCDF-4 write update. */
static int
test_upstream_preservation(void)
{
    int ncid, dim, var;
    char *before, *after;
    const char *path = "prov_upstream.nc";

    CHECK(nc_create(path, NC_NETCDF4, &ncid));
    CHECK(nc_def_dim(ncid, "x", 2, &dim));
    CHECK(nc_def_var(ncid, "v", NC_INT, 1, &dim, &var));
    CHECK(nc_close(ncid));
    REQUIRE((before = h5_ncprops(path)) != NULL);
    REQUIRE(strstr(before, "version=2,netcdf=") != NULL);

    CHECK(nc_open(path, NC_NEXTCDF4 | NC_WRITE, &ncid));
    {
        /* The upstream stamp is readable through NEXTCDF-4. */
        char buf[256];
        int iv;
        CHECK(nc_get_att_text(ncid, NC_GLOBAL, "_NCProperties", buf));
        REQUIRE(!strcmp(buf, before));
        CHECK(nc_get_att_int(ncid, NC_GLOBAL, "_IsNetcdf4", &iv));
        REQUIRE(iv == 1);
        CHECK(nc_put_att_text(ncid, NC_GLOBAL, "touched", 4, "yes!"));
        CHECK(nc_close(ncid));
    }
    REQUIRE((after = h5_ncprops(path)) != NULL);
    REQUIRE(!strcmp(before, after));
    free(before);
    free(after);
    return 0;
}

/* A file lacking _NCProperties that NEXTCDF-4 did not create is left
 * unstamped after a write update (upstream parity). */
static int
test_foreign_unstamped(void)
{
    hid_t fid;
    int ncid, dim, var;
    const char *path = "prov_foreign.nc";

    CHECK(nc_create(path, NC_NETCDF4, &ncid));
    CHECK(nc_def_dim(ncid, "x", 2, &dim));
    CHECK(nc_def_var(ncid, "v", NC_INT, 1, &dim, &var));
    CHECK(nc_close(ncid));
    /* Strip the upstream stamp so the file carries no _NCProperties. */
    REQUIRE((fid = H5Fopen(path, H5F_ACC_RDWR, H5P_DEFAULT)) >= 0);
    REQUIRE(H5Adelete(fid, "_NCProperties") >= 0);
    REQUIRE(H5Fclose(fid) >= 0);
    CHECK(nc_open(path, NC_NEXTCDF4 | NC_WRITE, &ncid));
    {
        int iv;
        REQUIRE(nc_inq_att(ncid, NC_GLOBAL, "_NCProperties", NULL, NULL) == NC_ENOTATT);
        /* The dimscale marker walk still identifies it as netCDF-4,
         * matching upstream's NC4_isnetcdf4. */
        CHECK(nc_get_att_int(ncid, NC_GLOBAL, "_IsNetcdf4", &iv));
        REQUIRE(iv == 1);
        CHECK(nc_put_att_text(ncid, NC_GLOBAL, "touched", 4, "yes!"));
        CHECK(nc_close(ncid));
    }
    REQUIRE((fid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT)) >= 0);
    REQUIRE(H5Aexists(fid, "_NCProperties") <= 0);
    REQUIRE(H5Fclose(fid) >= 0);
    return 0;
}

/* nc_sync on a created file writes the stamp immediately. */
static int
test_sync_stamp(void)
{
    int ncid;
    char *props;
    const char *path = "prov_sync.nc";

    CHECK(nc_create(path, NC_NEXTCDF4, &ncid));
    CHECK(nc_sync(ncid));
    /* The stamp must exist on the HDF5 object, not just in memory. */
    {
        NEXTCDF4_FILE_INFO_T *file;
        CHECK(NEXTCDF4_get_file(ncid, NULL, &file));
        REQUIRE(H5Aexists(file->rootid, "_NCProperties") > 0);
    }
    CHECK(nc_close(ncid));
    REQUIRE((props = h5_ncprops(path)) != NULL);
    REQUIRE(strstr(props, ",nextcdf4=") != NULL);
    free(props);
    return 0;
}

int
main(void)
{
    if (!NC_NEXTCDF4_initialize())
        return 1;
    if (run_mode(0) || run_mode(NC_NETCDF4_MODEL) ||
        run_mode(NC_CLASSIC_MODEL) || test_upstream_preservation() ||
        test_foreign_unstamped() || test_sync_stamp())
        return 1;
    return 0;
}

/**
 * @file tst_nextcdf4_ref.c
 * @brief Tests for reference types in NEXTCDF-4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include <hdf5.h>
#include "nep.h"

#define FILE_NAME "tst_nextcdf4_ref.h5"

static int
test_object_ref(void)
{
    int ncid, dimid, varid, targetid;
    size_t start[1] = {0};
    size_t count[1] = {1};
    nc_type xtype_in;
    hobj_ref_t ref;
    hobj_ref_t ref_in = {0};
    hid_t file = -1;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "target", NC_INT, 1, &dimid, &targetid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "refs", NC_REF_OBJECT, 1, &dimid, &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_enddef(ncid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    file = H5Fopen(FILE_NAME, H5F_ACC_RDWR, H5P_DEFAULT);
    if (file < 0) {
        fprintf(stderr, "H5Fopen failed\n");
        return 1;
    }
    if (H5Rcreate(&ref, file, "target", H5R_OBJECT, H5S_ALL) < 0) {
        H5Fclose(file);
        return 1;
    }
    H5Fclose(file);

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "refs", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_put_vara(ncid, varid, start, count, &ref) != NC_NOERR) {
        fprintf(stderr, "nc_put_vara object ref failed\n");
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "refs", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_inq_var(ncid, varid, NULL, &xtype_in, NULL, NULL, NULL) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (xtype_in != NC_REF_OBJECT) {
        fprintf(stderr, "type mismatch for object ref: got %d\n", xtype_in);
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start, count, &ref_in) != NC_NOERR) {
        fprintf(stderr, "nc_get_vara object ref failed\n");
        nc_close(ncid);
        return 1;
    }
    if (memcmp(&ref, &ref_in, sizeof(hobj_ref_t)) != 0) {
        fprintf(stderr, "object ref round-trip mismatch\n");
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

static int
test_region_ref(void)
{
    int ncid, dimid, varid, targetid;
    size_t start[1] = {0};
    size_t count[1] = {1};
    nc_type xtype_in;
    hdset_reg_ref_t ref;
    hdset_reg_ref_t ref_in;
    hid_t file = -1;
    hid_t target = -1;
    hid_t space = -1;
    hsize_t hstart[1] = {1};
    hsize_t hcount[1] = {2};
    memset(&ref, 0, sizeof(ref));
    memset(&ref_in, 0, sizeof(ref_in));

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "target", NC_INT, 1, &dimid, &targetid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "regrefs", NC_REF_REGION, 1, &dimid, &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_enddef(ncid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    file = H5Fopen(FILE_NAME, H5F_ACC_RDWR, H5P_DEFAULT);
    if (file < 0)
        return 1;
    target = H5Dopen2(file, "target", H5P_DEFAULT);
    if (target < 0) {
        H5Fclose(file);
        return 1;
    }
    space = H5Dget_space(target);
    if (space < 0 ||
        H5Sselect_hyperslab(space, H5S_SELECT_SET, hstart, NULL, hcount, NULL) < 0) {
        H5Dclose(target);
        H5Fclose(file);
        return 1;
    }
    if (H5Rcreate(&ref, file, "target", H5R_DATASET_REGION, space) < 0) {
        H5Sclose(space);
        H5Dclose(target);
        H5Fclose(file);
        return 1;
    }
    H5Sclose(space);
    H5Dclose(target);
    H5Fclose(file);

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "regrefs", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_put_vara(ncid, varid, start, count, &ref) != NC_NOERR) {
        fprintf(stderr, "nc_put_vara region ref failed\n");
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR)
        return 1;
    if (nc_inq_varid(ncid, "regrefs", &varid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_inq_var(ncid, varid, NULL, &xtype_in, NULL, NULL, NULL) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (xtype_in != NC_REF_REGION) {
        fprintf(stderr, "type mismatch for region ref: got %d\n", xtype_in);
        nc_close(ncid);
        return 1;
    }
    if (nc_get_vara(ncid, varid, start, count, &ref_in) != NC_NOERR) {
        fprintf(stderr, "nc_get_vara region ref failed\n");
        nc_close(ncid);
        return 1;
    }
    if (memcmp(&ref, &ref_in, sizeof(hdset_reg_ref_t)) != 0) {
        fprintf(stderr, "region ref round-trip mismatch\n");
        nc_close(ncid);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR)
        return 1;

    unlink(FILE_NAME);
    return 0;
}

static int
test_rejected_in_model(int mode)
{
    int ncid, dimid, varid;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER | mode, &ncid) != NC_NOERR)
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid) != NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    if (nc_def_var(ncid, "refs", NC_REF_OBJECT, 1, &dimid, &varid) == NC_NOERR) {
        nc_close(ncid);
        return 1;
    }
    nc_close(ncid);
    unlink(FILE_NAME);
    return 0;
}

int
main(void)
{
    int failed = 0;

    if (test_object_ref()) {
        fprintf(stderr, "test_object_ref failed\n");
        failed = 1;
    }
    if (test_region_ref()) {
        fprintf(stderr, "test_region_ref failed\n");
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

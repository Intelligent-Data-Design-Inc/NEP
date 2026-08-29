/**
 * @file tst_nextcdf4_meta.c
 * @brief Tests the NEXTCDF-4 Sprint 3 metadata model.
 * @author Edward Hartnett
 * @date 2026-08-28
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hdf5.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"
#include "nxt4internal.h"

/** NEXTCDF-4 test file. */
#define FILE_NAME "tst_nextcdf4_meta.h5"

/** @return Zero when the defined metadata matches expectations. */
static int
check_metadata(int ncid)
{
    int ndims, nvars, natts, unlim;
    int dimids[4];
    int varids[4];
    char name[NC_MAX_NAME + 1];
    size_t len;
    int i;
    int ret;

    if ((ret = nc_inq(ncid, &ndims, &nvars, &natts, &unlim)) != NC_NOERR) {
        fprintf(stderr, "nc_inq failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if (ndims != 3 || nvars != 4 || natts != 4 || unlim != 0) {
        fprintf(stderr, "nc_inq mismatch: ndims=%d nvars=%d natts=%d unlim=%d\n",
                ndims, nvars, natts, unlim);
        return 1;
    }

    if ((ret = nc_inq_dimids(ncid, NULL, dimids, 0)) != NC_NOERR) {
        fprintf(stderr, "nc_inq_dimids failed: %s\n", nc_strerror(ret));
        return 1;
    }
    for (i = 0; i < 3; i++) {
        if ((ret = nc_inq_dim(ncid, dimids[i], name, &len)) != NC_NOERR) {
            fprintf(stderr, "nc_inq_dim failed: %s\n", nc_strerror(ret));
            return 1;
        }
        if (i == 0 && (strcmp(name, "time") || len != 0)) {
            fprintf(stderr, "time dim mismatch: %s/%zu\n", name, len);
            return 1;
        }
        if (i == 1 && (strcmp(name, "x") || len != 10)) {
            fprintf(stderr, "x dim mismatch: %s/%zu\n", name, len);
            return 1;
        }
        if (i == 2 && (strcmp(name, "y") || len != 20)) {
            fprintf(stderr, "y dim mismatch: %s/%zu\n", name, len);
            return 1;
        }
    }

    if ((ret = nc_inq_varids(ncid, NULL, varids)) != NC_NOERR) {
        fprintf(stderr, "nc_inq_varids failed: %s\n", nc_strerror(ret));
        return 1;
    }
    for (i = 0; i < 4; i++) {
        nc_type xtype;
        int ndimsv;
        int vdims[NC_MAX_VAR_DIMS];
        int nattv;
        if ((ret = nc_inq_var(ncid, varids[i], name, &xtype, &ndimsv,
                              vdims, &nattv)) != NC_NOERR) {
            fprintf(stderr, "nc_inq_var failed: %s\n", nc_strerror(ret));
            return 1;
        }
        if (i == 0 && (strcmp(name, "scalar_int") || xtype != NC_INT ||
                       ndimsv != 0 || nattv != 1)) {
            fprintf(stderr, "scalar_int mismatch\n");
            return 1;
        }
        if (i == 1 && (strcmp(name, "temp") || xtype != NC_FLOAT ||
                       ndimsv != 3 || nattv != 1)) {
            fprintf(stderr, "temp mismatch\n");
            return 1;
        }
        if (i == 2 && (strcmp(name, "ubyte_2d") || xtype != NC_UBYTE ||
                       ndimsv != 2 || nattv != 0)) {
            fprintf(stderr, "ubyte_2d mismatch\n");
            return 1;
        }
        if (i == 3 && (strcmp(name, "int64_1d") || xtype != NC_INT64 ||
                       ndimsv != 1 || nattv != 0)) {
            fprintf(stderr, "int64_1d mismatch\n");
            return 1;
        }
    }

    {
        int iatt[3];
        double datt[2];
        char txt[NC_MAX_NAME + 1];
        short satt;
        size_t alen;

        if ((ret = nc_inq_att(ncid, NC_GLOBAL, "global_int", NULL, &alen)) != NC_NOERR ||
            alen != 3) {
            fprintf(stderr, "global_int att query failed\n");
            return 1;
        }
        if ((ret = nc_get_att_int(ncid, NC_GLOBAL, "global_int", iatt)) != NC_NOERR ||
            iatt[0] != 1 || iatt[1] != 2 || iatt[2] != 3) {
            fprintf(stderr, "global_int att mismatch\n");
            return 1;
        }
        if ((ret = nc_get_att_double(ncid, NC_GLOBAL, "global_double", datt)) != NC_NOERR ||
            datt[0] != 1.5 || datt[1] != 2.5) {
            fprintf(stderr, "global_double att mismatch\n");
            return 1;
        }
        if ((ret = nc_get_att_text(ncid, NC_GLOBAL, "global_char", txt)) != NC_NOERR ||
            strncmp(txt, "abc", 3)) {
            fprintf(stderr, "global_char att mismatch\n");
            return 1;
        }
        if ((ret = nc_get_att_short(ncid, NC_GLOBAL, "empty_short", &satt)) != NC_NOERR) {
            fprintf(stderr, "empty_short att get failed\n");
            return 1;
        }
        if ((ret = nc_inq_attlen(ncid, NC_GLOBAL, "empty_short", &alen)) != NC_NOERR ||
            alen != 0) {
            fprintf(stderr, "empty_short att length mismatch\n");
            return 1;
        }
    }

    return 0;
}

/** @return Zero when hidden HDF5 structure matches expectations. */
static int
check_hdf5_structure(void)
{
    hid_t file = -1;
    hid_t dset = -1;
    hid_t attr = -1;
    hid_t space = -1;
    hid_t type = -1;
    int dimid;
    hsize_t dims[3];
    int rank;
    int failed = 1;

    if ((file = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        goto done;

    if ((dset = H5Dopen2(file, "time", H5P_DEFAULT)) < 0)
        goto done;
    if (H5Aexists(dset, NEXTCDF4_DIMID_ATT) <= 0)
        goto done;
    if ((attr = H5Aopen(dset, NEXTCDF4_DIMID_ATT, H5P_DEFAULT)) < 0)
        goto done;
    if (H5Aread(attr, H5T_NATIVE_INT, &dimid) < 0 || dimid != 0)
        goto done;
    H5Aclose(attr);
    attr = -1;
    H5Dclose(dset);
    dset = -1;

    if ((dset = H5Dopen2(file, "temp", H5P_DEFAULT)) < 0)
        goto done;
    if ((space = H5Dget_space(dset)) < 0)
        goto done;
    if ((rank = H5Sget_simple_extent_ndims(space)) != 3)
        goto done;
    if (H5Sget_simple_extent_dims(space, dims, NULL) != 3)
        goto done;
    if (dims[0] != 0 || dims[1] != 10 || dims[2] != 20)
        goto done;
    if ((type = H5Dget_type(dset)) < 0)
        goto done;
    if (!H5Tequal(type, H5T_IEEE_F32LE) && !H5Tequal(type, H5T_IEEE_F32BE))
        goto done;
    failed = 0;

done:
    if (type >= 0)
        H5Tclose(type);
    if (space >= 0)
        H5Sclose(space);
    if (attr >= 0)
        H5Aclose(attr);
    if (dset >= 0)
        H5Dclose(dset);
    if (file >= 0)
        H5Fclose(file);
    return failed;
}

/** @return Zero when all Sprint 3 metadata checks pass. */
int
main(void)
{
    int ncid;
    int dimids[3];
    int varid;
    int ret;
    int global_i[3] = {1, 2, 3};
    double global_d[2] = {1.5, 2.5};
    short empty_s[1]; /* zero-length uses len 0 */

    unlink(FILE_NAME);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "create failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = nc_def_dim(ncid, "time", NC_UNLIMITED, &dimids[0])) != NC_NOERR) {
        fprintf(stderr, "def_dim time failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_dim(ncid, "x", 10, &dimids[1])) != NC_NOERR ||
        (ret = nc_def_dim(ncid, "y", 20, &dimids[2])) != NC_NOERR) {
        fprintf(stderr, "def_dim fixed failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = nc_def_var(ncid, "scalar_int", NC_INT, 0, NULL, &varid)) != NC_NOERR) {
        fprintf(stderr, "def_var scalar_int failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_att_int(ncid, varid, "att_one", NC_INT, 1, &global_i[0])) != NC_NOERR) {
        fprintf(stderr, "put_att scalar var failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_var(ncid, "temp", NC_FLOAT, 3, dimids, &varid)) != NC_NOERR) {
        fprintf(stderr, "def_var temp failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_att_float(ncid, varid, "att_fill", NC_FLOAT, 1,
                                (const float[]){1.0e20f})) != NC_NOERR) {
        fprintf(stderr, "put_att temp failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_var(ncid, "ubyte_2d", NC_UBYTE, 2, &dimids[1], &varid)) != NC_NOERR) {
        fprintf(stderr, "def_var ubyte_2d failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_def_var(ncid, "int64_1d", NC_INT64, 1, &dimids[0], &varid)) != NC_NOERR) {
        fprintf(stderr, "def_var int64_1d failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = nc_put_att_int(ncid, NC_GLOBAL, "global_int", NC_INT, 3, global_i)) != NC_NOERR) {
        fprintf(stderr, "put_att global_int failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_att_double(ncid, NC_GLOBAL, "global_double", NC_DOUBLE, 2, global_d)) != NC_NOERR) {
        fprintf(stderr, "put_att global_double failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_att_text(ncid, NC_GLOBAL, "global_char", 3, "abc")) != NC_NOERR) {
        fprintf(stderr, "put_att global_char failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if ((ret = nc_put_att_short(ncid, NC_GLOBAL, "empty_short", NC_SHORT, 0, empty_s)) != NC_NOERR) {
        fprintf(stderr, "put_att empty_short failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if ((ret = nc_enddef(ncid)) != NC_NOERR) {
        fprintf(stderr, "nc_enddef failed: %s\n", nc_strerror(ret));
        return 1;
    }

    if (check_metadata(ncid) || nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "initial metadata round-trip failed\n");
        return 1;
    }

    if (check_hdf5_structure()) {
        fprintf(stderr, "HDF5 structure check failed\n");
        return 1;
    }

    ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid);
    if (ret != NC_NOERR || check_metadata(ncid) || nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "reopen metadata round-trip failed: %s\n", nc_strerror(ret));
        return 1;
    }

    /* Verify write errors in read-only mode. */
    ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid);
    if (ret != NC_NOERR ||
        nc_def_dim(ncid, "bad", 1, &varid) != NC_EPERM ||
        nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "read-only write did not return expected error\n");
        return 1;
    }

    /* Verify enddef/redef transitions. */
    ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "write open for redef failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if (nc_enddef(ncid) != NC_EINVAL) {
        fprintf(stderr, "enddef outside define did not return NC_EINVAL\n");
        return 1;
    }
    if (nc_redef(ncid) != NC_NOERR) {
        fprintf(stderr, "nc_redef failed\n");
        return 1;
    }
    if (nc_put_att_int(ncid, NC_GLOBAL, "extra", NC_INT, 1, &global_i[0]) != NC_NOERR) {
        fprintf(stderr, "put_att in redef failed\n");
        return 1;
    }
    if (nc_enddef(ncid) != NC_NOERR) {
        fprintf(stderr, "enddef after redef failed\n");
        return 1;
    }
    nc_close(ncid);

    unlink(FILE_NAME);
    printf("OK: NEXTCDF-4 Sprint 3 metadata\n");
    return 0;
}

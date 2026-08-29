/**
 * @file tst_nextcdf4_file.c
 * @brief Tests the empty-file NEXTCDF-4 create, open, and close lifecycle.
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

/** NEXTCDF-4 file created by the lifecycle test. */
#define FILE_NAME "tst_nextcdf4_file.h5"
/** Unmarked HDF5 file used for rejection testing. */
#define OTHER_FILE "tst_nextcdf4_other.h5"
/** Plain-text file used for rejection testing. */
#define TEXT_FILE "tst_nextcdf4_text.txt"
/** Nonexistent path used for missing-file testing. */
#define MISSING_FILE "tst_nextcdf4_missing.h5"

/**
 * Verify the NetCDF view of an empty NEXTCDF-4 file.
 * @param ncid Open NetCDF file identifier.
 * @return Zero on success, or one when an inquiry differs from expectations.
 */
static int
check_empty(int ncid)
{
    int ndims = -1;
    int nvars = -1;
    int natts = -1;
    int unlim = -2;
    int format = 0;
    int formatx = 0;
    int mode = 0;

    if (nc_inq(ncid, &ndims, &nvars, &natts, &unlim) != NC_NOERR ||
        ndims != 0 || nvars != 0 || natts != 0 || unlim != -1)
        return 1;
    if (nc_inq_format(ncid, &format) != NC_NOERR || format != NC_FORMAT_NETCDF4)
        return 1;
    if (nc_inq_format_extended(ncid, &formatx, &mode) != NC_NOERR ||
        formatx != NC_FORMATX_UDF9 || !(mode & NC_NEXTCDF4))
        return 1;
    return 0;
}

/**
 * Verify the hidden HDF5 attributes written by NEXTCDF-4.
 * @param path Path to the HDF5 file.
 * @param model Nonzero when `_Nextcdf4Model` must be present.
 * @return Zero on success, or one when marker validation fails.
 */
static int
check_markers(const char *path, int model)
{
    hid_t file = -1;
    hid_t attr = -1;
    hid_t type = -1;
    char value[32] = {0};
    int model_value = 0;
    int failed = 1;

    if ((file = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        goto done;
    if ((attr = H5Aopen(file, "_Nextcdf4Backend", H5P_DEFAULT)) < 0)
        goto done;
    if ((type = H5Aget_type(attr)) < 0 || H5Aread(attr, type, value) < 0)
        goto done;
    if (strcmp(value, "NEXTCDF-4/1.0"))
        goto done;
    H5Tclose(type);
    type = -1;
    H5Aclose(attr);
    attr = -1;

    if (model) {
        if ((attr = H5Aopen(file, "_Nextcdf4Model", H5P_DEFAULT)) < 0 ||
            H5Aread(attr, H5T_NATIVE_INT, &model_value) < 0 || model_value != 1)
            goto done;
    } else if (H5Aexists(file, "_Nextcdf4Model") != 0) {
        goto done;
    }
    failed = 0;

done:
    if (type >= 0)
        H5Tclose(type);
    if (attr >= 0)
        H5Aclose(attr);
    if (file >= 0)
        H5Fclose(file);
    return failed;
}

/** @return Zero when all NEXTCDF-4 file lifecycle checks pass. */
int
main(void)
{
    FILE *text = NULL;
    hid_t other = -1;
    int closed_ncid;
    int ncid;
    int ret;

    unlink(FILE_NAME);
    unlink(OTHER_FILE);
    unlink(TEXT_FILE);
    unlink(MISSING_FILE);
    if (!NC_NEXTCDF4_initialize())
        return 1;

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid);
    closed_ncid = ncid;
    if (ret != NC_NOERR || check_empty(ncid) || nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "native create/close failed: %s\n", nc_strerror(ret));
        return 1;
    }
    if (nc_inq(closed_ncid, NULL, NULL, NULL, NULL) != NC_EBADID) {
        fprintf(stderr, "closed ncid remained valid\n");
        return 1;
    }
    if (check_markers(FILE_NAME, 0)) {
        fprintf(stderr, "native file markers are invalid\n");
        return 1;
    }

    ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid);
    if (ret != NC_NOERR || check_empty(ncid) || nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "read-only reopen failed: %s\n", nc_strerror(ret));
        return 1;
    }
    ret = nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid);
    if (ret != NC_NOERR || nc_sync(ncid) != NC_NOERR || nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "read/write reopen failed: %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_NOCLOBBER, &ncid);
    if (ret != NC_EEXIST) {
        fprintf(stderr, "NC_NOCLOBBER returned %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLASSIC_MODEL | NC_CLOBBER, &ncid);
    if (ret != NC_NOERR || nc_close(ncid) != NC_NOERR || check_markers(FILE_NAME, 0)) {
        fprintf(stderr, "classic-model create failed: %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_NETCDF4_MODEL | NC_CLOBBER, &ncid);
    if (ret != NC_NOERR || nc_close(ncid) != NC_NOERR || check_markers(FILE_NAME, 1)) {
        fprintf(stderr, "compatibility create failed: %s\n", nc_strerror(ret));
        return 1;
    }
    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_NETCDF4_MODEL | NC_CLASSIC_MODEL,
                    &ncid);
    if (ret != NC_EINVAL) {
        fprintf(stderr, "incompatible models returned %s\n", nc_strerror(ret));
        return 1;
    }

    ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid);
    if (ret != NC_NOERR || nc_abort(ncid) != NC_NOERR ||
        nc_open(FILE_NAME, NC_NEXTCDF4, &ncid) != NC_NOERR ||
        nc_close(ncid) != NC_NOERR) {
        fprintf(stderr, "abort lifecycle failed\n");
        return 1;
    }

    other = H5Fcreate(OTHER_FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (other < 0 || H5Fclose(other) < 0)
        return 1;
    ret = nc_open(OTHER_FILE, NC_NEXTCDF4, &ncid);
    if (ret == NC_NOERR) {
        nc_close(ncid);
        fprintf(stderr, "unmarked HDF5 file was accepted\n");
        return 1;
    }

    text = fopen(TEXT_FILE, "w");
    if (!text || fputs("not HDF5\n", text) < 0 || fclose(text) != 0)
        return 1;
    text = NULL;
    if (nc_open(TEXT_FILE, NC_NEXTCDF4, &ncid) == NC_NOERR) {
        nc_close(ncid);
        fprintf(stderr, "text input was accepted\n");
        return 1;
    }
    if (nc_open(MISSING_FILE, NC_NEXTCDF4, &ncid) == NC_NOERR) {
        nc_close(ncid);
        fprintf(stderr, "missing input was accepted\n");
        return 1;
    }

    unlink(FILE_NAME);
    unlink(OTHER_FILE);
    unlink(TEXT_FILE);
    printf("OK: NEXTCDF-4 empty-file lifecycle\n");
    return 0;
}

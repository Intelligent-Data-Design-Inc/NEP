/**
 * @file tst_nextcdf4_filter.c
 * @brief Tests HDF5 filter pipeline support for NEXTCDF-4 Sprint 1K.
 * @author Edward Hartnett
 * @date 2026-11-20
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include <netcdf_filter.h>
#include <hdf5.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

/** NEXTCDF-4 file used for the filter tests. */
#define FILE_NAME "tst_nextcdf4_filter.h5"

/* Filter ids not declared in hdf5.h live in netcdf_filter.h. */
#ifndef H5Z_FILTER_BZIP2
#define H5Z_FILTER_BZIP2 307
#endif
#ifndef H5Z_FILTER_ZSTD
#define H5Z_FILTER_ZSTD 32015
#endif

/* An HDF5 filter id outside the registered plugin range (512-32767),
 * so no installed plugin should provide it. */
#define MISSING_FILTER_ID 33000

/** @return Zero when deflate+shuffle round-trip correctly. */
static int
test_deflate(void)
{
    int ncid, dimid[2], varid;
    int data[8][8], data_in[8][8];
    int i, j;
    int shuffle_in, deflate_in, level_in;
    size_t nfilters;
    unsigned int ids[4], params[4];
    size_t nparams;
    int storage;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid[0]))
        return 1;
    if (nc_def_dim(ncid, "y", 8, &dimid[1]))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 2, dimid, &varid))
        return 1;
    if (nc_def_var_deflate(ncid, varid, 1, 1, 4))
        return 1;
    /* Defining a filter forces chunked storage. */
    if (nc_inq_var_chunking(ncid, varid, &storage, NULL))
        return 1;
    if (storage != NC_CHUNKED)
        return 1;
    /* Filter ids come back in pipeline order: shuffle, then deflate. */
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 2 || ids[0] != H5Z_FILTER_SHUFFLE ||
        ids[1] != H5Z_FILTER_DEFLATE)
        return 1;
    /* nc_inq_var_filter reports the first filter. */
    if (nc_inq_var_filter(ncid, varid, ids, &nparams, params))
        return 1;
    if (ids[0] != H5Z_FILTER_SHUFFLE || nparams != 0)
        return 1;
    /* nc_inq_var_filter_info returns deflate's level parameter. */
    if (nc_inq_var_filter_info(ncid, varid, H5Z_FILTER_DEFLATE,
                               &nparams, params))
        return 1;
    if (nparams != 1 || params[0] != 4)
        return 1;
    if (nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &level_in))
        return 1;
    if (shuffle_in != 1 || deflate_in != 1 || level_in != 4)
        return 1;
    if (nc_enddef(ncid))
        return 1;

    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            data[i][j] = i * 8 + j;
    if (nc_put_var_int(ncid, varid, &data[0][0]))
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Reopen: the pipeline survives, the data comes back. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 2 || ids[0] != H5Z_FILTER_SHUFFLE ||
        ids[1] != H5Z_FILTER_DEFLATE)
        return 1;
    if (nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &level_in))
        return 1;
    if (shuffle_in != 1 || deflate_in != 1 || level_in != 4)
        return 1;
    if (nc_get_var_int(ncid, varid, &data_in[0][0]))
        return 1;
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            if (data_in[i][j] != data[i][j])
                return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when fletcher32 lands first in the pipeline. */
static int
test_filter_order(void)
{
    int ncid, dimid, varid;
    size_t nfilters;
    unsigned int ids[4], params[4];
    size_t nparams;
    unsigned int level = 3;
    int f32;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    /* Define deflate first, then fletcher32 — fletcher32 must still
     * be reported first in the pipeline. */
    if (nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 1, &level))
        return 1;
    if (nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 2 || ids[0] != H5Z_FILTER_FLETCHER32 ||
        ids[1] != H5Z_FILTER_DEFLATE)
        return 1;
    if (nc_inq_var_filter(ncid, varid, ids, &nparams, params))
        return 1;
    if (ids[0] != H5Z_FILTER_FLETCHER32 || nparams != 0)
        return 1;
    if (nc_inq_var_fletcher32(ncid, varid, &f32))
        return 1;
    if (f32 != NC_FLETCHER32)
        return 1;
    /* Re-defining the deflate filter replaces its parameters in
     * place instead of adding a second entry. */
    level = 7;
    if (nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 1, &level))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 2 || ids[1] != H5Z_FILTER_DEFLATE)
        return 1;
    if (nc_inq_var_filter_info(ncid, varid, H5Z_FILTER_DEFLATE,
                               &nparams, params))
        return 1;
    if (nparams != 1 || params[0] != 7)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when szip round-trips or the filter is unavailable. */
static int
test_szip(void)
{
    int ncid, dimid, varid;
    int data[16], data_in[16];
    int i;
    size_t nfilters;
    unsigned int ids[4];
    int mask_in, ppb_in;
    int err;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 16, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_inq_filter_avail(ncid, H5Z_FILTER_SZIP)) {
        printf("szip filter not available; skipping szip test\n");
        return nc_close(ncid) ? 1 : 0;
    }
    /* Deflate and szip are mutually exclusive. */
    {
        unsigned int level = 1;
        if (nc_def_var_deflate(ncid, varid, 0, 1, 1))
            return 1;
        if ((err = nc_def_var_szip(ncid, varid, NC_SZIP_NN, 8)) != NC_EINVAL)
            return 1;
        /* Bad parameter counts and odd pixels-per-block are rejected. */
        if ((err = nc_def_var_szip(ncid, varid, NC_SZIP_NN, 7)) != NC_EINVAL)
            return 1;
        /* Remove deflate's claim by redefining with filter API? No —
         * filters cannot be removed. Use a second variable. */
        (void)level;
    }
    if (nc_def_var(ncid, "zdata", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_szip(ncid, varid, NC_SZIP_NN, 8))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 1 || ids[0] != H5Z_FILTER_SZIP)
        return 1;
    if (nc_inq_var_szip(ncid, varid, &mask_in, &ppb_in))
        return 1;
    if (ppb_in != 8)
        return 1;
    if (nc_enddef(ncid))
        return 1;
    for (i = 0; i < 16; i++)
        data[i] = i;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 1 || ids[0] != H5Z_FILTER_SZIP)
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 16; i++)
        if (data_in[i] != data[i])
            return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a plugin filter round-trips or is unavailable. */
static int
test_plugin_filter(unsigned int id, const char *name)
{
    int ncid, dimid, varid;
    int data[16], data_in[16];
    int i;
    size_t nfilters;
    unsigned int ids[4], params[4];
    size_t nparams;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 16, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_inq_filter_avail(ncid, id)) {
        printf("%s filter not available; skipping plugin test\n", name);
        return nc_close(ncid) ? 1 : 0;
    }
    if (id == H5Z_FILTER_ZSTD) {
        unsigned int level = 3;
        if (nc_def_var_filter(ncid, varid, id, 1, &level))
            return 1;
    } else {
        if (nc_def_var_filter(ncid, varid, id, 0, NULL))
            return 1;
    }
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 1 || ids[0] != id)
        return 1;
    if (nc_inq_var_filter_info(ncid, varid, id, &nparams, params))
        return 1;
    if (id == H5Z_FILTER_ZSTD && (nparams != 1 || params[0] != 3))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    for (i = 0; i < 16; i++)
        data[i] = i * 3;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 1 || ids[0] != id)
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 16; i++)
        if (data_in[i] != data[i])
            return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when all the error paths return the right codes. */
static int
test_filter_errors(void)
{
    int ncid, dimid, varid, scalarid;
    int err;
    unsigned int level = 1;
    unsigned int fid;

    unlink(FILE_NAME);
    /* Every filter and storage setter is illegal under
     * NC_CLASSIC_MODEL. */
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLASSIC_MODEL | NC_CLOBBER,
                  &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if ((err = nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 1,
                                 &level)) != NC_EINVAL)
        return 1;
    if ((err = nc_def_var_deflate(ncid, varid, 0, 1, 1)) != NC_EINVAL)
        return 1;
    if ((err = nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32)) != NC_EINVAL)
        return 1;
    if ((err = nc_def_var_chunking(ncid, varid, NC_CHUNKED, NULL)) != NC_EINVAL)
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var(ncid, "scalar", NC_INT, 0, NULL, &scalarid))
        return 1;

    /* Scalars cannot have filters. */
    if ((err = nc_def_var_filter(ncid, scalarid, H5Z_FILTER_DEFLATE, 1,
                                 &level)) != NC_EINVAL)
        return 1;
    if ((err = nc_def_var_deflate(ncid, scalarid, 0, 1, 1)) != NC_EINVAL)
        return 1;
    /* Bad parameter handling. */
    if ((err = nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 1,
                                 NULL)) != NC_EINVAL)
        return 1;
    if ((err = nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 2,
                                 &level)) != NC_EFILTER)
        return 1;
    /* Unknown filter ids fail availability. */
    if (nc_inq_filter_avail(ncid, MISSING_FILTER_ID) != NC_ENOFILTER)
        return 1;
    if ((err = nc_def_var_filter(ncid, varid, MISSING_FILTER_ID, 0,
                                 NULL)) != NC_ENOFILTER)
        return 1;
    /* Deflate availability is always there. */
    if (nc_inq_filter_avail(ncid, H5Z_FILTER_DEFLATE))
        return 1;
    /* Deflate level out of range. */
    if ((err = nc_def_var_deflate(ncid, varid, 0, 1, 10)) != NC_EINVAL)
        return 1;
    /* Once a filter exists the variable cannot go contiguous. */
    if (nc_def_var_filter(ncid, varid, H5Z_FILTER_DEFLATE, 1, &level))
        return 1;
    if ((err = nc_def_var_chunking(ncid, varid, NC_CONTIGUOUS,
                                   NULL)) != NC_EINVAL)
        return 1;
    /* A variable with no filters reports an empty list. */
    {
        size_t nfilters = 99;
        if (nc_inq_var_filter_ids(ncid, scalarid, &nfilters, NULL))
            return 1;
        if (nfilters != 0)
            return 1;
        fid = 999;
        if (nc_inq_var_filter(ncid, scalarid, &fid, NULL, NULL))
            return 1;
        if (fid != 0)
            return 1;
    }
    /* Filter definitions outside define mode fail. */
    if (nc_enddef(ncid))
        return 1;
    if ((err = nc_def_var_filter(ncid, varid, H5Z_FILTER_SHUFFLE, 0,
                                 NULL)) != NC_ENOTINDEFINE)
        return 1;
    if (nc_close(ncid))
        return 1;

    /* On reopen + redef, the variable is committed: NC_ELATEDEF. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid))
        return 1;
    if (nc_redef(ncid))
        return 1;
    if ((err = nc_def_var_filter(ncid, varid, H5Z_FILTER_SHUFFLE, 0,
                                 NULL)) != NC_ELATEDEF)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a variable filtered by a missing plugin keeps
 * its metadata but rejects data I/O with NC_ENOFILTER. */
static int
test_missing_filter(void)
{
    int ncid, dimid, varid;
    int data[8], data_in[8];
    int i, err;
    size_t nfilters;
    unsigned int ids[4];

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    /* bzip2 is a plugin filter; without it this test cannot build a
     * file that loses its filter at open. */
    if (nc_inq_filter_avail(ncid, H5Z_FILTER_BZIP2)) {
        printf("bzip2 filter not available; skipping missing-filter test\n");
        return nc_close(ncid) ? 1 : 0;
    }
    if (nc_def_var_filter(ncid, varid, H5Z_FILTER_BZIP2, 0, NULL))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    for (i = 0; i < 8; i++)
        data[i] = i;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Hide the plugin: unregister it and disable plugin loading so
     * HDF5 cannot pull it back in from the plugin path. */
    if (H5Zunregister(H5Z_FILTER_BZIP2) < 0)
        return 1;
    if (H5PLset_loading_state(0) < 0)
        return 1;

    /* Metadata — including the filter pipeline — stays readable. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 1 || ids[0] != H5Z_FILTER_BZIP2)
        return 1;
    /* Data I/O reports the missing filter. */
    if ((err = nc_get_var_int(ncid, varid, data_in)) != NC_ENOFILTER)
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Re-enable plugin loading for any later tests in the process. */
    if (H5PLset_loading_state(H5PL_ALL_PLUGIN) < 0)
        return 1;
    return 0;
}

/** @return Zero when a filtered coordinate variable keeps working
 * for dependent variables defined after the filter. */
static int
test_coord_var(void)
{
    int ncid, dimid, varid, dataid;
    int xdata[8], xdata_in[8], data_in[8];
    int i;
    int storage, dimids_in[1];
    size_t nfilters;
    unsigned int ids[4];

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 8, &dimid))
        return 1;
    if (nc_def_var(ncid, "x", NC_INT, 1, &dimid, &varid))
        return 1;
    /* Filters on a coordinate variable must be set before dependent
     * variables attach its dimension scale. */
    if (nc_def_var_deflate(ncid, varid, 0, 1, 3))
        return 1;
    if (nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32))
        return 1;
    /* The dependent variable is defined after the filters. */
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &dataid))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    for (i = 0; i < 8; i++)
        xdata[i] = i * 10;
    if (nc_put_var_int(ncid, varid, xdata))
        return 1;
    if (nc_put_var_int(ncid, dataid, xdata))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    /* The coordinate variable is chunked and carries its filters. */
    if (nc_inq_var_chunking(ncid, varid, &storage, NULL))
        return 1;
    if (storage != NC_CHUNKED)
        return 1;
    if (nc_inq_var_filter_ids(ncid, varid, &nfilters, ids))
        return 1;
    if (nfilters != 2 || ids[0] != H5Z_FILTER_FLETCHER32 ||
        ids[1] != H5Z_FILTER_DEFLATE)
        return 1;
    /* The dependent variable's dimension still resolves to x. */
    if (nc_inq_vardimid(ncid, dataid, dimids_in))
        return 1;
    if (dimids_in[0] != dimid)
        return 1;
    if (nc_get_var_int(ncid, varid, xdata_in))
        return 1;
    if (nc_get_var_int(ncid, dataid, data_in))
        return 1;
    for (i = 0; i < 8; i++)
        if (xdata_in[i] != xdata[i] || data_in[i] != xdata[i])
            return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

int
main(void)
{
    int ret = 0;
    printf("*** Testing NEXTCDF-4 filter support.\n");
    printf("    test_deflate: %s\n", test_deflate() ? (ret++, "FAIL") : "ok");
    printf("    test_filter_order: %s\n",
           test_filter_order() ? (ret++, "FAIL") : "ok");
    printf("    test_szip: %s\n", test_szip() ? (ret++, "FAIL") : "ok");
    printf("    test_bzip2: %s\n",
           test_plugin_filter(H5Z_FILTER_BZIP2, "bzip2") ? (ret++, "FAIL") : "ok");
    printf("    test_zstd: %s\n",
           test_plugin_filter(H5Z_FILTER_ZSTD, "zstd") ? (ret++, "FAIL") : "ok");
    printf("    test_filter_errors: %s\n",
           test_filter_errors() ? (ret++, "FAIL") : "ok");
    printf("    test_missing_filter: %s\n",
           test_missing_filter() ? (ret++, "FAIL") : "ok");
    printf("    test_coord_var: %s\n",
           test_coord_var() ? (ret++, "FAIL") : "ok");
    if (ret)
        printf("*** %d filter test(s) FAILED\n", ret);
    else
        printf("*** SUCCESS!\n");
    unlink(FILE_NAME);
    return ret;
}

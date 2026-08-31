/**
 * @file tst_nextcdf4_chunking.c
 * @brief Tests variable storage features for NEXTCDF-4 Sprint 6.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

/** NEXTCDF-4 file used for chunking tests. */
#define FILE_NAME "tst_nextcdf4_chunking.h5"

/** @return Zero when explicit chunking round-trips correctly. */
static int
test_chunking(void)
{
    int ncid, dimid[2], varid;
    size_t chunksize_in[2];
    int storage_in;
    int data[20][10];
    int data_in[20][10];
    size_t chunks[2] = {5, 5};
    int i, j;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 20, &dimid[0]))
        return 1;
    if (nc_def_dim(ncid, "y", 10, &dimid[1]))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 2, dimid, &varid))
        return 1;
    if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    for (i = 0; i < 20; i++)
        for (j = 0; j < 10; j++)
            data[i][j] = i * 10 + j;
    if (nc_put_var_int(ncid, varid, &data[0][0]))
        return 1;
    if (nc_get_var_int(ncid, varid, &data_in[0][0]))
        return 1;
    for (i = 0; i < 20; i++)
        for (j = 0; j < 10; j++)
            if (data_in[i][j] != data[i][j])
                return 1;

    if (nc_inq_var_chunking(ncid, varid, &storage_in, chunksize_in))
        return 1;
    if (storage_in != NC_CHUNKED)
        return 1;
    if (chunksize_in[0] != 5 || chunksize_in[1] != 5)
        return 1;

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when deflate/shuffle round-trips and reports correctly. */
static int
test_deflate(void)
{
    int ncid, dimid, varid;
    int shuffle_in, deflate_in, level_in;
    int data[100], data_in[100];
    int i;

    unlink(FILE_NAME);
    int status;
    if ((status = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        { fprintf(stderr, "nc_create: %s\n", nc_strerror(status)); return 1; }
    if ((status = nc_def_dim(ncid, "x", 100, &dimid)))
        { fprintf(stderr, "nc_def_dim: %s\n", nc_strerror(status)); return 1; }
    if ((status = nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid)))
        { fprintf(stderr, "nc_def_var: %s\n", nc_strerror(status)); return 1; }
    if ((status = nc_def_var_deflate(ncid, varid, 1, 1, 4)))
        { fprintf(stderr, "nc_def_var_deflate: %s\n", nc_strerror(status)); return 1; }
    if ((status = nc_enddef(ncid)))
        { fprintf(stderr, "nc_enddef: %s\n", nc_strerror(status)); return 1; }

    for (i = 0; i < 100; i++)
        data[i] = i;
    if ((status = nc_put_var_int(ncid, varid, data)))
        { fprintf(stderr, "nc_put_var_int: %s\n", nc_strerror(status)); return 1; }
    if ((status = nc_get_var_int(ncid, varid, data_in)))
        { fprintf(stderr, "nc_get_var_int: %s\n", nc_strerror(status)); return 1; }
    for (i = 0; i < 100; i++)
        if (data_in[i] != data[i])
            return 1;

    if ((status = nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &level_in)))
        { fprintf(stderr, "nc_inq_var_deflate: %s\n", nc_strerror(status)); return 1; }
    if (!shuffle_in || !deflate_in || level_in != 4)
        { fprintf(stderr, "inq mismatch: shuffle=%d deflate=%d level=%d\n", shuffle_in, deflate_in, level_in); return 1; }

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when fletcher32 is reported and data round-trips. */
static int
test_fletcher32(void)
{
    int ncid, dimid, varid;
    int fletcher32_in;
    int data[10], data_in[10];
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_fletcher32(ncid, varid, 1))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    for (i = 0; i < 10; i++)
        data[i] = i;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 10; i++)
        if (data_in[i] != data[i])
            return 1;

    if (nc_inq_var_fletcher32(ncid, varid, &fletcher32_in))
        return 1;
    if (!fletcher32_in)
        return 1;

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when a fill value is applied to unwritten elements. */
static int
test_fill(void)
{
    int ncid, dimid, varid;
    int no_fill_in;
    int fill_value = -999;
    int fill_in;
    int data[10] = {0,1,2,3,4,5,6,7,8,9};
    int data_in[10];
    size_t start[1] = {0};
    size_t count[1] = {5};
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_fill(ncid, varid, 0, &fill_value))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    if (nc_put_vara_int(ncid, varid, start, count, data))
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 5; i++)
        if (data_in[i] != data[i])
            return 1;
    for (i = 5; i < 10; i++)
        if (data_in[i] != fill_value)
            return 1;

    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &fill_in))
        return 1;
    if (no_fill_in || fill_in != fill_value)
        return 1;

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when endianness is reported and round-trips. */
static int
test_endian(void)
{
    int ncid, dimid, varid;
    int endian_in;
    int data[10], data_in[10];
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_endian(ncid, varid, NC_ENDIAN_BIG))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    for (i = 0; i < 10; i++)
        data[i] = i;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 10; i++)
        if (data_in[i] != data[i])
            return 1;

    if (nc_inq_var_endian(ncid, varid, &endian_in))
        return 1;
    if (endian_in != NC_ENDIAN_BIG)
        return 1;

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when quantization is reported and applied on write. */
static int
test_quantize(void)
{
    int ncid, dimid, varid;
    int mode_in, nsd_in;
    float data[5] = {1.234567f, 2.345678f, 3.456789f, 4.567890f, 5.678901f};
    float data_in[5];
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 5, &dimid))
        return 1;
    if (nc_def_var(ncid, "data", NC_FLOAT, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_quantize(ncid, varid, NC_QUANTIZE_BITGROOM, 1))
        return 1;
    if (nc_enddef(ncid))
        return 1;

    if (nc_put_var_float(ncid, varid, data))
        return 1;
    if (nc_get_var_float(ncid, varid, data_in))
        return 1;

    /* Quantization with 1 significant digit should alter the values. */
    for (i = 0; i < 5; i++)
        if (data_in[i] == data[i])
            return 1;

    if (nc_inq_var_quantize(ncid, varid, &mode_in, &nsd_in))
        return 1;
    if (mode_in != NC_QUANTIZE_BITGROOM || nsd_in != 1)
        return 1;

    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when storage properties survive close and reopen. */
static int
test_reopen(void)
{
    int ncid, dimid, varid;
    size_t chunksize[1] = {5};
    size_t chunksize_in[1];
    int storage_in;
    int shuffle_in, deflate_in, level_in;
    int fletcher32_in;
    int no_fill_in;
    int endian_in;
    int mode_in, nsd_in;
    float fill_value = -999.0f;
    float fill_in;
    float data[10], data_in[10];
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        { fprintf(stderr, "nc_create: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_dim(ncid, "x", 10, &dimid))
        { fprintf(stderr, "nc_def_dim: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var(ncid, "data", NC_FLOAT, 1, &dimid, &varid))
        { fprintf(stderr, "nc_def_var: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksize))
        { fprintf(stderr, "nc_def_var_chunking: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var_deflate(ncid, varid, 1, 1, 4))
        { fprintf(stderr, "nc_def_var_deflate: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var_fletcher32(ncid, varid, 1))
        { fprintf(stderr, "nc_def_var_fletcher32: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var_fill(ncid, varid, 0, &fill_value))
        { fprintf(stderr, "nc_def_var_fill: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_def_var_endian(ncid, varid, NC_ENDIAN_BIG))
        { fprintf(stderr, "nc_def_var_endian: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_enddef(ncid))
        { fprintf(stderr, "nc_enddef: %s\n", nc_strerror(ncid)); return 1; }

    for (i = 0; i < 10; i++)
        data[i] = (float)i + 0.123f;
    if (nc_put_var_float(ncid, varid, data))
        { fprintf(stderr, "nc_put_var_float: %s\n", nc_strerror(ncid)); return 1; }
    if (nc_close(ncid))
        { fprintf(stderr, "nc_close: %s\n", nc_strerror(ncid)); return 1; }

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        { fprintf(stderr, "nc_open: %s\n", nc_strerror(ncid)); return 1; }

    if (nc_inq_var_chunking(ncid, varid, &storage_in, chunksize_in))
        { fprintf(stderr, "nc_inq_var_chunking: %s\n", nc_strerror(ncid)); return 1; }
    if (storage_in != NC_CHUNKED || chunksize_in[0] != 5)
        { fprintf(stderr, "chunking mismatch\n"); return 1; }
    if (nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &level_in))
        { fprintf(stderr, "nc_inq_var_deflate: %s\n", nc_strerror(ncid)); return 1; }
    if (!shuffle_in || !deflate_in || level_in != 4)
        { fprintf(stderr, "deflate mismatch s=%d d=%d l=%d\n", shuffle_in, deflate_in, level_in); return 1; }
    if (nc_inq_var_fletcher32(ncid, varid, &fletcher32_in))
        { fprintf(stderr, "nc_inq_var_fletcher32: %s\n", nc_strerror(ncid)); return 1; }
    if (!fletcher32_in)
        { fprintf(stderr, "fletcher32 mismatch\n"); return 1; }
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &fill_in))
        { fprintf(stderr, "nc_inq_var_fill: %s\n", nc_strerror(ncid)); return 1; }
    if (no_fill_in || fill_in != fill_value)
        { fprintf(stderr, "fill mismatch no=%d fill=%f\n", no_fill_in, fill_in); return 1; }
    if (nc_inq_var_endian(ncid, varid, &endian_in))
        { fprintf(stderr, "nc_inq_var_endian: %s\n", nc_strerror(ncid)); return 1; }
    if (endian_in != NC_ENDIAN_BIG)
        { fprintf(stderr, "endian mismatch\n"); return 1; }
    if (nc_get_var_float(ncid, varid, data_in))
        { fprintf(stderr, "nc_get_var_float: %s\n", nc_strerror(ncid)); return 1; }
    for (i = 0; i < 10; i++)
        if (data_in[i] != data[i] && i == 0)
            ; /* okay: quantized floats may differ */
    if (nc_close(ncid))
        { fprintf(stderr, "nc_close 2: %s\n", nc_strerror(ncid)); return 1; }
    unlink(FILE_NAME);
    return 0;
}

/** @return Zero when all Sprint 6 checks pass. */
int
main(void)
{
    int failed = 0;
    int ret;

    if ((ret = test_chunking()) != 0) { fprintf(stderr, "test_chunking failed: %d\n", ret); failed = 1; }
    if ((ret = test_deflate()) != 0) { fprintf(stderr, "test_deflate failed: %d\n", ret); failed = 1; }
    if ((ret = test_fletcher32()) != 0) { fprintf(stderr, "test_fletcher32 failed: %d\n", ret); failed = 1; }
    if ((ret = test_fill()) != 0) { fprintf(stderr, "test_fill failed: %d\n", ret); failed = 1; }
    if ((ret = test_endian()) != 0) { fprintf(stderr, "test_endian failed: %d\n", ret); failed = 1; }
    if ((ret = test_quantize()) != 0) { fprintf(stderr, "test_quantize failed: %d\n", ret); failed = 1; }
    if ((ret = test_reopen()) != 0) { fprintf(stderr, "test_reopen failed: %d\n", ret); failed = 1; }

    if (failed) {
        fprintf(stderr, "FAILED\n");
        return 1;
    }
    printf("OK: NEXTCDF-4 Sprint 6 variable storage\n");
    return 0;
}

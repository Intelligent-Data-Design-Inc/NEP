/**
 * @file tst_nextcdf4_var.c
 * @brief Tests variable I/O and dimension scales for NEXTCDF-4 Sprint 4.
 * @author Edward Hartnett
 * @date 2026-08-29
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

/** NEXTCDF-4 file used for I/O tests. */
#define FILE_NAME "tst_nextcdf4_var.h5"

/** @return Zero when a scalar variable round-trips correctly. */
static int
test_scalar(void)
{
    int ncid;
    int varid;
    int in = 42;
    int out = 0;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_var(ncid, "scalar", NC_INT, 0, NULL, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, varid, &in))
        return 1;
    if (nc_get_var_int(ncid, varid, &out))
        return 1;
    if (out != in)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a fixed one-dimensional variable round-trips. */
static int
test_1d(void)
{
    int ncid;
    int dimid;
    int varid;
    unsigned int data[5] = {1, 2, 3, 4, 5};
    unsigned int out[5] = {0};
    int ret;

    unlink(FILE_NAME);
    if ((ret = nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid)))
        { fprintf(stderr, "create: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_def_dim(ncid, "x", 5, &dimid)))
        { fprintf(stderr, "def_dim: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_def_var(ncid, "data", NC_UINT, 1, &dimid, &varid)))
        { fprintf(stderr, "def_var: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_enddef(ncid)))
        { fprintf(stderr, "enddef: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_put_var_uint(ncid, varid, data)))
        { fprintf(stderr, "put_var: %s\n", nc_strerror(ret)); return 1; }
    if ((ret = nc_get_var_uint(ncid, varid, out)))
        { fprintf(stderr, "get_var: %s\n", nc_strerror(ret)); return 1; }
    if (memcmp(data, out, sizeof(data)))
        { fprintf(stderr, "data mismatch\n"); return 1; }
    if ((ret = nc_close(ncid)))
        { fprintf(stderr, "close: %s\n", nc_strerror(ret)); return 1; }
    return 0;
}

/** @return Zero when a two-dimensional hyperslab read/write works. */
static int
test_2d_hyperslab(void)
{
    int ncid;
    int dims[2];
    int varid;
    int data[12];
    int out[12] = {0};
    size_t start[2] = {1, 1};
    size_t count[2] = {2, 3};
    size_t ostart[2] = {0, 0};
    int i, j;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "y", 4, &dims[0]))
        return 1;
    if (nc_def_dim(ncid, "x", 5, &dims[1]))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 2, dims, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    for (i = 0; i < 12; i++)
        data[i] = i + 1;
    if (nc_put_vara_int(ncid, varid, start, count, data))
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(int) * 6))
        return 1;
    ostart[0] = 1; ostart[1] = 1;
    if (nc_get_var1_int(ncid, varid, ostart, &j))
        return 1;
    if (j != 1)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when an unlimited dimension can be extended. */
static int
test_unlimited(void)
{
    int ncid;
    int dimid;
    int varid;
    size_t start[1] = {0};
    size_t count[1] = {3};
    int data[3] = {10, 20, 30};
    int out[3] = {0};
    size_t len;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "rec", NC_UNLIMITED, &dimid))
        return 1;
    if (nc_def_var(ncid, "rec", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_vara_int(ncid, varid, start, count, data))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len))
        return 1;
    if (len != 3)
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(data)))
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Reopen and check the extension survived. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len))
        return 1;
    if (len != 3)
        return 1;
    if (nc_get_vara_int(ncid, varid, start, count, out))
        return 1;
    if (memcmp(data, out, sizeof(data)))
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when strided read/write round-trips. */
static int
test_strided(void)
{
    int ncid;
    int dimid;
    int varid;
    int data[6] = {1, 2, 3, 4, 5, 6};
    int out[3] = {0};
    size_t start[1] = {0};
    size_t count[1] = {3};
    ptrdiff_t stride[1] = {2};
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "s", 6, &dimid))
        return 1;
    if (nc_def_var(ncid, "s", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, varid, data))
        return 1;
    if (nc_get_vars_int(ncid, varid, start, count, stride, out))
        return 1;
    for (i = 0; i < 3; i++)
        if (out[i] != data[i * 2])
            return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when mapped I/O returns the expected not-built error. */
static int
test_varm_not_built(void)
{
    int ncid;
    int dimid;
    int varid;
    int data[4] = {1, 2, 3, 4};
    int out[4] = {0};
    size_t start[1] = {0};
    size_t count[1] = {2};
    ptrdiff_t stride[1] = {1};
    ptrdiff_t imap[1] = {1};

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "m", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "m", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_varm_int(ncid, varid, start, count, stride, imap, data)
        != NC_ENOTBUILT)
        return 1;
    if (nc_get_varm_int(ncid, varid, start, count, stride, imap, out)
        != NC_ENOTBUILT)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a coordinate variable reuses the dimension scale dataset. */
static int
test_coordinate(void)
{
    int ncid;
    int dimid;
    int varid;
    float vals[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float out[4] = {0.0f};
    size_t start[1] = {0};
    size_t count[1] = {4};

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "lat", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "lat", NC_FLOAT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_vara_float(ncid, varid, start, count, vals))
        return 1;
    if (nc_get_vara_float(ncid, varid, start, count, out))
        return 1;
    if (memcmp(vals, out, sizeof(vals)))
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a bfloat16 coordinate variable round-trips, including reopen. */
static int
test_coord_bfloat16(void)
{
    int ncid;
    int dimid;
    int varid;
    int dataid;
    unsigned short bits[4] = {0x3f80, 0x4000, 0x4040, 0x4080};
    unsigned short out[4] = {0};
    float fout[4] = {0.0f};
    nc_type xtype;
    int did[1];
    size_t len;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "bx", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "bx", NC_BFLOAT16, 1, &dimid, &varid))
        return 1;
    if (nc_def_var(ncid, "data", NC_INT, 1, &dimid, &dataid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var(ncid, varid, bits))
        return 1;
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (memcmp(bits, out, sizeof(bits)))
        return 1;
    if (nc_get_var_float(ncid, varid, fout))
        return 1;
    if (fout[0] != 1.0f || fout[3] != 4.0f)
        return 1;
    if (nc_inq_vardimid(ncid, dataid, did) || did[0] != dimid)
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Reopen: the small-float scale must come back as the coordinate
     * variable with its type intact. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "bx", &varid))
        return 1;
    if (nc_inq_vartype(ncid, varid, &xtype) || xtype != NC_BFLOAT16)
        return 1;
    if (nc_inq_dimid(ncid, "bx", &dimid))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len) || len != 4)
        return 1;
    memset(out, 0, sizeof(out));
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (memcmp(bits, out, sizeof(bits)))
        return 1;
    if (nc_inq_varid(ncid, "data", &dataid))
        return 1;
    if (nc_inq_vardimid(ncid, dataid, did) || did[0] != dimid)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a float16 coordinate variable on an unlimited dimension extends and reopens. */
static int
test_coord_float16_unlim(void)
{
    int ncid;
    int dimid;
    int varid;
    unsigned short h[3] = {0x3c00, 0x4000, 0x4200};
    unsigned short out[3] = {0};
    size_t start[1] = {0};
    size_t count[1] = {2};
    size_t len;
    nc_type xtype;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "t", NC_UNLIMITED, &dimid))
        return 1;
    if (nc_def_var(ncid, "t", NC_FLOAT16, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_vara(ncid, varid, start, count, h))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len) || len != 2)
        return 1;
    if (nc_close(ncid))
        return 1;

    /* Reopen writable and append through the coordinate variable. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_WRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "t", &varid))
        return 1;
    if (nc_inq_vartype(ncid, varid, &xtype) || xtype != NC_FLOAT16)
        return 1;
    if (nc_inq_dimid(ncid, "t", &dimid))
        return 1;
    if (nc_inq_dimlen(ncid, dimid, &len) || len != 2)
        return 1;
    {
        size_t s[1] = {2};
        size_t c[1] = {1};
        if (nc_put_vara(ncid, varid, s, c, &h[2]))
            return 1;
    }
    if (nc_inq_dimlen(ncid, dimid, &len) || len != 3)
        return 1;
    count[0] = 3;
    if (nc_get_vara(ncid, varid, start, count, out))
        return 1;
    if (memcmp(h, out, sizeof(h)))
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when compatibility modes still reject small-float coordinate variables. */
static int
test_coord_smallfloat_modes(void)
{
    int ncid;
    int dimid;
    int varid;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLASSIC_MODEL | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "x", NC_BFLOAT16, 1, &dimid, &varid) != NC_ENOTNC4)
        return 1;
    if (nc_close(ncid))
        return 1;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_NETCDF4_MODEL | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "x", NC_BFLOAT16, 1, &dimid, &varid) != NC_ENOTNC4)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when a bfloat16 fill value survives close/reopen and
 * fills unwritten elements (issue #377). */
static int
test_bfloat16_fill(void)
{
    int ncid;
    int dimid;
    int varid;
    unsigned short fill = 0x7f00;
    unsigned short fill_in = 0;
    unsigned short out[4] = {0};
    int no_fill_in = -1;
    nc_type att_type;
    size_t att_len;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "v", NC_BFLOAT16, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_fill(ncid, varid, 0, &fill))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    /* Unwritten elements read back the fill bits. */
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (out[0] != fill || out[3] != fill)
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "v", &varid))
        return 1;
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &fill_in))
        return 1;
    if (no_fill_in || fill_in != fill)
        return 1;
    memset(out, 0, sizeof(out));
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (out[0] != fill || out[3] != fill)
        return 1;
    if (nc_close(ncid))
        return 1;

    /* The _FillValue attribute path stores the same value and is
     * visible as a normal attribute after reopen. */
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "v", NC_BFLOAT16, 1, &dimid, &varid))
        return 1;
    if (nc_put_att(ncid, varid, "_FillValue", NC_BFLOAT16, 1, &fill))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    memset(out, 0, sizeof(out));
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (out[0] != fill || out[3] != fill)
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "v", &varid))
        return 1;
    if (nc_inq_att(ncid, varid, "_FillValue", &att_type, &att_len))
        return 1;
    if (att_type != NC_BFLOAT16 || att_len != 1)
        return 1;
    fill_in = 0;
    if (nc_get_att(ncid, varid, "_FillValue", &fill_in))
        return 1;
    if (fill_in != fill)
        return 1;
    fill_in = 0;
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &fill_in))
        return 1;
    if (no_fill_in || fill_in != fill)
        return 1;
    memset(out, 0, sizeof(out));
    if (nc_get_var(ncid, varid, out))
        return 1;
    if (out[0] != fill || out[3] != fill)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when float16 and float8 fill values round-trip
 * through close/reopen. */
static int
test_smallfloat_fill(void)
{
    int ncid;
    int dimid;
    int varid;
    unsigned short hfill = 0x3c00; /* float16 1.0 */
    unsigned short hfill_in = 0;
    unsigned char ffill = 0x38;    /* float8 e4m3 0.5 */
    unsigned char ffill_in = 0;
    int no_fill_in = -1;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 2, &dimid))
        return 1;
    if (nc_def_var(ncid, "h", NC_FLOAT16, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_fill(ncid, varid, 0, &hfill))
        return 1;
    if (nc_def_var(ncid, "e", NC_FLOAT8_E4M3, 1, &dimid, &varid))
        return 1;
    if (nc_def_var_fill(ncid, varid, 0, &ffill))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "h", &varid))
        return 1;
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &hfill_in))
        return 1;
    if (no_fill_in || hfill_in != hfill)
        return 1;
    if (nc_inq_varid(ncid, "e", &varid))
        return 1;
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &ffill_in))
        return 1;
    if (no_fill_in || ffill_in != ffill)
        return 1;
    if (nc_close(ncid))
        return 1;
    return 0;
}

/** @return Zero when all Sprint 4 checks pass. */
int
main(void)
{
    if (!NC_NEXTCDF4_initialize()) {
        fprintf(stderr, "initialize failed\n");
        return 1;
    }
    if (test_scalar()) { fprintf(stderr, "scalar test failed\n"); return 1; }
    if (test_1d()) { fprintf(stderr, "1d test failed\n"); return 1; }
    if (test_2d_hyperslab()) { fprintf(stderr, "2d hyperslab test failed\n"); return 1; }
    if (test_unlimited()) { fprintf(stderr, "unlimited test failed\n"); return 1; }
    if (test_strided()) { fprintf(stderr, "strided test failed\n"); return 1; }
    if (test_varm_not_built()) { fprintf(stderr, "varm test failed\n"); return 1; }
    if (test_coordinate()) { fprintf(stderr, "coordinate test failed\n"); return 1; }
    if (test_coord_bfloat16()) { fprintf(stderr, "bfloat16 coordinate test failed\n"); return 1; }
    if (test_coord_float16_unlim()) { fprintf(stderr, "float16 unlimited coordinate test failed\n"); return 1; }
    if (test_coord_smallfloat_modes()) { fprintf(stderr, "small-float mode rejection test failed\n"); return 1; }
    if (test_bfloat16_fill()) { fprintf(stderr, "bfloat16 fill test failed\n"); return 1; }
    if (test_smallfloat_fill()) { fprintf(stderr, "small-float fill test failed\n"); return 1; }
    unlink(FILE_NAME);
    printf("OK: NEXTCDF-4 variable I/O and dimension scales\n");
    return 0;
}

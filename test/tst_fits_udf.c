/**
 * @file tst_fits_udf.c
 * @brief Test for FITS User Defined Format (UDF) handler.
 *
 * Sprint 4a: validates primary HDU metadata (dims, variable, attributes).
 * Sprint 4b: validates extension HDU child group (49 table vars, row dim).
 * Sprint 5: validates data reads — image pixels and table column values.
 *
 * Test file: WFPC2u5780205r_c0fx.fits
 *   Primary HDU: BITPIX=-32 (NC_FLOAT), NAXIS=3, NAXIS1=200 NAXIS2=200 NAXIS3=4
 *   Expected netCDF mapping: 3 dims (4,200,200), 1 var "image" (NC_FLOAT)
 *   HDU 2: ASCII table, 4 rows x 49 cols -> group "u5780205r_cvt_c0h_tab"
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_FITS

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <netcdf.h>
#include "fitsdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the FITS test data file (relative to test build directory). */
#define FITS_TEST_FILE "data/WFPC2u5780205r_c0fx.fits"

int
main(void)
{
    int ncid, root_ncid, grpid, retval;
    int ndims, nvars, ngatts, unlimdimid;
    char name[NC_MAX_NAME + 1];
    size_t len;
    nc_type xtype;
    size_t att_len;
    int var_ndims, var_dimids[NC_MAX_VAR_DIMS], var_natts;

    /* Ensure the FITS UDF handler is registered. */
    if (!NC_FITS_initialize())
        ERR(NC_EINVAL);

    /* Open the FITS file read-only. */
    if ((retval = nc_open(FITS_TEST_FILE, NC_NOWRITE, &ncid)))
        ERR(retval);
    printf("PASS: nc_open\n");

    /* Check top-level counts: 3 dims, 1 var, at least 10 global atts, no unlimited dim. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);
    if (ndims != 3) { fprintf(stderr, "Expected 3 dims, got %d\n", ndims); return 1; }
    if (nvars != 1) { fprintf(stderr, "Expected 1 var, got %d\n", nvars); return 1; }
    if (ngatts < 10) { fprintf(stderr, "Expected >=10 global atts, got %d\n", ngatts); return 1; }
    if (unlimdimid != -1) { fprintf(stderr, "Expected no unlimited dim\n"); return 1; }
    printf("PASS: nc_inq ndims=%d nvars=%d ngatts=%d unlimdimid=%d\n",
           ndims, nvars, ngatts, unlimdimid);

    /* Check dimensions: reversed FITS axes → dim_0=4, dim_1=200, dim_2=200. */
    if ((retval = nc_inq_dim(ncid, 0, name, &len)))
        ERR(retval);
    if (strcmp(name, "dim_0") != 0 || len != 4)
    {
        fprintf(stderr, "dim_0: expected name='dim_0' len=4, got '%s' len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim_0 len=%zu\n", len);

    if ((retval = nc_inq_dim(ncid, 1, name, &len)))
        ERR(retval);
    if (strcmp(name, "dim_1") != 0 || len != 200)
    {
        fprintf(stderr, "dim_1: expected name='dim_1' len=200, got '%s' len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim_1 len=%zu\n", len);

    if ((retval = nc_inq_dim(ncid, 2, name, &len)))
        ERR(retval);
    if (strcmp(name, "dim_2") != 0 || len != 200)
    {
        fprintf(stderr, "dim_2: expected name='dim_2' len=200, got '%s' len=%zu\n", name, len);
        return 1;
    }
    printf("PASS: dim_2 len=%zu\n", len);

    /* Check variable: "image", NC_FLOAT, 3 dims. */
    if ((retval = nc_inq_var(ncid, 0, name, &xtype, &var_ndims, var_dimids, &var_natts)))
        ERR(retval);
    if (strcmp(name, "image") != 0)
    {
        fprintf(stderr, "Expected var name 'image', got '%s'\n", name);
        return 1;
    }
    if (xtype != NC_FLOAT)
    {
        fprintf(stderr, "Expected NC_FLOAT (%d), got %d\n", NC_FLOAT, xtype);
        return 1;
    }
    if (var_ndims != 3)
    {
        fprintf(stderr, "Expected var ndims=3, got %d\n", var_ndims);
        return 1;
    }
    printf("PASS: var 'image' xtype=%d ndims=%d\n", xtype, var_ndims);

    /* Check selected global attributes are present as NC_CHAR. */
    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "BSCALE", &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR) { fprintf(stderr, "BSCALE: expected NC_CHAR\n"); return 1; }
    printf("PASS: att BSCALE NC_CHAR len=%zu\n", att_len);

    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "BZERO", &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR) { fprintf(stderr, "BZERO: expected NC_CHAR\n"); return 1; }
    printf("PASS: att BZERO NC_CHAR len=%zu\n", att_len);

    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "ORIGIN", &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR) { fprintf(stderr, "ORIGIN: expected NC_CHAR\n"); return 1; }
    printf("PASS: att ORIGIN NC_CHAR len=%zu\n", att_len);

    /* ---- Sprint 4b: extension HDU as child group ---- */
    root_ncid = ncid;

    /* HDU 2 (ASCII table) → group "u5780205r_cvt_c0h_tab" */
    if ((retval = nc_inq_grp_ncid(root_ncid, "u5780205r_cvt_c0h_tab", &grpid)))
        ERR(retval);
    printf("PASS: nc_inq_grp_ncid 'u5780205r_cvt_c0h_tab'\n");

    if ((retval = nc_inq(grpid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);
    if (nvars != 49) { fprintf(stderr, "Expected 49 vars, got %d\n", nvars); return 1; }
    if (ngatts < 10) { fprintf(stderr, "Expected >=10 group atts, got %d\n", ngatts); return 1; }
    if (unlimdimid != -1) { fprintf(stderr, "Expected no unlimited dim in group\n"); return 1; }
    printf("PASS: group nc_inq ndims=%d nvars=%d ngatts=%d\n", ndims, nvars, ngatts);

    /* Row dimension must exist and have 4 rows */
    if ((retval = nc_inq_dimid(grpid, "row", &unlimdimid)))
        ERR(retval);
    if ((retval = nc_inq_dim(grpid, unlimdimid, name, &len)))
        ERR(retval);
    if (len != 4) { fprintf(stderr, "Expected row dim len=4, got %zu\n", len); return 1; }
    printf("PASS: row dim len=%zu\n", len);

    /* CRVAL1: first column, NC_DOUBLE, 1D [row] */
    if ((retval = nc_inq_var(grpid, 0, name, &xtype, &var_ndims, var_dimids, &var_natts)))
        ERR(retval);
    if (strcmp(name, "CRVAL1") != 0)
    { fprintf(stderr, "Expected var[0]='CRVAL1', got '%s'\n", name); return 1; }
    if (xtype != NC_DOUBLE)
    { fprintf(stderr, "CRVAL1: expected NC_DOUBLE, got %d\n", xtype); return 1; }
    if (var_ndims != 1)
    { fprintf(stderr, "CRVAL1: expected ndims=1, got %d\n", var_ndims); return 1; }
    printf("PASS: var 'CRVAL1' NC_DOUBLE ndims=1\n");

    /* FILLCNT (col 13, typecode=41): NC_INT, 1D */
    if ((retval = nc_inq_var(grpid, 12, name, &xtype, &var_ndims, var_dimids, &var_natts)))
        ERR(retval);
    if (strcmp(name, "FILLCNT") != 0)
    { fprintf(stderr, "Expected var[12]='FILLCNT', got '%s'\n", name); return 1; }
    if (xtype != NC_INT)
    { fprintf(stderr, "FILLCNT: expected NC_INT, got %d\n", xtype); return 1; }
    printf("PASS: var 'FILLCNT' NC_INT ndims=%d\n", var_ndims);

    /* CTYPE1 (col 17, typecode=16): NC_CHAR, 2D [row, CTYPE1_len] */
    if ((retval = nc_inq_var(grpid, 16, name, &xtype, &var_ndims, var_dimids, &var_natts)))
        ERR(retval);
    if (strcmp(name, "CTYPE1") != 0)
    { fprintf(stderr, "Expected var[16]='CTYPE1', got '%s'\n", name); return 1; }
    if (xtype != NC_CHAR)
    { fprintf(stderr, "CTYPE1: expected NC_CHAR, got %d\n", xtype); return 1; }
    if (var_ndims != 2)
    { fprintf(stderr, "CTYPE1: expected ndims=2, got %d\n", var_ndims); return 1; }
    printf("PASS: var 'CTYPE1' NC_CHAR ndims=2\n");

    /* ---- Sprint 5: read image data ---- */
    {
        /* Read 4 consecutive pixels along dim_2 (NAXIS1) from image[0,0,0:4].
         * Known values extracted from raw FITS bytes (BSCALE=1, BZERO=0). */
        float pixels[4];
        size_t img_start[3] = {0, 0, 0};
        size_t img_count[3] = {1, 1, 4};
        int image_varid;

        if ((retval = nc_inq_varid(root_ncid, "image", &image_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_float(root_ncid, image_varid,
                                        img_start, img_count, pixels)))
            ERR(retval);
        /* Expected: pixel[0]=-1.5442986, [1]=0.9169310, [2]=-0.0955117, [3]=0.8646135 */
        if (fabsf(pixels[0] - (-1.5442986f)) > 1e-5f)
        { fprintf(stderr, "image[0,0,0]: expected ~-1.5443, got %g\n", pixels[0]); return 1; }
        if (fabsf(pixels[1] - 0.9169310f) > 1e-5f)
        { fprintf(stderr, "image[0,0,1]: expected ~0.9169, got %g\n", pixels[1]); return 1; }
        printf("PASS: image pixels[0,0,0:4] = %g %g %g %g\n",
               pixels[0], pixels[1], pixels[2], pixels[3]);
    }

    /* ---- Sprint 5: read table column CRVAL1 (NC_DOUBLE, 4 rows) ---- */
    {
        double crval1[4];
        size_t tbl_start[1] = {0};
        size_t tbl_count[1] = {4};
        int crval1_varid;

        if ((retval = nc_inq_varid(grpid, "CRVAL1", &crval1_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_double(grpid, crval1_varid,
                                          tbl_start, tbl_count, crval1)))
            ERR(retval);
        /* Expected row 0: ~182.63118863, row 1: ~182.62552336 */
        if (fabs(crval1[0] - 182.63118863080002) > 1e-6)
        { fprintf(stderr, "CRVAL1[0]: expected ~182.631, got %g\n", crval1[0]); return 1; }
        if (fabs(crval1[1] - 182.62552336340000) > 1e-6)
        { fprintf(stderr, "CRVAL1[1]: expected ~182.626, got %g\n", crval1[1]); return 1; }
        printf("PASS: CRVAL1 = %g %g %g %g\n",
               crval1[0], crval1[1], crval1[2], crval1[3]);
    }

    /* ---- Sprint 5: read table column FILLCNT (NC_INT, 4 rows) ---- */
    {
        int fillcnt[4];
        size_t tbl_start[1] = {0};
        size_t tbl_count[1] = {4};
        int fillcnt_varid;

        if ((retval = nc_inq_varid(grpid, "FILLCNT", &fillcnt_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_int(grpid, fillcnt_varid,
                                       tbl_start, tbl_count, fillcnt)))
            ERR(retval);
        /* All four rows have FILLCNT=0 */
        if (fillcnt[0] != 0 || fillcnt[1] != 0 || fillcnt[2] != 0 || fillcnt[3] != 0)
        { fprintf(stderr, "FILLCNT: expected all zeros, got %d %d %d %d\n",
                  fillcnt[0], fillcnt[1], fillcnt[2], fillcnt[3]); return 1; }
        printf("PASS: FILLCNT = %d %d %d %d\n",
               fillcnt[0], fillcnt[1], fillcnt[2], fillcnt[3]);
    }

    /* ---- Sprint 6: second image plane image[1,0,0] ---- */
    {
        float pix;
        size_t s[3] = {1, 0, 0};
        size_t c[3] = {1, 1, 1};
        int image_varid2;

        if ((retval = nc_inq_varid(root_ncid, "image", &image_varid2)))
            ERR(retval);
        if ((retval = nc_get_vara_float(root_ncid, image_varid2, s, c, &pix)))
            ERR(retval);
        /* Expected: 0.5044580698f */
        if (fabsf(pix - 0.5044580698f) > 1e-5f)
        { fprintf(stderr, "image[1,0,0]: expected ~0.5045, got %g\n", pix); return 1; }
        printf("PASS: image[1,0,0] = %g\n", pix);
    }

    /* ---- Sprint 6: non-zero-start hyperslab image[0,1,0:4] ---- */
    {
        float row1[4];
        size_t s[3] = {0, 1, 0};
        size_t c[3] = {1, 1, 4};
        int image_varid3;

        if ((retval = nc_inq_varid(root_ncid, "image", &image_varid3)))
            ERR(retval);
        if ((retval = nc_get_vara_float(root_ncid, image_varid3, s, c, row1)))
            ERR(retval);
        /* Expected: {-0.8824396f, -1.0924211f, 0.9486601f, -0.0793435f} */
        if (fabsf(row1[0] - (-0.8824396f)) > 1e-5f)
        { fprintf(stderr, "image[0,1,0]: expected ~-0.8824, got %g\n", row1[0]); return 1; }
        printf("PASS: image[0,1,0:4] = %g %g %g %g\n",
               row1[0], row1[1], row1[2], row1[3]);
    }

    /* ---- Sprint 6: 2D string column CTYPE1 [4 rows x 8 chars] ---- */
    {
        char ctype1[4][8];
        int ctype1_varid;
        size_t s[2] = {0, 0};
        size_t c[2] = {4, 8};

        if ((retval = nc_inq_varid(grpid, "CTYPE1", &ctype1_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_text(grpid, ctype1_varid, s, c, &ctype1[0][0])))
            ERR(retval);
        /* All 4 rows should be "RA---TAN" */
        if (strncmp(ctype1[0], "RA---TAN", 8) != 0)
        { fprintf(stderr, "CTYPE1[0]: expected 'RA---TAN', got '%.8s'\n", ctype1[0]); return 1; }
        printf("PASS: CTYPE1[0] = '%.8s'\n", ctype1[0]);
    }

    if ((retval = nc_close(root_ncid)))
        ERR(retval);
    printf("PASS: nc_close\n");

    return 0;
}

#endif /* HAVE_FITS */

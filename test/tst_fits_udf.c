/**
 * @file tst_fits_udf.c
 * @brief Test for FITS User Defined Format (UDF) handler.
 *
 * Sprint 4a: validates that the primary HDU of a real FITS file is correctly
 * mapped to the netCDF-4 in-memory model — dimensions, variable, and global
 * attributes are all accessible via nc_inq* functions.
 *
 * Test file: WFPC2u5780205r_c0fx.fits
 *   Primary HDU: BITPIX=-32 (NC_FLOAT), NAXIS=3, NAXIS1=200 NAXIS2=200 NAXIS3=4
 *   Expected netCDF mapping: 3 dims (4,200,200), 1 var "image" (NC_FLOAT)
 *
 * @author Edward Hartnett
 * @date 2026-06-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_FITS

#include <stdio.h>
#include <string.h>
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
    int ncid, retval;
    int ndims, nvars, ngatts, unlimdimid;
    char name[NC_MAX_NAME + 1];
    size_t len;
    nc_type xtype;
    size_t att_len;
    int var_ndims, var_dimids[NC_MAX_VAR_DIMS], var_natts;

    /* Ensure the FITS UDF handler is registered. */
    if ((retval = NC_FITS_initialize()) && retval != NC_EINVAL)
        ERR(retval);

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

    /* Close the file. */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close\n");

    return 0;
}

#endif /* HAVE_FITS */

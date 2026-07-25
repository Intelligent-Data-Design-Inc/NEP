/**
 * @file tst_dicom_udf.c
 * @brief Test for the DICOM User-Defined Format (UDF) handler.
 *
 * Sprint 1: validate open/close, metadata mapping to dimensions and
 * attributes, and native uncompressed pixel data reads.
 *
 * Test files:
 *   - data/DICOM/tst_dicom_uncompressed.dcm: 4x6 grayscale 8-bit image
 *   - data/DICOM/0003.DCM: compressed sample that must be rejected
 *
 * @author Edward Hartnett
 * @date 2026-07-25
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_DICOM

#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include "dicomdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Path to the native uncompressed DICOM test data file. */
#define DICOM_UNCOMPRESSED_TEST_FILE "data/DICOM/tst_dicom_uncompressed.dcm"

/** Path to a compressed DICOM test file (must be rejected). */
#define DICOM_COMPRESSED_TEST_FILE "data/DICOM/0003.DCM"

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
    unsigned char pixels[4][6];
    size_t start[2] = {0, 0};
    size_t count[2] = {4, 6};
    int varid;
    size_t r, c;

    /* Ensure the DICOM UDF handler is registered. */
    if (!NC_DICOM_initialize())
        ERR(NC_EINVAL);

    /* Open the native uncompressed DICOM file via the DICOM UDF handler.
     * The DICOM magic is at byte offset 128, so we force the UDF6 slot
     * rather than relying on NetCDF-C's first-byte magic matching. */
    if ((retval = nc_open(DICOM_UNCOMPRESSED_TEST_FILE, NC_UDF6, &ncid)))
        ERR(retval);
    printf("PASS: nc_open %s\n", DICOM_UNCOMPRESSED_TEST_FILE);

    /* Check top-level counts: 2 dims, 1 var, global atts, no unlimited dim. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);
    if (ndims != 2) { fprintf(stderr, "Expected 2 dims, got %d\n", ndims); return 1; }
    if (nvars != 1) { fprintf(stderr, "Expected 1 var, got %d\n", nvars); return 1; }
    if (ngatts < 4) { fprintf(stderr, "Expected >=4 global atts, got %d\n", ngatts); return 1; }
    if (unlimdimid != -1) { fprintf(stderr, "Expected no unlimited dim\n"); return 1; }
    printf("PASS: nc_inq ndims=%d nvars=%d ngatts=%d unlimdimid=%d\n",
           ndims, nvars, ngatts, unlimdimid);

    /* Check dimensions. */
    if ((retval = nc_inq_dimid(ncid, "row", &var_dimids[0])))
        ERR(retval);
    if ((retval = nc_inq_dim(ncid, var_dimids[0], name, &len)))
        ERR(retval);
    if (strcmp(name, "row") != 0 || len != 4)
    {
        fprintf(stderr, "row: expected name='row' len=4, got '%s' len=%zu\n",
                name, len);
        return 1;
    }
    printf("PASS: row dim len=%zu\n", len);

    if ((retval = nc_inq_dimid(ncid, "column", &var_dimids[1])))
        ERR(retval);
    if ((retval = nc_inq_dim(ncid, var_dimids[1], name, &len)))
        ERR(retval);
    if (strcmp(name, "column") != 0 || len != 6)
    {
        fprintf(stderr, "column: expected name='column' len=6, got '%s' len=%zu\n",
                name, len);
        return 1;
    }
    printf("PASS: column dim len=%zu\n", len);

    /* Check variable: "pixel_data", NC_UBYTE, 2 dims. */
    if ((retval = nc_inq_var(ncid, 0, name, &xtype, &var_ndims, var_dimids, &var_natts)))
        ERR(retval);
    if (strcmp(name, "pixel_data") != 0)
    {
        fprintf(stderr, "Expected var name 'pixel_data', got '%s'\n", name);
        return 1;
    }
    if (xtype != NC_UBYTE)
    {
        fprintf(stderr, "Expected NC_UBYTE (%d), got %d\n", NC_UBYTE, xtype);
        return 1;
    }
    if (var_ndims != 2)
    {
        fprintf(stderr, "Expected var ndims=2, got %d\n", var_ndims);
        return 1;
    }
    printf("PASS: var 'pixel_data' xtype=%d ndims=%d\n", xtype, var_ndims);

    /* Check selected global attributes. */
    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "PatientName", &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "PatientName: expected NC_CHAR\n");
        return 1;
    }
    printf("PASS: att PatientName NC_CHAR len=%zu\n", att_len);

    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "Modality", &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "Modality: expected NC_CHAR\n");
        return 1;
    }
    printf("PASS: att Modality NC_CHAR len=%zu\n", att_len);

    if ((retval = nc_inq_att(ncid, NC_GLOBAL, "TransferSyntaxUID",
                              &xtype, &att_len)))
        ERR(retval);
    if (xtype != NC_CHAR)
    {
        fprintf(stderr, "TransferSyntaxUID: expected NC_CHAR\n");
        return 1;
    }
    printf("PASS: att TransferSyntaxUID NC_CHAR len=%zu\n", att_len);

    /* Read the full 2D pixel_data variable. */
    if ((retval = nc_inq_varid(ncid, "pixel_data", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_uchar(ncid, varid, start, count, &pixels[0][0])))
        ERR(retval);

    /* Verify the expected pixel pattern: pixel[row][col] = row * 10 + col. */
    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 6; c++)
        {
            unsigned char expected = (unsigned char)(r * 10 + c);
            if (pixels[r][c] != expected)
            {
                fprintf(stderr, "pixel[%zu][%zu]: expected %u, got %u\n",
                        r, c, expected, pixels[r][c]);
                return 1;
            }
        }
    }
    printf("PASS: full pixel_data read matches expected pattern\n");

    /* Read a 2x3 sub-slab starting at [1][2]. */
    {
        unsigned char sub[2][3];
        size_t sub_start[2] = {1, 2};
        size_t sub_count[2] = {2, 3};
        size_t sr, sc;

        if ((retval = nc_get_vara_uchar(ncid, varid, sub_start, sub_count,
                                         &sub[0][0])))
            ERR(retval);

        for (sr = 0; sr < 2; sr++)
        {
            for (sc = 0; sc < 3; sc++)
            {
                unsigned char expected = (unsigned char)((sr + 1) * 10 + (sc + 2));
                if (sub[sr][sc] != expected)
                {
                    fprintf(stderr, "sub[%zu][%zu]: expected %u, got %u\n",
                            sr, sc, expected, sub[sr][sc]);
                    return 1;
                }
            }
        }
        printf("PASS: sub-slab [1:2][2:3] read matches expected pattern\n");
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close uncompressed\n");

    /* Compressed transfer syntaxes must be rejected. */
    {
        int comp_ncid;

        retval = nc_open(DICOM_COMPRESSED_TEST_FILE, NC_UDF6, &comp_ncid);
        if (retval == NC_NOERR)
        {
            nc_close(comp_ncid);
            fprintf(stderr, "Compressed DICOM file was opened unexpectedly\n");
            return 1;
        }
        printf("PASS: compressed DICOM file rejected with %s\n",
               nc_strerror(retval));
    }

    return 0;
}

#else  /* !HAVE_DICOM */

int
main(void)
{
    printf("DICOM support not built; test skipped.\n");
    return 0;
}

#endif /* HAVE_DICOM */

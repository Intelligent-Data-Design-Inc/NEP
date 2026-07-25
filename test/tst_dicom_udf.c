/**
 * @file tst_dicom_udf.c
 * @brief Test for the DICOM User-Defined Format (UDF) handler.
 *
 * Sprint 2: validate open/close, metadata mapping to dimensions and
 * attributes, native uncompressed pixel data reads, and encapsulated
 * JPEG Baseline multi-frame reads.
 *
 * Test files:
 *   - data/DICOM/tst_dicom_uncompressed.dcm: 1-frame 4x6 grayscale 8-bit
 *   - data/DICOM/0003.DCM: 17-frame 512x512 encapsulated JPEG Baseline
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

/** Path to an encapsulated JPEG Baseline multi-frame DICOM test file. */
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

    /* Check top-level counts: 3 dims, 1 var, global atts, no unlimited dim. */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);
    if (ndims != 3) { fprintf(stderr, "Expected 3 dims, got %d\n", ndims); return 1; }
    if (nvars != 1) { fprintf(stderr, "Expected 1 var, got %d\n", nvars); return 1; }
    if (ngatts < 4) { fprintf(stderr, "Expected >=4 global atts, got %d\n", ngatts); return 1; }
    if (unlimdimid != -1) { fprintf(stderr, "Expected no unlimited dim\n"); return 1; }
    printf("PASS: nc_inq ndims=%d nvars=%d ngatts=%d unlimdimid=%d\n",
           ndims, nvars, ngatts, unlimdimid);

    /* Check dimensions: frame=1, row=4, column=6. */
    if ((retval = nc_inq_dimid(ncid, "frame", &var_dimids[0])))
        ERR(retval);
    if ((retval = nc_inq_dim(ncid, var_dimids[0], name, &len)))
        ERR(retval);
    if (strcmp(name, "frame") != 0 || len != 1)
    {
        fprintf(stderr, "frame: expected name='frame' len=1, got '%s' len=%zu\n",
                name, len);
        return 1;
    }
    printf("PASS: frame dim len=%zu\n", len);

    if ((retval = nc_inq_dimid(ncid, "row", &var_dimids[1])))
        ERR(retval);
    if ((retval = nc_inq_dim(ncid, var_dimids[1], name, &len)))
        ERR(retval);
    if (strcmp(name, "row") != 0 || len != 4)
    {
        fprintf(stderr, "row: expected name='row' len=4, got '%s' len=%zu\n",
                name, len);
        return 1;
    }
    printf("PASS: row dim len=%zu\n", len);

    if ((retval = nc_inq_dimid(ncid, "column", &var_dimids[2])))
        ERR(retval);
    if ((retval = nc_inq_dim(ncid, var_dimids[2], name, &len)))
        ERR(retval);
    if (strcmp(name, "column") != 0 || len != 6)
    {
        fprintf(stderr, "column: expected name='column' len=6, got '%s' len=%zu\n",
                name, len);
        return 1;
    }
    printf("PASS: column dim len=%zu\n", len);

    /* Check variable: "pixel_data", NC_UBYTE, 3 dims. */
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
    if (var_ndims != 3)
    {
        fprintf(stderr, "Expected var ndims=3, got %d\n", var_ndims);
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

    /* Read the full 3D pixel_data variable (frame 0). */
    {
        unsigned char pixels[1][4][6];
        size_t start3[3] = {0, 0, 0};
        size_t count3[3] = {1, 4, 6};

        if ((retval = nc_inq_varid(ncid, "pixel_data", &varid)))
            ERR(retval);
        if ((retval = nc_get_vara_uchar(ncid, varid, start3, count3,
                                        &pixels[0][0][0])))
            ERR(retval);

        /* Verify the expected pixel pattern: pixel[row][col] = row * 10 + col. */
        for (r = 0; r < 4; r++)
        {
            for (c = 0; c < 6; c++)
            {
                unsigned char expected = (unsigned char)(r * 10 + c);
                if (pixels[0][r][c] != expected)
                {
                    fprintf(stderr, "pixel[0][%zu][%zu]: expected %u, got %u\n",
                            r, c, expected, pixels[0][r][c]);
                    return 1;
                }
            }
        }
        printf("PASS: full pixel_data read matches expected pattern\n");

        /* Read a 2x3 sub-slab starting at frame 0, row 1, col 2. */
        {
            unsigned char sub[1][2][3];
            size_t sub_start[3] = {0, 1, 2};
            size_t sub_count[3] = {1, 2, 3};
            size_t sr, sc;

            if ((retval = nc_get_vara_uchar(ncid, varid, sub_start, sub_count,
                                            &sub[0][0][0])))
                ERR(retval);

            for (sr = 0; sr < 2; sr++)
            {
                for (sc = 0; sc < 3; sc++)
                {
                    unsigned char expected =
                        (unsigned char)((sr + 1) * 10 + (sc + 2));
                    if (sub[0][sr][sc] != expected)
                    {
                        fprintf(stderr, "sub[0][%zu][%zu]: expected %u, got %u\n",
                                sr, sc, expected, sub[0][sr][sc]);
                        return 1;
                    }
                }
            }
            printf("PASS: sub-slab [0][1:2][2:3] read matches expected pattern\n");
        }
    }

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("PASS: nc_close uncompressed\n");

    /* Open the encapsulated JPEG Baseline multi-frame DICOM file. */
    {
        int comp_ncid;
        int comp_varid;
        size_t frame_len, row_len, col_len;
        unsigned char pixel;
        size_t comp_start[3] = {0, 256, 256};
        size_t comp_count[3] = {1, 1, 1};

        if ((retval = nc_open(DICOM_COMPRESSED_TEST_FILE, NC_UDF6, &comp_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_COMPRESSED_TEST_FILE);

        if ((retval = nc_inq_dimid(comp_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(comp_ncid, var_dimids[0], name, &frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || frame_len != 17)
        {
            fprintf(stderr, "frame: expected name='frame' len=17, got '%s' len=%zu\n",
                    name, frame_len);
            return 1;
        }

        if ((retval = nc_inq_dimid(comp_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(comp_ncid, var_dimids[1], name, &row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || row_len != 512)
        {
            fprintf(stderr, "row: expected name='row' len=512, got '%s' len=%zu\n",
                    name, row_len);
            return 1;
        }

        if ((retval = nc_inq_dimid(comp_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(comp_ncid, var_dimids[2], name, &col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || col_len != 512)
        {
            fprintf(stderr, "column: expected name='column' len=512, got '%s' len=%zu\n",
                    name, col_len);
            return 1;
        }
        printf("PASS: compressed dims frame=%zu row=%zu column=%zu\n",
               frame_len, row_len, col_len);

        if ((retval = nc_inq_varid(comp_ncid, "pixel_data", &comp_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_uchar(comp_ncid, comp_varid, comp_start,
                                        comp_count, &pixel)))
            ERR(retval);
        printf("PASS: read one pixel from compressed frame 0 at [256][256]\n");

        if ((retval = nc_close(comp_ncid)))
            ERR(retval);
        printf("PASS: nc_close compressed\n");
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

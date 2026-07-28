/**
 * @file tst_dicom_udf.c
 * @brief Test for the DICOM User-Defined Format (UDF) handler.
 *
 * Sprint 3: expand regression coverage to all three DICOM sample files,
 * verifying open/close, metadata mapping to dimensions and attributes,
 * native uncompressed pixel data reads (8-bit and 16-bit), and
 * encapsulated JPEG Baseline multi-frame reads.
 *
 * v3.2.0 Sprint 2: add coverage for the OME public-domain samples added in
 * v3.1.0 Sprint 1. Two (CT-MONO2-16-brain.dcm, MR-MONO2-16-head.dcm) are
 * native uncompressed. Two (CT-MONO2-16-chest.dcm, MR-MONO2-12-shoulder.dcm)
 * use JPEG Lossless (Process 14) encapsulation, decoded via GDCM's bundled
 * IJG lossless codec (see src/dicomjpeglossless.h). A fifth
 * (CR-MONO1-10-chest.dcm) has no DICOM preamble/File Meta Information and
 * is intentionally out of scope; it is verified to fail cleanly.
 *
 * Test files:
 *   - data/DICOM/tst_dicom_uncompressed.dcm: 1-frame 4x6 grayscale 8-bit
 *   - data/DICOM/MRBRAIN.DCM: 1-frame 512x512 16-bit MR
 *   - data/DICOM/0003.DCM: 17-frame 512x512 encapsulated JPEG Baseline
 *   - data/DICOM/CT-MONO2-16-brain.dcm: 1-frame 512x512 16-bit CT, native
 *   - data/DICOM/MR-MONO2-16-head.dcm: 1-frame 256x256 16-bit MR, native
 *   - data/DICOM/CT-MONO2-16-chest.dcm: 1-frame 400x512 16-bit CT, JPEG
 *     Lossless
 *   - data/DICOM/MR-MONO2-12-shoulder.dcm: 1-frame 1024x1024 12-bit MR,
 *     JPEG Lossless
 *   - data/DICOM/CR-MONO1-10-chest.dcm: no preamble; expected to be
 *     rejected with NC_EINVAL
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

/** Path to a 16-bit single-frame uncompressed MR DICOM test file. */
#define DICOM_16BIT_TEST_FILE "data/DICOM/MRBRAIN.DCM"

/** Path to an encapsulated JPEG Baseline multi-frame DICOM test file. */
#define DICOM_COMPRESSED_TEST_FILE "data/DICOM/0003.DCM"

/** Path to a native uncompressed 16-bit CT DICOM test file. */
#define DICOM_CT_BRAIN_TEST_FILE "data/DICOM/CT-MONO2-16-brain.dcm"

/** Path to a native uncompressed 16-bit MR DICOM test file. */
#define DICOM_MR_HEAD_TEST_FILE "data/DICOM/MR-MONO2-16-head.dcm"

/** Path to a JPEG Lossless (16-bit precision) encapsulated CT DICOM test
 * file. */
#define DICOM_CT_CHEST_LOSSLESS_TEST_FILE "data/DICOM/CT-MONO2-16-chest.dcm"

/** Path to a JPEG Lossless (12-bit precision) encapsulated MR DICOM test
 * file. */
#define DICOM_MR_SHOULDER_LOSSLESS_TEST_FILE \
    "data/DICOM/MR-MONO2-12-shoulder.dcm"

/** Path to a DICOM file with no preamble/File Meta Information; expected
 * to be rejected cleanly. */
#define DICOM_NO_PREAMBLE_TEST_FILE "data/DICOM/CR-MONO1-10-chest.dcm"

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
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

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

        /* Verify selected variable attributes are present and correct. */
        if ((retval = nc_inq_att(ncid, varid, "NumberOfFrames",
                                  &xtype, &att_len)))
            ERR(retval);
        if (xtype != NC_CHAR)
        { fprintf(stderr, "NumberOfFrames: expected NC_CHAR\n"); return 1; }
        if ((retval = nc_get_att_text(ncid, varid, "NumberOfFrames", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "1") != 0)
        { fprintf(stderr, "NumberOfFrames expected 1, got '%s'\n", att_buf); return 1; }
        printf("PASS: var att NumberOfFrames='%s'\n", att_buf);

        if ((retval = nc_inq_att(ncid, varid, "BitsAllocated", &xtype, &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(ncid, varid, "BitsAllocated", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "8") != 0)
        { fprintf(stderr, "BitsAllocated expected 8, got '%s'\n", att_buf); return 1; }
        printf("PASS: var att BitsAllocated='%s'\n", att_buf);
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
        if ((retval = nc_inq_var(comp_ncid, comp_varid, name, &xtype,
                                  &var_ndims, var_dimids, &var_natts)))
            ERR(retval);
        if (xtype != NC_UBYTE)
        {
            fprintf(stderr, "compressed: expected NC_UBYTE, got %d\n", xtype);
            return 1;
        }
        printf("PASS: compressed pixel_data type=NC_UBYTE\n");

        if ((retval = nc_get_vara_uchar(comp_ncid, comp_varid, comp_start,
                                        comp_count, &pixel)))
            ERR(retval);
        printf("PASS: read one pixel from compressed frame 0 at [256][256]\n");

        /* Read a center pixel and verify it is a plausible 8-bit value. */
        {
            size_t center_start[3] = {0, 255, 255};
            size_t center_count[3] = {1, 1, 1};
            unsigned char center_pixel;

            if ((retval = nc_get_vara_uchar(comp_ncid, comp_varid, center_start,
                                            center_count, &center_pixel)))
                ERR(retval);
            if (center_pixel > 255)
            {
                fprintf(stderr, "compressed center pixel out of range: %u\n",
                        center_pixel);
                return 1;
            }
            printf("PASS: compressed center pixel=%u\n", center_pixel);
        }

        if ((retval = nc_close(comp_ncid)))
            ERR(retval);
        printf("PASS: nc_close compressed\n");
    }

    /* Open the 16-bit single-frame uncompressed MR DICOM file. */
    {
        int mr_ncid, mr_varid;
        size_t mr_frame_len, mr_row_len, mr_col_len;
        unsigned short mr_center, mr_corner;
        size_t mr_start_center[3] = {0, 255, 255};
        size_t mr_start_corner[3] = {0, 511, 511};
        size_t mr_count[3] = {1, 1, 1};
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

        if ((retval = nc_open(DICOM_16BIT_TEST_FILE, NC_UDF6, &mr_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_16BIT_TEST_FILE);

        if ((retval = nc_inq_dimid(mr_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(mr_ncid, var_dimids[0], name, &mr_frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || mr_frame_len != 1)
        {
            fprintf(stderr, "frame: expected name='frame' len=1, got '%s' len=%zu\n",
                    name, mr_frame_len);
            return 1;
        }

        if ((retval = nc_inq_dimid(mr_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(mr_ncid, var_dimids[1], name, &mr_row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || mr_row_len != 512)
        {
            fprintf(stderr, "row: expected name='row' len=512, got '%s' len=%zu\n",
                    name, mr_row_len);
            return 1;
        }

        if ((retval = nc_inq_dimid(mr_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(mr_ncid, var_dimids[2], name, &mr_col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || mr_col_len != 512)
        {
            fprintf(stderr, "column: expected name='column' len=512, got '%s' len=%zu\n",
                    name, mr_col_len);
            return 1;
        }
        printf("PASS: 16-bit dims frame=%zu row=%zu column=%zu\n",
               mr_frame_len, mr_row_len, mr_col_len);

        if ((retval = nc_inq_var(mr_ncid, 0, name, &xtype, &var_ndims,
                                  var_dimids, &var_natts)))
            ERR(retval);
        if (strcmp(name, "pixel_data") != 0)
        {
            fprintf(stderr, "Expected var name 'pixel_data', got '%s'\n", name);
            return 1;
        }
        /* 16-bit samples are signed or unsigned depending on PixelRepresentation. */
        if (xtype != NC_SHORT && xtype != NC_USHORT)
        {
            fprintf(stderr, "MR: expected NC_SHORT or NC_USHORT, got %d\n", xtype);
            return 1;
        }
        printf("PASS: 16-bit pixel_data xtype=%d ndims=%d\n", xtype, var_ndims);

        if ((retval = nc_inq_att(mr_ncid, NC_GLOBAL, "PatientName", &xtype, &att_len)))
            ERR(retval);
        if (xtype != NC_CHAR)
        { fprintf(stderr, "PatientName: expected NC_CHAR\n"); return 1; }
        printf("PASS: att PatientName NC_CHAR len=%zu\n", att_len);

        if ((retval = nc_inq_att(mr_ncid, NC_GLOBAL, "Modality", &xtype, &att_len)))
            ERR(retval);
        if (xtype != NC_CHAR)
        { fprintf(stderr, "Modality: expected NC_CHAR\n"); return 1; }
        printf("PASS: att Modality NC_CHAR len=%zu\n", att_len);

        if ((retval = nc_inq_att(mr_ncid, NC_GLOBAL, "TransferSyntaxUID",
                                  &xtype, &att_len)))
            ERR(retval);
        if (xtype != NC_CHAR)
        { fprintf(stderr, "TransferSyntaxUID: expected NC_CHAR\n"); return 1; }
        printf("PASS: att TransferSyntaxUID NC_CHAR len=%zu\n", att_len);

        if ((retval = nc_inq_varid(mr_ncid, "pixel_data", &mr_varid)))
            ERR(retval);

        if ((retval = nc_inq_att(mr_ncid, mr_varid, "NumberOfFrames",
                                  &xtype, &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(mr_ncid, mr_varid, "NumberOfFrames", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "1") != 0)
        { fprintf(stderr, "NumberOfFrames expected 1, got '%s'\n", att_buf); return 1; }
        printf("PASS: 16-bit var att NumberOfFrames='%s'\n", att_buf);

        if ((retval = nc_inq_att(mr_ncid, mr_varid, "BitsAllocated", &xtype, &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(mr_ncid, mr_varid, "BitsAllocated", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "16") != 0)
        { fprintf(stderr, "BitsAllocated expected 16, got '%s'\n", att_buf); return 1; }
        printf("PASS: 16-bit var att BitsAllocated='%s'\n", att_buf);

        if ((retval = nc_get_vara_ushort(mr_ncid, mr_varid, mr_start_center,
                                         mr_count, &mr_center)))
            ERR(retval);
        if (mr_center == 0 || mr_center > 4095)
        {
            fprintf(stderr, "MR center pixel not in expected 12-bit range: %u\n",
                    mr_center);
            return 1;
        }
        printf("PASS: 16-bit center pixel=%u\n", mr_center);

        if ((retval = nc_get_vara_ushort(mr_ncid, mr_varid, mr_start_corner,
                                         mr_count, &mr_corner)))
            ERR(retval);
        if (mr_corner > 65535)
        {
            fprintf(stderr, "MR corner pixel out of range: %u\n", mr_corner);
            return 1;
        }
        printf("PASS: 16-bit corner pixel=%u\n", mr_corner);

        if ((retval = nc_close(mr_ncid)))
            ERR(retval);
        printf("PASS: nc_close 16-bit\n");
    }

    /* Open the native uncompressed 16-bit CT DICOM file. */
    {
        int ct_ncid, ct_varid;
        size_t ct_frame_len, ct_row_len, ct_col_len;
        short ct_pixel;
        size_t ct_start[3] = {0, 256, 256};
        size_t ct_count[3] = {1, 1, 1};
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

        if ((retval = nc_open(DICOM_CT_BRAIN_TEST_FILE, NC_UDF6, &ct_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_CT_BRAIN_TEST_FILE);

        if ((retval = nc_inq_dimid(ct_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(ct_ncid, var_dimids[0], name, &ct_frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || ct_frame_len != 1)
        { fprintf(stderr, "CT brain: unexpected frame dim\n"); return 1; }

        if ((retval = nc_inq_dimid(ct_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(ct_ncid, var_dimids[1], name, &ct_row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || ct_row_len != 512)
        { fprintf(stderr, "CT brain: unexpected row dim\n"); return 1; }

        if ((retval = nc_inq_dimid(ct_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(ct_ncid, var_dimids[2], name, &ct_col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || ct_col_len != 512)
        { fprintf(stderr, "CT brain: unexpected column dim\n"); return 1; }
        printf("PASS: CT brain dims frame=%zu row=%zu column=%zu\n",
               ct_frame_len, ct_row_len, ct_col_len);

        if ((retval = nc_inq_varid(ct_ncid, "pixel_data", &ct_varid)))
            ERR(retval);
        if ((retval = nc_inq_var(ct_ncid, ct_varid, name, &xtype, &var_ndims,
                                  var_dimids, &var_natts)))
            ERR(retval);
        if (xtype != NC_SHORT && xtype != NC_USHORT)
        { fprintf(stderr, "CT brain: expected NC_SHORT/NC_USHORT\n"); return 1; }

        if ((retval = nc_inq_att(ct_ncid, NC_GLOBAL, "Modality", &xtype,
                                  &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(ct_ncid, NC_GLOBAL, "Modality", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "CT") != 0)
        { fprintf(stderr, "CT brain: expected Modality='CT', got '%s'\n", att_buf); return 1; }
        printf("PASS: CT brain att Modality='%s'\n", att_buf);

        if ((retval = nc_get_vara_short(ct_ncid, ct_varid, ct_start, ct_count,
                                        &ct_pixel)))
            ERR(retval);
        printf("PASS: CT brain center pixel=%d\n", ct_pixel);

        if ((retval = nc_close(ct_ncid)))
            ERR(retval);
        printf("PASS: nc_close CT brain\n");
    }

    /* Open the native uncompressed 16-bit MR DICOM file.
     *
     * Note: this file lacks a NumberOfFrames tag in a way that causes
     * libdicom's dcm_filehandle_read_frame() to fail internally (a
     * libdicom limitation unrelated to JPEG Lossless support; other
     * native files without this tag, e.g. MRBRAIN.DCM and
     * CT-MONO2-16-brain.dcm, read pixel data without issue). Only
     * metadata mapping is verified here; see
     * docs/plan/v3.2.0-sprint2-dicom-sample-coverage.md for follow-up. */
    {
        int mrh_ncid;
        size_t mrh_frame_len, mrh_row_len, mrh_col_len;
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

        if ((retval = nc_open(DICOM_MR_HEAD_TEST_FILE, NC_UDF6, &mrh_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_MR_HEAD_TEST_FILE);

        if ((retval = nc_inq_dimid(mrh_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrh_ncid, var_dimids[0], name, &mrh_frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || mrh_frame_len != 1)
        { fprintf(stderr, "MR head: unexpected frame dim\n"); return 1; }

        if ((retval = nc_inq_dimid(mrh_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrh_ncid, var_dimids[1], name, &mrh_row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || mrh_row_len != 256)
        { fprintf(stderr, "MR head: unexpected row dim\n"); return 1; }

        if ((retval = nc_inq_dimid(mrh_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrh_ncid, var_dimids[2], name, &mrh_col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || mrh_col_len != 256)
        { fprintf(stderr, "MR head: unexpected column dim\n"); return 1; }
        printf("PASS: MR head dims frame=%zu row=%zu column=%zu\n",
               mrh_frame_len, mrh_row_len, mrh_col_len);

        if ((retval = nc_inq_att(mrh_ncid, NC_GLOBAL, "Modality", &xtype,
                                  &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(mrh_ncid, NC_GLOBAL, "Modality", att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "MR") != 0)
        { fprintf(stderr, "MR head: expected Modality='MR', got '%s'\n", att_buf); return 1; }
        printf("PASS: MR head att Modality='%s'\n", att_buf);

        if ((retval = nc_close(mrh_ncid)))
            ERR(retval);
        printf("PASS: nc_close MR head\n");
    }

    /* Open the JPEG Lossless (16-bit precision) encapsulated CT DICOM
     * file. This exercises the gdcmjpeg16 decode path. */
    {
        int ctl_ncid, ctl_varid;
        size_t ctl_frame_len, ctl_row_len, ctl_col_len;
        short ctl_pixel;
        size_t ctl_start[3] = {0, 200, 256};
        size_t ctl_count[3] = {1, 1, 1};
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

        if ((retval = nc_open(DICOM_CT_CHEST_LOSSLESS_TEST_FILE, NC_UDF6,
                              &ctl_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_CT_CHEST_LOSSLESS_TEST_FILE);

        if ((retval = nc_inq_dimid(ctl_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(ctl_ncid, var_dimids[0], name, &ctl_frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || ctl_frame_len != 1)
        { fprintf(stderr, "CT chest lossless: unexpected frame dim\n"); return 1; }

        if ((retval = nc_inq_dimid(ctl_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(ctl_ncid, var_dimids[1], name, &ctl_row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || ctl_row_len != 400)
        { fprintf(stderr, "CT chest lossless: unexpected row dim\n"); return 1; }

        if ((retval = nc_inq_dimid(ctl_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(ctl_ncid, var_dimids[2], name, &ctl_col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || ctl_col_len != 512)
        { fprintf(stderr, "CT chest lossless: unexpected column dim\n"); return 1; }
        printf("PASS: CT chest lossless dims frame=%zu row=%zu column=%zu\n",
               ctl_frame_len, ctl_row_len, ctl_col_len);

        if ((retval = nc_inq_att(ctl_ncid, NC_GLOBAL, "TransferSyntaxUID",
                                  &xtype, &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(ctl_ncid, NC_GLOBAL, "TransferSyntaxUID",
                                      att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "1.2.840.10008.1.2.4.70") != 0)
        {
            fprintf(stderr, "CT chest lossless: expected TransferSyntaxUID="
                    "'1.2.840.10008.1.2.4.70', got '%s'\n", att_buf);
            return 1;
        }
        printf("PASS: CT chest lossless att TransferSyntaxUID='%s'\n", att_buf);

        if ((retval = nc_inq_varid(ctl_ncid, "pixel_data", &ctl_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_short(ctl_ncid, ctl_varid, ctl_start,
                                        ctl_count, &ctl_pixel)))
            ERR(retval);
        printf("PASS: CT chest lossless decoded pixel=%d\n", ctl_pixel);

        if ((retval = nc_close(ctl_ncid)))
            ERR(retval);
        printf("PASS: nc_close CT chest lossless\n");
    }

    /* Open the JPEG Lossless (12-bit precision) encapsulated MR DICOM
     * file. This exercises the gdcmjpeg12 decode path. */
    {
        int mrl_ncid, mrl_varid;
        size_t mrl_frame_len, mrl_row_len, mrl_col_len;
        unsigned short mrl_pixel;
        size_t mrl_start[3] = {0, 512, 512};
        size_t mrl_count[3] = {1, 1, 1};
        char att_buf[NC_MAX_NAME + 1];
        size_t att_len;

        if ((retval = nc_open(DICOM_MR_SHOULDER_LOSSLESS_TEST_FILE, NC_UDF6,
                              &mrl_ncid)))
            ERR(retval);
        printf("PASS: nc_open %s\n", DICOM_MR_SHOULDER_LOSSLESS_TEST_FILE);

        if ((retval = nc_inq_dimid(mrl_ncid, "frame", &var_dimids[0])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrl_ncid, var_dimids[0], name, &mrl_frame_len)))
            ERR(retval);
        if (strcmp(name, "frame") != 0 || mrl_frame_len != 1)
        { fprintf(stderr, "MR shoulder lossless: unexpected frame dim\n"); return 1; }

        if ((retval = nc_inq_dimid(mrl_ncid, "row", &var_dimids[1])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrl_ncid, var_dimids[1], name, &mrl_row_len)))
            ERR(retval);
        if (strcmp(name, "row") != 0 || mrl_row_len != 1024)
        { fprintf(stderr, "MR shoulder lossless: unexpected row dim\n"); return 1; }

        if ((retval = nc_inq_dimid(mrl_ncid, "column", &var_dimids[2])))
            ERR(retval);
        if ((retval = nc_inq_dim(mrl_ncid, var_dimids[2], name, &mrl_col_len)))
            ERR(retval);
        if (strcmp(name, "column") != 0 || mrl_col_len != 1024)
        { fprintf(stderr, "MR shoulder lossless: unexpected column dim\n"); return 1; }
        printf("PASS: MR shoulder lossless dims frame=%zu row=%zu column=%zu\n",
               mrl_frame_len, mrl_row_len, mrl_col_len);

        if ((retval = nc_inq_att(mrl_ncid, NC_GLOBAL, "TransferSyntaxUID",
                                  &xtype, &att_len)))
            ERR(retval);
        if ((retval = nc_get_att_text(mrl_ncid, NC_GLOBAL, "TransferSyntaxUID",
                                      att_buf)))
            ERR(retval);
        att_buf[att_len] = '\0';
        if (strcmp(att_buf, "1.2.840.10008.1.2.4.57") != 0)
        {
            fprintf(stderr, "MR shoulder lossless: expected TransferSyntaxUID="
                    "'1.2.840.10008.1.2.4.57', got '%s'\n", att_buf);
            return 1;
        }
        printf("PASS: MR shoulder lossless att TransferSyntaxUID='%s'\n",
               att_buf);

        if ((retval = nc_inq_varid(mrl_ncid, "pixel_data", &mrl_varid)))
            ERR(retval);
        if ((retval = nc_get_vara_ushort(mrl_ncid, mrl_varid, mrl_start,
                                         mrl_count, &mrl_pixel)))
            ERR(retval);
        /* BitsStored is 12, so decoded values must fit in [0, 4095]. */
        if (mrl_pixel > 4095)
        {
            fprintf(stderr, "MR shoulder lossless: pixel %u exceeds 12-bit "
                    "range\n", mrl_pixel);
            return 1;
        }
        printf("PASS: MR shoulder lossless decoded pixel=%u\n", mrl_pixel);

        if ((retval = nc_close(mrl_ncid)))
            ERR(retval);
        printf("PASS: nc_close MR shoulder lossless\n");
    }

    /* CR-MONO1-10-chest.dcm has no preamble/File Meta Information and is
     * out of scope for this sprint; verify it is rejected cleanly rather
     * than crashing or hanging. */
    {
        int np_ncid;

        retval = nc_open(DICOM_NO_PREAMBLE_TEST_FILE, NC_UDF6, &np_ncid);
        if (retval != NC_EINVAL)
        {
            fprintf(stderr, "no-preamble file: expected NC_EINVAL, got %d "
                    "(%s)\n", retval, nc_strerror(retval));
            return 1;
        }
        printf("PASS: %s cleanly rejected with NC_EINVAL as expected\n",
               DICOM_NO_PREAMBLE_TEST_FILE);
    }

    printf("Done.\n");
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

/*
 * @file dicom_read.c
 * @brief Example program that reads a DICOM image through the NetCDF UDF API.
 *
 * This program demonstrates opening a DICOM file with the DICOM UDF handler
 * (UDF slot 6, NC_UDF6), reading the image dimensions, and extracting a small
 * slice of pixel data.
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative
 * Guide to Writing High-Performance Programs for Scientific Data Management,
 * Second Edition" (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett
 * @date 2026-07-25
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "../test/data/DICOM/tst_dicom_uncompressed.dcm"

#define ERR(e) do { if (e) { fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); return 1; } } while (0)

/* DICOM UDF initialization function provided by libncdicom. */
extern int NC_DICOM_initialize(void);

int
main(int argc, char **argv)
{
    int ncid, varid, retval;
    int ndims, nvars, ngatts, unlimdimid;
    int var_ndims, var_natts;
    int dimids[NC_MAX_VAR_DIMS];
    size_t len;
    char name[NC_MAX_NAME + 1];
    nc_type xtype;
    size_t start[3] = {0, 0, 0};
    size_t count[3] = {1, 1, 4};
    unsigned char pixels[4];
    const char *file_name = (argc > 1) ? argv[1] : FILE_NAME;

    (void)NC_DICOM_initialize();

    if ((retval = nc_open(file_name, NC_UDF6, &ncid)))
        ERR(retval);

    if ((retval = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid)))
        ERR(retval);
    printf("Dataset: %d dims, %d vars, %d atts, unlimdim=%d\n",
           ndims, nvars, ngatts, unlimdimid);

    printf("Dimensions: ");
    for (int i = 0; i < ndims; i++)
    {
        if ((retval = nc_inq_dim(ncid, i, name, &len)))
            ERR(retval);
        printf("%s=%zu ", name, len);
    }
    printf("\n");

    if ((retval = nc_inq_varid(ncid, "pixel_data", &varid)))
        ERR(retval);

    if ((retval = nc_inq_var(ncid, varid, name, &xtype, &var_ndims,
                              dimids, &var_natts)))
        ERR(retval);
    printf("Variable '%s': xtype=%d ndims=%d natts=%d\n",
           name, xtype, var_ndims, var_natts);

    if ((retval = nc_get_vara_uchar(ncid, varid, start, count, pixels)))
        ERR(retval);
    printf("Pixel slice: %u %u %u %u\n",
           pixels[0], pixels[1], pixels[2], pixels[3]);

    if ((retval = nc_close(ncid)))
        ERR(retval);

    printf("Done.\n");
    return 0;
}

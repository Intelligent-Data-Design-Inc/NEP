/**
 * @file nczarr_compression.c
 * @brief Demonstrate deflate+shuffle compression in a local NcZarr dataset.
 *
 * This example extends nczarr_chunking.c by adding compression filters on
 * top of explicit chunking. The recommended workflow for compressed storage
 * is: (1) set the chunk shape with nc_def_var_chunking(), then (2) enable
 * compression with nc_def_var_deflate(). Both steps must occur in define
 * mode before nc_enddef().
 *
 * The program creates the same 4x5 temperature grid with 2x5 chunks, then
 * applies the shuffle filter and deflate (zlib) compression at level 4.
 * After writing, it reopens the dataset and verifies the compression
 * metadata via nc_inq_var_deflate(), then reads and validates all data
 * values. Decompression is fully transparent — nc_get_var_float() returns
 * the original data without any extra API calls.
 *
 * **Learning Objectives:**
 * - Apply deflate compression and the shuffle filter to a NcZarr variable
 * - Understand the recommended order: set chunks first, then compression
 * - Query compression metadata (shuffle, deflate, level) after reopen
 * - Verify that decompression is transparent through the netCDF API
 *
 * **Key Concepts:**
 * - **nc_def_var_deflate(ncid, varid, shuffle, deflate, level)**:
 *   - shuffle=1: enables the shuffle filter (reorders bytes for better
 *     compression of typed data)
 *   - deflate=1: enables deflate (zlib) compression
 *   - level=0..9: compression level (0=none, 9=max; 4 is a balanced choice)
 * - **Chunk + compress workflow**: Compression requires chunked storage;
 *   always set chunk shape explicitly for predictable behavior
 * - **nc_inq_var_deflate()**: Returns current shuffle, deflate, and level
 *
 * **Related Examples:**
 * - nczarr_chunking.c - Chunking without compression
 * - nczarr_simple.c - Basic NcZarr create/read/write
 * - compression_performance.c - Comparing compression levels and filters
 *   (performance example)
 *
 * **Compilation:**
 * @code
 * gcc -o nczarr_compression nczarr_compression.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./nczarr_compression
 * ncdump 'file://nczarr_compression.zarr#mode=nczarr'
 * @endcode
 *
 * **Expected Output:**
 * Creates the directory nczarr_compression.zarr containing:
 * - 2 dimensions: y(4), x(5)
 * - 1 variable: temperature(y, x) of type NC_FLOAT, chunked [2, 5],
 *   with shuffle + deflate level 4
 * - Attributes: units="K", long_name="Temperature", _FillValue=-999.f
 * - A 4x5 temperature data grid stored compressed in 2 chunks
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026-06-26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_URL "file://nczarr_compression.zarr#mode=nczarr"
#define NY 4
#define NX 5
#define NDIMS 2
#define CHUNK_Y 2
#define CHUNK_X 5
#define DEFLATE_LEVEL 4

#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

int main(void)
{
    int ncid, dimids[NDIMS], varid, retval;
    int ndims_in, nvars_in;
    int shuffle_in, deflate_in, deflate_level_in;
    size_t i, j;
    size_t ylen, xlen, chunksizes[NDIMS];
    float fill_value = -999.0f;
    float data[NY][NX], data_in[NY][NX];

    /* Generate a simple 2D temperature field. */
    for (j = 0; j < NY; j++)
        for (i = 0; i < NX; i++)
            data[j][i] = 280.0f + (float)j * 2.0f + (float)i * 0.5f;

    /* Create a local NcZarr dataset with chunking and compression. */
    if ((retval = nc_create(FILE_URL, NC_CLOBBER | NC_NETCDF4, &ncid)))
        ERR(retval);

    if ((retval = nc_def_dim(ncid, "y", NY, &dimids[0])))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "x", NX, &dimids[1])))
        ERR(retval);

    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
        ERR(retval);

    /* Step 1: Set chunk shape — required before compression. */
    chunksizes[0] = CHUNK_Y;
    chunksizes[1] = CHUNK_X;
    if ((retval = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)))
        ERR(retval);

    /* Step 2: Enable shuffle filter and deflate compression (level 4). */
    if ((retval = nc_def_var_deflate(ncid, varid, 1, 1, DEFLATE_LEVEL)))
        ERR(retval);

    if ((retval = nc_put_att_text(ncid, varid, "units", 1, "K")))
        ERR(retval);
    if ((retval = nc_put_att_text(ncid, varid, "long_name", 11, "Temperature")))
        ERR(retval);
    if ((retval = nc_put_att_float(ncid, varid, "_FillValue", NC_FLOAT, 1, &fill_value)))
        ERR(retval);

    if ((retval = nc_enddef(ncid)))
        ERR(retval);
    if ((retval = nc_put_var_float(ncid, varid, &data[0][0])))
        ERR(retval);
    if ((retval = nc_close(ncid)))
        ERR(retval);

    printf("Created %s with deflate=%d, shuffle=on\n", FILE_URL, DEFLATE_LEVEL);

    /* Reopen and verify compression metadata. */
    if ((retval = nc_open(FILE_URL, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq(ncid, &ndims_in, &nvars_in, NULL, NULL)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, 0, &ylen)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, 1, &xlen)))
        ERR(retval);

    printf("Dataset: %d dims, %d vars, y=%zu, x=%zu\n", ndims_in, nvars_in, ylen, xlen);

    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    if ((retval = nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &deflate_level_in)))
        ERR(retval);

    printf("Compression: shuffle=%s, deflate=%s, level=%d\n",
           shuffle_in ? "on" : "off",
           deflate_in ? "on" : "off",
           deflate_level_in);

    if (!shuffle_in || !deflate_in || deflate_level_in != DEFLATE_LEVEL) {
        fprintf(stderr, "Compression metadata mismatch\n");
        return 1;
    }

    if ((retval = nc_get_var_float(ncid, varid, &data_in[0][0])))
        ERR(retval);
    if ((retval = nc_close(ncid)))
        ERR(retval);

    /* Verify data. */
    for (j = 0; j < NY; j++)
        for (i = 0; i < NX; i++)
            if (data_in[j][i] != data[j][i]) {
                fprintf(stderr, "Mismatch at %zu,%zu: got %f, expected %f\n",
                        j, i, data_in[j][i], data[j][i]);
                return 1;
            }

    printf("Read %d values, all correct. Done.\n", NY * NX);
    return 0;
}

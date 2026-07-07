/**
 * @file nczarr_chunking.c
 * @brief Demonstrate chunked storage in a local NcZarr dataset.
 *
 * This example extends nczarr_simple.c by adding explicit chunk shape
 * selection. Chunking controls how a variable's data is split into
 * separate storage units (chunks) in the underlying Zarr directory tree.
 * Each chunk becomes a separate file on disk (or object in cloud storage),
 * so the chunk shape directly affects I/O access patterns.
 *
 * The program creates the same 4x5 temperature grid used in nczarr_simple.c
 * but specifies a 2x5 (y x x) chunk shape using nc_def_var_chunking().
 * This splits the 4-row array into two 2-row chunks. After writing, the
 * program reopens the dataset and verifies the chunk metadata via
 * nc_inq_var_chunking(), then reads and validates all data values.
 *
 * **Learning Objectives:**
 * - Set explicit chunk shape for a variable in a NcZarr dataset
 * - Query chunk metadata (storage type and chunk dimensions) after reopen
 * - Understand that the same nc_def_var_chunking() / nc_inq_var_chunking()
 *   APIs used for NetCDF-4/HDF5 work identically with NcZarr
 * - Observe that each chunk maps to a separate file in the Zarr store
 *
 * **Key Concepts:**
 * - **NC_CHUNKED**: Storage mode that splits data into fixed-size chunks
 * - **nc_def_var_chunking()**: Sets the chunk shape (must be called in
 *   define mode, before nc_enddef)
 * - **nc_inq_var_chunking()**: Returns storage type and chunk dimensions
 * - **Chunk alignment**: Best practice is to choose chunk dimensions that
 *   evenly divide the variable dimensions
 *
 * **Related Examples:**
 * - nczarr_simple.c - Basic NcZarr create/read/write (default chunking)
 * - nczarr_compression.c - Adding compression on top of chunking
 * - chunking_performance.c - Comparing access patterns with different
 *   chunk shapes (performance example)
 *
 * **Compilation:**
 * @code
 * gcc -o nczarr_chunking nczarr_chunking.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./nczarr_chunking
 * ncdump 'file://nczarr_chunking.zarr#mode=nczarr'
 * @endcode
 *
 * **Expected Output:**
 * Creates the directory nczarr_chunking.zarr containing:
 * - 2 dimensions: y(4), x(5)
 * - 1 variable: temperature(y, x) of type NC_FLOAT with chunks [2, 5]
 * - Attributes: units="K", long_name="Temperature", _FillValue=-999.f
 * - A 4x5 temperature data grid stored in 2 chunks of 2x5 each
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
 * High-Performance Programs for Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026-06-26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_URL "file://nczarr_chunking.zarr#mode=nczarr"
#define NY 4
#define NX 5
#define NDIMS 2
#define CHUNK_Y 2
#define CHUNK_X 5

#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

int main(void)
{
    int ncid, dimids[NDIMS], varid, retval;
    int ndims_in, nvars_in, storage_in;
    size_t i, j;
    size_t ylen, xlen, chunksizes[NDIMS], chunks_in[NDIMS];
    float fill_value = -999.0f;
    float data[NY][NX], data_in[NY][NX];

    /* Generate a simple 2D temperature field. */
    for (j = 0; j < NY; j++)
        for (i = 0; i < NX; i++)
            data[j][i] = 280.0f + (float)j * 2.0f + (float)i * 0.5f;

    /* Create a local NcZarr dataset with explicit chunking. */
    if ((retval = nc_create(FILE_URL, NC_CLOBBER | NC_NETCDF4, &ncid)))
        ERR(retval);

    if ((retval = nc_def_dim(ncid, "y", NY, &dimids[0])))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "x", NX, &dimids[1])))
        ERR(retval);

    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
        ERR(retval);

    /* Set chunk sizes: 2 rows x 5 columns per chunk. */
    chunksizes[0] = CHUNK_Y;
    chunksizes[1] = CHUNK_X;
    if ((retval = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes)))
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

    printf("Created %s with chunks [%d, %d]\n", FILE_URL, CHUNK_Y, CHUNK_X);

    /* Reopen and verify chunking metadata. */
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
    if ((retval = nc_inq_var_chunking(ncid, varid, &storage_in, chunks_in)))
        ERR(retval);

    printf("Chunking: storage=%s, chunks=[%zu, %zu]\n",
           storage_in == NC_CHUNKED ? "chunked" : "contiguous",
           chunks_in[0], chunks_in[1]);

    if (storage_in != NC_CHUNKED || chunks_in[0] != CHUNK_Y || chunks_in[1] != CHUNK_X) {
        fprintf(stderr, "Chunk metadata mismatch\n");
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

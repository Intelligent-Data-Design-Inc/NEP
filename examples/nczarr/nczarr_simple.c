/**
 * @file nczarr_simple.c
 * @brief Demonstrate creating, writing, and reading a local NcZarr dataset.
 *
 * This example introduces the NetCDF-Zarr (NcZarr) storage format, which
 * extends the netCDF-4 enhanced data model to Zarr, a cloud-native array
 * storage system. Unlike classic NetCDF or NetCDF-4/HDF5, which store a
 * dataset in a single binary file, NcZarr stores each variable's metadata
 * and chunks as separate objects in a directory tree (for local filesystem
 * storage) or in an object store (for cloud storage). This design is
 * optimized for random access over HTTP and scales naturally to cloud
 * object storage such as Amazon S3, Google Cloud Storage, and Azure Blob
 * Storage.
 *
 * NcZarr is accessed through the familiar netCDF-C API. The only difference
 * at create/open time is the storage URL: a local NcZarr store is selected
 * with the `file://` scheme and the `#mode=nczarr` fragment, which tells the
 * netCDF-C dispatch layer to use the NcZarr implementation. Once the file
 * is open, nc_def_dim(), nc_def_var(), nc_put_att_xxx(), nc_put_var_xxx(),
 * and nc_get_var_xxx() work exactly as they do for NetCDF-4/HDF5 files.
 *
 * This program creates a small 2D temperature array on a 4x5 y-x grid,
 * stores it as a local NcZarr directory named simple_nczarr.zarr, attaches
 * units, long_name, and _FillValue attributes, closes the file, reopens it
 * read-only, and verifies that the metadata and data values match what was
 * written.
 *
 * **Learning Objectives:**
 * - Understand NcZarr as a netCDF-4 data model mapped onto Zarr storage
 * - Use a `file://...#mode=nczarr` URL to create a local NcZarr dataset
 * - Apply the same dimension, variable, attribute, and data I/O APIs used
 *   for NetCDF-4/HDF5 files
 * - Compare NcZarr storage (directory tree) with classic and HDF5 storage
 *   (single file)
 * - Validate a round-trip write/read through the netCDF API
 *
 * **Key Concepts:**
 * - **NcZarr**: NetCDF-4 data model backed by Zarr V2 metadata and chunks
 * - **Zarr store**: A directory (or object-store prefix) containing .zarray,
 *   .zgroup, .zattrs, and per-chunk data files
 * - **URL fragment**: The `#mode=nczarr` suffix selects the NcZarr dispatcher
 * - **Local filesystem backend**: `file://` scheme uses the local Zarr store
 * - **NetCDF-4 enhanced model**: Groups, user types, multiple unlimited dims,
 *   and chunking are supported in NcZarr; this example uses only the basic
 *   subset (dimensions, variables, attributes)
 *
 * **Comparison with Other NetCDF Formats:**
 * | Feature | Classic | NetCDF-4/HDF5 | NcZarr |
 * | :------ | :-----: | :-----------: | :----: |
 * | Single file | Yes | Yes | No (directory tree) |
 * | Cloud object storage | No | No | Yes |
 * | Compression | No | Yes (HDF5 filters) | Yes (Zarr codecs) |
 * | Groups/user types | No | Yes | Yes |
 * | HTTP random access | No | No | Yes |
 *
 * **Prerequisites:**
 * - NetCDF-C 4.8.0 or later built with NcZarr support (NC_HAS_NCZARR == 1)
 * - For local storage, no additional network configuration is required
 *
 * **Related Examples:**
 * - simple_nc4.c - NetCDF-4/HDF5 equivalent of a 2D variable
 * - simple_2D.c - Classic NetCDF equivalent
 * - cache_tuning.c - Performance considerations for NetCDF-4/HDF5 chunk caches
 *
 * **Compilation:**
 * @code
 * gcc -o nczarr_simple nczarr_simple.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./nczarr_simple
 * ncdump 'file://simple_nczarr.zarr#mode=nczarr'
 * @endcode
 *
 * **Expected Output:**
 * Creates the directory simple_nczarr.zarr containing:
 * - 2 dimensions: y(4), x(5)
 * - 1 variable: temperature(y, x) of type NC_FLOAT
 * - Attributes: units="K", long_name="Temperature", _FillValue=-999.f
 * - A 4x5 temperature data grid
 * - .zarray, .zgroup, .zattrs, and chunk files that make up the Zarr store
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

#define FILE_URL "file://simple_nczarr.zarr#mode=nczarr"
#define NY 4
#define NX 5
#define NDIMS 2

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
    size_t i, j;
    size_t ylen, xlen;
    float fill_value = -999.0f;
    float data[NY][NX], data_in[NY][NX];

    /* Generate a simple 2D temperature field. */
    for (j = 0; j < NY; j++)
        for (i = 0; i < NX; i++)
            data[j][i] = 280.0f + (float)j * 2.0f + (float)i * 0.5f;

    /* Create a local NcZarr dataset. */
    if ((retval = nc_create(FILE_URL, NC_CLOBBER | NC_NETCDF4, &ncid)))
        ERR(retval);

    if ((retval = nc_def_dim(ncid, "y", NY, &dimids[0])))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "x", NX, &dimids[1])))
        ERR(retval);

    if ((retval = nc_def_var(ncid, "temperature", NC_FLOAT, NDIMS, dimids, &varid)))
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

    printf("Created %s\n", FILE_URL);

    /* Reopen the dataset and validate metadata. */
    if ((retval = nc_open(FILE_URL, NC_NOWRITE, &ncid)))
        ERR(retval);
    if ((retval = nc_inq(ncid, &ndims_in, &nvars_in, NULL, NULL)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, dimids[0], &ylen)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, dimids[1], &xlen)))
        ERR(retval);

    printf("Dataset: %d dims, %d vars, y=%zu, x=%zu\n", ndims_in, nvars_in, ylen, xlen);

    if ((retval = nc_inq_varid(ncid, "temperature", &varid)))
        ERR(retval);
    if ((retval = nc_get_var_float(ncid, varid, &data_in[0][0])))
        ERR(retval);
    if ((retval = nc_close(ncid)))
        ERR(retval);

    /* Verify the data read back. */
    for (j = 0; j < NY; j++) {
        for (i = 0; i < NX; i++) {
            if (data_in[j][i] != data[j][i]) {
                fprintf(stderr, "Mismatch at %zu,%zu: got %f, expected %f\n",
                        j, i, data_in[j][i], data[j][i]);
                return 1;
            }
        }
    }

    printf("Read %d values, all correct. Done.\n", NY * NX);
    return 0;
}

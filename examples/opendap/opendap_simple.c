/* Copyright 2026, Intelligent Data Design Inc. All rights reserved. */
/* See the COPYRIGHT file for copyright and license information. */

/**
 * @file opendap_simple.c
 * @brief Simple OPeNDAP example - open, read, and close a remote dataset.
 *
 * This is the most basic OPeNDAP example, demonstrating the core concept:
 * remote datasets are accessed using the same NetCDF API calls as local files.
 *
 * What this example does:
 * 1. Opens a remote sea surface temperature dataset from the public
 *    OPeNDAP test server at test.opendap.org
 * 2. Queries dataset metadata (dimensions, variables, attributes)
 * 3. Reads information about the 'sst' variable and its dimensions
 * 4. Reads a small 5x5 spatial subset from the first time step
 * 5. Closes the remote connection
 *
 * Key OPeNDAP concepts demonstrated:
 * - Using an HTTP URL instead of a local file path in nc_open()
 * - The NetCDF library handles all HTTP communication transparently
 * - Data is transferred over the network only when read functions are called
 * - Subsetting with start/count arrays reduces data transfer
 *
 * The dataset accessed is sst.mnmean.nc.gz, a classic netCDF-3 file containing
 * NOAA Extended Reconstructed Sea Surface Temperature data. The server
 * transparently decompresses and serves the data via OPeNDAP.
 *
 * **Learning Objectives:**
 * - Open a remote dataset by passing an HTTP URL to nc_open()
 * - Query remote dataset metadata (dimensions, variables, attributes)
 * - Read a spatial subset using start/count arrays to minimize data transfer
 * - Understand that the netCDF API is transport-agnostic (local file vs remote URL)
 * - Recognize that data transfer occurs only on nc_get_var_*() calls, not on open
 *
 * **Key Concepts:**
 * - **OPeNDAP**: Open-source protocol for accessing remote scientific data via HTTP
 * - **Transparent Access**: nc_open() with a URL triggers the OPeNDAP dispatch layer;
 *   all subsequent nc_*() calls work identically to local file access
 * - **Lazy Data Transfer**: Opening a URL fetches only metadata (DAS/DDS); actual
 *   array data is transferred only when nc_get_var_*() is called
 * - **Client-Side Subsetting**: Use start[] and count[] arrays in nc_get_vara_*()
 *   to request only needed data, reducing network bandwidth
 * - **NC_NOWRITE**: Remote datasets are always read-only; this flag is required
 *
 * **Prerequisites:**
 * - simple_2D.c - Basic nc_open/nc_inq/nc_get_var workflow
 * - coord_vars.c - Understanding dimension and variable relationships
 * - A NetCDF-C library built with OPeNDAP support (NC_HAS_DAP2 or NC_HAS_DAP4)
 *
 * **Related Examples:**
 * - opendap_subset.c - Client-side subsetting with multiple access patterns
 * - opendap_constraint.c - Server-side subsetting via constraint expressions
 *
 * **Compilation:**
 * @code
 * gcc -o opendap_simple opendap_simple.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./opendap_simple
 * @endcode
 *
 * **Expected Output:**
 * - Prints dataset metadata (dimensions, variables, attributes)
 * - Prints dimension names and sizes
 * - Reads and displays a 5x5 spatial subset of SST from the first time step
 *
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

/* Error handling macro like coord.c */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    int ncid, varid, retval;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    int var_ndims, var_natts;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;
    char var_name[NC_MAX_NAME + 1];
    char dim_name[NC_MAX_NAME + 1];
    
    /* Public OPeNDAP test server dataset - Sea Surface Temperature */
    const char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    printf("OPeNDAP Simple Example: %s\n\n", url);

    /* Open the remote dataset */
    if ((retval = nc_open(url, NC_NOWRITE, &ncid)))
        ERR(retval);

    /* Query and print dataset metadata compactly */
    if ((retval = nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid)))
        ERR(retval);
    printf("Dataset: %d dims, %d vars, %d atts, unlimdim=%d\n", ndims, nvars, natts, unlimdimid);

    /* Query and print dimensions compactly */
    printf("Dimensions: ");
    for (int i = 0; i < ndims; i++) {
        if ((retval = nc_inq_dim(ncid, i, dim_name, &dimlen)))
            ERR(retval);
        printf("%s=%zu ", dim_name, dimlen);
    }
    printf("\n\n");
    
    /* Get variable 'sst' info and read subset */
    if (nc_inq_varid(ncid, "sst", &varid) == NC_NOERR) {
        if ((retval = nc_inq_var(ncid, varid, var_name, &xtype, &var_ndims, var_dimids, &var_natts)))
            ERR(retval);

        /* Print variable info compactly */
        printf("Variable '%s': type=%d, ndims=%d, natts=%d, shape=[", var_name, xtype, var_ndims, var_natts);
        for (int i = 0; i < var_ndims; i++) {
            if ((retval = nc_inq_dimlen(ncid, var_dimids[i], &dimlen)))
                ERR(retval);
            printf("%zu%s", dimlen, (i < var_ndims - 1) ? "," : "");
        }
        printf("]\n\n");

        /* Read a 5x5 spatial subset from first time step */
        {
            size_t start[3] = {0, 0, 0};
            size_t count[3] = {1, 5, 5};
            float data[5][5];

            printf("Reading subset [0:0][0:4][0:4]:\n");
            if ((retval = nc_get_vara_float(ncid, varid, start, count, &data[0][0])))
                ERR(retval);

            for (int i = 0; i < 5; i++) {
                printf("  ");
                for (int j = 0; j < 5; j++)
                    printf("%.2f ", data[i][j]);
                printf("\n");
            }
        }
    } else {
        printf("Variable 'sst' not found.\n");
    }
    
    /* Close the dataset */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("\nDone.\n");
    
    return 0;
}

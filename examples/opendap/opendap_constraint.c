/* Copyright 2026, Intelligent Data Design Inc. All rights reserved. */
/* See the COPYRIGHT file for copyright and license information. */

/**
 * @file opendap_constraint.c
 * @brief OPeNDAP constraint expression example - server-side subsetting.
 *
 * This example demonstrates server-side subsetting using OPeNDAP constraint
 * expressions appended to URLs. The server extracts only the requested data,
 * significantly reducing network bandwidth compared to downloading full files.
 *
 * What this example does:
 * 1. Constructs a constrained URL by appending ?sst[0:2][0:88][0:179] to the
 *    base URL, requesting only the first 3 time steps of the SST variable
 * 2. Opens the constrained dataset - the server sees only the subset
 * 3. Demonstrates that dimension sizes reflect the CONSTRAINED subset,
 *    not the original full dataset dimensions
 * 4. Reads the entire (already subsetted) variable data
 * 5. Reopens with a stride constraint [0:2:12] to sample every 2nd time step
 *
 * Constraint expression syntax:
 *   ?variable[start:stop]         - Range selection (inclusive)
 *   ?variable[start:stride:stop]  - Range with stride
 *   ?var1[...],var2[...]          - Multiple variables
 *
 * Key concept: With constraints, nc_inq_dimlen() returns the SUBSET size, not
 * the original dataset size. This is different from client-side subsetting
 * where the full dataset is opened and start/count arrays select subsets.
 *
 * When to use constraints:
 * - When you know exactly what subset you need before opening
 * - For large reductions in data size (e.g., one region from global data)
 * - When making a single request for a specific time/space subset
 *
 * **Learning Objectives:**
 * - Construct OPeNDAP constraint expressions appended to URLs
 * - Understand that constrained dimensions report subset sizes, not full sizes
 * - Use range selection [start:stop] and stride [start:stride:stop] syntax
 * - Compare server-side constraints with client-side subsetting (start/count)
 * - Recognize when server-side subsetting is more bandwidth-efficient
 *
 * **Key Concepts:**
 * - **Constraint Expression**: URL suffix (?var[start:stop]) that tells the server
 *   to extract and return only the specified subset
 * - **Server-Side Subsetting**: The OPeNDAP server processes the constraint and
 *   sends only the requested data — reduces network transfer significantly
 * - **Dimension Size After Constraint**: nc_inq_dimlen() returns the constrained
 *   subset size, not the original full dimension size
 * - **Stride Selection**: [start:stride:stop] samples every Nth element along
 *   a dimension, useful for quick-look previews of large datasets
 * - **Multiple Variables**: Comma-separated constraints (?var1[...],var2[...])
 *   can select subsets of multiple variables in one open
 *
 * **Prerequisites:**
 * - opendap_simple.c - Basic OPeNDAP access pattern
 * - simple_2D.c - Understanding start/count arrays for local subsetting
 * - A NetCDF-C library built with OPeNDAP support (NC_HAS_DAP2 or NC_HAS_DAP4)
 *
 * **Related Examples:**
 * - opendap_simple.c - Simplest OPeNDAP access (no constraints)
 * - opendap_subset.c - Client-side subsetting (multiple requests, one open)
 *
 * **Compilation:**
 * @code
 * gcc -o opendap_constraint opendap_constraint.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./opendap_constraint
 * @endcode
 *
 * **Expected Output:**
 * - Opens a constrained URL requesting first 3 time steps of SST
 * - Prints constrained dimension sizes (subset, not full dataset)
 * - Reads the entire constrained variable
 * - Reopens with a stride constraint to sample every 2nd time step
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
 * High-Performance Programs for Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int var_ndims;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;
    char dim_name[NC_MAX_NAME + 1];
    
    /* Base URL for the test dataset */
    const char *base_url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    /* URL with constraint expression - get first 3 time steps, all lat/lon */
    char url[512];
    snprintf(url, sizeof(url), "%s?sst[0:2][0:88][0:179]", base_url);
    
    printf("OPeNDAP Constraint Example: %s\n\n", url);

    /* Open constrained dataset */
    if ((retval = nc_open(url, NC_NOWRITE, &ncid)))
        ERR(retval);

    /* Get variable and constrained dimension info */
    if ((retval = nc_inq_varid(ncid, "sst", &varid)))
        ERR(retval);
    if ((retval = nc_inq_varndims(ncid, varid, &var_ndims)))
        ERR(retval);
    if ((retval = nc_inq_vardimid(ncid, varid, var_dimids)))
        ERR(retval);

    /* Print constrained dimensions (these are the SUBSET sizes) */
    printf("Constrained dimensions (subset sizes): ");
    for (int i = 0; i < var_ndims; i++) {
        if ((retval = nc_inq_dim(ncid, var_dimids[i], dim_name, &dimlen)))
            ERR(retval);
        printf("%s=%zu ", dim_name, dimlen);
    }
    printf("\n\n");
    
    /* Read and display sample of constrained data */
    {
        size_t total_elements = 1;
        for (int i = 0; i < var_ndims; i++) {
            if ((retval = nc_inq_dimlen(ncid, var_dimids[i], &dimlen)))
                ERR(retval);
            total_elements *= dimlen;
        }
        printf("Reading %zu elements...\n", total_elements);

        float *data = malloc(total_elements * sizeof(float));
        if (!data) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }

        if ((retval = nc_get_var_float(ncid, varid, data)))
            ERR(retval);

        /* Get lat dimension size for 2D indexing */
        if ((retval = nc_inq_dimlen(ncid, var_dimids[1], &dimlen)))
            ERR(retval);

        printf("Sample [0:4][0:4] of first time step:\n");
        for (int i = 0; i < 5 && i < (int)dimlen; i++) {
            printf("  ");
            for (int j = 0; j < 5; j++)
                printf("%.2f ", data[i * dimlen + j]);
            printf("\n");
        }
        free(data);
    }
    
    /* Demonstrate stride constraint */
    printf("\nStride constraint: %s\n", url);
    if ((retval = nc_close(ncid)))
        ERR(retval);
    snprintf(url, sizeof(url), "%s?sst[0:2:12][0:88][0:179]", base_url);
    if ((retval = nc_open(url, NC_NOWRITE, &ncid)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "sst", &varid)))
        ERR(retval);
    if ((retval = nc_inq_vardimid(ncid, varid, var_dimids)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, var_dimids[0], &dimlen)))
        ERR(retval);
    printf("Time dimension with stride: %zu (was 3 without stride)\n", dimlen);

    if ((retval = nc_close(ncid)))
        ERR(retval);
    printf("\nDone.\n");
    
    return 0;
}

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
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>

/* Error handling macro */
#define NC_CHK(ret) do { \
    if ((ret) != NC_NOERR) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(ret), __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    int ncid, varid;
    int ndims, nvars, natts, unlimdimid;
    nc_type xtype;
    int var_ndims, var_natts;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;
    char var_name[NC_MAX_NAME + 1];
    char dim_name[NC_MAX_NAME + 1];
    
    /* Public OPeNDAP test server dataset - Sea Surface Temperature */
    const char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    printf("OPeNDAP Simple Example\n");
    printf("======================\n\n");
    printf("Opening remote dataset:\n  %s\n\n", url);
    
    /* Open the remote dataset */
    NC_CHK(nc_open(url, NC_NOWRITE, &ncid));
    printf("Dataset opened successfully.\n\n");
    
    /* Query dataset metadata */
    NC_CHK(nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid));
    printf("Dataset contains:\n");
    printf("  Dimensions: %d\n", ndims);
    printf("  Variables: %d\n", nvars);
    printf("  Global attributes: %d\n", natts);
    printf("  Unlimited dimension ID: %d\n\n", unlimdimid);
    
    /* Query dimension information */
    printf("Dimensions:\n");
    for (int i = 0; i < ndims; i++) {
        NC_CHK(nc_inq_dim(ncid, i, dim_name, &dimlen));
        printf("  [%d] %s = %zu\n", i, dim_name, dimlen);
    }
    printf("\n");
    
    /* Get variable ID for 'sst' (sea surface temperature) */
    if (nc_inq_varid(ncid, "sst", &varid) == NC_NOERR) {
        printf("Found variable 'sst' (ID: %d)\n", varid);
        
        /* Query variable information */
        NC_CHK(nc_inq_var(ncid, varid, var_name, &xtype, &var_ndims, var_dimids, &var_natts));
        printf("  Type: %d\n", xtype);
        printf("  Number of dimensions: %d\n", var_ndims);
        printf("  Number of attributes: %d\n", var_natts);
        printf("  Dimension IDs: ");
        for (int i = 0; i < var_ndims; i++) {
            printf("%d ", var_dimids[i]);
        }
        printf("\n");
        
        /* Get dimension sizes for this variable */
        printf("  Shape: [");
        for (int i = 0; i < var_ndims; i++) {
            NC_CHK(nc_inq_dimlen(ncid, var_dimids[i], &dimlen));
            printf("%zu", dimlen);
            if (i < var_ndims - 1) printf(", ");
        }
        printf("]\n\n");
        
        /* Read a small subset of the data (first time step, small spatial subset) */
        {
            size_t start[3] = {0, 0, 0};
            size_t count[3] = {1, 5, 5};
            float data[5][5];
            
            printf("Reading subset [0:0][0:4][0:4] (1 time step, 5x5 spatial):\n");
            NC_CHK(nc_get_vara_float(ncid, varid, start, count, &data[0][0]));
            
            printf("  Sample values:\n");
            for (int i = 0; i < 5; i++) {
                printf("    Row %d: ", i);
                for (int j = 0; j < 5; j++) {
                    printf("%.2f ", data[i][j]);
                }
                printf("\n");
            }
        }
    } else {
        printf("Variable 'sst' not found in dataset.\n");
    }
    
    /* Close the dataset */
    NC_CHK(nc_close(ncid));
    printf("\nDataset closed successfully.\n");
    
    return 0;
}

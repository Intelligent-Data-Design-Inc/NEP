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
 * @author Edward Hartnett
 * @date 6/15/26
 */

#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int var_ndims;
    int var_dimids[NC_MAX_VAR_DIMS];
    size_t dimlen;
    char dim_name[NC_MAX_NAME + 1];
    
    /* Base URL for the test dataset */
    const char *base_url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    /* URL with constraint expression - get first 3 time steps, all lat/lon */
    char url[512];
    snprintf(url, sizeof(url), "%s?sst[0:2][0:88][0:179]", base_url);
    
    printf("OPeNDAP Constraint Expression Example\n");
    printf("=====================================\n\n");
    printf("Base URL: %s\n", base_url);
    printf("Constraint: sst[0:2][0:88][0:179]\n");
    printf("Full URL: %s\n\n", url);
    
    printf("Opening dataset with constraint expression...\n");
    NC_CHK(nc_open(url, NC_NOWRITE, &ncid));
    printf("Dataset opened successfully.\n\n");
    
    /* Get variable information */
    NC_CHK(nc_inq_varid(ncid, "sst", &varid));
    NC_CHK(nc_inq_varndims(ncid, varid, &var_ndims));
    printf("Variable 'sst' has %d dimensions:\n", var_ndims);
    
    /* Query the constrained dimensions */
    NC_CHK(nc_inq_vardimid(ncid, varid, var_dimids));
    for (int i = 0; i < var_ndims; i++) {
        NC_CHK(nc_inq_dim(ncid, var_dimids[i], dim_name, &dimlen));
        printf("  Dimension %d: %s = %zu\n", i, dim_name, dimlen);
    }
    printf("\n");
    
    printf("Note: The dimension sizes reflect the CONSTRAINTED subset,\n");
    printf("not the original full dataset.\n\n");
    
    /* Read all the constrained data */
    {
        /* Get total size */
        size_t total_elements = 1;
        for (int i = 0; i < var_ndims; i++) {
            NC_CHK(nc_inq_dimlen(ncid, var_dimids[i], &dimlen));
            total_elements *= dimlen;
        }
        printf("Reading %zu total elements...\n", total_elements);
        
        /* Allocate and read */
        float *data = malloc(total_elements * sizeof(float));
        if (!data) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        
        NC_CHK(nc_get_var_float(ncid, varid, data));
        
        /* Print sample of first time step */
        printf("\nSample data from first time step [0:4][0:4]:\n");
        NC_CHK(nc_inq_dimlen(ncid, var_dimids[1], &dimlen)); /* lat dimension size */
        size_t lat_size = dimlen;
        
        for (int i = 0; i < 5 && i < (int)lat_size; i++) {
            printf("  Lat %d: ", i);
            for (int j = 0; j < 5; j++) {
                printf("%.2f ", data[i * lat_size + j]);
            }
            printf("\n");
        }
        
        free(data);
    }
    
    /* Demonstrate another constraint with stride */
    printf("\n----------------------------------------\n\n");
    snprintf(url, sizeof(url), "%s?sst[0:2:12][0:88][0:179]", base_url);
    printf("New constraint with stride (every 2nd time step):\n");
    printf("URL: %s\n\n", url);
    
    NC_CHK(nc_close(ncid));
    NC_CHK(nc_open(url, NC_NOWRITE, &ncid));
    
    NC_CHK(nc_inq_varid(ncid, "sst", &varid));
    NC_CHK(nc_inq_vardimid(ncid, varid, var_dimids));
    NC_CHK(nc_inq_dimlen(ncid, var_dimids[0], &dimlen));
    printf("Time dimension after stride constraint: %zu (was 3 without stride)\n", dimlen);
    
    NC_CHK(nc_close(ncid));
    printf("\nExample completed successfully.\n");
    
    return 0;
}

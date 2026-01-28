/**
 * @file groups.c
 * @brief Demonstrates NetCDF-4 hierarchical groups, nested groups, and dimension visibility
 *
 * This example showcases NetCDF-4's hierarchical group feature, which enables organizing
 * datasets into logical groupings similar to directories in a filesystem. Groups provide
 * namespace isolation for variables while allowing dimensions to be shared across the
 * hierarchy through dimension visibility rules.
 *
 * The program creates a three-level group hierarchy (root → SubGroup1, root → SubGroup2 → 
 * NestedGroup), demonstrates dimension visibility across group boundaries, and showcases
 * all five new NetCDF-4 integer types (NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64).
 *
 * **Learning Objectives:**
 * - Understand NetCDF-4 hierarchical group structures
 * - Learn to create and navigate nested groups
 * - Master dimension visibility rules across group boundaries
 * - Work with all five new NetCDF-4 integer types
 * - Recognize when groups provide organizational benefits
 *
 * **Key Concepts:**
 * - **Hierarchical Groups**: Organize datasets into logical groupings (like directories)
 * - **Dimension Visibility**: Parent dimensions visible in all child groups
 * - **Variable Scoping**: Variables only visible in their defining group
 * - **Group Navigation**: Use nc_inq_grp_ncid() to navigate by name
 * - **New Integer Types**: NC_UBYTE, NC_USHORT, NC_UINT, NC_INT64, NC_UINT64
 *
 * **NetCDF-4 Group Architecture:**
 * - Groups implemented via NC_GRP_INFO_T structures (libsrc4/libhdf5)
 * - Dimensions visible in child groups via parent chain lookup
 * - Variables NOT inherited (scoped to defining group only)
 * - Requires NC_NETCDF4 flag (HDF5 backend)
 * - Not compatible with NC_CLASSIC_MODEL
 *
 * **Dimension Visibility Rules:**
 * - Dimensions defined in a group are visible in that group and all descendants
 * - Root dimensions (x, y) visible in SubGroup1, SubGroup2, and NestedGroup
 * - Local dimensions (z in NestedGroup) only visible in defining group
 * - Dimension lookup walks parent chain: child → parent → root
 *
 * **Use Cases for Groups:**
 * - **Multi-instrument datasets**: Group data by instrument or sensor
 * - **Model ensembles**: Separate ensemble members into groups
 * - **Quality levels**: Organize raw, calibrated, and derived products
 * - **Temporal organization**: Group data by year, month, or campaign
 * - **Namespace management**: Avoid variable name conflicts
 *
 * **Prerequisites:**
 * - simple_nc4.c - Basic NetCDF-4 file operations
 * - multi_unlimited.c - Advanced NetCDF-4 features
 *
 * **Related Examples:**
 * - f_groups.f90 - Fortran equivalent (future)
 * - user_types.c - User-defined compound types in groups
 *
 * **Compilation:**
 * @code
 * gcc -o groups groups.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./groups
 * ncdump groups.nc
 * ncdump -h groups.nc  # Header only
 * @endcode
 *
 * **Expected Output:**
 * Creates groups.nc in NetCDF-4/HDF5 format containing:
 * - Root group with dimensions x(3), y(4) and variable ubyte_var(NC_UBYTE)
 * - SubGroup1 with variable ushort_var(NC_USHORT)
 * - SubGroup2 with variable uint_var(NC_UINT)
 * - NestedGroup (under SubGroup2) with dimension z(2) and variables:
 *   - int64_var(NC_INT64, 2D: x, y)
 *   - uint64_var(NC_UINT64, 3D: x, y, z)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "groups.nc"
#define NX 3
#define NY 4
#define NZ 2
#define NDIMS_2D 2
#define NDIMS_3D 3
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int main()
{
    int ncid, grp1_id, grp2_id, nested_id;
    int x_dimid, y_dimid, z_dimid;
    int dimids_2d[NDIMS_2D], dimids_3d[NDIMS_3D];
    int ubyte_varid, ushort_varid, uint_varid, int64_varid, uint64_varid;
    int retval;
    
    /* Data arrays for all five new integer types */
    unsigned char ubyte_data[NY][NX];
    unsigned short ushort_data[NY][NX];
    unsigned int uint_data[NY][NX];
    long long int64_data[NY][NX];
    unsigned long long uint64_data[NY][NX][NZ];
    
    printf("NetCDF-4 Groups Example\n");
    printf("=======================\n\n");
    
    /* ========== WRITE PHASE ========== */
    printf("=== Phase 1: Create file with group hierarchy ===\n");
    
    /* Create the NetCDF-4 file */
    printf("Creating NetCDF-4 file: %s\n", FILE_NAME);
    if ((retval = nc_create(FILE_NAME, NC_CLOBBER|NC_NETCDF4, &ncid)))
        ERR(retval);
    
    /* Define root dimensions (visible in all groups) */
    printf("Defining root dimensions: x=%d, y=%d\n", NX, NY);
    if ((retval = nc_def_dim(ncid, "x", NX, &x_dimid)))
        ERR(retval);
    if ((retval = nc_def_dim(ncid, "y", NY, &y_dimid)))
        ERR(retval);
    
    /* Create SubGroup1 */
    printf("Creating SubGroup1\n");
    if ((retval = nc_def_grp(ncid, "SubGroup1", &grp1_id)))
        ERR(retval);
    
    /* Create SubGroup2 */
    printf("Creating SubGroup2\n");
    if ((retval = nc_def_grp(ncid, "SubGroup2", &grp2_id)))
        ERR(retval);
    
    /* Create NestedGroup under SubGroup2 */
    printf("Creating NestedGroup under SubGroup2\n");
    if ((retval = nc_def_grp(grp2_id, "NestedGroup", &nested_id)))
        ERR(retval);
    
    /* Define local dimension z in NestedGroup */
    printf("Defining local dimension z=%d in NestedGroup\n", NZ);
    if ((retval = nc_def_dim(nested_id, "z", NZ, &z_dimid)))
        ERR(retval);
    
    /* Define variables in each group using all 5 new integer types */
    printf("\nDefining variables with new integer types:\n");
    
    /* Root group: NC_UBYTE variable (2D: x, y) */
    printf("  Root: ubyte_var (NC_UBYTE, 2D: x, y)\n");
    dimids_2d[0] = y_dimid;
    dimids_2d[1] = x_dimid;
    if ((retval = nc_def_var(ncid, "ubyte_var", NC_UBYTE, NDIMS_2D, dimids_2d, &ubyte_varid)))
        ERR(retval);
    
    /* SubGroup1: NC_USHORT variable (2D: x, y) */
    printf("  SubGroup1: ushort_var (NC_USHORT, 2D: x, y)\n");
    if ((retval = nc_def_var(grp1_id, "ushort_var", NC_USHORT, NDIMS_2D, dimids_2d, &ushort_varid)))
        ERR(retval);
    
    /* SubGroup2: NC_UINT variable (2D: x, y) */
    printf("  SubGroup2: uint_var (NC_UINT, 2D: x, y)\n");
    if ((retval = nc_def_var(grp2_id, "uint_var", NC_UINT, NDIMS_2D, dimids_2d, &uint_varid)))
        ERR(retval);
    
    /* NestedGroup: NC_INT64 variable (2D: x, y) */
    printf("  NestedGroup: int64_var (NC_INT64, 2D: x, y)\n");
    if ((retval = nc_def_var(nested_id, "int64_var", NC_INT64, NDIMS_2D, dimids_2d, &int64_varid)))
        ERR(retval);
    
    /* NestedGroup: NC_UINT64 variable (3D: x, y, z) */
    printf("  NestedGroup: uint64_var (NC_UINT64, 3D: x, y, z)\n");
    dimids_3d[0] = y_dimid;
    dimids_3d[1] = x_dimid;
    dimids_3d[2] = z_dimid;
    if ((retval = nc_def_var(nested_id, "uint64_var", NC_UINT64, NDIMS_3D, dimids_3d, &uint64_varid)))
        ERR(retval);
    
    /* End define mode */
    if ((retval = nc_enddef(ncid)))
        ERR(retval);
    
    /* Initialize data with sequential values starting from 1 */
    printf("\nInitializing data with sequential values (1, 2, 3, ...):\n");
    int value = 1;
    
    /* NC_UBYTE data (3x4 = 12 values) */
    for (int i = 0; i < NY; i++)
        for (int j = 0; j < NX; j++)
            ubyte_data[i][j] = (unsigned char)value++;
    
    /* NC_USHORT data (3x4 = 12 values) */
    for (int i = 0; i < NY; i++)
        for (int j = 0; j < NX; j++)
            ushort_data[i][j] = (unsigned short)value++;
    
    /* NC_UINT data (3x4 = 12 values) */
    for (int i = 0; i < NY; i++)
        for (int j = 0; j < NX; j++)
            uint_data[i][j] = (unsigned int)value++;
    
    /* NC_INT64 data (3x4 = 12 values) */
    for (int i = 0; i < NY; i++)
        for (int j = 0; j < NX; j++)
            int64_data[i][j] = (long long)value++;
    
    /* NC_UINT64 data (3x4x2 = 24 values) */
    for (int i = 0; i < NY; i++)
        for (int j = 0; j < NX; j++)
            for (int k = 0; k < NZ; k++)
                uint64_data[i][j][k] = (unsigned long long)value++;
    
    /* Write data to all variables */
    printf("Writing data to all variables...\n");
    if ((retval = nc_put_var_uchar(ncid, ubyte_varid, &ubyte_data[0][0])))
        ERR(retval);
    if ((retval = nc_put_var_ushort(grp1_id, ushort_varid, &ushort_data[0][0])))
        ERR(retval);
    if ((retval = nc_put_var_uint(grp2_id, uint_varid, &uint_data[0][0])))
        ERR(retval);
    if ((retval = nc_put_var_longlong(nested_id, int64_varid, &int64_data[0][0])))
        ERR(retval);
    if ((retval = nc_put_var_ulonglong(nested_id, uint64_varid, &uint64_data[0][0][0])))
        ERR(retval);
    
    /* Close the file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    printf("*** SUCCESS writing file!\n");
    
    /* ========== READ AND VALIDATE PHASE ========== */
    printf("\n=== Phase 2: Read and validate file structure ===\n");
    
    /* Open the file for reading */
    printf("Reopening file for validation...\n");
    if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
        ERR(retval);
    
    /* Query and validate number of groups in root */
    int ngrps;
    int grpids[NC_MAX_VARS];
    if ((retval = nc_inq_grps(ncid, &ngrps, grpids)))
        ERR(retval);
    
    if (ngrps != 2) {
        printf("Error: Expected 2 groups in root, found %d\n", ngrps);
        exit(ERRCODE);
    }
    printf("Verified: Root has %d child groups\n", ngrps);
    
    /* Navigate to groups by name */
    printf("\nNavigating to groups by name:\n");
    if ((retval = nc_inq_grp_ncid(ncid, "SubGroup1", &grp1_id)))
        ERR(retval);
    printf("  Found SubGroup1 (ncid=%d)\n", grp1_id);
    
    if ((retval = nc_inq_grp_ncid(ncid, "SubGroup2", &grp2_id)))
        ERR(retval);
    printf("  Found SubGroup2 (ncid=%d)\n", grp2_id);
    
    if ((retval = nc_inq_grp_ncid(grp2_id, "NestedGroup", &nested_id)))
        ERR(retval);
    printf("  Found NestedGroup (ncid=%d)\n", nested_id);
    
    /* Validate group names */
    char grpname[NC_MAX_NAME + 1];
    size_t grpname_len;
    
    if ((retval = nc_inq_grpname(grp1_id, grpname)))
        ERR(retval);
    if (strcmp(grpname, "SubGroup1") != 0) {
        printf("Error: Expected group name 'SubGroup1', found '%s'\n", grpname);
        exit(ERRCODE);
    }
    
    if ((retval = nc_inq_grpname_len(grp2_id, &grpname_len)))
        ERR(retval);
    if ((retval = nc_inq_grpname(grp2_id, grpname)))
        ERR(retval);
    if (strcmp(grpname, "SubGroup2") != 0) {
        printf("Error: Expected group name 'SubGroup2', found '%s'\n", grpname);
        exit(ERRCODE);
    }
    
    if ((retval = nc_inq_grpname(nested_id, grpname)))
        ERR(retval);
    if (strcmp(grpname, "NestedGroup") != 0) {
        printf("Error: Expected group name 'NestedGroup', found '%s'\n", grpname);
        exit(ERRCODE);
    }
    printf("Verified: All group names correct\n");
    
    /* Test dimension visibility across group boundaries */
    printf("\n=== Phase 3: Test dimension visibility ===\n");
    printf("Testing that root dimensions (x, y) are visible in all groups:\n");
    
    int test_dimid;
    
    /* Test x dimension visibility in SubGroup1 */
    if ((retval = nc_inq_dimid(grp1_id, "x", &test_dimid)))
        ERR(retval);
    printf("  ✓ SubGroup1 can see dimension 'x' from root\n");
    
    /* Test y dimension visibility in SubGroup1 */
    if ((retval = nc_inq_dimid(grp1_id, "y", &test_dimid)))
        ERR(retval);
    printf("  ✓ SubGroup1 can see dimension 'y' from root\n");
    
    /* Test x dimension visibility in SubGroup2 */
    if ((retval = nc_inq_dimid(grp2_id, "x", &test_dimid)))
        ERR(retval);
    printf("  ✓ SubGroup2 can see dimension 'x' from root\n");
    
    /* Test y dimension visibility in SubGroup2 */
    if ((retval = nc_inq_dimid(grp2_id, "y", &test_dimid)))
        ERR(retval);
    printf("  ✓ SubGroup2 can see dimension 'y' from root\n");
    
    /* Test x dimension visibility in NestedGroup */
    if ((retval = nc_inq_dimid(nested_id, "x", &test_dimid)))
        ERR(retval);
    printf("  ✓ NestedGroup can see dimension 'x' from root\n");
    
    /* Test y dimension visibility in NestedGroup */
    if ((retval = nc_inq_dimid(nested_id, "y", &test_dimid)))
        ERR(retval);
    printf("  ✓ NestedGroup can see dimension 'y' from root\n");
    
    /* Test local dimension z in NestedGroup */
    if ((retval = nc_inq_dimid(nested_id, "z", &test_dimid)))
        ERR(retval);
    printf("  ✓ NestedGroup can see its local dimension 'z'\n");
    
    printf("Verified: Dimension visibility follows parent chain rules\n");
    
    /* Validate dimension sizes */
    printf("\nValidating dimension sizes:\n");
    size_t len_x, len_y, len_z;
    
    if ((retval = nc_inq_dimid(ncid, "x", &x_dimid)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, x_dimid, &len_x)))
        ERR(retval);
    if (len_x != NX) {
        printf("Error: Expected x dimension = %d, found %zu\n", NX, len_x);
        exit(ERRCODE);
    }
    
    if ((retval = nc_inq_dimid(ncid, "y", &y_dimid)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, y_dimid, &len_y)))
        ERR(retval);
    if (len_y != NY) {
        printf("Error: Expected y dimension = %d, found %zu\n", NY, len_y);
        exit(ERRCODE);
    }
    
    if ((retval = nc_inq_dimid(nested_id, "z", &z_dimid)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(nested_id, z_dimid, &len_z)))
        ERR(retval);
    if (len_z != NZ) {
        printf("Error: Expected z dimension = %d, found %zu\n", NZ, len_z);
        exit(ERRCODE);
    }
    
    printf("  x = %zu, y = %zu, z = %zu\n", len_x, len_y, len_z);
    printf("Verified: All dimension sizes correct\n");
    
    /* Query and validate all variable metadata */
    printf("\n=== Phase 4: Validate variable metadata ===\n");
    
    char varname[NC_MAX_NAME + 1];
    nc_type vartype;
    int varndims;
    
    /* Validate ubyte_var in root */
    if ((retval = nc_inq_varid(ncid, "ubyte_var", &ubyte_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(ncid, ubyte_varid, varname, &vartype, &varndims, NULL, NULL)))
        ERR(retval);
    if (vartype != NC_UBYTE || varndims != NDIMS_2D) {
        printf("Error: ubyte_var has wrong type or dimensions\n");
        exit(ERRCODE);
    }
    printf("  ✓ Root: ubyte_var (NC_UBYTE, %dD)\n", varndims);
    
    /* Validate ushort_var in SubGroup1 */
    if ((retval = nc_inq_varid(grp1_id, "ushort_var", &ushort_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp1_id, ushort_varid, varname, &vartype, &varndims, NULL, NULL)))
        ERR(retval);
    if (vartype != NC_USHORT || varndims != NDIMS_2D) {
        printf("Error: ushort_var has wrong type or dimensions\n");
        exit(ERRCODE);
    }
    printf("  ✓ SubGroup1: ushort_var (NC_USHORT, %dD)\n", varndims);
    
    /* Validate uint_var in SubGroup2 */
    if ((retval = nc_inq_varid(grp2_id, "uint_var", &uint_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(grp2_id, uint_varid, varname, &vartype, &varndims, NULL, NULL)))
        ERR(retval);
    if (vartype != NC_UINT || varndims != NDIMS_2D) {
        printf("Error: uint_var has wrong type or dimensions\n");
        exit(ERRCODE);
    }
    printf("  ✓ SubGroup2: uint_var (NC_UINT, %dD)\n", varndims);
    
    /* Validate int64_var in NestedGroup */
    if ((retval = nc_inq_varid(nested_id, "int64_var", &int64_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(nested_id, int64_varid, varname, &vartype, &varndims, NULL, NULL)))
        ERR(retval);
    if (vartype != NC_INT64 || varndims != NDIMS_2D) {
        printf("Error: int64_var has wrong type or dimensions\n");
        exit(ERRCODE);
    }
    printf("  ✓ NestedGroup: int64_var (NC_INT64, %dD)\n", varndims);
    
    /* Validate uint64_var in NestedGroup */
    if ((retval = nc_inq_varid(nested_id, "uint64_var", &uint64_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(nested_id, uint64_varid, varname, &vartype, &varndims, NULL, NULL)))
        ERR(retval);
    if (vartype != NC_UINT64 || varndims != NDIMS_3D) {
        printf("Error: uint64_var has wrong type or dimensions\n");
        exit(ERRCODE);
    }
    printf("  ✓ NestedGroup: uint64_var (NC_UINT64, %dD)\n", varndims);
    
    printf("Verified: All variable metadata correct\n");
    
    /* Read and validate all data */
    printf("\n=== Phase 5: Read and validate data values ===\n");
    
    unsigned char ubyte_in[NY][NX];
    unsigned short ushort_in[NY][NX];
    unsigned int uint_in[NY][NX];
    long long int64_in[NY][NX];
    unsigned long long uint64_in[NY][NX][NZ];
    
    if ((retval = nc_get_var_uchar(ncid, ubyte_varid, &ubyte_in[0][0])))
        ERR(retval);
    if ((retval = nc_get_var_ushort(grp1_id, ushort_varid, &ushort_in[0][0])))
        ERR(retval);
    if ((retval = nc_get_var_uint(grp2_id, uint_varid, &uint_in[0][0])))
        ERR(retval);
    if ((retval = nc_get_var_longlong(nested_id, int64_varid, &int64_in[0][0])))
        ERR(retval);
    if ((retval = nc_get_var_ulonglong(nested_id, uint64_varid, &uint64_in[0][0][0])))
        ERR(retval);
    
    /* Validate data correctness */
    int errors = 0;
    value = 1;
    
    /* Validate NC_UBYTE data */
    for (int i = 0; i < NY; i++) {
        for (int j = 0; j < NX; j++) {
            if (ubyte_in[i][j] != (unsigned char)value) {
                printf("Error: ubyte_var[%d][%d] = %u, expected %d\n", 
                       i, j, ubyte_in[i][j], value);
                errors++;
            }
            value++;
        }
    }
    
    /* Validate NC_USHORT data */
    for (int i = 0; i < NY; i++) {
        for (int j = 0; j < NX; j++) {
            if (ushort_in[i][j] != (unsigned short)value) {
                printf("Error: ushort_var[%d][%d] = %u, expected %d\n", 
                       i, j, ushort_in[i][j], value);
                errors++;
            }
            value++;
        }
    }
    
    /* Validate NC_UINT data */
    for (int i = 0; i < NY; i++) {
        for (int j = 0; j < NX; j++) {
            if (uint_in[i][j] != (unsigned int)value) {
                printf("Error: uint_var[%d][%d] = %u, expected %d\n", 
                       i, j, uint_in[i][j], value);
                errors++;
            }
            value++;
        }
    }
    
    /* Validate NC_INT64 data */
    for (int i = 0; i < NY; i++) {
        for (int j = 0; j < NX; j++) {
            if (int64_in[i][j] != (long long)value) {
                printf("Error: int64_var[%d][%d] = %lld, expected %d\n", 
                       i, j, int64_in[i][j], value);
                errors++;
            }
            value++;
        }
    }
    
    /* Validate NC_UINT64 data */
    for (int i = 0; i < NY; i++) {
        for (int j = 0; j < NX; j++) {
            for (int k = 0; k < NZ; k++) {
                if (uint64_in[i][j][k] != (unsigned long long)value) {
                    printf("Error: uint64_var[%d][%d][%d] = %llu, expected %d\n", 
                           i, j, k, uint64_in[i][j][k], value);
                    errors++;
                }
                value++;
            }
        }
    }
    
    if (errors > 0) {
        printf("*** FAILED: %d data validation errors\n", errors);
        exit(ERRCODE);
    }
    
    int total_values = NX * NY * 4 + NX * NY * NZ;  /* 4 2D arrays + 1 3D array */
    printf("Verified: All %d data values correct (sequential 1 to %d)\n", 
           total_values, total_values);
    
    /* Close the file */
    if ((retval = nc_close(ncid)))
        ERR(retval);
    
    /* Summary */
    printf("\n=== Summary ===\n");
    printf("Group hierarchy:\n");
    printf("  Root\n");
    printf("  ├── SubGroup1\n");
    printf("  └── SubGroup2\n");
    printf("      └── NestedGroup\n");
    printf("\nDimensions:\n");
    printf("  Root: x=%d, y=%d (visible in all groups)\n", NX, NY);
    printf("  NestedGroup: z=%d (local only)\n", NZ);
    printf("\nVariables (all 5 new integer types):\n");
    printf("  Root: ubyte_var (NC_UBYTE)\n");
    printf("  SubGroup1: ushort_var (NC_USHORT)\n");
    printf("  SubGroup2: uint_var (NC_UINT)\n");
    printf("  NestedGroup: int64_var (NC_INT64), uint64_var (NC_UINT64)\n");
    printf("\nKey Concepts Demonstrated:\n");
    printf("  ✓ Hierarchical group structures (3 levels)\n");
    printf("  ✓ Nested groups (NestedGroup under SubGroup2)\n");
    printf("  ✓ Dimension visibility across group boundaries\n");
    printf("  ✓ All 5 new NetCDF-4 integer types\n");
    printf("  ✓ Variable scoping to defining group\n");
    
    printf("\n*** SUCCESS: All validation checks passed!\n");
    printf("Use 'ncdump groups.nc' to view the file structure.\n");
    
    return 0;
}

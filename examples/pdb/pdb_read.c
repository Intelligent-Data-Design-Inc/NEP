/*
 * @file pdb_read.c
 * @brief Example program that reads a legacy PDB protein structure file
 * through the NetCDF UDF API.
 *
 * This program demonstrates opening a legacy Protein Data Bank (PDB) file
 * with the PDB UDF handler (UDF slot 7, NC_UDF7), reading the atom/model
 * dimensions, and extracting a small slice of atom coordinate data.
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative
 * Guide to Writing High-Performance Programs for Scientific Data Management,
 * Second Edition" (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "../test/data/PDB/4HHB.pdb"

#define ERR(e) do { if (e) { fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); return 1; } } while (0)

/* PDB UDF initialization function provided by libncpdb. */
extern int NC_PDB_initialize(void);

int
main(int argc, char **argv)
{
    int ncid, x_varid, y_varid, z_varid, retval;
    int ndims, nvars, ngatts, unlimdimid;
    int dimids[NC_MAX_VAR_DIMS];
    size_t len;
    char name[NC_MAX_NAME + 1];
    nc_type xtype;
    int var_ndims, var_natts;
    size_t start[2] = {0, 0};
    size_t count[2] = {1, 4};
    float x[4], y[4], z[4];
    const char *file_name = (argc > 1) ? argv[1] : FILE_NAME;

    (void)NC_PDB_initialize();

    if ((retval = nc_open(file_name, NC_UDF7, &ncid)))
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

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_x", &x_varid)))
        ERR(retval);
    if ((retval = nc_inq_var(ncid, x_varid, name, &xtype, &var_ndims,
                              dimids, &var_natts)))
        ERR(retval);
    printf("Variable '%s': xtype=%d ndims=%d natts=%d\n",
           name, xtype, var_ndims, var_natts);

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_y", &y_varid)))
        ERR(retval);
    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_z", &z_varid)))
        ERR(retval);

    if ((retval = nc_get_vara_float(ncid, x_varid, start, count, x)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, y_varid, start, count, y)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, z_varid, start, count, z)))
        ERR(retval);

    printf("First 4 atom coordinates:\n");
    for (int i = 0; i < 4; i++)
        printf("  atom %d: (%.3f, %.3f, %.3f)\n", i, x[i], y[i], z[i]);

    if ((retval = nc_close(ncid)))
        ERR(retval);

    printf("Done.\n");
    return 0;
}

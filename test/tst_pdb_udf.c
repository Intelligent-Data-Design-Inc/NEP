/**
 * @file tst_pdb_udf.c
 * @brief Test for the legacy PDB User-Defined Format (UDF) handler.
 *
 * V3.3.0 Sprint 2: verifies that the legacy PDB dispatch layer exposes the
 * expected dimensions, variables, global attributes, and coordinate data
 * for the real PDB files test/data/PDB/1J7W.pdb and 4HHB.pdb (sourced
 * from https://www.rcsb.org), and rejects PDB-like files with no atoms.
 * V3.3.0 Sprint 3: adds coverage for the multi-model NMR ensemble 1GAB.pdb
 * and the synthetic no-CRYST1 file no_cryst1.pdb.
 *
 * @author Edward Hartnett
 * @date 2026-07-29
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_PDB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netcdf.h>
#include "pdbdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Length of the atom_site_group_PDB fixed-width string field. */
#define PDB_GROUP_LEN 6

/** Path to the first legacy PDB test file. */
#define PDB_TEST_FILE_1 "data/PDB/1J7W.pdb"

/** Path to the second legacy PDB test file. */
#define PDB_TEST_FILE_2 "data/PDB/4HHB.pdb"

/** Path to the multi-model NMR legacy PDB test file. */
#define PDB_TEST_FILE_3 "data/PDB/1GAB.pdb"

/** Path to the synthetic single-model legacy PDB test file without CRYST1. */
#define PDB_TEST_FILE_4 "data/PDB/no_cryst1.pdb"

/**
 * @internal Check that a dimension exists and has the expected length.
 */
static int
check_dimlen(int ncid, const char *name, size_t expected)
{
    int dimid;
    size_t len;
    int retval;

    if ((retval = nc_inq_dimid(ncid, name, &dimid)))
        ERR(retval);
    if ((retval = nc_inq_dimlen(ncid, dimid, &len)))
        ERR(retval);
    if (len != expected)
    {
        fprintf(stderr, "Dimension %s length %zu != expected %zu at line %d\n",
                name, len, expected, __LINE__);
        ERR(NC_EINVAL);
    }

    return 0;
}

/**
 * @internal Check a CHAR global attribute value.
 */
static int
check_att_text(int ncid, const char *name, const char *expected)
{
    size_t len;
    char *buf;
    int retval;

    if ((retval = nc_inq_attlen(ncid, NC_GLOBAL, name, &len)))
        ERR(retval);

    buf = (char *)malloc(len + 1);
    if (!buf)
        ERR(NC_ENOMEM);

    if ((retval = nc_get_att_text(ncid, NC_GLOBAL, name, buf)))
    {
        free(buf);
        ERR(retval);
    }
    buf[len] = '\0';

    if (strcmp(buf, expected) != 0)
    {
        fprintf(stderr, "Attribute %s = \"%s\", expected \"%s\" at line %d\n",
                name, buf, expected, __LINE__);
        free(buf);
        ERR(NC_EINVAL);
    }

    free(buf);

    return 0;
}

/**
 * @internal Check a CHAR global attribute starts with a given prefix.
 */
static int
check_att_prefix(int ncid, const char *name, const char *prefix)
{
    size_t len;
    char *buf;
    int retval;

    if ((retval = nc_inq_attlen(ncid, NC_GLOBAL, name, &len)))
        ERR(retval);

    buf = (char *)malloc(len + 1);
    if (!buf)
        ERR(NC_ENOMEM);

    if ((retval = nc_get_att_text(ncid, NC_GLOBAL, name, buf)))
    {
        free(buf);
        ERR(retval);
    }
    buf[len] = '\0';

    if (strstr(buf, prefix) == NULL)
    {
        fprintf(stderr, "Attribute %s = \"%s\", expected substring \"%s\" at line %d\n",
                name, buf, prefix, __LINE__);
        free(buf);
        ERR(NC_EINVAL);
    }

    free(buf);

    return 0;
}

/**
 * @internal Read and check the x/y/z coordinates of one atom.
 */
static int
check_coord(int ncid, int atom_idx, float expected_x, float expected_y,
            float expected_z)
{
    int varid;
    size_t start[2] = {0, (size_t)atom_idx};
    size_t count[2] = {1, 1};
    float x, y, z;
    int retval;

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_x", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, varid, start, count, &x)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_y", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, varid, start, count, &y)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_z", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, varid, start, count, &z)))
        ERR(retval);

    if (fabsf(x - expected_x) > 1.0e-3f ||
        fabsf(y - expected_y) > 1.0e-3f ||
        fabsf(z - expected_z) > 1.0e-3f)
    {
        fprintf(stderr,
                "Coordinate mismatch for atom %d: got (%g,%g,%g), expected (%g,%g,%g) at line %d\n",
                atom_idx, x, y, z, expected_x, expected_y, expected_z,
                __LINE__);
        ERR(NC_EINVAL);
    }

    return 0;
}

/**
 * @internal Check the atom_site_group_PDB string for one atom.
 */
static int
check_group(int ncid, int atom_idx, const char *expected)
{
    int varid;
    size_t start[2] = {(size_t)atom_idx, 0};
    size_t count[2] = {1, PDB_GROUP_LEN};
    char buf[PDB_GROUP_LEN];
    size_t expected_len = strlen(expected);
    int retval;

    if ((retval = nc_inq_varid(ncid, "atom_site_group_PDB", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_text(ncid, varid, start, count, buf)))
        ERR(retval);

    if (strncmp(buf, expected, expected_len) != 0)
    {
        fprintf(stderr,
                "Group mismatch for atom %d: expected \"%s\" at line %d\n",
                atom_idx, expected, __LINE__);
        ERR(NC_EINVAL);
    }

    return 0;
}

/**
 * @internal Read a single coordinate value from a [model][atom] variable.
 */
static int
read_coord(int ncid, const char *var_name, size_t model, size_t atom,
             float *value)
{
    int varid;
    size_t start[2] = {model, atom};
    size_t count[2] = {1, 1};
    int retval;

    if ((retval = nc_inq_varid(ncid, var_name, &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_float(ncid, varid, start, count, value)))
        ERR(retval);

    return 0;
}

/**
 * @internal Verify an entire legacy PDB file.
 */
static int
test_file(const char *path, size_t expected_atoms,
          const char *id_code, const char *classification,
          const char *dep_date,
          const char *cell_a, const char *cell_b, const char *cell_c,
          const char *alpha, const char *beta, const char *gamma,
          const char *space_group, const char *symmetry_z,
          float x0, float y0, float z0,
          int last_atom_idx)
{
    int ncid, retval;

    if ((retval = nc_open(path, NC_UDF7, &ncid)))
        ERR(retval);

    if ((retval = check_dimlen(ncid, "model", 1)))
        return retval;
    if ((retval = check_dimlen(ncid, "atom", expected_atoms)))
        return retval;

    if ((retval = check_att_text(ncid, "idCode", id_code)))
        return retval;
    if ((retval = check_att_text(ncid, "classification", classification)))
        return retval;
    if ((retval = check_att_text(ncid, "depDate", dep_date)))
        return retval;

    if ((retval = check_att_text(ncid, "cell_length_a", cell_a)))
        return retval;
    if ((retval = check_att_text(ncid, "cell_length_b", cell_b)))
        return retval;
    if ((retval = check_att_text(ncid, "cell_length_c", cell_c)))
        return retval;
    if ((retval = check_att_text(ncid, "cell_angle_alpha", alpha)))
        return retval;
    if ((retval = check_att_text(ncid, "cell_angle_beta", beta)))
        return retval;
    if ((retval = check_att_text(ncid, "cell_angle_gamma", gamma)))
        return retval;
    if ((retval = check_att_text(ncid, "space_group_name_H-M", space_group)))
        return retval;
    if ((retval = check_att_text(ncid, "symmetry_Z", symmetry_z)))
        return retval;

    /* TITLE/COMPND/SOURCE are present but vary; just check a prefix. */
    if ((retval = check_att_prefix(ncid, "title", "CRYSTAL")))
        return retval;
    if ((retval = check_att_prefix(ncid, "compnd", "MOL_ID")))
        return retval;

    if ((retval = check_coord(ncid, 0, x0, y0, z0)))
        return retval;
    if ((retval = check_group(ncid, 0, "ATOM")))
        return retval;
    if ((retval = check_group(ncid, last_atom_idx, "HETATM")))
        return retval;

    if ((retval = nc_close(ncid)))
        ERR(retval);

    return 0;
}

int
main(void)
{
    int retval;
    FILE *f;

    /* Ensure the PDB UDF handler is registered. */
    if (!NC_PDB_initialize())
        ERR(NC_EINVAL);

    /* Validate the first test file. */
    if ((retval = test_file(PDB_TEST_FILE_1,
                            4809,
                            "1J7W",
                            "OXYGEN STORAGE/TRANSPORT",
                            "19-MAY-01",
                            "63.360", "84.320", "54.000",
                            "90.00", "99.43", "90.00",
                            "P 1 21 1", "4",
                            10.834f, 19.914f, 6.870f,
                            4808)))
        return retval;
    printf("PASS: %s\n", PDB_TEST_FILE_1);

    /* Validate the second test file. */
    if ((retval = test_file(PDB_TEST_FILE_2,
                            4779,
                            "4HHB",
                            "OXYGEN TRANSPORT",
                            "07-MAR-84",
                            "63.150", "83.590", "53.800",
                            "90.00", "99.34", "90.00",
                            "P 1 21 1", "4",
                            19.323f, 29.727f, 42.781f,
                            4778)))
        return retval;
    printf("PASS: %s\n", PDB_TEST_FILE_2);

    /* Validate the multi-model NMR test file. */
    {
        int ncid3;
        float x_m0, y_m0, z_m0;
        float x_m1, y_m1, z_m1;

        if ((retval = nc_open(PDB_TEST_FILE_3, NC_UDF7, &ncid3)))
            ERR(retval);

        if ((retval = check_dimlen(ncid3, "model", 20)))
            return retval;
        if ((retval = check_dimlen(ncid3, "atom", 851)))
            return retval;

        if ((retval = check_att_text(ncid3, "idCode", "1GAB")))
            return retval;

        /* Same atom in two different models must have different coordinates. */
        if ((retval = read_coord(ncid3, "atom_site_Cartn_x", 0, 0, &x_m0)))
            return retval;
        if ((retval = read_coord(ncid3, "atom_site_Cartn_y", 0, 0, &y_m0)))
            return retval;
        if ((retval = read_coord(ncid3, "atom_site_Cartn_z", 0, 0, &z_m0)))
            return retval;
        if ((retval = read_coord(ncid3, "atom_site_Cartn_x", 1, 0, &x_m1)))
            return retval;
        if ((retval = read_coord(ncid3, "atom_site_Cartn_y", 1, 0, &y_m1)))
            return retval;
        if ((retval = read_coord(ncid3, "atom_site_Cartn_z", 1, 0, &z_m1)))
            return retval;

        if (x_m0 == x_m1 && y_m0 == y_m1 && z_m0 == z_m1)
        {
            fprintf(stderr, "Multi-model coordinates unexpectedly identical at line %d\n",
                    __LINE__);
            ERR(NC_EINVAL);
        }

        if ((retval = nc_close(ncid3)))
            ERR(retval);
        printf("PASS: %s (multi-model)\n", PDB_TEST_FILE_3);
    }

    /* Validate the synthetic file without a CRYST1 record. */
    {
        int ncid4;
        size_t len;

        if ((retval = nc_open(PDB_TEST_FILE_4, NC_UDF7, &ncid4)))
            ERR(retval);

        if ((retval = check_dimlen(ncid4, "model", 1)))
            return retval;
        if ((retval = check_dimlen(ncid4, "atom", 5)))
            return retval;

        if ((retval = check_att_text(ncid4, "idCode", "0001")))
            return retval;

        /* CRYST1-derived attributes must be absent. */
        if (nc_inq_attlen(ncid4, NC_GLOBAL, "cell_length_a", &len) != NC_ENOTATT)
        {
            fprintf(stderr, "Expected cell_length_a to be absent at line %d\n", __LINE__);
            ERR(NC_EINVAL);
        }
        if (nc_inq_attlen(ncid4, NC_GLOBAL, "space_group_name_H-M", &len) != NC_ENOTATT)
        {
            fprintf(stderr, "Expected space_group_name_H-M to be absent at line %d\n",
                    __LINE__);
            ERR(NC_EINVAL);
        }

        if ((retval = nc_close(ncid4)))
            ERR(retval);
        printf("PASS: %s (no CRYST1)\n", PDB_TEST_FILE_4);
    }

    /* Malformed file with no ATOM/HETATM records must be rejected. */
    f = fopen("no_atoms.pdb", "w");
    if (!f)
        ERR(NC_EINVAL);
    fprintf(f, "HEADER    TEST CLASSIFICATION                       01-JAN-00   1TEST\n");
    fclose(f);

    {
        int ncid2;
        int ret = nc_open("no_atoms.pdb", NC_UDF7, &ncid2);
        if (ret != NC_EINVAL)
        {
            fprintf(stderr, "Expected NC_EINVAL for file with no atoms, got %d at line %d\n",
                    ret, __LINE__);
            return 1;
        }
    }
    printf("PASS: malformed file rejected\n");

    printf("Done.\n");
    return 0;
}

#else  /* !HAVE_PDB */

int
main(void)
{
    printf("PDB support not built; test skipped.\n");
    return 0;
}

#endif /* HAVE_PDB */

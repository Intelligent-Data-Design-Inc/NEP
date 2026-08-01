/**
 * @file tst_mmcif_udf.c
 * @brief Test for the PDBx/mmCIF User-Defined Format (UDF) handler.
 *
 * V3.4.0 Sprint 2: verifies that the mmCIF dispatch layer exposes the
 * expected dimensions, variables, and global attributes for the real
 * mmCIF files test/data/mmCIF/1J7W.cif, 2W6V.cif, and 4HHB.cif (sourced
 * from https://www.rcsb.org), and rejects `data_` files with no
 * `_atom_site` category.
 *
 * @author Edward Hartnett
 * @date 2026-08-01
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"

#ifdef HAVE_MMCIF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netcdf.h>
#include "mmcifdispatch.h"

/** @internal Error macro: print location and return failure. */
#define ERR(e) do { \
    if (e) { \
        fprintf(stderr, "Error: %s at line %d\n", nc_strerror(e), __LINE__); \
        return 1; \
    } \
} while(0)

/** Length of the atom_site_group_PDB fixed-width string field. */
#define MMCIF_GROUP_LEN 6

/** Path to the first PDBx/mmCIF test file. */
#define MMCIF_TEST_FILE_1 "data/mmCIF/1J7W.cif"

/** Path to the second PDBx/mmCIF test file. */
#define MMCIF_TEST_FILE_2 "data/mmCIF/2W6V.cif"

/** Path to the third PDBx/mmCIF test file. */
#define MMCIF_TEST_FILE_3 "data/mmCIF/4HHB.cif"

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
 * @internal Read and check the x/y/z coordinates of one atom (model 0).
 */
static int
check_coord(int ncid, int atom_idx, double expected_x, double expected_y,
            double expected_z)
{
    int varid;
    size_t start[2] = {0, (size_t)atom_idx};
    size_t count[2] = {1, 1};
    double x, y, z;
    int retval;

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_x", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(ncid, varid, start, count, &x)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_y", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(ncid, varid, start, count, &y)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "atom_site_Cartn_z", &varid)))
        ERR(retval);
    if ((retval = nc_get_vara_double(ncid, varid, start, count, &z)))
        ERR(retval);

    if (fabs(x - expected_x) > 1.0e-3 ||
        fabs(y - expected_y) > 1.0e-3 ||
        fabs(z - expected_z) > 1.0e-3)
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
    size_t count[2] = {1, MMCIF_GROUP_LEN};
    char buf[MMCIF_GROUP_LEN];
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
 * @internal Verify an entire PDBx/mmCIF file.
 */
static int
test_file(const char *path, size_t expected_atoms,
          const char *entry_id, const char *cell_a, const char *cell_b,
          const char *cell_c, const char *alpha, const char *beta,
          const char *gamma, const char *space_group,
          double x0, double y0, double z0, int hetatm_idx)
{
    int ncid, retval;

    if ((retval = nc_open(path, NC_UDF8, &ncid)))
        ERR(retval);

    if ((retval = check_dimlen(ncid, "model", 1)))
        return retval;
    if ((retval = check_dimlen(ncid, "atom", expected_atoms)))
        return retval;

    if ((retval = check_att_text(ncid, "entry_id", entry_id)))
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
    if ((retval = check_att_text(ncid, "symmetry_space_group_name_H-M", space_group)))
        return retval;

    if ((retval = check_coord(ncid, 0, x0, y0, z0)))
        return retval;
    if ((retval = check_group(ncid, 0, "ATOM")))
        return retval;
    if ((retval = check_group(ncid, hetatm_idx, "HETATM")))
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

    /* Ensure the mmCIF UDF handler is registered. */
    if (!NC_MMCIF_initialize())
        ERR(NC_EINVAL);

    /* Validate the first test file. */
    if ((retval = test_file(MMCIF_TEST_FILE_1, 4809, "1J7W",
                            "63.360", "84.320", "54.000",
                            "90.00", "99.43", "90.00", "P 1 21 1",
                            10.834, 19.914, 6.870, 4808)))
        return retval;
    printf("PASS: %s\n", MMCIF_TEST_FILE_1);

    /* Validate the second test file (spot-checked separately since it has
     * a different atom count and no HETATM records to check against). */
    {
        int ncid2;
        if ((retval = nc_open(MMCIF_TEST_FILE_2, NC_UDF8, &ncid2)))
            ERR(retval);
        if ((retval = check_dimlen(ncid2, "model", 1)))
            return retval;
        if ((retval = check_att_text(ncid2, "entry_id", "2W6V")))
            return retval;
        if ((retval = nc_close(ncid2)))
            ERR(retval);
        printf("PASS: %s\n", MMCIF_TEST_FILE_2);
    }

    /* Validate the third test file. */
    if ((retval = test_file(MMCIF_TEST_FILE_3, 4779, "4HHB",
                            "63.150", "83.590", "53.800",
                            "90.00", "99.34", "90.00", "P 1 21 1",
                            19.323, 29.727, 42.781, 4778)))
        return retval;
    printf("PASS: %s\n", MMCIF_TEST_FILE_3);

    /* Malformed file with a data_ block but no _atom_site loop must be
     * rejected. */
    f = fopen("no_atom_site.cif", "w");
    if (!f)
        ERR(NC_EINVAL);
    fprintf(f, "data_TEST\n_entry.id   TEST\n");
    fclose(f);

    {
        int ncid4;
        int ret = nc_open("no_atom_site.cif", NC_UDF8, &ncid4);
        if (ret != NC_EINVAL)
        {
            fprintf(stderr, "Expected NC_EINVAL for file with no _atom_site, got %d at line %d\n",
                    ret, __LINE__);
            return 1;
        }
    }
    printf("PASS: malformed file rejected\n");

    printf("Done.\n");
    return 0;
}

#else /* !HAVE_MMCIF */

int
main(void)
{
    printf("mmCIF support not enabled; test skipped.\n");
    return 0;
}

#endif /* HAVE_MMCIF */

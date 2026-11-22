/* Test deferred def-var setter rebuilds for NEXTCDF-4 (issue #382).
 *
 * Setters record their settings and mark the variable dirty;
 * _enddef rebuilds the dataset once, so user attributes and
 * dimension-scale references survive recreation, and setters called
 * on a committed variable return NC_ELATEDEF.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NAME "tst_nextcdf4_setters.h5"

/* A user attribute must survive a def-var setter's dataset rebuild. */
static int
test_att_survives_setter(void)
{
    int ncid, dimid, varid;
    int attval = 42;
    int att_in = 0;
    int natts = -1;
    float fill = -999.0f;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "v", NC_FLOAT, 1, &dimid, &varid))
        return 1;
    if (nc_put_att_int(ncid, varid, "myatt", NC_INT, 1, &attval))
        return 1;
    /* This setter marks the variable dirty; _enddef rebuilds the
     * dataset and must restore the attribute. */
    if (nc_def_var_fill(ncid, varid, 0, &fill))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varnatts(ncid, varid, &natts) || natts != 1)
        return 1;
    if (nc_get_att_int(ncid, varid, "myatt", &att_in))
        return 1;
    if (att_in != 42)
        return 1;
    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/* Every def-var setter on a committed variable returns NC_ELATEDEF,
 * and the rejection must not destroy data written earlier. */
static int
test_setters_latedef(void)
{
    int ncid, dimid, varid;
    float fill = -1.0f;
    unsigned int params[1] = {4};
    int data[4] = {1, 2, 3, 4};
    int data_in[4] = {0};
    size_t chunks[1] = {2};
    int i, ret = 0;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    if (nc_def_var(ncid, "v", NC_INT, 1, &dimid, &varid))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, varid, data))
        return 1;

    if (nc_redef(ncid))
        return 1;
    if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_deflate(ncid, varid, 1, 1, 4) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_filter(ncid, varid, 1, 1, params) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_fill(ncid, varid, 0, &fill) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_endian(ncid, varid, NC_ENDIAN_BIG) != NC_ELATEDEF)
        ret = 1;
    if (nc_def_var_quantize(ncid, varid, NC_QUANTIZE_BITGROOM, 1) != NC_ELATEDEF)
        ret = 1;
    if (nc_close(ncid))
        return 1;
    if (ret)
        return 1;

    /* The rejected setters must not have destroyed the data. */
    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "v", &varid))
        return 1;
    if (nc_get_var_int(ncid, varid, data_in))
        return 1;
    for (i = 0; i < 4; i++)
        if (data_in[i] != data[i])
            return 1;
    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/* Settings recorded by the deferred setters reach the file. */
static int
test_setters_reach_file(void)
{
    int ncid, dimid, varid;
    float fill = -999.0f;
    float fill_in = 0.0f;
    int no_fill_in = -1;
    int endian_in = 0;
    int shuffle_in, deflate_in, level_in;
    char note_in[8] = {0};

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 10, &dimid))
        return 1;
    if (nc_def_var(ncid, "v", NC_FLOAT, 1, &dimid, &varid))
        return 1;
    if (nc_put_att_text(ncid, varid, "note", 3, "abc"))
        return 1;
    /* All three setters mark the same variable dirty; _enddef
     * rebuilds it once with every new property. */
    if (nc_def_var_fill(ncid, varid, 0, &fill))
        return 1;
    if (nc_def_var_endian(ncid, varid, NC_ENDIAN_BIG))
        return 1;
    if (nc_def_var_deflate(ncid, varid, 1, 1, 4))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "v", &varid))
        return 1;
    if (nc_inq_var_fill(ncid, varid, &no_fill_in, &fill_in))
        return 1;
    if (no_fill_in || fill_in != fill)
        return 1;
    if (nc_inq_var_endian(ncid, varid, &endian_in))
        return 1;
    if (endian_in != NC_ENDIAN_BIG)
        return 1;
    if (nc_inq_var_deflate(ncid, varid, &shuffle_in, &deflate_in, &level_in))
        return 1;
    if (!shuffle_in || !deflate_in || level_in != 4)
        return 1;
    if (nc_get_att_text(ncid, varid, "note", note_in))
        return 1;
    if (strcmp(note_in, "abc"))
        return 1;
    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

/* A variable defined before its dimension's coordinate variable
 * stays correctly attached: the coord var's dataset replaces the
 * placeholder scale and dependents are reattached to it. */
static int
test_coord_dependents(void)
{
    int ncid, dimid, tvar, xvar;
    int tdata[4] = {10, 20, 30, 40};
    int t_in[4] = {0};
    int did[1] = {-1};
    int i;

    unlink(FILE_NAME);
    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid))
        return 1;
    if (nc_def_dim(ncid, "x", 4, &dimid))
        return 1;
    /* Dependent first, coordinate variable second. */
    if (nc_def_var(ncid, "t", NC_INT, 1, &dimid, &tvar))
        return 1;
    if (nc_def_var(ncid, "x", NC_INT, 1, &dimid, &xvar))
        return 1;
    if (nc_enddef(ncid))
        return 1;
    if (nc_put_var_int(ncid, tvar, tdata))
        return 1;
    if (nc_close(ncid))
        return 1;

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid))
        return 1;
    if (nc_inq_varid(ncid, "t", &tvar))
        return 1;
    if (nc_inq_vardimid(ncid, tvar, did))
        return 1;
    if (did[0] != dimid)
        return 1;
    if (nc_get_var_int(ncid, tvar, t_in))
        return 1;
    for (i = 0; i < 4; i++)
        if (t_in[i] != tdata[i])
            return 1;
    if (nc_close(ncid))
        return 1;
    unlink(FILE_NAME);
    return 0;
}

int
main(void)
{
    if (!NC_NEXTCDF4_initialize()) {
        fprintf(stderr, "initialize failed\n");
        return 1;
    }
    if (test_att_survives_setter()) { fprintf(stderr, "att-survival test failed\n"); return 1; }
    if (test_setters_latedef()) { fprintf(stderr, "ELATEDEF test failed\n"); return 1; }
    if (test_setters_reach_file()) { fprintf(stderr, "setter-settings test failed\n"); return 1; }
    if (test_coord_dependents()) { fprintf(stderr, "coord-dependent test failed\n"); return 1; }
    unlink(FILE_NAME);
    printf("OK: NEXTCDF-4 deferred setter rebuilds\n");
    return 0;
}

/*! \file

Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014,
2015, 2016, 2017, 2018
University Corporation for Atmospheric Research/Unidata.

See \ref copyright file for more info.

*/

/*
 * Test renames of vars and dims. It's a surprisingly tricky business.
 *
 * Quincey Koziol, Ed Hartnett
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "netcdf.h"
#include "nep.h"

/* Error macros (derived from netcdf-c err_macros.h) */
static int total_err = 0, err = 0;
#define ERR do {                                                        \
    fflush(stdout);                                                     \
    err++;                                                              \
    fprintf(stderr, "Sorry! Unexpected result, %s, line: %d\n",        \
            __FILE__, __LINE__);                                        \
    fflush(stderr);                                                     \
    return 2;                                                           \
} while (0)

#define SUMMARIZE_ERR do {                      \
    if (err)                                    \
    {                                           \
        printf("%d failures\n", err);           \
        total_err += err;                       \
        err = 0;                                \
    }                                           \
    else                                        \
        printf("ok.\n");                       \
} while (0)

#define FINAL_RESULTS do {                                      \
    if (total_err)                                              \
    {                                                           \
        printf("%d errors detected! Sorry!\n", total_err);     \
        return 2;                                               \
    }                                                           \
    printf("*** Tests successful!\n");                        \
    return 0;                                                   \
} while (0)

#define FILE_NAME3 "tst_rename_fix3.nc"
#define FILE_NAME4 "tst_rename_fix4.nc"
#define ODIM_NAME "lat"         /* name for coord dim */
#define LAT "lat"
#define TAL "tal"
#define TAL1 "tal1"
#define TAL2 "tal2"
#define RH "rh"
#define NDIM_NAME "tal"         /* new name for coord dim */
#define OVAR_NAME "lat"         /* name for coord var */
#define NVAR_NAME "tal"         /* new name for coord var */
#define OVAR2_NAME "rh"         /* name for non-coord var that uses coord dim */
#define VAR_RANK 1              /* all vars in this test are of same rank */
#define DIM_LEN 2               /* all dims in this test are of same len */

/* For the tests based on Charlie Zender's rename bug test. */
#define CHARLIE_TEST_FILE "tst_rename_charlie.nc"
#define LON "lon"
#define LONGITUDE "longitude"
#define DIM1_LEN 4
#define NDIM1 1

/* Test data. */
int lats[DIM_LEN] = {-90, 90};
float rh[DIM_LEN] = {0.25, 0.75};

/* For renaming tests.  Create small test file of specified format
 * with a coordinate dimension, corresponding coordinate variable, and
 * a non-coordinate variable that uses the coordinate dimension. */
int
create_test_file(char *path, int cmode)
{
   int ncid, varid, var2id;
   int dims[VAR_RANK];

      if (nc_create(path, cmode, &ncid)) ERR;
   if (nc_def_dim(ncid, LAT, DIM_LEN, &dims[0])) ERR;
   if (nc_def_var(ncid, LAT, NC_INT, VAR_RANK, dims, &varid)) ERR;
   if (nc_def_var(ncid, RH, NC_FLOAT, VAR_RANK, dims, &var2id)) ERR;
   if (nc_enddef(ncid)) ERR;    /* not necessary for netCDF-4 files */
   if (nc_put_var_int(ncid, varid, lats)) ERR;
   if (nc_put_var_float(ncid, var2id, rh)) ERR;
   if (nc_close(ncid)) ERR;
   return 0;
}

/* Check the file that was produced by create_test_file(). Only the
 * names have been changed... */
int
check_file(int ncid, char *var0_name, char *var1_name, char *dim_name)
{
   int varid;
   int var2id;
   int dimid;
   int lats_in[DIM_LEN];
   float rh_in[DIM_LEN];
   int ii;

   /* printf("checking for vars %s and %s, dim %s\n", var0_name, var1_name, */
   /*        dim_name); */

   /* Check vars. Ids will change because of rename. */
   if (nc_inq_varid(ncid, var0_name, &varid)) ERR;
   if (nc_inq_varid(ncid, var1_name, &var2id)) ERR;

   /* Check dim. */
   if (nc_inq_dimid(ncid, dim_name, &dimid)) ERR;
   if (dimid != 0) ERR;

   /* Check the lats. */
   if (nc_get_var_int(ncid, varid, lats_in)) ERR;
   for (ii = 0; ii < DIM_LEN; ii++)
      if (lats_in[ii] != lats[ii])
         ERR;

   /* Check the RH. */
   if (nc_get_var_float(ncid, var2id, rh_in)) ERR;
   for (ii = 0; ii < DIM_LEN; ii++)
      if (rh_in[ii] != rh[ii])
         ERR;

   return 0;
}

/* Check the file created in Charlie Zender's test. See github
 * #597. */
int
check_charlies_file(char *file, char *dim_name, char *var_name)
{
   int ncid;
   int varid, dimid;
   int err;

   if ((err = nc_open(file, NC_NEXTCDF4, &ncid))) {
      fprintf(stderr, "nc_open error for %s: %s\n", file, nc_strerror(err));
      ERR;
   }
   if (nc_inq_varid(ncid, var_name, &varid)) ERR;
   if (nc_inq_dimid(ncid, dim_name, &dimid)) ERR;
   if (varid || dimid) ERR;
   if (nc_close(ncid)) ERR;
   return NC_NOERR;
}

/* Check a data-free version of the already-open file created in a
 * test. */
int
check_charlies_no_enddef_file(int ncid, char *dim_name, char *var_name)
{
   int varid, dimid;

   if (nc_inq_varid(ncid, var_name, &varid)) ERR;
   if (nc_inq_dimid(ncid, dim_name, &dimid)) ERR;
   if (varid || dimid) ERR;
   return NC_NOERR;
}

int
main(int argc, char **argv)
{
#define NUM_FORMATS 2
   int create_modes[NUM_FORMATS] = {NC_NEXTCDF4 | NC_CLOBBER, NC_NEXTCDF4 | NC_CLASSIC_MODEL | NC_CLOBBER};
   int open_modes[NUM_FORMATS] = {NC_NEXTCDF4 | NC_WRITE, NC_NEXTCDF4 | NC_CLASSIC_MODEL | NC_WRITE};
   char *fmt_names[] = {"netCDF-4", "netCDF-4 classic model"};
   char *file_names[] = {FILE_NAME3, FILE_NAME4};
   int format;

   printf("*** Testing netcdf rename bugs and fixes.\n");
   /* nc_set_log_level(5); */

   /* for (format = 0; format < NUM_FORMATS; format++) */
   for (format = 0; format < 1; format++)
   {
      int ncid, dimid, varid, var2id;
      int lats[DIM_LEN] = {-90, 90};
      int lats_in[DIM_LEN];
      float rh[DIM_LEN] = {0.25, 0.75};
      float rh_in[DIM_LEN];
      int ii;

      printf("*** Test Charlie's test for renaming without enddef...");
      {
         int ncid, dimid, varid;

         /* Create a nice, simple file. This file will contain one
          * dataset, "lon", which is a dimscale. */
         if (nc_create(CHARLIE_TEST_FILE, create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LON, DIM1_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LON, NC_FLOAT, NDIM1, &dimid, &varid)) ERR;

         /* Check the file. */
         if (check_charlies_no_enddef_file(ncid, LON, LON)) ERR;

         /* Rename the dimension. This will cause lon to stop being a
          * coord var and dimscale, and prepare to create a new
          * dimscale without var dataset "longitude". Dataset "lon"
          * will point to "longitude" as its dimscale. */
         if (nc_rename_dim(ncid, 0, LONGITUDE)) ERR;
         if (check_charlies_no_enddef_file(ncid, LONGITUDE, LON)) ERR;

         /* Rename the variable. This will remove the (as yet
          * unwritten) dimscale-only dataset "longitude" and rename
          * the existing dataset "lon" to "longitude". Variable
          * "longitude" will become a coordinate var. */
         if (nc_rename_var(ncid, 0, LONGITUDE)) ERR;
         if (check_charlies_no_enddef_file(ncid, LONGITUDE, LONGITUDE)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen the file and check. */
         if (check_charlies_file(CHARLIE_TEST_FILE, LONGITUDE, LONGITUDE)) ERR;

      }
      SUMMARIZE_ERR;
      printf("*** Test Charlie's test for renaming with one enddef...");
      {
         int ncid, dimid, varid;

#ifdef DEBUG
         nc_set_log_level(5);
#endif

         /* Create a nice, simple file. This file will contain one
          * dataset, "lon", which is a dimscale. */
         if (nc_create(CHARLIE_TEST_FILE, create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LON, DIM1_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LON, NC_FLOAT, NDIM1, &dimid, &varid)) ERR;

         /* Check the file. */
         if (check_charlies_no_enddef_file(ncid, LON, LON)) ERR;

         /* Rename the dimension. This will cause lon to stop being a
          * coord var and dimscale, and prepare to create a new
          * dimscale without var dataset "longitude". Dataset "lon"
          * will point to "longitude" as its dimscale. */
         if (nc_rename_dim(ncid, 0, LONGITUDE)) ERR;
         if (check_charlies_no_enddef_file(ncid, LONGITUDE, LON)) ERR;

         /* Trigger write to disk. */
         if (nc_enddef(ncid)) ERR;
         if (nc_redef(ncid)) ERR;
         if (check_charlies_no_enddef_file(ncid, LONGITUDE, LON)) ERR;

         /* Rename the variable. This will remove the (as yet
          * unwritten) dimscale-only dataset "longitude" and rename
          * the existing dataset "lon" to "longitude". Variable
          * "longitude" will become a coordinate var. */
         if (nc_rename_var(ncid, 0, LONGITUDE)) ERR;
         if (check_charlies_no_enddef_file(ncid, LONGITUDE, LONGITUDE)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen the file and check. */
         if (check_charlies_file(CHARLIE_TEST_FILE, LONGITUDE, LONGITUDE)) ERR;

      }
      SUMMARIZE_ERR;
      printf("*** Test Charlie's test for renaming with enddef...");
      {
         int ncid, dimid, varid;
         float data[DIM1_LEN] = {0, 90.0, 180.0, 270.0};

         /* Create a nice, simple file. This file will contain one
          * dataset, "lon", which is a dimscale. */
         if (nc_create(CHARLIE_TEST_FILE, create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LON, DIM1_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LON, NC_FLOAT, NDIM1, &dimid, &varid)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_put_var_float(ncid, varid, data)) ERR;
         if (nc_close(ncid)) ERR;

         /* Check the file. */
         if (check_charlies_file(CHARLIE_TEST_FILE, LON, LON)) ERR;

         /* Open the file and rename the dimension. This will cause
          * lon to stop being a coord var and dimscale, and create a
          * new dimscale without var dataset "longitude". Dataset
          * "lon" will point to "longitude" as its dimscale. */
         if (nc_open(CHARLIE_TEST_FILE, NC_NEXTCDF4 | NC_WRITE, &ncid)) ERR;
         if (nc_redef(ncid)) ERR;
         if (nc_rename_dim(ncid, 0, LONGITUDE)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen the file to check. */
         if (check_charlies_file(CHARLIE_TEST_FILE, LONGITUDE, LON)) ERR;

         /* Open the file and rename the variable. This will remove
          * the dimscale-only dataset "longitude" and rename the
          * existing dataset "lon" to "longitude". Variable
          * "longitude" will become a coordinate var. */
         if (nc_open(CHARLIE_TEST_FILE, NC_NEXTCDF4 | NC_WRITE, &ncid)) ERR;
         if (nc_redef(ncid)) ERR;
         if (nc_rename_var(ncid, 0, LONGITUDE)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen the file to check. */
         if (check_charlies_file(CHARLIE_TEST_FILE, LONGITUDE, LONGITUDE)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** testing renaming before enddef for %s...", fmt_names[format]);
      {
         int ncid, varid, var2id;
         int dimid;

         if (nc_create(file_names[format], create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LAT, DIM_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LAT, NC_INT, VAR_RANK, &dimid, &varid)) ERR;
         if (nc_def_var(ncid, RH, NC_FLOAT, VAR_RANK, &dimid, &var2id)) ERR;

         /* Now rename the dim. */
         if (nc_rename_dim(ncid, dimid, TAL)) ERR;
         if (nc_rename_var(ncid, varid, TAL)) ERR;

         if (nc_enddef(ncid)) ERR;    /* not necessary for netCDF-4 files */
         if (nc_put_var_int(ncid, varid, lats)) ERR;
         if (nc_put_var_float(ncid, var2id, rh)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen and check. */
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (check_file(ncid, TAL, RH, TAL)) ERR;
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** testing renaming after enddef for %s...", fmt_names[format]);
      {
         int ncid, varid, var2id;
         int dimid;

         if (nc_create(file_names[format], create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LAT, DIM_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, TAL1, NC_INT, VAR_RANK, &dimid, &varid)) ERR;
         if (nc_def_var(ncid, RH, NC_FLOAT, VAR_RANK, &dimid, &var2id)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_redef(ncid)) ERR;

         if (nc_rename_var(ncid, varid, TAL2)) ERR;
         if (nc_rename_dim(ncid, dimid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;

         if (nc_put_var_int(ncid, varid, lats)) ERR;
         if (nc_put_var_float(ncid, var2id, rh)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen and check. */
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (check_file(ncid, TAL2, RH, TAL)) ERR;
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;
      printf("*** testing more renaming after enddef for %s...", fmt_names[format]);
      {
         int ncid, varid, var2id;
         int dimid;

         /* This will create a HDF5 file with two datasets, RH, and
          * LAT. LAT is a dimscale. RH points to dimscale LAT. Life is
          * so simple. */
         if (nc_create(file_names[format], create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LAT, DIM_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LAT, NC_INT, VAR_RANK, &dimid, &varid)) ERR;
         if (nc_def_var(ncid, RH, NC_FLOAT, VAR_RANK, &dimid, &var2id)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_redef(ncid)) ERR;

         /* This will cause dataset LAT to be renamed TAL. It will no
          * longer be a dimscale. Dataset LAT will be created as a
          * dimscale without a variable. Datasets RH and TAL will
          * re-point to (new) dimscale LAT. */
         if (nc_rename_var(ncid, varid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;

         if (nc_redef(ncid)) ERR;

         /* This will cause dimscale-only dataset LAT to be
          * deleted. Existing dataset TAL will become the dimscale
          * dataset. Dataset RH will re-point to dimscale TAL. */
         if (nc_rename_dim(ncid, dimid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;

         /* Varids have changed, so get them again. */
         if (nc_inq_varid(ncid, TAL, &varid)) ERR;
         if (nc_inq_varid(ncid, RH, &var2id)) ERR;

         /* Write some data. */
         if (nc_put_var_int(ncid, varid, lats)) ERR;
         if (nc_put_var_float(ncid, var2id, rh)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen and check. */
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (check_file(ncid, TAL, RH, TAL)) ERR;
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** testing renaming after enddef for %s...", fmt_names[format]);
      {
         int ncid, varid, var2id;
         int dimid;

         if (nc_create(file_names[format], create_modes[format], &ncid)) ERR;
         if (nc_def_dim(ncid, LAT, DIM_LEN, &dimid)) ERR;
         if (nc_def_var(ncid, LAT, NC_INT, VAR_RANK, &dimid, &varid)) ERR;
         if (nc_def_var(ncid, RH, NC_FLOAT, VAR_RANK, &dimid, &var2id)) ERR;
         if (nc_enddef(ncid)) ERR;    /* not necessary for netCDF-4 files */
         if (nc_redef(ncid)) ERR;    /* not necessary for netCDF-4 files */

         /* Now rename the dim. */
         if (nc_rename_dim(ncid, dimid, TAL)) ERR;
         if (nc_rename_var(ncid, varid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;    /* not necessary for netCDF-4 files */

         if (nc_put_var_int(ncid, varid, lats)) ERR;
         if (nc_put_var_float(ncid, var2id, rh)) ERR;
         if (nc_close(ncid)) ERR;

         /* Reopen and check. */
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (check_file(ncid, TAL, RH, TAL)) ERR;
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** testing renaming after enddef for %s...", fmt_names[format]);
      {
         /* Create a file with datasets LAT, RH. LAT is a dimscale. RH
          * points to LAT. */
         if (create_test_file(file_names[format], create_modes[format])) ERR;
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (nc_inq_dimid(ncid, LAT, &dimid)) ERR;
         if (nc_inq_varid(ncid, LAT, &varid)) ERR;
         if (nc_inq_varid(ncid, RH, &var2id)) ERR;
         if (check_file(ncid, LAT, RH, LAT)) ERR;
         if (nc_redef(ncid)) ERR;

         /* Rename the dim. This creates new dataset TAL. LAT is no
          * longer a dimscale. RH is repointed to TAL. LAT is pointed
          * to TAL. */
         if (nc_rename_dim(ncid, dimid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_redef(ncid)) ERR;

         /* Rename the var. This will remove dimscale-only dataset
          * TAL. LAT will become a dimscale. RH will point to LAT. */
         if (nc_rename_var(ncid, varid, TAL)) ERR;
         if (nc_enddef(ncid)) ERR;
         /* This should work but does not. It is a known rename
          * bug. Rename is coming! */
         /* if (check_file(ncid, LAT, RH, TAL)) ERR; */
         if (nc_close(ncid)) ERR;

         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         /* Should work but does not. Raname issues. */
         /* if (check_file(ncid, LAT, RH, TAL)) ERR; */
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** Test renaming just coordinate variable for %s...",
             fmt_names[format]);
      {
         if (create_test_file(file_names[format], create_modes[format])) ERR;
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (nc_inq_dimid(ncid, ODIM_NAME, &dimid)) ERR;
         if (nc_inq_varid(ncid, OVAR_NAME, &varid)) ERR;
         if (nc_inq_varid(ncid, OVAR2_NAME, &var2id)) ERR;
         if (nc_redef(ncid)) ERR;
         if (nc_rename_var(ncid, varid, NVAR_NAME)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_get_var_int(ncid, varid, lats_in)) ERR;
         for (ii = 0; ii < DIM_LEN; ii++) {
            if (lats_in[ii] != lats[ii])
               fprintf(stderr, "\tlats_in[%d] is %d, should be %d\n", ii, lats_in[ii], lats[ii]);
         }
         if (nc_get_var_float(ncid, var2id, rh_in)) ERR;
         for (ii = 0; ii < DIM_LEN; ii++) {
            if (rh_in[ii] != rh[ii])
               fprintf(stderr, "\trh_in[%d] is %g, should be %g\n", ii, rh_in[ii], rh[ii]);
         }
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      printf("*** Test renaming just coordinate dimension for %s...",
             fmt_names[format]);
      {
         if (create_test_file(file_names[format], create_modes[format])) ERR;
         if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
         if (nc_inq_dimid(ncid, ODIM_NAME, &dimid)) ERR;
         if (nc_inq_varid(ncid, OVAR_NAME, &varid)) ERR;
         if (nc_inq_varid(ncid, OVAR2_NAME, &var2id)) ERR;
         if (nc_redef(ncid)) ERR;
         if (nc_rename_dim(ncid, dimid, NDIM_NAME)) ERR;
         if (nc_enddef(ncid)) ERR;
         if (nc_get_var_int(ncid, varid, lats_in)) ERR;
         for (ii = 0; ii < DIM_LEN; ii++) {
            if (lats_in[ii] != lats[ii])
               fprintf(stderr, "\tlats_in[%d] is %d, should be %d\n", ii, lats_in[ii], lats[ii]);
         }
         if (nc_get_var_float(ncid, var2id, rh_in)) ERR;
         for (ii = 0; ii < DIM_LEN; ii++) {
            if (rh_in[ii] != rh[ii])
               fprintf(stderr, "\trh_in[%d] is %g, should be %g\n", ii, rh_in[ii], rh[ii]);
         }
         if (nc_close(ncid)) ERR;
      }
      SUMMARIZE_ERR;

      if (format == 0)
      {
         printf("*** Test renaming attribute in sub-group for %s...",
                fmt_names[format]);
         {
#define DIMNAME "lon"
#define VARNAME "lon"
#define G1_VARNAME "lon"
#define OLD_NAME "units"
#define NEW_NAME "new_units"
#define CONTENTS "degrees_east"
#define RANK_lon 1
#define GRP_NAME "g1"
#define RANK_g1_lon 1

            /* IDs of file, groups, dimensions, variables, attributes */
            int ncid, g1_grp, lon_dim, lon_var, g1_lon_var;
            size_t lon_len = 4;
            char *data_in;

            /* variable shapes */
            int lon_dims[RANK_lon];
            int g1_lon_dims[RANK_g1_lon];

            if (!(data_in = malloc(strlen(CONTENTS) + 1))) ERR;

            /* Create test file */
            if (nc_create(file_names[format], NC_NEXTCDF4 | NC_CLOBBER, &ncid)) ERR;
            /* Create subgroup and outer dimension */
            if (nc_def_grp(ncid, GRP_NAME, &g1_grp)) ERR;
            if (nc_def_dim(ncid, DIMNAME, lon_len, &lon_dim)) ERR;
            /* Create outer variable and subgroup variable */
            lon_dims[0] = lon_dim;
            if (nc_def_var(ncid, VARNAME, NC_FLOAT, RANK_lon, lon_dims, &lon_var)) ERR;
            g1_lon_dims[0] = lon_dim;
            if (nc_def_var(g1_grp, G1_VARNAME, NC_FLOAT, RANK_g1_lon, g1_lon_dims, &g1_lon_var)) ERR;
            /* assign per-variable attributes */
            if (nc_put_att_text(ncid, lon_var, OLD_NAME, strlen(CONTENTS), CONTENTS)) ERR;
            if (nc_put_att_text(g1_grp, g1_lon_var, OLD_NAME, strlen(CONTENTS), CONTENTS)) ERR;
            if (nc_enddef (ncid)) ERR;
            /* write variable data */
            {
               float lon_data[4] = {0, 90, 180, 270};
               size_t start[] = {0};
               size_t count[] = {4};
               if (nc_put_vara(ncid, lon_var, start, count, lon_data)) ERR;
            }
            {
               float g1_lon_data[4] = {0, 90, 180, 270};
               size_t start[] = {0};
               size_t count[] = {4};
               if (nc_put_vara(g1_grp, g1_lon_var, start, count, g1_lon_data)) ERR;
            }
            if (nc_close(ncid)) ERR;

            /* reopen the file and rename the attribute in the subgroup */
            if (nc_open(file_names[format], open_modes[format], &ncid)) ERR;
            if (nc_inq_grp_ncid(ncid, GRP_NAME, &g1_grp)) ERR;
            if (nc_inq_varid(g1_grp,VARNAME,&g1_lon_var)) ERR;
            if (nc_rename_att(g1_grp, g1_lon_var, OLD_NAME, NEW_NAME)) ERR;
            if (nc_close(ncid)) ERR;

            /* reopen the file again and see if renamed attribute exists and
               has expected value */
            {

               if (nc_open(file_names[format], NC_NEXTCDF4 | NC_NOWRITE, &ncid)) ERR;
               if (nc_inq_grp_ncid(ncid, GRP_NAME, &g1_grp)) ERR;
               if (nc_inq_varid(g1_grp, VARNAME, &g1_lon_var)) ERR;
               if (nc_get_att_text(g1_grp, g1_lon_var, NEW_NAME, data_in)) ERR;
               if (strncmp(CONTENTS, data_in, strlen(CONTENTS))) ERR;
               if (nc_close(ncid)) ERR;
            }
            free(data_in);
         }
         SUMMARIZE_ERR;
      }
   } /* next format */
   FINAL_RESULTS;
}

/* Test the jpeg compression in ncsqueeze.

   Edward Hartnett, Intelligent Data Design, Inc., 2025
   9/13/25
*/

#include "config.h"
#include "ncsqueeze.h"
#include "ncsqueeze_test.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <netcdf.h>
#include <math.h> /* Define fabs(), powf(), round() */

#define TEST_NAME "tst_jpeg"
#define STR_LEN 255
#define MAX_LEN 1024
#define X_NAME "X"
#define Y_NAME "Y"
#define NDIM2 2
#define NUM_ELEMENTS 6
#define MAX_NAME_LEN 50
#define ELEMENTS_NAME "Elements"
#define VAR_NAME "Wacky_Woolies"
#define NDIMS 2
#define NX 60
#define NY 120
#define DEFLATE_LEVEL 3
#define SIMPLE_VAR_NAME "data"

/* Err is used to keep track of errors within each set of tests,
 * total_err is the number of errors in the entire test program, which
 * generally cosists of several sets of tests. */
static int total_err = 0, err = 0;

int
main()
{
    printf("\n*** Checking HDF5 Jpeg compression.\n");
    {
        int ncid;
        int dimid[NDIM2];
        int varid;
        int data_out[NX][NY];
        int x, y;
        int quality_factor_in, nx_in, ny_in, rgb_in, jpeg;
	char file_name[NC_MAX_NAME + 1];
	int f;

	for (f = 0; f < 2; f++)
	{
	    if (f)
		printf("*** Checking simple jpeg filter...");
	    else
		printf("*** Creating uncompressed file...");
	    
	    /* Create some data to write. */
	    for (x = 0; x < NX; x++)
		for (y = 0; y < NY; y++)
		    data_out[x][y] = x * NY + y;

	    

	    /* Create file. */
	    sprintf(file_name, "%s_%s.nc", TEST_NAME, (f ? "jpeg" : "none"));
	    if (nc_create(file_name, NC_NETCDF4, &ncid)) ERR;

	    /* Create dims. */
	    if (nc_def_dim(ncid, X_NAME, NX, &dimid[0])) ERR;
	    if (nc_def_dim(ncid, Y_NAME, NY, &dimid[1])) ERR;

	    /* Create the variable. */
	    if (nc_def_var(ncid, VAR_NAME, NC_INT, NDIM2, dimid, &varid)) ERR;

	    /* These won't work. */
	    if (nc_def_var_jpeg(ncid, varid, -1, 1, 1, 1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 0, 1, 1, 1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 101, 1, 1, 1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 1, -1, 1, 1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 1, 1, -1, 1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 1, 1, 1, -1) != NC_EINVAL) ERR;
	    if (nc_def_var_jpeg(ncid, varid, 1, 1, 1, 2) != NC_EINVAL) ERR;

	    /* Check setting. */
	    if (nc_inq_var_jpeg(ncid, varid, &jpeg, &quality_factor_in, &nx_in, &ny_in, &rgb_in)) ERR;
	    if (jpeg) ERR;

	    /* Set up compression. */
	    if (f)
		if (nc_def_var_jpeg(ncid, varid, 10, 8, 8, 1)) ERR;

	    /* Check setting. */
	    if (nc_inq_var_jpeg(ncid, varid, &jpeg, &quality_factor_in, &nx_in, &ny_in, &rgb_in)) ERR;
	    if (f)
	    {
		if (!jpeg || quality_factor_in != 10 || nx_in != 8 || ny_in != 8 || rgb_in != 1) ERR;
		quality_factor_in = 0;
		nx_in = 0;
		ny_in = 0;
		rgb_in = 42;
		jpeg = 0;
		if (nc_inq_var_jpeg(ncid, varid, &jpeg, NULL, NULL, NULL, NULL)) ERR;
		if (!jpeg) ERR;
		if (nc_inq_var_jpeg(ncid, varid, NULL, &quality_factor_in, NULL, NULL, NULL)) ERR;
		if (quality_factor_in != 10) ERR;
		if (nc_inq_var_jpeg(ncid, varid, NULL, NULL, &nx_in, NULL, NULL)) ERR;
		if (nx_in != 8) ERR;
		if (nc_inq_var_jpeg(ncid, varid, NULL, NULL, NULL, &ny_in, NULL)) ERR;
		if (ny_in != 8) ERR;
		if (nc_inq_var_jpeg(ncid, varid, NULL, NULL, NULL, NULL, &rgb_in)) ERR;
		if (rgb_in != 1) ERR;
		if (nc_inq_var_jpeg(ncid, varid, NULL, NULL, NULL, NULL, NULL)) ERR;
	    }
	    else
	    {
		if (jpeg) ERR;
	    }

	    /* Write the data. */
	    if (nc_put_var(ncid, varid, data_out)) ERR;

	    /* Close the file. */
	    if (nc_close(ncid)) ERR;

	    {
		int data_in[NX][NY];

		/* Now reopen the file and check. */
		if (nc_open(file_name, NC_NETCDF4, &ncid)) ERR;

		/* Check setting. */
		if (nc_inq_var_jpeg(ncid, varid, &jpeg, &quality_factor_in, &nx_in, &ny_in, &rgb_in)) ERR;
		if (f)
		{
		    if (!jpeg || quality_factor_in != 10 || nx_in != 8 || ny_in != 8 || rgb_in != 1) ERR;
		}
		else
		    if (jpeg) ERR;
			

		/* Read the data. */
		if (nc_get_var(ncid, varid, data_in)) ERR;

		/* Check the data. */
		for (x = 0; x < NX; x++)
		    for (y = 0; y < NY; y++)
			if (data_in[x][y] != data_out[x][y]) ERR;

		/* Close the file. */
		if (nc_close(ncid)) ERR;
	    } 
	    SUMMARIZE_ERR;
        }  /* next f */
    }
    FINAL_RESULTS;
}

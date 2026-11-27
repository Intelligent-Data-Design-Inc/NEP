/**
 * Fun with 16-bit floats.
 *
 * 16-bit floats come in two flavors. Let's have some fun with them.
 *
 * @author Edward Hartnett 11/26/26
 */

#include <netcdf.h>
#include <nep.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAME "fun_float16.nc"
#define NDIM2 2
#define NY 3
#define NX 5
#define DIM_Y_NAME "Y"
#define DIM_X_NAME "X"
#define VAR1_NAME "var1"
#define VAR2_NAME "var2"
#define PI 3.1415927
/* Half the worst-case mantissa spacing for values up to 16. */
#define BFLOAT16_EPSILON (1.0f/16)   /* 7 mantissa bits, spacing 1/8 */
#define FLOAT16_EPSILON (1.0f/128)   /* 10 mantissa bits, spacing 1/64 */

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

int
main(int argc, char **argv)
{
    int ncid, dimid[NDIM2], varid_1, varid_2;
    float data[NY][NX], data_in[NY][NX];
    int x, y, ret;

    printf("Having fun with float16 types...\n");
    nep_set_log_level(4);
    for (y = 0; y < NY; y++)
	for (x = 0; x < NX; x++)
	    data[y][x] =  PI + y * NY + x;

    /* Create file. */
    printf("\nCreating file with float16 types...\n");
    if ((ret = nc_create(FILENAME, NC_NEXTCDF4, &ncid)))
	ERR(ret);

    /* Create dims. */
    if ((ret = nc_def_dim(ncid, DIM_Y_NAME, NY, &dimid[0])))
	ERR(ret);
    if ((ret = nc_def_dim(ncid, DIM_X_NAME, NX, &dimid[1])))
	ERR(ret);

    /* Create float16 var and bfloat16 var. */
    if ((ret = nc_def_var(ncid, VAR1_NAME, NC_BFLOAT16, NDIM2, dimid, &varid_1)))
	ERR(ret);
    if ((ret = nc_def_var(ncid, VAR2_NAME, NC_FLOAT16, NDIM2, dimid, &varid_2)))
	ERR(ret);
    
    /* Write some data. */
    if ((ret = nc_put_var_float(ncid, varid_1, &data[0][0])))
	ERR(ret);
    if ((ret = nc_put_var_float(ncid, varid_2, &data[0][0])))
	ERR(ret);

    /* Close the file. */
    if ((ret = nc_close(ncid)))
	ERR(ret);

    /* Reopen the file. */
    printf("\nOpening file with float16 types...\n");
    if ((ret = nc_open(FILENAME, NC_NEXTCDF4, &ncid)))
	ERR(ret);

    /* Look up the vars in the reopened file. */
    if ((ret = nc_inq_varid(ncid, VAR1_NAME, &varid_1)))
	ERR(ret);
    if ((ret = nc_inq_varid(ncid, VAR2_NAME, &varid_2)))
	ERR(ret);

    /* Read var1 (bfloat16) and check the data within bfloat16
     * rounding tolerance. */
    printf("\nReading and checking bfloat16 data...\n");
    if ((ret = nc_get_var_float(ncid, varid_1, &data_in[0][0])))
	ERR(ret);
    for (y = 0; y < NY; y++)
	for (x = 0; x < NX; x++)
	    if (fabsf(data_in[y][x] - data[y][x]) > BFLOAT16_EPSILON)
	    {
		printf("bfloat16 mismatch at (%d, %d): got %g expected %g\n",
		       y, x, data_in[y][x], data[y][x]);
		return ERRCODE;
	    }

    /* Read var2 (float16) and check it within float16 rounding
     * tolerance. */
    printf("\nReading and checking float16 data...\n");
    if ((ret = nc_get_var_float(ncid, varid_2, &data_in[0][0])))
	ERR(ret);
    for (y = 0; y < NY; y++)
	for (x = 0; x < NX; x++)
	    if (fabsf(data_in[y][x] - data[y][x]) > FLOAT16_EPSILON)
	    {
		printf("float16 mismatch at (%d, %d): got %g expected %g\n",
		       y, x, data_in[y][x], data[y][x]);
		return ERRCODE;
	    }

    /* Close the file. */
    if ((ret = nc_close(ncid)))
	ERR(ret);
    printf("SUCCESS!\n");
    return 0;
}

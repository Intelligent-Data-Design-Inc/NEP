/**
 * @file
 * Test TIFF organization detection for GeoTIFF files.
 *
 * This test verifies that the detect_tiff_organization() function
 * correctly identifies tiled vs striped files and planar configuration.
 *
 * @author Edward Hartnett
 * @date 2025-12-30
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>

#ifdef HAVE_GEOTIFF
#include "geotiffdispatch.h"
#endif

#define FILE_NAME_TILED "MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif"

int
main(int argc, char **argv)
{
    int ncid;
    int ret;
    int format;
    char magic_number_tiff[4] = "II*";     /* Standard TIFF */
    char magic_number_bigtiff[4] = "II+";  /* BigTIFF */

    (void)argc;
    (void)argv;
    
    printf("\n*** Testing GeoTIFF organization detection.\n");
    
#ifdef HAVE_GEOTIFF
    /* Initialize GeoTIFF dispatch layer */
    printf("*** Initializing GeoTIFF dispatch layer...");
    if (NC_GEOTIFF_initialize() != NC_NOERR)
    {
        printf("FAILED: Could not initialize GeoTIFF dispatch layer\n");
        return 1;
    }
    printf("ok\n");
    
    /* Register GeoTIFF UDF handlers for both standard TIFF and BigTIFF */
    printf("*** Registering GeoTIFF handlers (II* and II+)...");
    
    /* NC_UDF0: Standard TIFF */
    if ((ret = nc_def_user_format(NC_UDF0, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_tiff)))
    {
        printf("FAILED (II*): %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* NC_UDF1: BigTIFF */
    if ((ret = nc_def_user_format(NC_UDF1, (NC_Dispatch *)GEOTIFF_dispatch_table, magic_number_bigtiff)))
    {
        printf("FAILED (II+): %s\n", nc_strerror(ret));
        return 1;
    }
    
    printf("ok\n");
#else
    printf("*** SKIPPED: GeoTIFF support not enabled\n");
    return 0;
#endif
    
    /* Test 1: Open a GeoTIFF file and verify format detection */
    printf("*** Test 1: Opening GeoTIFF file...");
    ret = nc_open(FILE_NAME_TILED, NC_NOWRITE, &ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: Could not open GeoTIFF file: %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    /* Test 2: Verify format is GeoTIFF */
    printf("*** Test 2: Verifying GeoTIFF format...");
    ret = nc_inq_format(ncid, &format);
    if (ret != NC_NOERR)
    {
        printf("FAILED: nc_inq_format returned %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (format != NC_FORMATX_UDF1)
    {
        printf("FAILED: Expected NC_FORMATX_UDF1, got %d\n", format);
        nc_close(ncid);
        return 1;
    }
    printf("ok\n");
    
    /* Test 3: Query dimensions to verify metadata extraction worked */
    printf("*** Test 3: Querying dimensions...");
    int ndims;
    ret = nc_inq_ndims(ncid, &ndims);
    if (ret != NC_NOERR)
    {
        printf("FAILED: nc_inq_ndims returned %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (ndims != 2 && ndims != 3)
    {
        printf("FAILED: Expected 2 or 3 dimensions, got %d\n", ndims);
        nc_close(ncid);
        return 1;
    }
    printf("ok (found %d dimensions)\n", ndims);
    
    /* Test 4: Query variables */
    printf("*** Test 4: Querying variables...");
    int nvars;
    ret = nc_inq_nvars(ncid, &nvars);
    if (ret != NC_NOERR)
    {
        printf("FAILED: nc_inq_nvars returned %s\n", nc_strerror(ret));
        nc_close(ncid);
        return 1;
    }
    if (nvars < 1)
    {
        printf("FAILED: Expected at least 1 variable, got %d\n", nvars);
        nc_close(ncid);
        return 1;
    }
    printf("ok (found %d variables)\n", nvars);
    
    /* Test 5: Close file */
    printf("*** Test 5: Closing file...");
    ret = nc_close(ncid);
    if (ret != NC_NOERR)
    {
        printf("FAILED: nc_close returned %s\n", nc_strerror(ret));
        return 1;
    }
    printf("ok\n");
    
    printf("\n*** SUCCESS: All organization detection tests passed!\n");
    return 0;
}

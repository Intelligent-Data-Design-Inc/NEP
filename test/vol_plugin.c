#include "grib2_vol_connector.h"

#include <hdf5.h>
#include <stdlib.h>

/* herr_t values from H5private.h */
#define SUCCEED    0                                                             
#define FAIL    (-1) 

/* Testing macros from h5test.h
 *
 * The name of the test is printed by saying TESTING("something") which will     
 * result in the string `Testing something' being flushed to standard output.    
 * If a test passes, fails, or is skipped then the PASSED(), H5_FAILED(), or     
 * SKIPPED() macro should be called.  After H5_FAILED() or SKIPPED() the caller  
 * should print additional information to stdout indented by at least four       
 * spaces.  If the h5_errors() is used for automatic error handling then         
 * the H5_FAILED() macro is invoked automatically when an API function fails.    
 */                                                                              

#define AT()                printf ("   at %s:%d...\n", __FILE__, __LINE__);   
#define TESTING(WHAT)       {printf("Testing %-62s", WHAT); fflush(stdout);}       
#define PASSED()            {puts(" PASSED"); fflush(stdout);}                          
#define H5_FAILED()         {puts("*FAILED*"); fflush(stdout);}                      
#define H5_WARNING()        {puts("*WARNING*"); fflush(stdout);}                    
#define SKIPPED()           {puts(" -SKIP-"); fflush(stdout);}                         
#define PUTS_ERROR(s)       {puts(s); AT(); goto error;}                           
#define TEST_ERROR          {H5_FAILED(); AT(); goto error;}   
#define ERR                 {H5_FAILED(); AT(); goto error;}                         
#define STACK_ERROR         {H5Eprint2(H5E_DEFAULT, stdout); goto error;}            
#define FAIL_STACK_ERROR    {H5_FAILED(); AT(); H5Eprint2(H5E_DEFAULT, stdout); goto error;}
#define FAIL_PUTS_ERROR(s)  {H5_FAILED(); AT(); puts(s); goto error;} 



/*-------------------------------------------------------------------------
 * Function:    test_registration_by_value()
 *
 * Purpose:     Tests if we can load, register, and close a VOL
 *              connector by value.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
test_registration_by_value(void)
{
    htri_t  is_registered   = FAIL;
    hid_t   vol_id          = H5I_INVALID_HID;

    TESTING("VOL registration by value");

    /* The VOL connector should not be registered at the start of the test */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    /* Register the connector by value */
    if((vol_id = H5VLregister_connector_by_value(GRIB2_VOL_CONNECTOR_VALUE, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* The connector should be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(false == is_registered)
        FAIL_PUTS_ERROR("VOL connector was not registered");

    /* Unregister the connector */
    if(H5VLunregister_connector(vol_id) < 0)
        TEST_ERROR;

    /* The connector should not be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    PASSED();
    return SUCCEED;

error:
    H5E_BEGIN_TRY {
        H5VLunregister_connector(vol_id);
    } H5E_END_TRY;
    return FAIL;

} /* end test_registration_by_value() */


/*-------------------------------------------------------------------------
 * Function:    test_registration_by_name()
 *
 * Purpose:     Tests if we can load, register, and close a VOL
 *              connector by name.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
test_registration_by_name(void)
{
    htri_t  is_registered   = FAIL;
    hid_t   vol_id          = H5I_INVALID_HID;

    TESTING("VOL registration by name");

    /* The VOL connector should not be registered at the start of the test */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    /* Register the connector by name */
    if((vol_id = H5VLregister_connector_by_name(GRIB2_VOL_CONNECTOR_NAME, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* The connector should be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(false == is_registered)
        FAIL_PUTS_ERROR("VOL connector was not registered");

    /* Unregister the connector */
    if(H5VLunregister_connector(vol_id) < 0)
        TEST_ERROR;

    /* The connector should not be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    PASSED();
    return SUCCEED;

error:
    H5E_BEGIN_TRY {
        H5VLunregister_connector(vol_id);
    } H5E_END_TRY;
    return FAIL;

} /* end test_registration_by_name() */

/*-------------------------------------------------------------------------
 * Function:    test_registration_by_name()
 *
 * Purpose:     Tests if we can load, register, and close a VOL
 *              connector by name.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
test_file_open_close(void)
{
    htri_t  is_registered   = FAIL;
    hid_t   vol_id          = H5I_INVALID_HID;
#define GRIB2_TEST_FILE "gdaswave.t00z.wcoast.0p16.f000"

    TESTING("VOL file open/close");

    /* The VOL connector should not be registered at the start of the test */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    /* Register the connector by name */
    if((vol_id = H5VLregister_connector_by_name(GRIB2_VOL_CONNECTOR_NAME, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* The connector should be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(false == is_registered)
        FAIL_PUTS_ERROR("VOL connector was not registered");


    /* Open our test GRIB2 file. */
    printf("\n*** Checking GRIB2 file open and close...");
    {
        ssize_t objs;
        hid_t fileid, access_plist, access_plist2;
        #define FILE_NAME "tst_vol_plugin.h5"

        printf("\nabout to create HDF5 file.\n");
        /* Set the access list so that closes will fail if something is
         * still open in the file. */
        if ((access_plist = H5Pcreate(H5P_FILE_ACCESS)) < 0) ERR;
        if (H5Pset_fclose_degree(access_plist, H5F_CLOSE_SEMI)) ERR;

        /* Create file. */
        if ((fileid = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT,
                                access_plist)) < 0) ERR;

        /* Turn off HDF5 error messages. */
        /* if (H5Eset_auto1(NULL, NULL) < 0) ERR; */

        /* Now close the file. */
        if (H5Fclose(fileid) < 0) ERR;

        printf("about to open HDF5 file.\n");
        /* Open the file. */
        if ((fileid = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0) ERR;

        /* Now close the file. */
        if (H5Fclose(fileid) < 0) ERR;

        printf("about to create new access plist\n");
        if ((access_plist2 = H5Pcreate(H5P_FILE_ACCESS)) < 0) ERR;

     }
  

    
    /* Close the GRIB2 file. */

    /* Unregister the connector */
    if(H5VLunregister_connector(vol_id) < 0)
        TEST_ERROR;

    /* The connector should not be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    PASSED();
    return SUCCEED;

error:
    H5E_BEGIN_TRY {
        H5VLunregister_connector(vol_id);
    } H5E_END_TRY;
    return FAIL;

} /* end test_file_open_close() */
/*-------------------------------------------------------------------------
 * Function:    test_multiple_registration()
 *
 * Purpose:     Tests if we can register a VOL connector multiple times.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
#define N_REGISTRATIONS 10
static herr_t
test_multiple_registration(void)
{
    htri_t  is_registered   = FAIL;
    hid_t   vol_ids[N_REGISTRATIONS];
    int     i;

    TESTING("registering a VOL connector multiple times");

    /* The VOL connector should not be registered at the start of the test */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    /* Register the connector multiple times */
    for(i = 0; i < N_REGISTRATIONS; i++) {
        if((vol_ids[i] = H5VLregister_connector_by_name(GRIB2_VOL_CONNECTOR_NAME, H5P_DEFAULT)) < 0)
            TEST_ERROR;
    }

    /* The connector should be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(false == is_registered)
        FAIL_PUTS_ERROR("VOL connector was not registered");

    /* Unregister the connector */
    for(i = 0; i < N_REGISTRATIONS; i++) {
        if(H5VLunregister_connector(vol_ids[i]) < 0)
            TEST_ERROR;
        /* Also test close on some of the IDs. This call currently works
         * identically to unregister.
         */
        i++;
        if(H5VLclose(vol_ids[i]) < 0)
            TEST_ERROR;
    }

    /* The connector should not be registered now */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    PASSED();
    return SUCCEED;

error:
    H5E_BEGIN_TRY {
        for(i = 0; i < N_REGISTRATIONS; i++)
            H5VLunregister_connector(vol_ids[i]);
    } H5E_END_TRY;
    return FAIL;

} /* end test_multiple_registration() */


/*-------------------------------------------------------------------------
 * Function:    test_getters()
 *
 * Purpose:     Tests H5VL getters
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
test_getters(void)
{
    htri_t  is_registered   = FAIL;
    hid_t   vol_id          = H5I_INVALID_HID;
    hid_t   vol_id_out      = H5I_INVALID_HID;

    TESTING("VOL getters");

    /* The VOL connector should not be registered at the start of the test */
    if((is_registered = H5VLis_connector_registered_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(true == is_registered)
        FAIL_PUTS_ERROR("VOL connector is inappropriately registered");

    /* Register the connector by name */
    if((vol_id = H5VLregister_connector_by_name(GRIB2_VOL_CONNECTOR_NAME, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* Get the connector's ID by name */
    if((vol_id_out = H5VLget_connector_id_by_name(GRIB2_VOL_CONNECTOR_NAME)) < 0)
        TEST_ERROR;
    if(vol_id != vol_id_out)
        FAIL_PUTS_ERROR("VOL connector IDs don't match");

    /* Unregister the connector */
    if(H5VLunregister_connector(vol_id) < 0)
        TEST_ERROR;

    PASSED();
    return SUCCEED;

error:
    H5E_BEGIN_TRY {
        H5VLunregister_connector(vol_id);
    } H5E_END_TRY;
    return FAIL;

} /* end test_getters() */


/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Tests VOL connector plugin operations
 *
 * Return:      EXIT_SUCCESS/EXIT_FAILURE
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    int nerrors = 0;
    char *path = NULL;

    puts("Testing VOL connector plugin functionality.");

    path = getenv("HDF5_PLUGIN_PATH");
    printf("HDF5_PLUGIN_PATH = ");
    if (path)
        printf("%s\n", path);
    else
        printf("NULL\n");

    nerrors += test_registration_by_name() < 0          ? 1 : 0;
    nerrors += test_file_open_close() < 0          ? 1 : 0;
    nerrors += test_registration_by_value() < 0         ? 1 : 0;
    nerrors += test_multiple_registration() < 0         ? 1 : 0;
    nerrors += test_getters() < 0                       ? 1 : 0;

    if (nerrors) {
        printf("***** %d VOL connector plugin TEST%s FAILED! *****\n",
            nerrors, nerrors > 1 ? "S" : "");
        exit(EXIT_FAILURE);
    }

    puts("All VOL connector plugin tests passed.");

    exit(EXIT_SUCCESS);

} /* end main() */


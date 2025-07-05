/**
 * @file    grib2_vol_connector.c
 * @brief   Implementation of the GRIB2 VOL Connector for HDF5
 *
 * @details This file implements the GRIB2 VOL connector for HDF5, which provides
 *          a bridge between HDF5 and GRIB2 file formats. The connector implements
 *          the HDF5 Virtual Object Layer (VOL) interface to enable HDF5 applications
 *          to work with GRIB2 files transparently.
 *
 *          The implementation includes the VOL class structure initialization
 *          and the necessary plugin interface functions required by HDF5.
 *
 * @copyright Copyright Intelligent Data Design, Inc.
 *            All rights reserved.
 *
 */

/* This connector's header */
#include "grib2_vol_connector.h"

/* HDF5 headers */
#include <hdf5.h>
#include <H5PLextern.h>

/* Standard library headers */
#include <stdlib.h>

/**
 * @brief The GRIB2 VOL class structure
 *
 * This structure defines the GRIB2 VOL connector's class and contains function
 * pointers to all the VOL callback functions. The structure is initialized with
 * NULL values for most callbacks, indicating that the default HDF5 behavior
 * should be used.
 *
 * The structure follows the H5VL_class_t type definition from the HDF5 VOL API.
 */
static const H5VL_class_t grib2_class_g = {
    3,                                              /* VOL class struct version */
    GRIB2_VOL_CONNECTOR_VALUE,                   /* value                    */
    GRIB2_VOL_CONNECTOR_NAME,                    /* name                     */
    1,                                              /* version                  */
    0,                                              /* capability flags         */
    NULL,                                           /* initialize               */
    NULL,                                           /* terminate                */
    {   /* info_cls */
        (size_t)0,                                  /* size    */
        NULL,                                       /* copy    */
        NULL,                                       /* compare */
        NULL,                                       /* free    */
        NULL,                                       /* to_str  */
        NULL,                                       /* from_str */
    },
    {   /* wrap_cls */
        NULL,                                       /* get_object   */
        NULL,                                       /* get_wrap_ctx */
        NULL,                                       /* wrap_object  */
        NULL,                                       /* unwrap_object */
        NULL,                                       /* free_wrap_ctx */
    },
    {   /* attribute_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* read         */
        NULL,                                       /* write        */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* dataset_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* read         */
        NULL,                                       /* write        */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* datatype_cls */
        NULL,                                       /* commit       */
        NULL,                                       /* open         */
        NULL,                                       /* get_size     */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* file_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* group_cls */
        NULL,                                       /* create       */
        NULL,                                       /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* close        */
    },
    {   /* link_cls */
        NULL,                                       /* create       */
        NULL,                                       /* copy         */
        NULL,                                       /* move         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* object_cls */
        NULL,                                       /* open         */
        NULL,                                       /* copy         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* introscpect_cls */
        NULL,                                       /* get_conn_cls  */
        NULL,                                       /* get_cap_flags */
        NULL                                        /* opt_query     */
    },
    {   /* request_cls */
        NULL,                                       /* wait         */
        NULL,                                       /* notify       */
        NULL,                                       /* cancel       */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        NULL                                        /* free         */
    },
    {   /* blob_cls */
        NULL,                                       /* put          */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL                                        /* optional     */
    },
    {   /* token_cls */
        NULL,                                       /* cmp          */
        NULL,                                       /* to_str       */
        NULL                                        /* from_str     */
    },
    NULL                                            /* optional     */
};

/**
 * @name Plugin Interface Functions
 *
 * These functions are required for the HDF5 plugin system to load and
 * identify this VOL connector. They provide the necessary entry points
 * for HDF5 to discover and initialize the connector.
 */
/**@{*/

/**
 * @brief Get the plugin type
 *
 * This function is called by the HDF5 library to determine the type of plugin.
 * For VOL connectors, it should return H5PL_TYPE_VOL.
 *
 * @return The plugin type (H5PL_TYPE_VOL for VOL connectors)
 */

H5PL_type_t H5PLget_plugin_type(void)
{
    return H5PL_TYPE_VOL;
}

/**
 * @brief Get plugin information
 *
 * This function is called by the HDF5 library to get information about
 * the plugin. It should set the plugin's VOL class structure.
 *
 * @param[out] info Pointer to the plugin info structure to be filled
 */
void H5PLget_plugin_info(void) {return &grib2_class_g;}

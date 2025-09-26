/**
 * @file grib2_vol_connector.c
 * @brief Implementation of a simple virtual object layer (VOL) connector for HDF5.
 *
 * Copyright Intelligent Data Design, Inc.
 * All rights reserved.
 *
 * Purpose: A simple virtual object layer (VOL) connector with almost no
 * functionality that can serve as a template for creating other connectors.
 */

/* This connector's header */
#include "config.h"
#include "grib2_vol_connector.h"

#include <hdf5.h>
#include <H5PLextern.h>
#include <H5PLpublic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* HDF5 return codes */
#ifndef SUCCEED
#define SUCCEED 0
#endif
#ifndef FAIL  
#define FAIL (-1)
#endif

/* NCEPLIBS-g2 library for GRIB2 file operations */
#include <grib2.h>

/**
 * @brief Structure to hold GRIB2 file information for VOL connector
 */
typedef struct grib2_file_t {
    char *filename;     /* Name of the GRIB2 file */
    int g2cid;          /* ID of the open GRIB2 file */
    int is_open;       /* Flag indicating if file is open */
} grib2_file_t;

/* Forward declarations */
static void *grib2_file_open(const char *name, unsigned flags, hid_t fapl_id, 
                             hid_t dxpl_id, void **req);
static herr_t grib2_file_close(void *file, hid_t dxpl_id, void **req);

/* The VOL class struct */
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
        grib2_file_open,                           /* open         */
        NULL,                                       /* get          */
        NULL,                                       /* specific     */
        NULL,                                       /* optional     */
        grib2_file_close                           /* close        */
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
 * @brief These two functions are necessary to load this plugin using the HDF5 library.
 */

/**
 * @brief Open a GRIB2 file through the VOL connector
 * 
 * @param name      Name of the GRIB2 file to open
 * @param flags     File access flags (unused for GRIB2)
 * @param fapl_id   File access property list (unused)
 * @param dxpl_id   Data transfer property list (unused)
 * @param req       Asynchronous request object (unused)
 * @return          Pointer to grib2_file_t structure on success, NULL on failure
 */
static void *
grib2_file_open(const char *name, unsigned flags, hid_t fapl_id, 
                hid_t dxpl_id, void **req)
{
    grib2_file_t *grib2_file = NULL;
    int g2cid;
    int ret;
    
    /* Unused parameters */
    (void)flags;
    (void)fapl_id;
    (void)dxpl_id;
    (void)req;
    
    /* Validate input */
    if (!name) {
        return NULL;
    }
    
    /* Attempt to open the GRIB2 file */
    ret = g2c_open(name, 0, &g2cid);
    if (!ret)
        return NULL;	

    /* Allocate and initialize grib2_file_t structure */
    grib2_file = (grib2_file_t *)malloc(sizeof(grib2_file_t));
    if (!grib2_file) {
        return NULL;
    }
    
    /* Initialize structure */
    grib2_file->filename = strdup(name);
    if (!grib2_file->filename) {
        free(grib2_file);
        return NULL;
    }
    
    grib2_file->g2cid = g2cid;
    grib2_file->is_open = 1;
    
    return (void *)grib2_file;
}

/**
 * @brief Close a GRIB2 file through the VOL connector
 * 
 * @param file      Pointer to grib2_file_t structure from grib2_file_open
 * @param dxpl_id   Data transfer property list (unused)
 * @param req       Asynchronous request object (unused)
 * @return          SUCCEED on success, FAIL on failure
 */
static herr_t
grib2_file_close(void *file, hid_t dxpl_id, void **req)
{
    grib2_file_t *grib2_file = (grib2_file_t *)file;
    
    /* Unused parameters */
    (void)dxpl_id;
    (void)req;
    
    /* Validate input */
    if (!grib2_file) {
        return FAIL;
    }
    
    /* Close file if open */
    if (grib2_file->g2cid && grib2_file->is_open) {
        g2c_close(grib2_file->g2cid);
        grib2_file->is_open = 0;
    }
    
    /* Free allocated memory */
    if (grib2_file->filename) {
        free(grib2_file->filename);
        grib2_file->filename = NULL;
    }
    
    free(grib2_file);
    
    return SUCCEED;
}

H5PL_type_t H5PLget_plugin_type(void) {return H5PL_TYPE_VOL;}
const void *H5PLget_plugin_info(void) {return &grib2_class_g;}


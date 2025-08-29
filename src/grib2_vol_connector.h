/**
 * @file grib2_vol_connector.h
 * @brief A simple virtual object layer (VOL) connector header for HDF5.
 *
 * This connector serves as a template for creating other connectors.
 */

#ifndef _grib2_vol_connector_H
#define _grib2_vol_connector_H

#include <hdf5.h>
#include <H5public.h>
#include <H5PLextern.h>
#include <H5Opublic.h>
#include <H5Lpublic.h>
#include <H5VLpublic.h>
#include <H5VLconnector.h>

/* The value must be between 256 and 65535 (inclusive) */
/**
 * @def GRIB2_VOL_CONNECTOR_VALUE
 * @brief The VOL connector value (between 256 and 65535).
 */
#define GRIB2_VOL_CONNECTOR_VALUE    ((H5VL_class_value_t)15555)

/**
 * @def GRIB2_VOL_CONNECTOR_NAME
 * @brief The VOL connector name string.
 */
#define GRIB2_VOL_CONNECTOR_NAME     "grib2_vol_connector"

#endif /* _grib2_vol_connector_H */


/**
 * @file grib2_vol_connector.h
 * @brief A simple virtual object layer (VOL) connector header for HDF5.
 *
 * Copyright Intelligent Data Design, Inc.  
 * All rights reserved.
 *
 * This connector serves as a template for creating other connectors.
 */

#ifndef _grib2_vol_connector_H
#define _grib2_vol_connector_H

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


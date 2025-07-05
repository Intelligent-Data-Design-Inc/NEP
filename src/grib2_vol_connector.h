/**
 * @file    grib2_vol_connector.h
 * @brief   GRIB2 VOL Connector for HDF5
 * 
 * @details This file defines the GRIB2 VOL connector for HDF5, which provides
 *          a bridge between HDF5 and GRIB2 file formats. The connector implements
 *          the HDF5 Virtual Object Layer (VOL) interface to enable HDF5 applications
 *          to work with GRIB2 files transparently.
 *
 *          The GRIB2 VOL connector allows HDF5 applications to read and write
 *          GRIB2 files using the standard HDF5 API, abstracting away the details
 *          of the GRIB2 format.
 *
 * @copyright Copyright Intelligent Data Design, Inc.
 *            All rights reserved.
 *
 */

#ifndef _grib2_vol_connector_H
#define _grib2_vol_connector_H

/**
 * @def GRIB2_VOL_CONNECTOR_VALUE
 * @brief Unique identifier for the GRIB2 VOL connector.
 * 
 * This value must be between 256 and 65535 (inclusive) and uniquely identifies
 * this VOL connector within the HDF5 VOL framework.
 */
#define GRIB2_VOL_CONNECTOR_VALUE    ((H5VL_class_value_t)15555)

/**
 * @def GRIB2_VOL_CONNECTOR_NAME
 * @brief The name of the GRIB2 VOL connector.
 * 
 * This string identifier is used when loading the connector via the HDF5_VOL_CONNECTOR
 * environment variable or when using the H5Pset_vol() function.
 */
#define GRIB2_VOL_CONNECTOR_NAME     "grib2_vol_connector"

#endif /* _grib2_vol_connector_H */


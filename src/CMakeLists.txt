# Set the GRIB2 VOL connector name
set (GRIB2_VOL_NAME grib2_vol_connector)

# Build the GRIB2 VOL
add_library (${GRIB2_VOL_NAME} SHARED grib2_vol_connector.c)
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES PUBLIC_HEADER "${GRIB2_VOL_NAME}.h")


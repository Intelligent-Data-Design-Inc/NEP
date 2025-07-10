# Set the GRIB2 VOL connector name
set (GRIB2_VOL_NAME grib2_vol_connector)

# Build the GRIB2 VOL
add_library (${GRIB2_VOL_NAME} SHARED grib2_vol_connector.c)
# Link external libraries (HDF5, g2c, etc.)
if (LINK_LIBS)
    target_link_libraries (${GRIB2_VOL_NAME} PRIVATE ${LINK_LIBS})
endif()
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES SOVERSION 1)
set_target_properties (${GRIB2_VOL_NAME} PROPERTIES PUBLIC_HEADER "${GRIB2_VOL_NAME}.h")

# Install the library and public header using GNUInstallDirs
install(TARGETS ${GRIB2_VOL_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# This is the build file for the src directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Build the main ncsqueeze library
add_library(ncsqueeze SHARED ncsqueeze.c)
target_link_libraries(ncsqueeze PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} ${NETCDF_LIBRARIES})
set_target_properties(ncsqueeze PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/ncsqueeze.h"
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS ncsqueeze
    EXPORT NEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Build GRIB2 VOL connector if enabled
if(HAVE_GRIB2)
    set(GRIB2_VOL_NAME grib2_vol_connector)
    add_library(${GRIB2_VOL_NAME} SHARED grib2_vol_connector.c)
    target_link_libraries(${GRIB2_VOL_NAME} PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} ${G2C_LIBRARY})
    set_target_properties(${GRIB2_VOL_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/${GRIB2_VOL_NAME}.h"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${GRIB2_VOL_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building GRIB2 VOL connector: ${GRIB2_VOL_NAME}")
endif()


# Build GeoTIFF VOL connector if enabled
if(HAVE_GEOTIFF)
    set(GEOTIFF_VOL_NAME geotiff_vol_connector)
    add_library(${GEOTIFF_VOL_NAME} SHARED geotiff_vol_connector.c)
    target_link_libraries(${GEOTIFF_VOL_NAME} PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} ${GEOTIFF_LIBRARY})
    set_target_properties(${GEOTIFF_VOL_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/${GEOTIFF_VOL_NAME}.h"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${GEOTIFF_VOL_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building GeoTIFF VOL connector: ${GEOTIFF_VOL_NAME}")
endif()

# Build CDF VOL connector if enabled
if(HAVE_CDF)
    set(CDF_VOL_NAME cdf_vol_connector)
    add_library(${CDF_VOL_NAME} SHARED cdf_vol_connector.c)
    target_link_libraries(${CDF_VOL_NAME} PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} ${CDF_LIBRARY})
    set_target_properties(${CDF_VOL_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/${CDF_VOL_NAME}.h"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${CDF_VOL_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building CDF VOL connector: ${CDF_VOL_NAME}")
endif()

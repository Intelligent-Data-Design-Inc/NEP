# This is the build file for the src directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Build the main nep library
add_library(nep SHARED nep.c)
target_link_libraries(nep PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} ${NETCDF_LIBRARIES})
set_target_properties(nep PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/nep.h;${CMAKE_SOURCE_DIR}/include/NEP.h"
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS nep
    EXPORT NEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# CDF UDF Handler - v1.3.0 Sprint 3
# Build CDF library if enabled (netCDF UDF support)
if(HAVE_CDF)
    set(CDF_LIB_NAME nccdf)
    add_library(${CDF_LIB_NAME} SHARED 
        cdfdispatch.c cdffile.c cdffunc.c cdfvar.c)
    target_include_directories(${CDF_LIB_NAME} PRIVATE ${CDF_INCLUDE_DIR})
    target_link_libraries(${CDF_LIB_NAME} PRIVATE 
        ${HDF5_C_${LIB_TYPE}_LIBRARY} ${NETCDF_LIBRARIES} ${CDF_LIBRARY})
    set_target_properties(${CDF_LIB_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${CDF_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building CDF UDF library: ${CDF_LIB_NAME}")
endif()

# GeoTIFF UDF Handler - v1.5.0
# Build GeoTIFF library if enabled (netCDF UDF support)
if(HAVE_GEOTIFF)
    set(GEOTIFF_LIB_NAME ncgeotiff)
    add_library(${GEOTIFF_LIB_NAME} SHARED 
        geotiffdispatch.c geotifffile.c)
    target_include_directories(${GEOTIFF_LIB_NAME} PRIVATE ${GEOTIFF_INCLUDE_DIR} ${TIFF_INCLUDE_DIR})
    target_link_libraries(${GEOTIFF_LIB_NAME} PRIVATE 
        ${HDF5_C_${LIB_TYPE}_LIBRARY} ${NETCDF_LIBRARIES} ${GEOTIFF_LIBRARY} ${TIFF_LIBRARY})
    set_target_properties(${GEOTIFF_LIB_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${GEOTIFF_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building GeoTIFF UDF library: ${GEOTIFF_LIB_NAME}")
endif()

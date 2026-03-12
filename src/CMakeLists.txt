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
install(FILES "${CMAKE_SOURCE_DIR}/include/grib2dispatch.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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

# GRIB2 UDF Handler - v1.7.0
# Build GRIB2 library if enabled (netCDF UDF support)
if(HAVE_GRIB2)
    # Locate jasper (required by libg2c at runtime)
    find_library(JASPER_LIBRARY NAMES jasper)
    find_path(JASPER_INCLUDE_DIR jasper/jasper.h)
    if(NOT JASPER_LIBRARY)
        message(FATAL_ERROR "libjasper not found (required by libg2c). Add its path to CMAKE_PREFIX_PATH.")
    endif()
    # Extract the directory containing libjasper for rpath embedding
    get_filename_component(JASPER_LIBRARY_DIR "${JASPER_LIBRARY}" DIRECTORY)
    get_filename_component(G2C_LIBRARY_DIR "${G2C_LIBRARY}" DIRECTORY)

    set(GRIB2_LIB_NAME ncgrib2)
    add_library(${GRIB2_LIB_NAME} SHARED
        grib2dispatch.c grib2file.c)
    target_include_directories(${GRIB2_LIB_NAME} PRIVATE ${G2C_INCLUDE_DIR})
    target_link_libraries(${GRIB2_LIB_NAME} PRIVATE
        ${HDF5_C_${LIB_TYPE}_LIBRARY} ${NETCDF_LIBRARIES} ${G2C_LIBRARY} ${JASPER_LIBRARY})
    set_target_properties(${GRIB2_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};${G2C_LIBRARY_DIR};${JASPER_LIBRARY_DIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH "${G2C_LIBRARY_DIR};${JASPER_LIBRARY_DIR}"
    )
    install(TARGETS ${GRIB2_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building GRIB2 UDF library: ${GRIB2_LIB_NAME}")
    message(STATUS "  g2c: ${G2C_LIBRARY}")
    message(STATUS "  jasper: ${JASPER_LIBRARY}")
endif()

# This is the build file for the root directory of the NFEP project.
# Edward Hartnett 8/27/25
cmake_minimum_required(VERSION 3.9)
project (NFEP VERSION 0.1.1 DESCRIPTION "NetCDF4/HDF5 Format Extension Pack" LANGUAGES C)

# Add option for debug build
option(DEBUG "Build with debugging symbols" OFF)

# VOL connector enable/disable options
option(ENABLE_GRIB2 "Enable GRIB2 VOL connector" ON)
option(ENABLE_BUFR "Enable BUFR VOL connector" ON)
option(ENABLE_GEOTIFF "Enable GeoTIFF VOL connector" ON)
option(ENABLE_CDF "Enable CDF VOL connector" ON)

# Documentation option
option(BUILD_DOCUMENTATION "Build documentation with Doxygen" ON)

# Set compiler flags based on DEBUG option
if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    message(STATUS "Debug mode enabled: Adding -g flag")
endif()

# Enable testing
enable_testing ()
include (CTest)

# Find HDF5
find_package(HDF5 1.10 REQUIRED COMPONENTS C)
link_directories (${HDF5_LIBRARY_DIRS})
include_directories (${HDF5_INCLUDE_DIR})
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# Initialize variables for enabled VOL connectors
set(ENABLED_VOLS "")
set(VOL_LIBRARIES "")
set(VOL_INCLUDE_DIRS "")

# Find GRIB2 dependencies if enabled
if(ENABLE_GRIB2)
    find_library(G2C_LIBRARY NAMES g2c)
    find_path(G2C_INCLUDE_DIR grib2.h)
    
    if(G2C_LIBRARY AND G2C_INCLUDE_DIR)
        message(STATUS "Found NCEPLIBS-g2: ${G2C_LIBRARY}")
        list(APPEND ENABLED_VOLS "GRIB2")
        list(APPEND VOL_LIBRARIES ${G2C_LIBRARY})
        list(APPEND VOL_INCLUDE_DIRS ${G2C_INCLUDE_DIR})
        set(HAVE_GRIB2 TRUE)
    else()
        message(FATAL_ERROR "GRIB2 VOL connector enabled but NCEPLIBS-g2 not found. Install NCEPLIBS-g2 or disable with -DENABLE_GRIB2=OFF")
    endif()
endif()

# Find BUFR dependencies if enabled
if(ENABLE_BUFR)
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    find_library(BUFR_LIBRARY NAMES bufr_4)
    find_path(BUFR_INCLUDE_DIR bufrlib.h PATH_SUFFIXES bufr_4)
    
    message(STATUS "BUFR_LIBRARY search result: ${BUFR_LIBRARY}")
    message(STATUS "BUFR_INCLUDE_DIR search result: ${BUFR_INCLUDE_DIR}")
    
    if(BUFR_LIBRARY AND BUFR_INCLUDE_DIR)
        message(STATUS "Found NCEPLIBS-bufr: ${BUFR_LIBRARY}")
        message(STATUS "Found NCEPLIBS-bufr headers: ${BUFR_INCLUDE_DIR}")
        list(APPEND ENABLED_VOLS "BUFR")
        list(APPEND VOL_LIBRARIES ${BUFR_LIBRARY})
        list(APPEND VOL_INCLUDE_DIRS ${BUFR_INCLUDE_DIR})
        set(HAVE_BUFR TRUE)
    else()
        message(FATAL_ERROR "BUFR VOL connector enabled but NCEPLIBS-bufr not found. Install NCEPLIBS-bufr or disable with -DENABLE_BUFR=OFF")
    endif()
endif()

# Find GeoTIFF dependencies if enabled
if(ENABLE_GEOTIFF)
    find_library(GEOTIFF_LIBRARY NAMES geotiff)
    find_path(GEOTIFF_INCLUDE_DIR geotiff.h)
    
    if(GEOTIFF_LIBRARY AND GEOTIFF_INCLUDE_DIR)
        message(STATUS "Found libgeotiff: ${GEOTIFF_LIBRARY}")
        list(APPEND ENABLED_VOLS "GeoTIFF")
        list(APPEND VOL_LIBRARIES ${GEOTIFF_LIBRARY})
        list(APPEND VOL_INCLUDE_DIRS ${GEOTIFF_INCLUDE_DIR})
        set(HAVE_GEOTIFF TRUE)
    else()
        message(FATAL_ERROR "GeoTIFF VOL connector enabled but libgeotiff not found. Install libgeotiff or disable with -DENABLE_GEOTIFF=OFF")
    endif()
endif()

# Find CDF dependencies if enabled
if(ENABLE_CDF)
    find_library(CDF_LIBRARY NAMES cdf)
    find_path(CDF_INCLUDE_DIR cdf.h)
    
    if(CDF_LIBRARY AND CDF_INCLUDE_DIR)
        message(STATUS "Found NASA CDF: ${CDF_LIBRARY}")
        list(APPEND ENABLED_VOLS "CDF")
        list(APPEND VOL_LIBRARIES ${CDF_LIBRARY})
        list(APPEND VOL_INCLUDE_DIRS ${CDF_INCLUDE_DIR})
        set(HAVE_CDF TRUE)
    else()
        message(FATAL_ERROR "CDF VOL connector enabled but NASA CDF library not found. Install NASA CDF library or disable with -DENABLE_CDF=OFF")
    endif()
endif()

# Check that at least one VOL connector is enabled
list(LENGTH ENABLED_VOLS NUM_ENABLED_VOLS)
if(NUM_ENABLED_VOLS EQUAL 0)
    message(FATAL_ERROR "No VOL connectors enabled. Enable at least one with -DENABLE_<VOL>=ON")
endif()

# Include directories for found dependencies
if(VOL_INCLUDE_DIRS)
    include_directories(${VOL_INCLUDE_DIRS})
endif()

# Set up link libraries
set(LINK_LIBS ${LINK_LIBS} ${VOL_LIBRARIES})

# Print summary of enabled VOL connectors
message(STATUS "Enabled VOL connectors: ${ENABLED_VOLS}")

# It's really easy to pick up the wrong HDF5 library if you set the path
# wrong. Turn this on for added confirmation that you got it right.
#message (DEPRECATION "Include: ${HDF5_INCLUDE_DIR}")

# Pass configuration to source directory
configure_file(${CMAKE_SOURCE_DIR}/src/nfep_config.h.in ${CMAKE_BINARY_DIR}/nfep_config.h)

include(GNUInstallDirs)
add_subdirectory (src)
add_subdirectory (test)

# Generate and install CMake config files
include(CMakePackageConfigHelpers)

# Configure the config file
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/NFEPConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/NFEPConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NFEP"
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/NFEPConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    "${CMAKE_BINARY_DIR}/NFEPConfig.cmake"
    "${CMAKE_BINARY_DIR}/NFEPConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NFEP"
)

# Install the export set for use with the install-tree
install(EXPORT NFEPTargets
    FILE NFEPTargets.cmake
    NAMESPACE NFEP::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NFEP"
)


# Doxygen documentation build
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        message(STATUS "Documentation generation enabled with doxygen")
    else()
        message(FATAL_ERROR "Documentation enabled but doxygen not found. Install doxygen or disable with -DBUILD_DOCUMENTATION=OFF")
    endif()
else()
    message(STATUS "Documentation generation disabled")
endif()



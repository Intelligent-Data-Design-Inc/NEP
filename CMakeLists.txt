cmake_minimum_required (VERSION 3.9)
project (grib2_vol_connector VERSION 0.0.1 DESCRIPTION "grib2 VOL connector" LANGUAGES C)

# Add option for debug build
option(DEBUG "Build with debugging symbols" OFF)

# Set compiler flags based on DEBUG option
if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    message(STATUS "Debug mode enabled: Adding -g flag")
endif()

# Enable testing
enable_testing ()
include (CTest)

# Find HDF5
find_package (HDF5 1.13 COMPONENTS C REQUIRED)
link_directories (${HDF5_LIBRARY_DIRS})
include_directories (${HDF5_INCLUDE_DIR})
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# Find g2c (NCEPLIBS-g2c) library
find_library (G2C_LIBRARY NAMES g2c REQUIRED)
find_path (G2C_INCLUDE_DIR grib2.h)
include_directories (${G2C_INCLUDE_DIR})
set (LINK_LIBS ${LINK_LIBS} ${G2C_LIBRARY})

# It's really easy to pick up the wrong HDF5 library if you set the path
# wrong. Turn this on for added confirmation that you got it right.
#message (DEPRECATION "Include: ${HDF5_INCLUDE_DIR}")

include(GNUInstallDirs)
add_subdirectory (src)
add_subdirectory (test)

# Doxygen documentation build
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found. Documentation target will not be available.")
endif()



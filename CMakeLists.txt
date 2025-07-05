cmake_minimum_required (VERSION 3.9)
project (grib2_vol_connector VERSION 1.0.0 DESCRIPTION "GRIB2 VOL Connector for HDF5" LANGUAGES C)

include(GNUInstallDirs)

# Version information
set(GRIB2_VOL_MAJOR_VERSION 1)
set(GRIB2_VOL_MINOR_VERSION 0)
set(GRIB2_VOL_PATCH_VERSION 0)
set(GRIB2_VOL_VERSION ${GRIB2_VOL_MAJOR_VERSION}.${GRIB2_VOL_MINOR_VERSION}.${GRIB2_VOL_PATCH_VERSION})

# Options
option(BUILD_DOCS "Build documentation" ON)

# Enable testing
enable_testing ()
include (CTest)

# Find HDF5
find_package (HDF5 1.13 COMPONENTS C REQUIRED)
link_directories (${HDF5_LIBRARY_DIRS})
include_directories (${HDF5_INCLUDE_DIR})
set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# It's really easy to pick up the wrong HDF5 library if you set the path
# wrong. Turn this on for added confirmation that you got it right.
#message (DEPRECATION "Include: ${HDF5_INCLUDE_DIR}")

add_subdirectory (src)
add_subdirectory (test)

# Documentation
if(BUILD_DOCS)
    # Find Doxygen
    find_package(Doxygen
        REQUIRED dot
        COMPONENTS dot
    )

    if(DOXYGEN_FOUND)
        # Configure Doxyfile
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
        )

        # Add custom target for documentation
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        # Install documentation
        install(
            DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen/html
            DESTINATION ${CMAKE_INSTALL_DOCDIR}
            OPTIONAL
        )
    else()
        message(WARNING "Doxygen not found - documentation will not be built")
    endif()
endif()


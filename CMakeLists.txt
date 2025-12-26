# This is the build file for the root directory of the NEP project.
# Edward Hartnett 8/27/25
cmake_minimum_required(VERSION 3.9)
project (NEP VERSION 0.1.1 DESCRIPTION "NetCDF4/HDF5 Format Extension Pack" LANGUAGES C)

# Read documentation/package version from version.txt so that documentation
# and installed package metadata reflect the release version configured there.
file(READ "${CMAKE_SOURCE_DIR}/version.txt" NEP_VERSION_FROM_FILE_RAW)
string(STRIP "${NEP_VERSION_FROM_FILE_RAW}" NEP_VERSION_FROM_FILE)

# Add option for debug build
option(DEBUG "Build with debugging symbols" OFF)

# Add option to control building Fortran wrappers and tests
option(ENABLE_FORTRAN "Build Fortran wrappers and related tests" ON)

# Build options for compression filters (mirror autotools options)
option(BUILD_LZ4 "Enable LZ4 filter support" ON)
option(BUILD_BZIP2 "Enable BZIP2 filter support" OFF)

# Define global preprocessor symbol to mirror Autotools AC_DEFINE([USE_HDF5],[yes])
# Use add_definitions for compatibility with CMake 3.9
add_definitions(-DUSE_HDF5=yes)

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# option(ENABLE_GRIB2 "Enable GRIB2 UDF support" ON)

# CDF library detection (v1.3.0) - default OFF, library detection only, no UDF yet
option(ENABLE_CDF "Enable CDF library detection" OFF)

# GeoTIFF library detection (v1.5.0) - default OFF, library detection only, no UDF yet
option(ENABLE_GEOTIFF "Enable GeoTIFF library detection" OFF)

# Documentation option
option(BUILD_DOCUMENTATION "Build documentation with Doxygen" ON)

# Testing option - enable by default
option(BUILD_TESTING "Build tests" ON)

# Set compiler flags based on DEBUG option
if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    message(STATUS "Debug mode enabled: Adding -g flag")
endif()

# Determine if we're doing a documentation-only build
set(DOCS_ONLY OFF)
if(BUILD_DOCUMENTATION AND NOT BUILD_TESTING)
    set(DOCS_ONLY ON)
    message(STATUS "Documentation-only build - skipping dependencies")
endif()

# Enable testing (skip if documentation-only build)
if(NOT DOCS_ONLY)
    enable_testing ()
    include (CTest)
endif()

# Find HDF5 (skip if documentation-only build)
if(NOT DOCS_ONLY)
    find_package(HDF5 1.10 REQUIRED COMPONENTS C)
    link_directories (${HDF5_LIBRARY_DIRS})
    include_directories (${HDF5_INCLUDE_DIR})
    set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY})

    # Find NetCDF-C
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETCDF REQUIRED netcdf)
    include_directories(${NETCDF_INCLUDE_DIRS})
    link_directories(${NETCDF_LIBRARY_DIRS})
    set (LINK_LIBS ${LINK_LIBS} ${NETCDF_LIBRARIES})

    # Ensure runtime can locate the exact HDF5/NetCDF shared libraries selected at configure time.
    # This avoids mixing system HDF5 with a custom HDF5 prefix and fixes missing libhdf5_hl.so at runtime.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_BUILD_RPATH "${HDF5_LIBRARY_DIRS};${NETCDF_LIBRARY_DIRS}")
endif()

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# Find GRIB2 dependencies if enabled
# if(ENABLE_GRIB2)
#     find_library(G2C_LIBRARY NAMES g2c)
#     find_path(G2C_INCLUDE_DIR grib2.h)
#     
#     if(G2C_LIBRARY AND G2C_INCLUDE_DIR)
#         message(STATUS "Found NCEPLIBS-g2: ${G2C_LIBRARY}")
#         include_directories(${G2C_INCLUDE_DIR})
#         set(HAVE_GRIB2 TRUE)
#         message(STATUS "GRIB2 UDF support enabled")
#     else()
#         message(FATAL_ERROR "GRIB2 UDF enabled but NCEPLIBS-g2 not found. Install NCEPLIBS-g2 or disable with -DENABLE_GRIB2=OFF")
#     endif()
# endif()

# CDF library detection (v1.3.0) - library detection only, no UDF implementation yet
if(ENABLE_CDF)
    find_library(CDF_LIBRARY NAMES cdf)
    find_path(CDF_INCLUDE_DIR cdf.h)
    
    if(CDF_LIBRARY AND CDF_INCLUDE_DIR)
        message(STATUS "Found NASA CDF library: ${CDF_LIBRARY}")
        include_directories(${CDF_INCLUDE_DIR})
        set(HAVE_CDF TRUE)
        message(STATUS "CDF library detection enabled (library location only, no UDF yet)")
    else()
        message(FATAL_ERROR "CDF enabled but NASA CDF library not found. Install NASA CDF library or disable with -DENABLE_CDF=OFF")
    endif()
endif()

# GeoTIFF library detection (v1.5.0) - library detection only, no UDF implementation yet
if(ENABLE_GEOTIFF)
    find_library(GEOTIFF_LIBRARY NAMES geotiff)
    find_path(GEOTIFF_INCLUDE_DIR geotiff/geotiff.h)
    find_library(TIFF_LIBRARY NAMES tiff)
    find_path(TIFF_INCLUDE_DIR tiff.h)
    
    if(GEOTIFF_LIBRARY AND GEOTIFF_INCLUDE_DIR AND TIFF_LIBRARY AND TIFF_INCLUDE_DIR)
        message(STATUS "Found libgeotiff: ${GEOTIFF_LIBRARY}")
        message(STATUS "Found libgeotiff headers: ${GEOTIFF_INCLUDE_DIR}")
        message(STATUS "Found libtiff: ${TIFF_LIBRARY}")
        message(STATUS "Found libtiff headers: ${TIFF_INCLUDE_DIR}")
        include_directories(${GEOTIFF_INCLUDE_DIR} ${TIFF_INCLUDE_DIR})
        set(HAVE_GEOTIFF TRUE)
        message(STATUS "GeoTIFF library detection enabled (library location only, no UDF yet)")
    else()
        message(FATAL_ERROR "GeoTIFF enabled but libgeotiff or libtiff not found. Install libgeotiff and libtiff or disable with -DENABLE_GEOTIFF=OFF")
    endif()
endif()

# It's really easy to pick up the wrong HDF5 library if you set the path
# wrong. Turn this on for added confirmation that you got it right.
#message (DEPRECATION "Include: ${HDF5_INCLUDE_DIR}")

# Set package information variables for config.h (matching autotools).
# Use NEP_VERSION_FROM_FILE so documentation and package metadata use the
# version configured in version.txt.
set(PACKAGE "nep")
set(PACKAGE_NAME "NEP")
set(PACKAGE_STRING "NEP ${NEP_VERSION_FROM_FILE}")
set(PACKAGE_TARNAME "nep")
set(PACKAGE_VERSION "${NEP_VERSION_FROM_FILE}")
set(PACKAGE_BUGREPORT "help@intelligentdatadesign.com")
set(PACKAGE_URL "")
set(VERSION "${NEP_VERSION_FROM_FILE}")

# Set HDF5 availability (only if not docs-only build)
if(NOT DOCS_ONLY)
    set(HAVE_HDF5 TRUE)
endif()

# Set standard header availability (these are typically available on modern systems)
set(STDC_HEADERS 1)
set(HAVE_STDIO_H 1)
set(HAVE_STDLIB_H 1)
set(HAVE_STRING_H 1)
set(HAVE_STRINGS_H 1)
set(HAVE_INTTYPES_H 1)
set(HAVE_STDINT_H 1)
set(HAVE_UNISTD_H 1)
set(HAVE_SYS_STAT_H 1)
set(HAVE_SYS_TYPES_H 1)
set(HAVE_DLFCN_H 1)

# Pass configuration to source directory (skip if docs-only build)
if(NOT DOCS_ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake.in ${CMAKE_BINARY_DIR}/config.h)

    include(GNUInstallDirs)
    include(ExternalProject)

    if(ENABLE_FORTRAN)
        enable_language(Fortran)
        add_subdirectory(fsrc)
    endif()

    add_subdirectory (src)
    
    # Build the LZ4 HDF5 filter plugin with CMake (BZIP2 is expected to be provided by the NetCDF/HDF5 installation)
    set(NEP_HDF5_PLUGIN_INSTALL_DIR "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin")
    file(MAKE_DIRECTORY "${NEP_HDF5_PLUGIN_INSTALL_DIR}")

    add_custom_target(hdf5_plugins)

    if(BUILD_LZ4)
        pkg_check_modules(LZ4 REQUIRED liblz4)

        set(NEP_LZ4_PLUGIN_SRC "${CMAKE_SOURCE_DIR}/hdf5_plugins/LZ4/src/H5Zlz4.c")
        add_library(nep_h5lz4 MODULE "${NEP_LZ4_PLUGIN_SRC}")
        set_target_properties(nep_h5lz4 PROPERTIES
            OUTPUT_NAME "h5lz4"
        )

        set(NEP_LZ4_CONFIG_H "${CMAKE_BINARY_DIR}/lz4_config.h")
        file(WRITE "${NEP_LZ4_CONFIG_H}" "/* Generated by NEP CMake build */\n\n#define HAVE_HDF5_H 1\n#define HAVE_LZ4_H 1\n#define HAVE_LIBLZ4 1\n#define HAVE_LIBM 1\n#define STDC_HEADERS 1\n#define HAVE_STDIO_H 1\n#define HAVE_STDLIB_H 1\n#define HAVE_STRING_H 1\n#define HAVE_STRINGS_H 1\n#define HAVE_INTTYPES_H 1\n#define HAVE_STDINT_H 1\n#define HAVE_UNISTD_H 1\n#define HAVE_SYS_STAT_H 1\n#define HAVE_SYS_TYPES_H 1\n#define HAVE_DLFCN_H 1\n#define HAVE_ARPA_INET_H 1\n#define HAVE_NETINET_IN_H 1\n")

        target_include_directories(nep_h5lz4 PRIVATE
            "${CMAKE_BINARY_DIR}"
            "${HDF5_INCLUDE_DIR}"
            ${LZ4_INCLUDE_DIRS}
        )
        target_link_libraries(nep_h5lz4 PRIVATE
            ${LZ4_LIBRARIES}
            ${HDF5_C_${LIB_TYPE}_LIBRARY}
            m
        )

        add_custom_command(
            TARGET nep_h5lz4
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:nep_h5lz4>"
                "${NEP_HDF5_PLUGIN_INSTALL_DIR}/$<TARGET_FILE_NAME:nep_h5lz4>"
        )

        install(TARGETS nep_h5lz4
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/plugin"
        )

        add_dependencies(hdf5_plugins nep_h5lz4)
    endif()
    # GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
    # Will be re-enabled in v2.0.0 for GRIB2 UDF support
    # If GRIB2 layer has been built, test it.
    # if(HAVE_GRIB2)
    #     add_subdirectory (test_grib2)
    # endif()

    # CDF test - v1.3.0 Sprint 2: CDF library integration test
    if(HAVE_CDF)
        add_subdirectory (test_cdf)
    endif()
    
    # Make tests depend on plugins being built
    # Add test directory last so tst_cdf_udf runs after tst_cdf_basic
    add_subdirectory (test)

    # Generate and install CMake config files
    include(CMakePackageConfigHelpers)

    # Configure the config file
    configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/NEPConfig.cmake.in"
        "${CMAKE_BINARY_DIR}/NEPConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEP"
        PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
    )

    # Generate version file
    write_basic_package_version_file(
        "${CMAKE_BINARY_DIR}/NEPConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install config files
    install(FILES
        "${CMAKE_BINARY_DIR}/NEPConfig.cmake"
        "${CMAKE_BINARY_DIR}/NEPConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEP"
    )

    # Install the export set for use with the install-tree
    install(EXPORT NEPTargets
        FILE NEPTargets.cmake
        NAMESPACE NEP::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEP"
    )
endif()


# Doxygen documentation build
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Add build date for documentation
        string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)
        set(abs_top_srcdir "${CMAKE_SOURCE_DIR}")
        set(abs_top_builddir "${CMAKE_BINARY_DIR}")
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(docs
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



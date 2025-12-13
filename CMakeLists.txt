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
option(BUILD_BZIP2 "Enable BZIP2 filter support" ON)

# Define global preprocessor symbol to mirror Autotools AC_DEFINE([USE_HDF5],[yes])
# Use add_definitions for compatibility with CMake 3.9
add_definitions(-DUSE_HDF5=yes)

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# option(ENABLE_GRIB2 "Enable GRIB2 UDF support" ON)

# CDF library detection (v1.3.0) - default OFF, library detection only, no UDF yet
option(ENABLE_CDF "Enable CDF library detection" OFF)

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
    
    # Build HDF5 filter plugins using Autotools (LZ4 and BZIP2 for v1.0.0)
    # The hdf5_plugins directory uses Autotools, so we use ExternalProject to build it
    # Use h5cc wrapper which handles all HDF5 flags automatically
    
    # Build configure flags based on BUILD_LZ4 and BUILD_BZIP2 options
    set(PLUGIN_CONFIGURE_FLAGS "--disable-jpeg --disable-lzf")
    if(BUILD_LZ4)
        list(APPEND PLUGIN_CONFIGURE_FLAGS "--enable-lz4")
    else()
        list(APPEND PLUGIN_CONFIGURE_FLAGS "--disable-lz4")
    endif()
    if(BUILD_BZIP2)
        list(APPEND PLUGIN_CONFIGURE_FLAGS "--enable-bzip2")
    else()
        list(APPEND PLUGIN_CONFIGURE_FLAGS "--disable-bzip2")
    endif()
    string(REPLACE ";" " " PLUGIN_CONFIGURE_FLAGS_STR "${PLUGIN_CONFIGURE_FLAGS}")
    
    set(PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/hdf5_plugins_build")
    set(PLUGIN_INSTALL_DIR "${CMAKE_BINARY_DIR}/hdf5_plugins_install")
    
    message(STATUS "Plugin build directory: ${PLUGIN_BUILD_DIR}")
    message(STATUS "Plugin install directory: ${PLUGIN_INSTALL_DIR}")
    message(STATUS "Plugin configure flags: ${PLUGIN_CONFIGURE_FLAGS_STR}")
    
    ExternalProject_Add(hdf5_plugins
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/hdf5_plugins
        BINARY_DIR ${PLUGIN_BUILD_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir ${PLUGIN_BUILD_DIR}
            ${CMAKE_COMMAND} -E env
            "CC=${HDF5_C_COMPILER_EXECUTABLE}"
            /bin/sh -c "cd ${CMAKE_SOURCE_DIR}/hdf5_plugins && (test -f configure || autoreconf -if) && cd ${PLUGIN_BUILD_DIR} && ${CMAKE_SOURCE_DIR}/hdf5_plugins/configure --prefix=${PLUGIN_INSTALL_DIR} --with-hdf5-plugin-path=${PLUGIN_INSTALL_DIR}/lib/plugin ${PLUGIN_CONFIGURE_FLAGS_STR}"
        BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${PLUGIN_BUILD_DIR} make
        INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir ${PLUGIN_BUILD_DIR} make install
        BUILD_IN_SOURCE 0
        LOG_CONFIGURE TRUE
        LOG_BUILD TRUE
        LOG_INSTALL TRUE
        STEP_TARGETS install
    )
    
    # Create a custom target that ensures plugins are built before main install
    add_custom_target(build_plugins ALL DEPENDS hdf5_plugins)
    
    # Install plugins by copying from the ExternalProject install location
    # The plugins are built during the build phase by the hdf5_plugins ExternalProject
    # and installed to PLUGIN_INSTALL_DIR. We copy them to the final install location here.
    install(DIRECTORY ${PLUGIN_INSTALL_DIR}/lib/plugin
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            USE_SOURCE_PERMISSIONS
            FILES_MATCHING PATTERN "*.so*"
            PATTERN "*.la" EXCLUDE
            PATTERN "*.a" EXCLUDE)
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



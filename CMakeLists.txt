# This is the build file for the root directory of the NEP project.
# Edward Hartnett 8/27/25
cmake_minimum_required(VERSION 3.9)
project (NEP VERSION 0.1.1 DESCRIPTION "NetCDF Expansion Pack" LANGUAGES C)

# Read documentation/package version from version.txt so that documentation
# and installed package metadata reflect the release version configured there.
file(READ "${CMAKE_SOURCE_DIR}/version.txt" NEP_VERSION_FROM_FILE_RAW)
string(STRIP "${NEP_VERSION_FROM_FILE_RAW}" NEP_VERSION_FROM_FILE)

# Add option for debug build
option(DEBUG "Build with debugging symbols" OFF)

# Add option to control building Fortran wrappers and tests
option(NEP_ENABLE_FORTRAN "Build Fortran wrappers and related tests" ON)

# Build options for compression filters
option(NEP_BUILD_LZ4 "Enable LZ4 filter support" ON)
option(NEP_BUILD_BZIP2 "Enable BZIP2 filter support" OFF)

# Define the global preprocessor symbol used by NEP.
# Use add_definitions for compatibility with CMake 3.9
add_definitions(-DUSE_HDF5=yes)

# CDF library detection (v1.3.0) - default OFF; enable with -DNEP_ENABLE_CDF=ON
option(NEP_ENABLE_CDF "Enable CDF library detection" OFF)

# GeoTIFF library detection (v1.5.0) - default OFF; enable with -DNEP_ENABLE_GEOTIFF=ON
option(NEP_ENABLE_GEOTIFF "Enable GeoTIFF library detection" OFF)

# Documentation option
option(NEP_BUILD_DOCUMENTATION "Build documentation with Doxygen" ON)

# Testing option - enable by default
option(BUILD_TESTING "Build tests" ON)

# Examples option - disabled by default in CMake due to complex HDF5 linking (v1.5.1)
option(NEP_BUILD_EXAMPLES "Build example programs" OFF)
option(NEP_ENABLE_VIZ_EXAMPLES "Enable Python visualization examples" OFF)

# Set compiler flags based on DEBUG option
if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    message(STATUS "Debug mode enabled: Adding -g flag")
endif()

# Determine if we're doing a documentation-only build
set(DOCS_ONLY OFF)
if(NEP_BUILD_DOCUMENTATION AND NOT BUILD_TESTING)
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
    find_package(HDF5 1.14 REQUIRED COMPONENTS C HL)
    if(NOT HDF5_LIBRARY_DIRS)
        list(GET HDF5_LIBRARIES 0 _hdf5_lib)
        get_filename_component(HDF5_LIBRARY_DIRS "${_hdf5_lib}" DIRECTORY)
    endif()
    list(APPEND HDF5_LIBRARIES hdf5_hl)
    link_directories (${HDF5_LIBRARY_DIRS})
    include_directories (${HDF5_INCLUDE_DIR})
    set (LINK_LIBS ${LINK_LIBS} ${HDF5_LIBRARIES})
    if(HDF5_IS_PARALLEL)
        set(HDF5_PARALLEL 1)
    endif()

    # Find NetCDF-C (requires 4.10.1+ for UDF slots > 2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETCDF REQUIRED netcdf>=4.10.1)
    pkg_get_variable(NETCDF_PREFIX netcdf prefix)
    include_directories(${NETCDF_INCLUDE_DIRS})
    link_directories(${NETCDF_LIBRARY_DIRS})
    set (LINK_LIBS ${LINK_LIBS} ${NETCDF_LIBRARIES})
    message(STATUS "Found NetCDF-C: ${NETCDF_VERSION}")

    # Sanity-check the installed NetCDF-C headers are at least 4.10.1.
    include(CheckCSourceCompiles)
    set(_saved_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
    set(CMAKE_REQUIRED_INCLUDES "${NETCDF_INCLUDE_DIRS}")
    check_c_source_compiles("#include <netcdf_meta.h>
#if NC_VERSION_MAJOR < 4 || (NC_VERSION_MAJOR == 4 && NC_VERSION_MINOR < 10) || (NC_VERSION_MAJOR == 4 && NC_VERSION_MINOR == 10 && NC_VERSION_PATCH < 1)
#error NetCDF-C >= 4.10.1 is required
#endif
int main(void){ return 0; }" NETCDF_VERSION_OK)
    set(CMAKE_REQUIRED_INCLUDES "${_saved_cmake_required_includes}")
    unset(_saved_cmake_required_includes)
    if(NOT NETCDF_VERSION_OK)
        message(FATAL_ERROR "NetCDF-C >= 4.10.1 is required for UDF slots > 2, but the installed headers are older.")
    endif()

    # Detect NetCDF-C logging support. NEP reuses nc_log()/nc_log_level from
    # the installed NetCDF-C, so LOGGING is enabled only when the host library
    # was built with --enable-logging / -DNETCDF_ENABLE_LOGGING=ON.
    set(_saved_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
    set(CMAKE_REQUIRED_INCLUDES "${NETCDF_INCLUDE_DIRS}")
    check_c_source_compiles("#include <netcdf_meta.h>
#if !NC_HAS_LOGGING
#error NetCDF-C logging is disabled
#endif
int main(void){ return 0; }" NETCDF_HAS_LOGGING)
    set(CMAKE_REQUIRED_INCLUDES "${_saved_cmake_required_includes}")
    unset(_saved_cmake_required_includes)
    if(NETCDF_HAS_LOGGING)
        set(LOGGING 1)
        message(STATUS "NetCDF-C logging support: enabled")
    else()
        message(STATUS "NetCDF-C logging support: disabled")
    endif()

    # Detect parallel NetCDF-C support (needed for matching internal struct layouts).
    find_program(NC_CONFIG nc-config HINTS "${NETCDF_PREFIX}/bin")
    if(NC_CONFIG)
        execute_process(COMMAND ${NC_CONFIG} --has-parallel4
                        OUTPUT_VARIABLE NETCDF_HAS_PARALLEL4
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NETCDF_HAS_PARALLEL4 STREQUAL "yes")
            set(USE_PARALLEL 1)
        endif()
    endif()

    # Find ncdump from the NetCDF-C installation
    find_program(NCDUMP ncdump HINTS "${NETCDF_PREFIX}/bin" REQUIRED)
    message(STATUS "Found ncdump: ${NCDUMP}")

    # Check for NcZarr support in NetCDF-C (v1.11.0)
    include(CheckSymbolExists)
    set(_saved_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
    set(CMAKE_REQUIRED_INCLUDES "${NETCDF_INCLUDE_DIRS}")
    check_symbol_exists(NC_HAS_NCZARR "netcdf_meta.h" HAVE_NCZARR)
    set(CMAKE_REQUIRED_INCLUDES "${_saved_cmake_required_includes}")
    unset(_saved_cmake_required_includes)
    if(HAVE_NCZARR)
        message(STATUS "NcZarr support: enabled")
    else()
        message(STATUS "NcZarr support: not available")
    endif()

    # Find NetCDF-Fortran (optional, for Fortran examples)
    if(NEP_ENABLE_FORTRAN)
        pkg_check_modules(NetCDF_Fortran netcdf-fortran)
        if(NetCDF_Fortran_FOUND)
            message(STATUS "Found NetCDF-Fortran: ${NetCDF_Fortran_LIBRARIES}")
        else()
            message(STATUS "NetCDF-Fortran not found, Fortran examples will be skipped")
        endif()
    endif()

    # Ensure runtime can locate the exact HDF5/NetCDF shared libraries selected at configure time.
    # This avoids mixing system HDF5 with a custom HDF5 prefix and fixes missing libhdf5_hl.so at runtime.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_BUILD_RPATH "${HDF5_LIBRARY_DIRS};${NETCDF_LIBRARY_DIRS}")
endif()

# FITS reader support (v2.0.0) - default OFF; enable with -DNEP_ENABLE_FITS=ON
option(NEP_ENABLE_FITS "Enable FITS reader support via CFITSIO" OFF)

# PDS4 reader support (v2.2.0) - default OFF; enable with -DNEP_ENABLE_PDS4=ON
option(NEP_ENABLE_PDS4 "Enable PDS4 reader support via libxml2" OFF)

# DICOM reader support (v3.0.0) - default OFF; enable with -DNEP_ENABLE_DICOM=ON
option(NEP_ENABLE_DICOM "Enable DICOM reader support via libdicom" OFF)

# Legacy PDB reader support (v3.3.0) - default ON now that the read-only
# dispatch layer is implemented and tested.
option(NEP_ENABLE_PDB "Enable legacy PDB reader support" ON)

# PDBx/mmCIF reader support (v3.4.0) - default OFF; enable with -DNEP_ENABLE_MMCIF=ON
option(NEP_ENABLE_MMCIF "Enable PDBx/mmCIF reader support" OFF)

# NEXTCDF-4 backend (v4.0.0) - default OFF while the backend is under development
option(NEP_ENABLE_NEXTCDF4 "Enable the NEXTCDF-4 UDF backend" OFF)

# GRIB2 library detection (v1.7.0) - default OFF; enable with -DNEP_ENABLE_GRIB2=ON
option(NEP_ENABLE_GRIB2 "Enable GRIB2 support" OFF)

# Parallel I/O test support (v1.9.0) - default OFF; enable with -DNEP_ENABLE_PARALLEL_TESTS=ON
option(NEP_ENABLE_PARALLEL_TESTS "Enable parallel I/O test programs" OFF)

# Performance benchmark support (v1.10.0) - default OFF; slow programs not for regular CI
option(NEP_ENABLE_BENCHMARKS "Enable performance benchmark programs (slow, not for regular CI)" OFF)

# OPeNDAP examples (v1.10.0) - default OFF; requires network access
option(NEP_ENABLE_OPENDAP_EXAMPLES "Enable OPeNDAP remote data access examples (requires network)" OFF)

# Map user-facing NEP_ options to the internal macro names expected by config.h.cmake.in.
set(BUILD_LZ4 ${NEP_BUILD_LZ4})
set(BUILD_BZIP2 ${NEP_BUILD_BZIP2})

if(NOT DOCS_ONLY)
    # CDF library detection (v1.3.0)
    if(NEP_ENABLE_CDF)
        find_library(CDF_LIBRARY NAMES cdf)
        find_path(CDF_INCLUDE_DIR cdf.h)
        
        if(CDF_LIBRARY AND CDF_INCLUDE_DIR)
            message(STATUS "Found NASA CDF library: ${CDF_LIBRARY}")
            include_directories(${CDF_INCLUDE_DIR})
            set(HAVE_CDF TRUE)
            message(STATUS "CDF library detection enabled (library location only, no UDF yet)")
        else()
            message(FATAL_ERROR "libcdf is required for the CDF dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use -DNEP_ENABLE_CDF=OFF")
        endif()
    endif()

    # GeoTIFF library detection (v1.5.0)
    if(NEP_ENABLE_GEOTIFF)
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
            message(FATAL_ERROR "libgeotiff is required for the GeoTIFF dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use -DNEP_ENABLE_GEOTIFF=OFF")
        endif()
    endif()

    # FITS (CFITSIO) library detection (v2.0.0)
    if(NEP_ENABLE_FITS)
        find_library(CFITSIO_LIBRARY NAMES cfitsio)
        find_path(CFITSIO_INCLUDE_DIR fitsio.h)

        if(CFITSIO_LIBRARY AND CFITSIO_INCLUDE_DIR)
            get_filename_component(CFITSIO_LIBRARY_DIR "${CFITSIO_LIBRARY}" DIRECTORY)
            message(STATUS "Found CFITSIO: ${CFITSIO_LIBRARY}")
            message(STATUS "Found CFITSIO headers: ${CFITSIO_INCLUDE_DIR}")
            include_directories(${CFITSIO_INCLUDE_DIR})
            set(HAVE_FITS TRUE)
        else()
            message(FATAL_ERROR "libcfitsio is required for the FITS reader. Install libcfitsio-dev or use -DNEP_ENABLE_FITS=OFF")
        endif()
    endif()

    # GRIB2 library detection
    if(NEP_ENABLE_GRIB2)
        find_library(G2C_LIBRARY NAMES g2c)
        find_path(G2C_INCLUDE_DIR grib2.h)

        if(G2C_LIBRARY AND G2C_INCLUDE_DIR)
            message(STATUS "Found g2c library: ${G2C_LIBRARY}")
            message(STATUS "Found g2c headers: ${G2C_INCLUDE_DIR}")
            include_directories(${G2C_INCLUDE_DIR})
            set(HAVE_GRIB2 TRUE)
        else()
            message(FATAL_ERROR "libg2c is required for the GRIB2 dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use -DNEP_ENABLE_GRIB2=OFF")
        endif()
    endif()

    # PDS4 (libxml2) library detection (v2.2.0)
    if(NEP_ENABLE_PDS4)
        find_package(LibXml2 REQUIRED)
        if(LIBXML2_FOUND)
            message(STATUS "Found libxml2: ${LIBXML2_LIBRARIES}")
            include_directories(${LIBXML2_INCLUDE_DIR})
            set(HAVE_PDS4 TRUE)
        else()
            message(FATAL_ERROR "libxml2 is required for the PDS4 reader. Install libxml2-dev or use -DNEP_ENABLE_PDS4=OFF")
        endif()
    endif()

    # DICOM (libdicom) library detection (v3.0.0)
    if(NEP_ENABLE_DICOM)
        find_library(DICOM_LIBRARY NAMES dicom)
        find_path(DICOM_INCLUDE_DIR dicom/dicom.h)
        find_package(JPEG REQUIRED)

        if(DICOM_LIBRARY AND DICOM_INCLUDE_DIR AND JPEG_FOUND)
            get_filename_component(DICOM_LIBRARY_DIR "${DICOM_LIBRARY}" DIRECTORY)
            get_filename_component(JPEG_LIBRARY_DIR "${JPEG_LIBRARIES}" DIRECTORY)
            message(STATUS "Found libdicom: ${DICOM_LIBRARY}")
            message(STATUS "Found libdicom headers: ${DICOM_INCLUDE_DIR}")
            message(STATUS "Found JPEG: ${JPEG_LIBRARIES}")
            include_directories(${DICOM_INCLUDE_DIR})
            set(HAVE_DICOM TRUE)
        else()
            message(FATAL_ERROR "libdicom, libjpeg, and GDCM (libgdcm-dev) are required for the DICOM reader. Install the dependencies or use -DNEP_ENABLE_DICOM=OFF")
        endif()

        # JPEG Lossless (Process 14) codec detection via GDCM's bundled IJG
        # libjpeg 6b + lossless patch (v3.2.0 Sprint 2). Mainline
        # libjpeg/libjpeg-turbo has no JPEG Lossless support, so GDCM is a
        # required dependency for the DICOM reader because most DICOM images
        # use lossless compression transfer syntaxes (.4.57/.4.70).
        find_path(GDCMJPEG_INCLUDE_DIR gdcmjpeg/8/jpeglib.h
            PATH_SUFFIXES gdcm-3.0 gdcm-3.1 gdcm-3.2)
        find_library(GDCMJPEG8_LIBRARY NAMES gdcmjpeg8)
        find_library(GDCMJPEG12_LIBRARY NAMES gdcmjpeg12)
        find_library(GDCMJPEG16_LIBRARY NAMES gdcmjpeg16)

        if(GDCMJPEG_INCLUDE_DIR AND GDCMJPEG8_LIBRARY AND GDCMJPEG12_LIBRARY AND GDCMJPEG16_LIBRARY)
            message(STATUS "Found GDCM lossless JPEG codec headers: ${GDCMJPEG_INCLUDE_DIR}")
            message(STATUS "  gdcmjpeg8: ${GDCMJPEG8_LIBRARY}")
            message(STATUS "  gdcmjpeg12: ${GDCMJPEG12_LIBRARY}")
            message(STATUS "  gdcmjpeg16: ${GDCMJPEG16_LIBRARY}")
            set(HAVE_DICOM_LOSSLESS TRUE)
        else()
            message(FATAL_ERROR "GDCM (libgdcm-dev) is required for the DICOM reader. Install libgdcm-dev or use -DNEP_ENABLE_DICOM=OFF")
        endif()
    endif()

    # Legacy PDB reader support (v3.3.0) - no external library required;
    # a custom fixed-column line reader is sufficient.
    if(NEP_ENABLE_PDB)
        set(HAVE_PDB TRUE)
        message(STATUS "Legacy PDB reader: enabled (no external library required)")
    endif()

    # PDBx/mmCIF reader support (v3.4.0 Sprint 1) - no external library required;
    # a custom STAR/CIF tokenizer will be added in Sprint 2.
    if(NEP_ENABLE_MMCIF)
        set(HAVE_MMCIF TRUE)
        message(STATUS "PDBx/mmCIF reader: enabled (no external library required)")
    endif()

    if(NEP_ENABLE_NEXTCDF4)
        if(HDF5_VERSION VERSION_LESS "1.14.0")
            message(FATAL_ERROR "NEP_ENABLE_NEXTCDF4 requires HDF5 1.14.0 or newer (found ${HDF5_VERSION})")
        endif()
        include(CheckSymbolExists)
        set(_saved_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
        set(CMAKE_REQUIRED_INCLUDES "${NETCDF_INCLUDE_DIRS}")
        check_symbol_exists(NC_UDF9 "netcdf.h" NETCDF_HAS_UDF9)
        set(CMAKE_REQUIRED_INCLUDES "${_saved_cmake_required_includes}")
        unset(_saved_cmake_required_includes)
        if(NOT NETCDF_HAS_UDF9)
            message(FATAL_ERROR "NEP_ENABLE_NEXTCDF4 requires NetCDF-C with NC_UDF9 support")
        endif()
        set(HAVE_NEXTCDF4 TRUE)
        message(STATUS "NEXTCDF-4 backend: enabled")
    endif()

    # Parallel I/O test support (v1.9.0)
    if(NEP_ENABLE_PARALLEL_TESTS)
        include(CheckSymbolExists)
        # Set up include paths for the symbol check
        set(CMAKE_REQUIRED_INCLUDES "${NETCDF_INCLUDE_DIRS}")
        check_symbol_exists(NC_HAS_PARALLEL4 "netcdf_meta.h" HAVE_PARALLEL_NETCDF)

        if(NOT HAVE_PARALLEL_NETCDF)
            message(FATAL_ERROR "NEP_ENABLE_PARALLEL_TESTS requires NetCDF-C built with parallel support (NC_HAS_PARALLEL4 not found in ${NETCDF_INCLUDE_DIRS})")
        endif()

        find_program(MPIEXEC_EXECUTABLE
            NAMES mpiexec mpirun srun
            DOC "MPI execution command for parallel tests"
        )

        if(NOT MPIEXEC_EXECUTABLE)
            message(FATAL_ERROR "NEP_ENABLE_PARALLEL_TESTS requires mpiexec/mpirun/srun (set MPIEXEC_EXECUTABLE to specify)")
        endif()

        set(HAVE_PARALLEL_TESTS ON)
        message(STATUS "Parallel NetCDF-C: Found (NC_HAS_PARALLEL4)")
        message(STATUS "mpiexec: ${MPIEXEC_EXECUTABLE}")
    endif()
endif()

if(NEP_ENABLE_VIZ_EXAMPLES)
    if(NOT NEP_BUILD_EXAMPLES)
        message(FATAL_ERROR "NEP_ENABLE_VIZ_EXAMPLES requires NEP_BUILD_EXAMPLES=ON")
    endif()
    if(NOT (HAVE_GEOTIFF OR HAVE_CDF OR HAVE_GRIB2 OR HAVE_FITS OR HAVE_PDS4 OR HAVE_DICOM OR HAVE_PDB OR HAVE_MMCIF))
        message(FATAL_ERROR "NEP_ENABLE_VIZ_EXAMPLES requires at least one enabled UDF reader")
    endif()
    set(PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    if(NOT EXISTS "${PYTHON_EXECUTABLE}")
        message(FATAL_ERROR "NEP_ENABLE_VIZ_EXAMPLES requires the project virtual environment at ${CMAKE_SOURCE_DIR}/.venv")
    endif()
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "import sys; raise SystemExit(0 if sys.prefix != sys.base_prefix else 1)"
        RESULT_VARIABLE _nep_viz_venv_result
    )
    if(NOT _nep_viz_venv_result EQUAL 0)
        message(FATAL_ERROR "NEP_ENABLE_VIZ_EXAMPLES requires a Python virtual environment")
    endif()
    foreach(_nep_viz_module netCDF4 matplotlib numpy)
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} -c "import ${_nep_viz_module}"
            RESULT_VARIABLE _nep_viz_python_result
            ERROR_VARIABLE _nep_viz_python_error
            OUTPUT_QUIET
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        if(NOT _nep_viz_python_result EQUAL 0)
            message(FATAL_ERROR "NEP_ENABLE_VIZ_EXAMPLES requires Python module ${_nep_viz_module}: ${_nep_viz_python_error}")
        endif()
    endforeach()
endif()

message(STATUS "Performance benchmarks: ${NEP_ENABLE_BENCHMARKS}")
message(STATUS "OPeNDAP examples: ${NEP_ENABLE_OPENDAP_EXAMPLES}")
message(STATUS "Visualization examples: ${NEP_ENABLE_VIZ_EXAMPLES}")
message(STATUS "PDBx/mmCIF reader: ${NEP_ENABLE_MMCIF}")

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
    # Parse NEP version from version.txt into components for nep_meta.h.
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)" _nep_version_match "${NEP_VERSION_FROM_FILE}")
    set(NEP_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(NEP_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(NEP_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(NEP_VERSION_NOTE "${CMAKE_MATCH_4}")
    set(NEP_VERSION "${NEP_VERSION_MAJOR}.${NEP_VERSION_MINOR}.${NEP_VERSION_PATCH}")

    # Build date, compiler, and feature flags for nep_meta.h.
    string(TIMESTAMP NEP_BUILD_DATE "%Y-%m-%d" UTC)
    set(NEP_COMPILER_NAME "${CMAKE_C_COMPILER_ID}")
    set(NEP_COMPILER_VERSION "${CMAKE_C_COMPILER_VERSION}")

    set(NEP_HAS_NETCDF 1)
    if(HAVE_HDF5)
        set(NEP_HAS_HDF5 1)
    else()
        set(NEP_HAS_HDF5 0)
    endif()
    if(NEP_BUILD_LZ4)
        set(NEP_HAS_LZ4 1)
    else()
        set(NEP_HAS_LZ4 0)
    endif()
    if(NEP_BUILD_BZIP2)
        set(NEP_HAS_BZIP2 1)
    else()
        set(NEP_HAS_BZIP2 0)
    endif()
    if(NEP_ENABLE_FORTRAN AND NetCDF_Fortran_FOUND)
        set(NEP_HAS_FORTRAN 1)
    else()
        set(NEP_HAS_FORTRAN 0)
    endif()
    if(HAVE_CDF)
        set(NEP_HAS_CDF 1)
    else()
        set(NEP_HAS_CDF 0)
    endif()
    if(HAVE_GEOTIFF)
        set(NEP_HAS_GEOTIFF 1)
    else()
        set(NEP_HAS_GEOTIFF 0)
    endif()
    if(HAVE_GRIB2)
        set(NEP_HAS_GRIB2 1)
    else()
        set(NEP_HAS_GRIB2 0)
    endif()
    if(HAVE_FITS)
        set(NEP_HAS_FITS 1)
    else()
        set(NEP_HAS_FITS 0)
    endif()
    if(HAVE_PDS4)
        set(NEP_HAS_PDS4 1)
    else()
        set(NEP_HAS_PDS4 0)
    endif()
    if(HAVE_DICOM)
        set(NEP_HAS_DICOM 1)
    else()
        set(NEP_HAS_DICOM 0)
    endif()
    if(HAVE_PDB)
        set(NEP_HAS_PDB 1)
    else()
        set(NEP_HAS_PDB 0)
    endif()
    if(HAVE_MMCIF)
        set(NEP_HAS_MMCIF 1)
    else()
        set(NEP_HAS_MMCIF 0)
    endif()
    if(HAVE_NEXTCDF4)
        set(NEP_HAS_NEXTCDF4 1)
    else()
        set(NEP_HAS_NEXTCDF4 0)
    endif()
    if(HAVE_NCZARR)
        set(NEP_HAS_NCZARR 1)
    else()
        set(NEP_HAS_NCZARR 0)
    endif()
    if(USE_PARALLEL)
        set(NEP_HAS_PARALLEL 1)
    else()
        set(NEP_HAS_PARALLEL 0)
    endif()
    if(NEP_ENABLE_BENCHMARKS)
        set(NEP_HAS_BENCHMARKS 1)
    else()
        set(NEP_HAS_BENCHMARKS 0)
    endif()
    if(NEP_ENABLE_VIZ_EXAMPLES)
        set(NEP_HAS_VIZ_EXAMPLES 1)
    else()
        set(NEP_HAS_VIZ_EXAMPLES 0)
    endif()
    if(NEP_ENABLE_OPENDAP_EXAMPLES)
        set(NEP_HAS_OPENDAP 1)
    else()
        set(NEP_HAS_OPENDAP 0)
    endif()
    if(NETCDF_HAS_LOGGING)
        set(NEP_HAS_LOGGING 1)
    else()
        set(NEP_HAS_LOGGING 0)
    endif()

    # Generate the build metadata header (committed source is include/nep_meta.h.in).
    configure_file(${CMAKE_SOURCE_DIR}/include/nep_meta.h.in ${CMAKE_BINARY_DIR}/include/nep_meta.h @ONLY)

    configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake.in ${CMAKE_BINARY_DIR}/config.h)

    include(GNUInstallDirs)
    include(ExternalProject)

    if(NEP_ENABLE_FORTRAN)
        enable_language(Fortran)
        add_subdirectory(fsrc)
    endif()

    # Derive HDF5 and NetCDF library directories for test runtime environment.
    # This ensures all CTest tests find shared libraries without needing
    # LD_LIBRARY_PATH to be set in the shell.
    set(_nep_test_lib_dirs "")
    if(HDF5_LIBRARY_DIRS)
        list(APPEND _nep_test_lib_dirs ${HDF5_LIBRARY_DIRS})
    elseif(HDF5_DIR)
        # HDF5 found via CMake config package; derive lib dir from config dir.
        get_filename_component(_hdf5_root "${HDF5_DIR}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_hdf5_root}/lib")
    endif()
    foreach(_lib ${HDF5_LIBRARIES})
        if(IS_ABSOLUTE "${_lib}")
            get_filename_component(_d "${_lib}" DIRECTORY)
            list(APPEND _nep_test_lib_dirs "${_d}")
        endif()
    endforeach()
    if(NETCDF_LIBRARY_DIRS)
        list(APPEND _nep_test_lib_dirs ${NETCDF_LIBRARY_DIRS})
    elseif(DEFINED NETCDF_PREFIX)
        list(APPEND _nep_test_lib_dirs "${NETCDF_PREFIX}/lib")
    endif()
    list(APPEND _nep_test_lib_dirs "${CMAKE_BINARY_DIR}/src")
    if(HAVE_FITS AND CFITSIO_LIBRARY_DIR)
        list(APPEND _nep_test_lib_dirs "${CFITSIO_LIBRARY_DIR}")
    endif()
    if(HAVE_PDS4 AND LIBXML2_LIBRARIES)
        get_filename_component(_xml2_lib_dir "${LIBXML2_LIBRARIES}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_xml2_lib_dir}")
    endif()
    if(HAVE_DICOM AND DICOM_LIBRARY_DIR)
        list(APPEND _nep_test_lib_dirs "${DICOM_LIBRARY_DIR}")
    endif()
    if(HAVE_DICOM AND JPEG_LIBRARY_DIR)
        list(APPEND _nep_test_lib_dirs "${JPEG_LIBRARY_DIR}")
    endif()
    if(HAVE_CDF AND CDF_LIBRARY)
        get_filename_component(_cdf_lib_dir "${CDF_LIBRARY}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_cdf_lib_dir}")
    endif()
    if(HAVE_GRIB2 AND G2C_LIBRARY)
        get_filename_component(_g2c_lib_dir "${G2C_LIBRARY}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_g2c_lib_dir}")
    endif()
    if(HAVE_GRIB2 AND JASPER_LIBRARY)
        get_filename_component(_jasper_lib_dir "${JASPER_LIBRARY}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_jasper_lib_dir}")
    endif()
    if(HAVE_GEOTIFF AND GEOTIFF_LIBRARY)
        get_filename_component(_geotiff_lib_dir "${GEOTIFF_LIBRARY}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_geotiff_lib_dir}")
    endif()
    if(HAVE_GEOTIFF AND TIFF_LIBRARY)
        get_filename_component(_tiff_lib_dir "${TIFF_LIBRARY}" DIRECTORY)
        list(APPEND _nep_test_lib_dirs "${_tiff_lib_dir}")
    endif()
    if(NEP_ENABLE_FORTRAN AND NetCDF_Fortran_LIBRARY_DIRS)
        list(APPEND _nep_test_lib_dirs ${NetCDF_Fortran_LIBRARY_DIRS})
    endif()
    list(REMOVE_DUPLICATES _nep_test_lib_dirs)
    string(REPLACE ";" ":" _nep_test_ld_path "${_nep_test_lib_dirs}")
    file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "")

    add_subdirectory (src)

    if(NEP_ENABLE_FORTRAN)
        add_subdirectory(ftest)
    endif()
    
    # Build the LZ4 HDF5 filter plugin with CMake (BZIP2 is expected to be provided by the NetCDF/HDF5 installation)
    set(NEP_HDF5_PLUGIN_INSTALL_DIR "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin")
    file(MAKE_DIRECTORY "${NEP_HDF5_PLUGIN_INSTALL_DIR}")

    add_custom_target(hdf5_plugins)

    if(NEP_BUILD_LZ4)
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
    # CDF test - v1.3.0 Sprint 2: CDF library integration test
    if(HAVE_CDF)
        add_subdirectory (test_cdf)
    endif()
    
    # GeoTIFF test - v1.5.0: GeoTIFF format detection test
    if(HAVE_GEOTIFF)
        add_subdirectory (test_geotiff)
    endif()

    # Parallel I/O tests - v1.9.0
    if(HAVE_PARALLEL_TESTS)
        add_subdirectory (test_parallel)
    endif()

    # Make tests depend on plugins being built
    # Add test directory last so tst_cdf_udf runs after tst_cdf_basic
    add_subdirectory (test)
    if(NEP_BUILD_LZ4 OR NEP_BUILD_BZIP2)
        add_subdirectory (test_h5)
    endif()

    # .ncrc: assemble from fragments for enabled formats (issue #134)
    if(HAVE_GEOTIFF OR HAVE_CDF OR HAVE_GRIB2 OR HAVE_FITS OR HAVE_PDS4 OR HAVE_DICOM OR HAVE_PDB OR HAVE_MMCIF OR HAVE_NEXTCDF4)
        set(NEP_NCRC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}"
            CACHE PATH "Directory where .ncrc is installed")

        # Start with header fragment
        set(NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc.in")
        if(HAVE_GEOTIFF)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-geotiff.in")
        endif()
        if(HAVE_CDF)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-cdf.in")
        endif()
        if(HAVE_GRIB2)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-grib2.in")
        endif()
        if(HAVE_FITS)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-fits.in")
        endif()
        if(HAVE_PDS4)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-pds4.in")
        endif()
        if(HAVE_DICOM)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-dicom.in")
        endif()
        if(HAVE_PDB)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-pdb.in")
        endif()
        if(HAVE_MMCIF)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-mmcif.in")
        endif()
        if(HAVE_NEXTCDF4)
            list(APPEND NCRC_FRAGMENTS "${CMAKE_SOURCE_DIR}/.ncrc-nextcdf4.in")
        endif()

        # Assemble a .ncrc for the build tree (tests point to the freshly built libraries)
        set(NEP_LIBDIR_BUILD "${CMAKE_BINARY_DIR}/src")
        set(NCRC_OUT "${CMAKE_BINARY_DIR}/.ncrc")
        file(WRITE "${NCRC_OUT}" "")
        foreach(_frag ${NCRC_FRAGMENTS})
            file(READ "${_frag}" _content)
            string(REPLACE "@NEP_LIBDIR@" "${NEP_LIBDIR_BUILD}" _content "${_content}")
            file(APPEND "${NCRC_OUT}" "${_content}")
        endforeach()

        # Assemble a .ncrc for installation (uses the install libdir)
        set(NEP_LIBDIR_INSTALL "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
        set(NCRC_INSTALL_OUT "${CMAKE_BINARY_DIR}/.ncrc-install")
        file(WRITE "${NCRC_INSTALL_OUT}" "")
        foreach(_frag ${NCRC_FRAGMENTS})
            file(READ "${_frag}" _content)
            string(REPLACE "@NEP_LIBDIR@" "${NEP_LIBDIR_INSTALL}" _content "${_content}")
            file(APPEND "${NCRC_INSTALL_OUT}" "${_content}")
        endforeach()

        install(FILES "${NCRC_INSTALL_OUT}"
            DESTINATION "${NEP_NCRC_INSTALL_DIR}"
            RENAME ".ncrc"
        )
        message(STATUS ".ncrc will be installed to: ${NEP_NCRC_INSTALL_DIR}")
    endif()

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

    # Write CTestCustom.cmake so CTest sets LD_LIBRARY_PATH for every test
    # without requiring the user to export it in the shell first.
    # CTEST_ENVIRONMENT entries are prepended to each test's environment.
    file(APPEND "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
        "list(APPEND CTEST_ENVIRONMENT \"LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}\")\n")
endif()


# Doxygen documentation build
if(NEP_BUILD_DOCUMENTATION)
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
        message(FATAL_ERROR "Documentation enabled but doxygen not found. Install doxygen or disable with -DNEP_BUILD_DOCUMENTATION=OFF")
    endif()
else()
    message(STATUS "Documentation generation disabled")
endif()

# Add examples subdirectory (v1.5.1)
if(NEP_BUILD_EXAMPLES AND NOT DOCS_ONLY)
    message(STATUS "Building example programs")
    add_subdirectory(examples)
else()
    message(STATUS "Example programs disabled")
endif()


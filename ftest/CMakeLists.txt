# Build file for the ftest directory of the NEP project.
# Edward Hartnett 2026-06-28

# Include directories to find the generated nep.mod and netcdf-fortran modules.
include_directories(${CMAKE_BINARY_DIR}/fsrc)
include_directories(${CMAKE_SOURCE_DIR}/fsrc)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find NetCDF-Fortran for Fortran tests.
find_package(PkgConfig REQUIRED)
pkg_check_modules(NetCDF_Fortran netcdf-fortran)

if(NOT NetCDF_Fortran_FOUND)
    message(STATUS "NetCDF-Fortran not found, Fortran tests will be skipped")
    return()
endif()

include_directories(${NetCDF_Fortran_INCLUDE_DIRS})
link_directories(${NetCDF_Fortran_LIBRARY_DIRS})

# Build the BZIP2 Fortran test if BZIP2 is enabled.
if(BUILD_BZIP2)
    add_executable(ftst_nep_bzip2 ftst_nep_bzip2.F90)
    target_link_libraries(ftst_nep_bzip2 PRIVATE nepf nep ${NetCDF_Fortran_LIBRARIES})
    target_include_directories(ftst_nep_bzip2 PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
endif()

# Build the LZ4 Fortran test if LZ4 is enabled.
if(BUILD_LZ4)
    add_executable(ftst_nep_lz4 ftst_nep_lz4.F90)
    target_link_libraries(ftst_nep_lz4 PRIVATE nepf nep ${NetCDF_Fortran_LIBRARIES})
    target_include_directories(ftst_nep_lz4 PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
endif()

# Build the FITS Fortran test if FITS is enabled.
if(HAVE_FITS)
    add_executable(ftst_fits_udf ftst_fits_udf.F90)
    target_link_libraries(ftst_fits_udf PRIVATE ncfits nepf nep ${NetCDF_Fortran_LIBRARIES})
    target_include_directories(ftst_fits_udf PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS} ${CFITSIO_INCLUDE_DIR})
    add_dependencies(ftst_fits_udf ncfits)
    add_test(NAME ftst_fits_udf COMMAND ftst_fits_udf)
    set_tests_properties(ftst_fits_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Configure run_tests.sh script with proper paths.
if(BUILD_LZ4)
    set(BUILD_LZ4 "yes")
else()
    set(BUILD_LZ4 "no")
endif()
if(BUILD_BZIP2)
    set(BUILD_BZIP2 "yes")
else()
    set(BUILD_BZIP2 "no")
endif()
if(HAVE_FITS)
    set(BUILD_FITS "yes")
else()
    set(BUILD_FITS "no")
endif()

# Set the HDF5 plugin path for the generated test script. This must include
# the NetCDF-C plugin directory (for bzip2/zstd) and the locally-built LZ4
# plugin directory used by the CMake build.
set(MY_HDF5_PLUGIN_PATH "${NETCDF_PREFIX}/hdf5/lib/plugin:${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh @ONLY)

file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Add the Fortran test script as a test.
add_test(NAME run_fortran_tests
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(run_fortran_tests PROPERTIES
    ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

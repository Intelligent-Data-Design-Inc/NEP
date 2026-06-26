# CMakeLists.txt for NcZarr examples
# Edward Hartnett, Intelligent Data Design, Inc.

# Find nc-config from the NetCDF library directories or prefix
if(NETCDF_LIBRARY_DIRS)
    set(_nc_config_hints ${NETCDF_LIBRARY_DIRS}/../bin)
elseif(NETCDF_PREFIX)
    set(_nc_config_hints ${NETCDF_PREFIX}/bin)
else()
    set(_nc_config_hints "")
endif()

find_program(NC_CONFIG nc-config
    HINTS ${_nc_config_hints}
    PATHS ${_nc_config_hints}
)

if(NOT NC_CONFIG)
    message(FATAL_ERROR "nc-config not found. Cannot build examples.")
endif()

# Get NetCDF compile and link flags using nc-config
execute_process(
    COMMAND ${NC_CONFIG} --cflags
    OUTPUT_VARIABLE NC_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${NC_CONFIG} --libs
    OUTPUT_VARIABLE NC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(NC_CFLAGS_LIST UNIX_COMMAND "${NC_CFLAGS}")
separate_arguments(NC_LIBS_LIST UNIX_COMMAND "${NC_LIBS}")

get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

execute_process(
    COMMAND ${NC_CONFIG} --libdir
    OUTPUT_VARIABLE NC_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(_nep_ld_env "LD_LIBRARY_PATH=${HDF5_LIB_DIR}:${NC_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_executable(nczarr_simple nczarr_simple.c)
target_compile_options(nczarr_simple PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(nczarr_simple PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(nczarr_simple PRIVATE ${NC_LIBS_LIST})
set_target_properties(nczarr_simple PROPERTIES
    BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}"
    SKIP_BUILD_RPATH OFF
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}")

if(BUILD_TESTING)
    add_test(NAME nczarr_simple
             COMMAND nczarr_simple
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nczarr_simple PROPERTIES ENVIRONMENT "${_nep_ld_env}")
endif()

# Configure validation script with ncdump path from top-level CMakeLists.txt
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh
               @ONLY)

if(BUILD_TESTING)
    add_test(NAME nczarr_simple_validate
             COMMAND bash -c "./nczarr_simple && bash validate_cdl.sh nczarr_simple 'file://simple_nczarr.zarr#mode=nczarr' ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/nczarr_simple_expected.cdl"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nczarr_simple_validate PROPERTIES
                         DEPENDS nczarr_simple
                         ENVIRONMENT "${_nep_ld_env}")
endif()

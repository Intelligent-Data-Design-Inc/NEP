# CMakeLists.txt for legacy PDB C example
# Edward Hartnett, Intelligent Data Design, Inc.

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
    message(FATAL_ERROR "nc-config not found. Cannot build PDB example.")
endif()

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
execute_process(
    COMMAND ${NC_CONFIG} --libdir
    OUTPUT_VARIABLE NC_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(NC_CFLAGS_LIST UNIX_COMMAND "${NC_CFLAGS}")
separate_arguments(NC_LIBS_LIST UNIX_COMMAND "${NC_LIBS}")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(pdb_read pdb_read.c)
target_compile_options(pdb_read PRIVATE ${NC_CFLAGS_LIST})
get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)
target_link_directories(pdb_read PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(pdb_read PRIVATE ${NC_LIBS_LIST} ncpdb nep)
set_target_properties(pdb_read PROPERTIES
    BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}"
    SKIP_BUILD_RPATH OFF
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}")

if(BUILD_TESTING)
    add_test(NAME pdb_read
             COMMAND pdb_read ${CMAKE_SOURCE_DIR}/test/data/PDB/4HHB.pdb
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(pdb_read PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

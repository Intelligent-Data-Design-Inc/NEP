# CMakeLists.txt for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# Find nc-config from the NetCDF library directories
find_program(NC_CONFIG nc-config
    HINTS ${NETCDF_LIBRARY_DIRS}/../bin
    PATHS ${NETCDF_LIBRARY_DIRS}/../bin
)

if(NOT NC_CONFIG)
    message(FATAL_ERROR "nc-config not found. Cannot build examples.")
endif()

# Get NetCDF compile and link flags using nc-config (includes all dependencies)
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

# Convert space-separated strings to CMake lists
separate_arguments(NC_CFLAGS_LIST UNIX_COMMAND "${NC_CFLAGS}")
separate_arguments(NC_LIBS_LIST UNIX_COMMAND "${NC_LIBS}")

# Get HDF5 library directory from HDF5_INCLUDE_DIR (parent scope)
get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

# List of NetCDF-4 example programs
set(NETCDF4_EXAMPLES
    simple_nc4
    format_variants
    compression
    chunking_performance
    multi_unlimited
    user_types
    groups
)

# Build each example using nc-config flags
foreach(example ${NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_compile_options(${example} PRIVATE ${NC_CFLAGS_LIST})
    target_link_directories(${example} PRIVATE ${HDF5_LIB_DIR})
    # nc-config --libs doesn't include HDF5, so we need to add it explicitly
    # NetCDF needs both libhdf5 and libhdf5_hl
    target_link_libraries(${example} PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

# Build dump_nc4_metadata separately (it requires a command-line argument,
# so it cannot use the generic no-args test above)
add_executable(dump_nc4_metadata dump_nc4_metadata.c)
target_compile_options(dump_nc4_metadata PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(dump_nc4_metadata PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(dump_nc4_metadata PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# Add CDL validation tests
if(BUILD_TESTING)
    add_test(NAME user_types_validate 
             COMMAND bash validate_cdl.sh user_types user_types.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/user_types_expected.cdl)
    set_tests_properties(user_types_validate PROPERTIES DEPENDS user_types)

    add_test(NAME groups_validate 
             COMMAND bash validate_cdl.sh groups groups.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/groups_expected.cdl)
    set_tests_properties(groups_validate PROPERTIES DEPENDS groups)

    # Multi-file examples: format_variants
    add_test(NAME format_variants_validate_classic
             COMMAND bash validate_cdl.sh format_variants format_classic.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_classic_expected.cdl)
    set_tests_properties(format_variants_validate_classic PROPERTIES DEPENDS format_variants)

    add_test(NAME format_variants_validate_64bit_offset
             COMMAND bash validate_cdl.sh format_variants format_64bit_offset.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_64bit_offset_expected.cdl)
    set_tests_properties(format_variants_validate_64bit_offset PROPERTIES DEPENDS format_variants)

    add_test(NAME format_variants_validate_64bit_data
             COMMAND bash validate_cdl.sh format_variants format_64bit_data.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_64bit_data_expected.cdl)
    set_tests_properties(format_variants_validate_64bit_data PROPERTIES DEPENDS format_variants)

    add_test(NAME format_variants_validate_netcdf4
             COMMAND bash validate_cdl.sh format_variants format_netcdf4.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_netcdf4_expected.cdl)
    set_tests_properties(format_variants_validate_netcdf4 PROPERTIES DEPENDS format_variants)

    add_test(NAME format_variants_validate_netcdf4_classic
             COMMAND bash validate_cdl.sh format_variants format_netcdf4_classic.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_netcdf4_classic_expected.cdl)
    set_tests_properties(format_variants_validate_netcdf4_classic PROPERTIES DEPENDS format_variants)

    # dump_nc4_metadata test: run user_types first to produce user_types.nc,
    # then compare dump_nc4_metadata output against expected text.
    # ASAN_OPTIONS=detect_leaks=0 because user_types triggers known HDF5 vlen leak.
    add_test(NAME dump_nc4_metadata_validate
             COMMAND bash -c "ASAN_OPTIONS=detect_leaks=0 ./user_types && ./dump_nc4_metadata user_types.nc > dump_nc4_metadata_output.txt && diff -u ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/dump_nc4_metadata_user_types_expected.txt dump_nc4_metadata_output.txt && rm -f dump_nc4_metadata_output.txt")
endif()

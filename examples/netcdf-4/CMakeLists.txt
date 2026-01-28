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

# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# Add CDL validation test for groups example
if(BUILD_TESTING)
    add_test(NAME groups_validate 
             COMMAND bash validate_cdl.sh groups groups.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/groups_expected.cdl)
    set_tests_properties(groups_validate PROPERTIES DEPENDS groups)
endif()

# CMakeLists.txt for classic NetCDF C examples
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

# List of classic example programs
set(CLASSIC_EXAMPLES
    quickstart
    simple_2D
    coord
    coord_vars
    format_variants
    size_limits
    unlimited_dim
    var4d
)

# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# Build each example using nc-config flags
foreach(example ${CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_compile_options(${example} PRIVATE ${NC_CFLAGS_LIST})
    target_link_directories(${example} PRIVATE ${HDF5_LIB_DIR})
    # nc-config --libs doesn't include HDF5, so we need to add it explicitly
    # NetCDF needs both libhdf5 and libhdf5_hl
    target_link_libraries(${example} PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl)
    
    # Add as test (run example only)
    if(BUILD_TESTING)
        add_test(NAME ${example}_run COMMAND ${example})
    endif()
endforeach()

# Add CDL validation tests for examples with expected output
if(BUILD_TESTING)
    add_test(NAME quickstart_validate 
             COMMAND bash validate_cdl.sh quickstart quickstart.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/quickstart_expected.cdl)
    set_tests_properties(quickstart_validate PROPERTIES DEPENDS quickstart_run)

    add_test(NAME coord_validate 
             COMMAND bash validate_cdl.sh coord coord.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/coord_expected.cdl)
    set_tests_properties(coord_validate PROPERTIES DEPENDS coord_run)
endif()

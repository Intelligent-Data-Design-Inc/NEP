# CMakeLists.txt for performance examples
# Edward Hartnett, Intelligent Data Design, Inc.

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

# Build both performance examples unconditionally (fast to compile)
add_executable(cache_tuning cache_tuning.c)
target_compile_options(cache_tuning PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(cache_tuning PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(cache_tuning PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(chunking chunking.c)
target_compile_options(chunking PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(chunking PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(chunking PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(deflate deflate.c)
target_compile_options(deflate PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(deflate PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(deflate PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(fill_values fill_values.c)
target_compile_options(fill_values PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(fill_values PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(fill_values PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(endianness endianness.c)
target_compile_options(endianness PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(endianness PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(endianness PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

# Performance examples run as tests only when ENABLE_BENCHMARKS=ON (slow; not for regular CI)
if(ENABLE_BENCHMARKS)
    target_compile_definitions(cache_tuning PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(chunking PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(deflate PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(fill_values PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(endianness PRIVATE ENABLE_BENCHMARKS=1)
    if(BUILD_TESTING)
        add_test(NAME cache_tuning
                 COMMAND cache_tuning
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        add_test(NAME chunking
                 COMMAND chunking
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        add_test(NAME deflate
                 COMMAND deflate
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        add_test(NAME fill_values
                 COMMAND fill_values
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        add_test(NAME endianness
                 COMMAND endianness
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()

# Configure validation script with ncdump path from top-level CMakeLists.txt
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh
               @ONLY)

# CDL validation for cache_tuning (only when benchmarks enabled)
if(ENABLE_BENCHMARKS AND BUILD_TESTING)
    add_test(NAME cache_tuning_validate
             COMMAND bash validate_cdl.sh cache_tuning cache_test.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/cache_tuning_expected.cdl)
    set_tests_properties(cache_tuning_validate PROPERTIES DEPENDS cache_tuning)
endif()

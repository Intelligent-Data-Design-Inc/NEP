# CMakeLists.txt for performance examples
# Edward Hartnett, Intelligent Data Design, Inc.

# Find nc-config from the NetCDF library directories or prefix
if(NETCDF_LIBRARY_DIRS)
    set(_nc_config_hints ${NETCDF_LIBRARY_DIRS}/../bin)
elseif(NETCDF_PREFIX)
    set(_nc_config_hints ${NETCDF_PREFIX}/bin)
else()
    # Allow search in system PATH
    set(_nc_config_hints "")
endif()

find_program(NC_CONFIG nc-config
    HINTS ${_nc_config_hints}
    PATHS ${_nc_config_hints}
)

if(NOT NC_CONFIG)
    message(FATAL_ERROR "nc-config not found. Cannot build examples. Set NETCDF_PREFIX or ensure nc-config is in PATH.")
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

# Check for SZIP library (optional - needed for szip, lossless, quantize examples)
find_library(SZIP_LIBRARY sz
    HINTS ${HDF5_LIB_DIR} ${NETCDF_LIBRARY_DIRS}
    PATHS /usr/lib /usr/local/lib
)
if(SZIP_LIBRARY)
    message(STATUS "SZIP library found: ${SZIP_LIBRARY}")
    set(HAVE_SZIP ON)
else()
    message(STATUS "SZIP library not found - skipping szip, lossless, quantize examples")
    set(HAVE_SZIP OFF)
endif()

# Add NEP include directories for examples that use NEP functions (e.g., lossless.c)
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Get HDF5 library directory from HDF5_INCLUDE_DIR (parent scope)
get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

# Get NetCDF library directory for rpath
execute_process(
    COMMAND ${NC_CONFIG} --libdir
    OUTPUT_VARIABLE NC_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Helper to apply common settings to every performance target
macro(perf_target _name)
    target_compile_options(${_name} PRIVATE ${NC_CFLAGS_LIST})
    target_link_directories(${_name} PRIVATE ${HDF5_LIB_DIR})
    set_target_properties(${_name} PROPERTIES
        BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}"
        SKIP_BUILD_RPATH OFF
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}")
endmacro()

# Build performance examples unconditionally (fast to compile)
add_executable(cache_tuning cache_tuning.c)
perf_target(cache_tuning)
target_link_libraries(cache_tuning PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(chunking chunking.c)
perf_target(chunking)
target_link_libraries(chunking PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(deflate deflate.c)
perf_target(deflate)
target_link_libraries(deflate PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(fill_values fill_values.c)
perf_target(fill_values)
target_link_libraries(fill_values PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(endianness endianness.c)
perf_target(endianness)
target_link_libraries(endianness PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(zstandard zstandard.c)
perf_target(zstandard)
target_link_libraries(zstandard PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

# SZIP-dependent examples (only if SZIP library is available)
if(HAVE_SZIP)
    add_executable(szip szip.c)
    perf_target(szip)
    target_link_libraries(szip PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl sz m)

    add_executable(lossless lossless.c)
    perf_target(lossless)
    target_link_libraries(lossless PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl sz nep m)

    add_executable(quantize quantize.c)
    perf_target(quantize)
    target_link_libraries(quantize PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl sz nep m)
endif()

add_executable(lz4 lz4.c)
perf_target(lz4)
target_link_libraries(lz4 PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

add_executable(bzip2 bzip2.c)
perf_target(bzip2)
target_link_libraries(bzip2 PRIVATE ${NC_LIBS_LIST} hdf5 hdf5_hl m)

# Performance examples run as tests only when NEP_ENABLE_BENCHMARKS=ON (slow; not for regular CI)
if(NEP_ENABLE_BENCHMARKS)
    target_compile_definitions(cache_tuning PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(chunking PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(deflate PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(fill_values PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(endianness PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(zstandard PRIVATE ENABLE_BENCHMARKS=1)
    if(HAVE_SZIP)
        target_compile_definitions(szip PRIVATE ENABLE_BENCHMARKS=1)
    endif()
    target_compile_definitions(lz4 PRIVATE ENABLE_BENCHMARKS=1)
    target_compile_definitions(bzip2 PRIVATE ENABLE_BENCHMARKS=1)
    if(HAVE_SZIP)
        target_compile_definitions(lossless PRIVATE ENABLE_BENCHMARKS=1)
        target_compile_definitions(quantize PRIVATE ENABLE_BENCHMARKS=1)
    endif()
    if(BUILD_TESTING)
        set(_bench_tests cache_tuning chunking deflate fill_values
                         endianness zstandard lz4 bzip2)
        if(HAVE_SZIP)
            list(APPEND _bench_tests szip lossless quantize)
        endif()
        foreach(_t ${_bench_tests})
            add_test(NAME ${_t}
                     COMMAND ${_t}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            set_tests_properties(${_t} PROPERTIES
                ENVIRONMENT "LD_LIBRARY_PATH=${HDF5_LIB_DIR}:${NC_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
        endforeach()
        set_tests_properties(cache_tuning PROPERTIES RUN_SERIAL TRUE)
    endif()
endif()

# Configure validation script with ncdump path from top-level CMakeLists.txt
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh
               @ONLY)

# CDL validation for cache_tuning (only when benchmarks enabled)
if(NEP_ENABLE_BENCHMARKS AND BUILD_TESTING)
    add_test(NAME cache_tuning_validate
             COMMAND bash -c "cp /dev/null /dev/null; TMPFILE=cache_validate_$$.nc; ./cache_tuning && mv cache_test.nc \$TMPFILE && bash validate_cdl.sh cache_tuning \$TMPFILE ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/cache_tuning_expected.cdl; STATUS=$?; rm -f \$TMPFILE; exit \$STATUS"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(cache_tuning_validate PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${HDF5_LIB_DIR}:${NC_LIBDIR}:$ENV{LD_LIBRARY_PATH}"
        RUN_SERIAL TRUE)
endif()

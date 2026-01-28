# CMakeLists.txt for NetCDF-4 Fortran examples
# Edward Hartnett 1/25/26

# Find nf-config from the NetCDF-Fortran library directories
find_program(NF_CONFIG nf-config
    HINTS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
    PATHS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
)

if(NOT NF_CONFIG)
    message(FATAL_ERROR "nf-config not found. Cannot build Fortran examples.")
endif()

# Get NetCDF-Fortran compile and link flags using nf-config (includes all dependencies)
execute_process(
    COMMAND ${NF_CONFIG} --fflags
    OUTPUT_VARIABLE NF_FFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${NF_CONFIG} --flibs
    OUTPUT_VARIABLE NF_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Convert space-separated strings to CMake lists
separate_arguments(NF_FFLAGS_LIST UNIX_COMMAND "${NF_FFLAGS}")
separate_arguments(NF_LIBS_LIST UNIX_COMMAND "${NF_LIBS}")

# Get HDF5 library directory from HDF5_INCLUDE_DIR (parent scope)
get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

# List of Fortran NetCDF-4 example programs
set(F_NETCDF4_EXAMPLES
    f_simple_nc4
    f_compression
    f_chunking_performance
    f_multi_unlimited
    f_user_types
    f_groups
)

# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# Build each example using nf-config flags
foreach(example ${F_NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.f90)
    target_compile_options(${example} PRIVATE ${NF_FFLAGS_LIST})
    target_link_directories(${example} PRIVATE ${HDF5_LIB_DIR})
    # nf-config --flibs may not include all transitive dependencies, add them explicitly
    # NetCDF needs both libhdf5 and libhdf5_hl
    target_link_libraries(${example} PRIVATE ${NF_LIBS_LIST} ${NETCDF_LIBRARIES} hdf5 hdf5_hl)
    
    # Add as test (run example only)
    if(BUILD_TESTING)
        add_test(NAME ${example}_run COMMAND ${example})
    endif()
endforeach()

# Add CDL validation tests for single-file examples
if(BUILD_TESTING)
    add_test(NAME f_simple_nc4_validate 
             COMMAND bash validate_cdl.sh f_simple_nc4 f_simple_nc4.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_simple_nc4_expected.cdl)
    set_tests_properties(f_simple_nc4_validate PROPERTIES DEPENDS f_simple_nc4_run)
    
    add_test(NAME f_multi_unlimited_validate 
             COMMAND bash validate_cdl.sh f_multi_unlimited f_multi_unlimited.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_multi_unlimited_expected.cdl)
    set_tests_properties(f_multi_unlimited_validate PROPERTIES DEPENDS f_multi_unlimited_run)
    
    add_test(NAME f_user_types_validate 
             COMMAND bash validate_cdl.sh f_user_types f_user_types.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_user_types_expected.cdl)
    set_tests_properties(f_user_types_validate PROPERTIES DEPENDS f_user_types_run)
    
    add_test(NAME f_groups_validate 
             COMMAND bash validate_cdl.sh f_groups f_groups.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_groups_expected.cdl)
    set_tests_properties(f_groups_validate PROPERTIES DEPENDS f_groups_run)
    
    # Multi-file examples: f_compression
    add_test(NAME f_compression_validate_none
             COMMAND bash validate_cdl.sh f_compression f_compress_none.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_none_expected.cdl)
    set_tests_properties(f_compression_validate_none PROPERTIES DEPENDS f_compression_run)
    
    add_test(NAME f_compression_validate_deflate1
             COMMAND bash validate_cdl.sh f_compression f_compress_deflate1.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_deflate1_expected.cdl)
    set_tests_properties(f_compression_validate_deflate1 PROPERTIES DEPENDS f_compression_run)
    
    add_test(NAME f_compression_validate_deflate5
             COMMAND bash validate_cdl.sh f_compression f_compress_deflate5.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_deflate5_expected.cdl)
    set_tests_properties(f_compression_validate_deflate5 PROPERTIES DEPENDS f_compression_run)
    
    add_test(NAME f_compression_validate_deflate9
             COMMAND bash validate_cdl.sh f_compression f_compress_deflate9.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_deflate9_expected.cdl)
    set_tests_properties(f_compression_validate_deflate9 PROPERTIES DEPENDS f_compression_run)
    
    add_test(NAME f_compression_validate_shuffle
             COMMAND bash validate_cdl.sh f_compression f_compress_shuffle.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_shuffle_expected.cdl)
    set_tests_properties(f_compression_validate_shuffle PROPERTIES DEPENDS f_compression_run)
    
    add_test(NAME f_compression_validate_shuffle_deflate5
             COMMAND bash validate_cdl.sh f_compression f_compress_shuffle_deflate5.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_compression_shuffle_deflate5_expected.cdl)
    set_tests_properties(f_compression_validate_shuffle_deflate5 PROPERTIES DEPENDS f_compression_run)
    
    # Multi-file examples: f_chunking_performance
    add_test(NAME f_chunking_performance_validate_contiguous
             COMMAND bash validate_cdl.sh f_chunking_performance f_chunk_contiguous.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_chunking_performance_contiguous_expected.cdl)
    set_tests_properties(f_chunking_performance_validate_contiguous PROPERTIES DEPENDS f_chunking_performance_run)
    
    add_test(NAME f_chunking_performance_validate_time_optimized
             COMMAND bash validate_cdl.sh f_chunking_performance f_chunk_time_optimized.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_chunking_performance_time_optimized_expected.cdl)
    set_tests_properties(f_chunking_performance_validate_time_optimized PROPERTIES DEPENDS f_chunking_performance_run)
    
    add_test(NAME f_chunking_performance_validate_spatial_optimized
             COMMAND bash validate_cdl.sh f_chunking_performance f_chunk_spatial_optimized.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_chunking_performance_spatial_optimized_expected.cdl)
    set_tests_properties(f_chunking_performance_validate_spatial_optimized PROPERTIES DEPENDS f_chunking_performance_run)
    
    add_test(NAME f_chunking_performance_validate_balanced
             COMMAND bash validate_cdl.sh f_chunking_performance f_chunk_balanced.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_chunking_performance_balanced_expected.cdl)
    set_tests_properties(f_chunking_performance_validate_balanced PROPERTIES DEPENDS f_chunking_performance_run)
endif()

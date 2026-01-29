# CMakeLists.txt for classic NetCDF Fortran examples
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

# List of Fortran classic example programs
set(F_CLASSIC_EXAMPLES
    f_quickstart
    f_simple_2D
    f_coord_vars
    f_format_variants
    f_size_limits
    f_unlimited_dim
    f_var4d
)

# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# Build each example using nf-config flags
foreach(example ${F_CLASSIC_EXAMPLES})
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
    add_test(NAME f_quickstart_validate 
             COMMAND bash validate_cdl.sh f_quickstart f_quickstart.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_quickstart_expected.cdl)
    set_tests_properties(f_quickstart_validate PROPERTIES DEPENDS f_quickstart_run)
    
    add_test(NAME f_simple_2D_validate 
             COMMAND bash validate_cdl.sh f_simple_2D f_simple_2D.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_simple_2D_expected.cdl)
    set_tests_properties(f_simple_2D_validate PROPERTIES DEPENDS f_simple_2D_run)
    
    add_test(NAME f_coord_vars_validate 
             COMMAND bash validate_cdl.sh f_coord_vars f_coord_vars.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_coord_vars_expected.cdl)
    set_tests_properties(f_coord_vars_validate PROPERTIES DEPENDS f_coord_vars_run)
    
    add_test(NAME f_unlimited_dim_validate 
             COMMAND bash validate_cdl.sh f_unlimited_dim f_unlimited_dim.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_unlimited_dim_expected.cdl)
    set_tests_properties(f_unlimited_dim_validate PROPERTIES DEPENDS f_unlimited_dim_run)
    
    add_test(NAME f_var4d_validate 
             COMMAND bash validate_cdl.sh f_var4d f_var4d.nc 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_var4d_expected.cdl)
    set_tests_properties(f_var4d_validate PROPERTIES DEPENDS f_var4d_run)
    
    # Multi-file examples: f_format_variants
    add_test(NAME f_format_variants_validate_classic
             COMMAND bash validate_cdl.sh f_format_variants f_format_classic.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_format_variants_classic_expected.cdl)
    set_tests_properties(f_format_variants_validate_classic PROPERTIES DEPENDS f_format_variants_run)
    
    add_test(NAME f_format_variants_validate_64bit_offset
             COMMAND bash validate_cdl.sh f_format_variants f_format_64bit_offset.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_format_variants_64bit_offset_expected.cdl)
    set_tests_properties(f_format_variants_validate_64bit_offset PROPERTIES DEPENDS f_format_variants_run)
    
    add_test(NAME f_format_variants_validate_64bit_data
             COMMAND bash validate_cdl.sh f_format_variants f_format_64bit_data.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_format_variants_64bit_data_expected.cdl)
    set_tests_properties(f_format_variants_validate_64bit_data PROPERTIES DEPENDS f_format_variants_run)
    
    # Multi-file examples: f_size_limits
    add_test(NAME f_size_limits_validate_classic
             COMMAND bash validate_cdl.sh f_size_limits f_size_limits_classic.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_size_limits_classic_expected.cdl)
    set_tests_properties(f_size_limits_validate_classic PROPERTIES DEPENDS f_size_limits_run)
    
    add_test(NAME f_size_limits_validate_64bit_offset
             COMMAND bash validate_cdl.sh f_size_limits f_size_limits_64bit_offset.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_size_limits_64bit_offset_expected.cdl)
    set_tests_properties(f_size_limits_validate_64bit_offset PROPERTIES DEPENDS f_size_limits_run)
    
    add_test(NAME f_size_limits_validate_64bit_data
             COMMAND bash validate_cdl.sh f_size_limits f_size_limits_64bit_data.nc
                     ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_size_limits_64bit_data_expected.cdl)
    set_tests_properties(f_size_limits_validate_64bit_data PROPERTIES DEPENDS f_size_limits_run)
endif()

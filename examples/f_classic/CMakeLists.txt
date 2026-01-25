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
    f_simple_2D
    f_coord_vars
    f_format_variants
    f_size_limits
    f_unlimited_dim
    f_var4d
)

# Build each example using nf-config flags
foreach(example ${F_CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.f90)
    target_compile_options(${example} PRIVATE ${NF_FFLAGS_LIST})
    target_link_directories(${example} PRIVATE ${HDF5_LIB_DIR})
    # nf-config --flibs may not include all transitive dependencies, add them explicitly
    # NetCDF needs both libhdf5 and libhdf5_hl
    target_link_libraries(${example} PRIVATE ${NF_LIBS_LIST} ${NETCDF_LIBRARIES} hdf5 hdf5_hl)
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

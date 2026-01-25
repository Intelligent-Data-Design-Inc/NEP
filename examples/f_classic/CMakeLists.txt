# CMakeLists.txt for classic NetCDF Fortran examples
# Edward Hartnett 1/25/26

# Get NetCDF-Fortran compiler and linker flags from nf-config
execute_process(COMMAND nf-config --fflags OUTPUT_VARIABLE NF_FFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND nf-config --flibs OUTPUT_VARIABLE NF_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

# Parse flags into lists for CMake (separate by spaces)
separate_arguments(NF_FFLAGS_LIST UNIX_COMMAND "${NF_FFLAGS}")
separate_arguments(NF_LIBS_LIST UNIX_COMMAND "${NF_LIBS}")

# List of Fortran classic example programs
set(F_CLASSIC_EXAMPLES
    f_simple_2D
    f_coord_vars
    f_format_variants
    f_size_limits
    f_unlimited_dim
    f_var4d
)

# Build each example
foreach(example ${F_CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.f90)
    target_compile_options(${example} PRIVATE ${NF_FFLAGS_LIST})
    target_link_libraries(${example} PRIVATE ${NF_LIBS_LIST})
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

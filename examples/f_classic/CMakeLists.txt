# CMakeLists.txt for classic NetCDF Fortran examples
# Edward Hartnett 1/25/26

# Get NetCDF-Fortran compile and link flags using nf-config (includes all dependencies)
execute_process(
    COMMAND nf-config --fflags
    OUTPUT_VARIABLE NF_FFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND nf-config --flibs
    OUTPUT_VARIABLE NF_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

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
    set_target_properties(${example} PROPERTIES
        COMPILE_FLAGS "${NF_FFLAGS}"
        LINK_FLAGS "${NF_LIBS}"
    )
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

# CMakeLists.txt for NetCDF-4 Fortran examples
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

# List of Fortran NetCDF-4 example programs
set(F_NETCDF4_EXAMPLES
    f_simple_nc4
    f_compression
    f_chunking_performance
    f_multi_unlimited
    f_user_types
)

# Build each example using nf-config flags
foreach(example ${F_NETCDF4_EXAMPLES})
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

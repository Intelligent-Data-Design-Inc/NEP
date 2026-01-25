# CMakeLists.txt for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# Get NetCDF compile and link flags using nc-config (includes all dependencies)
execute_process(
    COMMAND nc-config --cflags
    OUTPUT_VARIABLE NC_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND nc-config --libs
    OUTPUT_VARIABLE NC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# List of NetCDF-4 example programs
set(NETCDF4_EXAMPLES
    simple_nc4
    compression
    chunking_performance
    multi_unlimited
    user_types
)

# Build each example using nc-config flags
foreach(example ${NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.c)
    set_target_properties(${example} PROPERTIES
        COMPILE_FLAGS "${NC_CFLAGS}"
        LINK_FLAGS "${NC_LIBS} -lm"
    )
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

# CMakeLists.txt for classic NetCDF C examples
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

# List of classic example programs
set(CLASSIC_EXAMPLES
    simple_2D
    coord_vars
    format_variants
    size_limits
    unlimited_dim
    var4d
)

# Build each example using nc-config flags
foreach(example ${CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.c)
    set_target_properties(${example} PROPERTIES
        COMPILE_FLAGS "${NC_CFLAGS}"
        LINK_FLAGS "${NC_LIBS}"
    )
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

# CMakeLists.txt for classic NetCDF C examples
# Edward Hartnett 1/25/26

# List of classic example programs
set(CLASSIC_EXAMPLES
    simple_2D
    coord_vars
    format_variants
    size_limits
    unlimited_dim
    var4d
)

# Build each example
foreach(example ${CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_include_directories(${example} PRIVATE ${NETCDF_INCLUDE_DIRS})
    target_link_libraries(${example} PRIVATE ${NETCDF_LIBRARIES})
    target_link_directories(${example} PRIVATE ${NETCDF_LIBRARY_DIRS})
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

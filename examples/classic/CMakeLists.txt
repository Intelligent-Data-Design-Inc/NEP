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

# Build each example using NetCDF variables from parent scope
foreach(example ${CLASSIC_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_include_directories(${example} PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR})
    target_link_directories(${example} PRIVATE ${NETCDF_LIBRARY_DIRS} ${HDF5_LIBRARY_DIRS})
    target_link_libraries(${example} PRIVATE ${NETCDF_LIBRARIES} ${HDF5_C_${LIB_TYPE}_LIBRARY})
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

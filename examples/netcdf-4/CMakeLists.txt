# CMakeLists.txt for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# List of NetCDF-4 example programs
set(NETCDF4_EXAMPLES
    simple_nc4
    compression
    chunking_performance
    multi_unlimited
    user_types
)

# Build each example using NetCDF variables from parent scope
foreach(example ${NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.c)
    target_include_directories(${example} PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR})
    target_link_directories(${example} PRIVATE ${NETCDF_LIBRARY_DIRS} ${HDF5_LIBRARY_DIRS})
    target_link_libraries(${example} PRIVATE ${NETCDF_LIBRARIES} ${HDF5_C_${LIB_TYPE}_LIBRARY} m)
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

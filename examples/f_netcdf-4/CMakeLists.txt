# CMakeLists.txt for NetCDF-4 Fortran examples
# Edward Hartnett 1/25/26

# List of Fortran NetCDF-4 example programs
set(F_NETCDF4_EXAMPLES
    f_simple_nc4
    f_compression
    f_chunking_performance
    f_multi_unlimited
    f_user_types
)

# Build each example using NetCDF-Fortran variables from parent scope
foreach(example ${F_NETCDF4_EXAMPLES})
    add_executable(${example} ${example}.f90)
    target_include_directories(${example} PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
    target_link_directories(${example} PRIVATE ${NetCDF_Fortran_LIBRARY_DIRS})
    target_link_libraries(${example} PRIVATE ${NetCDF_Fortran_LIBRARIES})
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

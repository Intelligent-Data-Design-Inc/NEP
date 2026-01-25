# CMakeLists.txt for classic NetCDF Fortran examples
# Edward Hartnett 1/25/26

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
    target_include_directories(${example} PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
    target_link_libraries(${example} PRIVATE ${NetCDF_Fortran_LIBRARIES})
    target_link_directories(${example} PRIVATE ${NetCDF_Fortran_LIBRARY_DIRS})
    
    # Add as test
    if(BUILD_TESTING)
        add_test(NAME ${example} COMMAND ${example})
    endif()
endforeach()

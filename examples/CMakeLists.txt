# CMakeLists.txt for NEP examples
# Edward Hartnett 1/25/26

# C examples are always built when BUILD_EXAMPLES=ON
add_subdirectory(classic)
add_subdirectory(netcdf-4)

# Fortran examples only built when Fortran is enabled and NetCDF-Fortran is found
if(ENABLE_FORTRAN)
    if(NetCDF_Fortran_FOUND)
        add_subdirectory(f_classic)
        add_subdirectory(f_netcdf-4)
    else()
        message(STATUS "Skipping Fortran examples - NetCDF-Fortran not found")
    endif()
endif()

# Placeholder directories for future examples
# add_subdirectory(parallelIO)
# add_subdirectory(performance)

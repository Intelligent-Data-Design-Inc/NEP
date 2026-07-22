# CMakeLists.txt for NEP examples
# Edward Hartnett 1/25/26

# C examples are always built when NEP_BUILD_EXAMPLES=ON
add_subdirectory(classic)
add_subdirectory(netcdf-4)
add_subdirectory(performance)

if(NEP_ENABLE_VIZ_EXAMPLES)
    add_subdirectory(viz)
endif()

# Fortran examples only built when Fortran is enabled and NetCDF-Fortran is found
if(NEP_ENABLE_FORTRAN)
    if(NetCDF_Fortran_FOUND)
        add_subdirectory(f_classic)
        add_subdirectory(f_netcdf-4)
    else()
        message(STATUS "Skipping Fortran examples - NetCDF-Fortran not found")
    endif()
endif()

# Parallel I/O examples only built when NEP_ENABLE_PARALLEL_TESTS is ON
if(NEP_ENABLE_PARALLEL_TESTS AND HAVE_PARALLEL_TESTS)
    add_subdirectory(parallelIO)
endif()

# OPeNDAP examples only built when NEP_ENABLE_OPENDAP_EXAMPLES is ON (v1.10.0)
if(NEP_ENABLE_OPENDAP_EXAMPLES)
    add_subdirectory(opendap)
endif()

# NcZarr examples only built when NetCDF-C supports NcZarr (v1.11.0)
if(HAVE_NCZARR)
    add_subdirectory(nczarr)
endif()

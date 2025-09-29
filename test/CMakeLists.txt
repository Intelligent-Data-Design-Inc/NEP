# Build file for the test directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Build the test only if GRIB2 is enabled
if(HAVE_GRIB2)
    add_executable (test_grib2_vol test_grib2_vol.c)
    target_include_directories (test_grib2_vol PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries (test_grib2_vol ${HDF5_C_LIBRARIES})

    # Add the test
    add_test (test_grib2_vol test_grib2_vol)
    set_tests_properties(test_grib2_vol PROPERTIES
        ENVIRONMENT "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/src")
endif()

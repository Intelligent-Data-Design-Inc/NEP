# Build file for the test directory of the NFEP project.
# Edward Hartnett 8/27/25

# Build the test
add_executable (vol_plugin vol_plugin.c)
target_include_directories (vol_plugin PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries (vol_plugin ${HDF5_C_LIBRARIES})

# Copy test data file to the build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gdaswave.t00z.wcoast.0p16.f000.grib2 ${CMAKE_CURRENT_BINARY_DIR}/gdaswave.t00z.wcoast.0p16.f000.grib2 COPYONLY)

# Add the test
add_test (vol_plugin vol_plugin)
set_tests_properties(vol_plugin PROPERTIES
    ENVIRONMENT "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/src")

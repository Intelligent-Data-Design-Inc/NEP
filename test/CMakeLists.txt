# Build file for the test directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Enable testing
enable_testing()

# Find NetCDF library for tests
find_package(PkgConfig REQUIRED)
pkg_check_modules(NETCDF REQUIRED netcdf)
pkg_get_variable(NETCDF_PREFIX netcdf prefix)

# Build LZ4 test
add_executable(tst_lz4 tst_lz4.c)
target_link_libraries(tst_lz4 PRIVATE ncsqueeze ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
target_include_directories(tst_lz4 PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})

# Build BZIP2 test
add_executable(tst_bzip2 tst_bzip2.c)
target_link_libraries(tst_bzip2 PRIVATE ncsqueeze ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
target_include_directories(tst_bzip2 PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})

# Build performance test
add_executable(tst_perf tst_perf.c tst_utils.c)
target_link_libraries(tst_perf PRIVATE ncsqueeze ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
target_include_directories(tst_perf PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})

# Add tests with proper HDF5_PLUGIN_PATH environment
# Include both our built LZ4 plugin and NetCDF-C's BZIP2 plugin
set(PLUGIN_PATH "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin:${NETCDF_PREFIX}/hdf5/lib/plugin")

add_test(NAME test_bzip2 COMMAND tst_bzip2)
set_tests_properties(test_bzip2 PROPERTIES
    ENVIRONMENT "HDF5_PLUGIN_PATH=${PLUGIN_PATH}"
)

add_test(NAME test_lz4 COMMAND tst_lz4)
set_tests_properties(test_lz4 PROPERTIES
    ENVIRONMENT "HDF5_PLUGIN_PATH=${PLUGIN_PATH}"
)

add_test(NAME test_perf COMMAND tst_perf)
set_tests_properties(test_perf PROPERTIES
    ENVIRONMENT "HDF5_PLUGIN_PATH=${PLUGIN_PATH}"
)

# Make tests depend on hdf5_plugins being built
add_dependencies(tst_lz4 hdf5_plugins)
add_dependencies(tst_bzip2 hdf5_plugins)
add_dependencies(tst_perf hdf5_plugins)

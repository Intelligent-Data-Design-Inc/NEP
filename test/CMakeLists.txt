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

# Build CDF UDF test if CDF is enabled
if(HAVE_CDF)
    add_executable(tst_cdf_udf tst_cdf_udf.c)
    target_link_libraries(tst_cdf_udf PRIVATE nccdf ncsqueeze ${NETCDF_LIBRARIES} ${CDF_LIBRARY})
    target_include_directories(tst_cdf_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${CDF_INCLUDE_DIR})
    
    # Set rpath so the executable can find CDF library at runtime
    get_filename_component(CDF_LIB_DIR ${CDF_LIBRARY} DIRECTORY)
    set_target_properties(tst_cdf_udf PROPERTIES
        INSTALL_RPATH "${CDF_LIB_DIR}"
        BUILD_RPATH "${CDF_LIB_DIR}")
    
    # Add test - creates its own CDF file
    add_test(NAME tst_cdf_udf COMMAND tst_cdf_udf)
    
    # Build IMAP MAG CDF test
    add_executable(tst_imap_mag tst_imap_mag.c)
    target_link_libraries(tst_imap_mag PRIVATE nccdf ncsqueeze ${NETCDF_LIBRARIES} ${CDF_LIBRARY})
    target_include_directories(tst_imap_mag PRIVATE ${NETCDF_INCLUDE_DIRS} ${CDF_INCLUDE_DIR})
    
    # Set rpath so the executable can find CDF library at runtime
    set_target_properties(tst_imap_mag PROPERTIES
        INSTALL_RPATH "${CDF_LIB_DIR}"
        BUILD_RPATH "${CDF_LIB_DIR}")
    
    # Add test - uses existing IMAP MAG CDF file
    add_test(NAME tst_imap_mag COMMAND tst_imap_mag 
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Configure run_tests.sh script with proper paths
# For CMake builds, point to the installed plugin location
set(MY_HDF5_PLUGIN_PATH "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin:${NETCDF_PREFIX}/hdf5/lib/plugin")
set(BUILD_LZ4 "yes")
set(BUILD_BZIP2 "yes")
set(BUILD_JPEG "no")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh.in 
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh @ONLY)

# Make the script executable
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh 
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Add single test that runs the test script (like autotools)
# The script sets HDF5_PLUGIN_PATH itself from MY_HDF5_PLUGIN_PATH
add_test(NAME run_tests 
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Make test script depend on hdf5_plugins being built
if(HAVE_CDF)
    add_custom_target(test_executables ALL DEPENDS tst_lz4 tst_bzip2 tst_perf tst_cdf_udf tst_imap_mag)
else()
    add_custom_target(test_executables ALL DEPENDS tst_lz4 tst_bzip2 tst_perf)
endif()
add_dependencies(test_executables hdf5_plugins)

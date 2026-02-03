# This is the CMakeLists.txt file for the test_cdf directory of the NEP project.
# Edward Hartnett 2025-11-23

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Add HDF5 include directories to match AM_CPPFLAGS += $(HDF5_CPPFLAGS)
include_directories(${HDF5_INCLUDE_DIR})

# Build the test program (matches check_PROGRAMS = tst_cdf_basic)
# Test creates its own CDF file at runtime, no test data needed
add_executable(tst_cdf_basic tst_cdf_basic.c)

# Link against CDF library
target_link_libraries(tst_cdf_basic ${CDF_LIBRARY})

# Add CDF include directory
target_include_directories(tst_cdf_basic PRIVATE ${CDF_INCLUDE_DIR})

# Set rpath so the executable can find CDF library at runtime
get_filename_component(CDF_LIB_DIR ${CDF_LIBRARY} DIRECTORY)
set_target_properties(tst_cdf_basic PROPERTIES
    INSTALL_RPATH "${CDF_LIB_DIR}"
    BUILD_RPATH "${CDF_LIB_DIR}")

# Add the test (matches TESTS = tst_cdf_basic)
add_test(NAME tst_cdf_basic COMMAND tst_cdf_basic)

# Test for UDF self-loading functionality (v1.5.4 Sprint 3)
add_executable(tst_cdf_self_load tst_cdf_self_load.c)
target_link_libraries(tst_cdf_self_load 
    nccdf
    ${NETCDF_LIBRARIES}
    ${HDF5_LIBRARIES}
    ${CDF_LIBRARY})
target_include_directories(tst_cdf_self_load PRIVATE ${CDF_INCLUDE_DIR})
set_target_properties(tst_cdf_self_load PROPERTIES
    INSTALL_RPATH "${CDF_LIB_DIR}"
    BUILD_RPATH "${CDF_LIB_DIR}")
add_test(NAME tst_cdf_self_load COMMAND tst_cdf_self_load)
set_tests_properties(tst_cdf_self_load PROPERTIES 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

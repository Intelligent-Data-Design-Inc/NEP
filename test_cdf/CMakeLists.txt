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

# Add the test (matches TESTS = tst_cdf_basic)
add_test(NAME tst_cdf_basic COMMAND tst_cdf_basic)

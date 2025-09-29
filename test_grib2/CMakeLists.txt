# This is the CMakeLists.txt file for the test_grib2 directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Add HDF5 include directories to match AM_CPPFLAGS += $(HDF5_CPPFLAGS)
include_directories(${HDF5_INCLUDE_DIR})

# Build the test program (matches check_PROGRAMS = tst_read1)
add_executable(tst_read1 tst_read1.c)

# Link against ncgrib2 library and netcdf (matches tst_read1_LDADD = -lnetcdf ${top_builddir}/src/libncgrib2.la)
target_link_libraries(tst_read1 ncgrib2 ${NETCDF_LIBRARIES} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# Add the test (matches TESTS = tst_read1)
add_test(NAME tst_read1 COMMAND tst_read1)

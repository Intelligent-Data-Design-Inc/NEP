# This is the CMakeLists.txt file for the test_grib2 directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Add HDF5 include directories to match AM_CPPFLAGS += $(HDF5_CPPFLAGS)
include_directories(${HDF5_INCLUDE_DIR})

# Test data file - copy to build directory preserving data/ subdirectory (matches noinst_DATA)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/gdaswave.t00z.wcoast.0p16.f000.grib2 
               ${CMAKE_CURRENT_BINARY_DIR}/data/gdaswave.t00z.wcoast.0p16.f000.grib2 COPYONLY)

# Build the test program (matches check_PROGRAMS = tst_grib2)
add_executable(tst_grib2 tst_grib2.c)

# Link against ncgrib2 library and netcdf (matches tst_grib2_LDADD = -lnetcdf ${top_builddir}/src/libncgrib2.la)
target_link_libraries(tst_grib2 ncgrib2 ${NETCDF_LIBRARIES} ${HDF5_C_${LIB_TYPE}_LIBRARY})

# Add the test (matches TESTS = tst_grib2)
add_test(NAME tst_grib2 COMMAND tst_grib2)

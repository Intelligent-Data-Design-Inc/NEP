# This is the build file for the test_geotiff directory of the NEP project.
#
# Edward Hartnett 12/26/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
AM_CPPFLAGS += $(HDF5_CPPFLAGS) $(NETCDF_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# Only build tests if GeoTIFF is enabled
if HAVE_GEOTIFF

# Test programs
check_PROGRAMS = tst_geotiff_detect tst_geotiff_handle

# Test for GeoTIFF format detection
tst_geotiff_detect_SOURCES = tst_geotiff_detect.c
tst_geotiff_detect_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF file handle management
tst_geotiff_handle_SOURCES = tst_geotiff_handle.c
tst_geotiff_handle_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Define tests
TESTS = tst_geotiff_detect tst_geotiff_handle

endif # HAVE_GEOTIFF

# Extra files to include in distribution
EXTRA_DIST = generate_test_files.py data

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

# Copy test data files to build directory
check-local:
	cp -f $(top_srcdir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif $(builddir)/
	cp -f $(top_srcdir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif $(builddir)/

# Test programs
check_PROGRAMS = tst_geotiff_handle tst_geotiff_metadata tst_phase2_acceptance

# Test for GeoTIFF file handle management
tst_geotiff_handle_SOURCES = tst_geotiff_handle.c
tst_geotiff_handle_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 2: metadata extraction
tst_geotiff_metadata_SOURCES = tst_geotiff_metadata.c
tst_geotiff_metadata_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 2: acceptance criteria
tst_phase2_acceptance_SOURCES = tst_phase2_acceptance.c
tst_phase2_acceptance_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Define tests
TESTS = tst_geotiff_handle tst_geotiff_metadata tst_phase2_acceptance

endif # HAVE_GEOTIFF

# Extra files to include in distribution
EXTRA_DIST = generate_test_files.py data

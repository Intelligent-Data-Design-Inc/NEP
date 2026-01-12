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

# Copy test data files to build directory before running tests
check-local:
	cp -f $(top_srcdir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif $(builddir)/
	cp -f $(top_srcdir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif $(builddir)/
	cp -f $(top_srcdir)/test/data/ABBA_2022_C61_HNL.tif $(builddir)/

# Test programs
check_PROGRAMS = tst_geotiff_handle tst_geotiff_metadata tst_phase2_acceptance \
		 tst_geotiff_organization tst_single_band_read tst_large_geotiff \
		 tst_geotiff_errors tst_geotiff_edge_cases tst_simple_geotiff \
		 test_netcdf_crs

# Benchmark programs (not run automatically)
noinst_PROGRAMS = bench_geotiff_performance

# Test for GeoTIFF file handle management
tst_geotiff_handle_SOURCES = tst_geotiff_handle.c
tst_geotiff_handle_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 2: metadata extraction
tst_geotiff_metadata_SOURCES = tst_geotiff_metadata.c
tst_geotiff_metadata_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 2: acceptance criteria
tst_phase2_acceptance_SOURCES = tst_phase2_acceptance.c
tst_phase2_acceptance_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 3.1: organization detection
tst_geotiff_organization_SOURCES = tst_geotiff_organization.c
tst_geotiff_organization_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 3.2: single-band raster reading
tst_single_band_read_SOURCES = tst_single_band_read.c
tst_single_band_read_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 3.4/3.5: large file and tiled organization
tst_large_geotiff_SOURCES = tst_large_geotiff.c
tst_large_geotiff_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 3.5a: error handling
tst_geotiff_errors_SOURCES = tst_geotiff_errors.c
tst_geotiff_errors_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for GeoTIFF Phase 3.5a: edge cases
tst_geotiff_edge_cases_SOURCES = tst_geotiff_edge_cases.c
tst_geotiff_edge_cases_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for simple GeoTIFF value comparison (issue #84)
tst_simple_geotiff_SOURCES = tst_simple_geotiff.c
tst_simple_geotiff_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Test for CRS metadata extraction (issue #92)
test_netcdf_crs_SOURCES = test_netcdf_crs.c
test_netcdf_crs_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Benchmark for GeoTIFF Phase 3.5a: performance validation
bench_geotiff_performance_SOURCES = bench_geotiff_performance.c
bench_geotiff_performance_LDADD = $(top_builddir)/src/libncgeotiff.la $(NETCDF_LIBS) $(GEOTIFF_LIBS) $(HDF5_LIBS)

# Define tests
# Note: test_netcdf_crs requires command-line argument, run manually:
#   ./test_netcdf_crs MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif
TESTS = tst_geotiff_handle tst_geotiff_metadata tst_phase2_acceptance \
	tst_geotiff_organization tst_single_band_read tst_large_geotiff \
	tst_geotiff_errors tst_geotiff_edge_cases tst_simple_geotiff

endif # HAVE_GEOTIFF

# Extra files to include in distribution
EXTRA_DIST = generate_test_files.py data

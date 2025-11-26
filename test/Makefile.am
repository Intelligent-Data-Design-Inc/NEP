# This is the Makefile.am file for the test directory of the NEP project.

# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
AM_LDFLAGS = ${top_builddir}/src/libncsqueeze.la
LDADD = ${top_builddir}/src/libncsqueeze.la
check_PROGRAMS =
TESTS =

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# check_SCRIPTS = test_vol_plugin.sh

# if HAVE_GRIB2
# check_PROGRAMS += test_grib2_vol
# test_grib2_vol_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS)
# test_grib2_vol_LDADD = $(HDF5_LIBS)
# test_grib2_vol_SOURCES = test_grib2_vol.c

# TESTS += test_vol_plugin.sh

# # Copy test data to build directory only for out-of-tree builds and only during check
# check-local:
# 	if test "$(srcdir)" != "$(builddir)" ; then \
# 		$(MKDIR_P) $(builddir)/data; \
# 		$(INSTALL_DATA) $(srcdir)/$(TEST_DATA_FILE) $(builddir)/$(TEST_DATA_FILE); \
# 	fi
# endif

# Build LZ4 tests, if needed.
if BUILD_LZ4
check_PROGRAMS += tst_lz4
tst_lz4_LDADD = ${top_builddir}/src/libncsqueeze.la
endif

# Build BZIP2 tests, if needed.
if BUILD_BZIP2
check_PROGRAMS += tst_bzip2
endif

# Build JPEG tests, if needed.
if BUILD_JPEG
check_PROGRAMS += tst_jpeg
endif

# Build the performance tests, if all filters have been built.
check_PROGRAMS += tst_perf
tst_perf_SOURCES = tst_perf.c tst_utils.c

# Build CDF UDF tests, if CDF is enabled.
if HAVE_CDF
check_PROGRAMS += tst_cdf_udf tst_imap_mag
tst_cdf_udf_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libncsqueeze.la -lcdf
tst_imap_mag_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libncsqueeze.la -lcdf
TESTS += tst_cdf_udf tst_imap_mag
endif

# Run all the appropriate tests in this script.
TESTS += run_tests.sh

# Delete files created in testing.
CLEANFILES = tst_*.nc

EXTRA_DIST = run_tests.sh.in

DISTCLEANFILES = run_tests.sh


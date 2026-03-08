# This is the Makefile.am file for the test directory of the NEP project.

# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
AM_LDFLAGS = ${top_builddir}/src/libnep.la
LDADD = ${top_builddir}/src/libnep.la
check_PROGRAMS =
TESTS =

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# check_SCRIPTS = test_vol_plugin.sh

# Build GRIB2 UDF placeholder test, if GRIB2 is enabled.
if HAVE_GRIB2
check_PROGRAMS += tst_grib2_udf
tst_grib2_udf_LDADD = ${top_builddir}/src/libncgrib2.la ${top_builddir}/src/libnep.la
TESTS += tst_grib2_udf

# Copy GRIB2 test data to build directory for out-of-tree builds
check-local:
	if test "$(srcdir)" != "$(builddir)" ; then \
		$(MKDIR_P) $(builddir)/data; \
		cp $(srcdir)/data/gdaswave.t00z.wcoast.0p16.f000.grib2 $(builddir)/data/; \
	fi
endif

# Build LZ4 tests, if needed.
if BUILD_LZ4
check_PROGRAMS += tst_lz4
tst_lz4_LDADD = ${top_builddir}/src/libnep.la
endif

# Build BZIP2 tests, if needed.
if BUILD_BZIP2
check_PROGRAMS += tst_bzip2
endif

# Build the performance tests, if all filters have been built.
check_PROGRAMS += tst_perf
tst_perf_SOURCES = tst_perf.c tst_utils.c
tst_perf_LDADD = ${top_builddir}/src/libnep.la -lnetcdf -lhdf5_hl -lhdf5 -lz -ldl -lm

# Build CDF UDF tests, if CDF is enabled.
if HAVE_CDF
check_PROGRAMS += tst_cdf_udf tst_imap_mag
tst_cdf_udf_LDFLAGS = $(CDF_LDFLAGS)
tst_cdf_udf_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libnep.la -lcdf
tst_imap_mag_LDFLAGS = $(CDF_LDFLAGS)
tst_imap_mag_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libnep.la -lcdf
TESTS += tst_cdf_udf tst_imap_mag

# Copy test data to build directory for out-of-tree builds
check-local:
	if test "$(srcdir)" != "$(builddir)" ; then \
		$(MKDIR_P) $(builddir)/data; \
		cp $(srcdir)/data/imap_mag_l1b-calibration_20240229_v001.cdf $(builddir)/data/; \
	fi
endif

TESTS_ENVIRONMENT = LD_LIBRARY_PATH=@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH

# Run all the appropriate tests in this script.
TESTS += run_tests.sh

# Delete files created in testing.
CLEANFILES = tst_*.nc

EXTRA_DIST = run_tests.sh.in data/imap_mag_l1b-calibration_20240229_v001.cdf data/gdaswave.t00z.wcoast.0p16.f000.grib2

DISTCLEANFILES = run_tests.sh


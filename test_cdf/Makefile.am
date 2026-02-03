# This is the Makefile.am file for the test_cdf directory of the NEP project.
# Edward Hartnett 2025-11-23

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# Add CDF include and library paths
AM_CPPFLAGS += $(CDF_CPPFLAGS)
AM_LDFLAGS = $(CDF_LDFLAGS)
LDADD = $(CDF_LIBS)

# Build the CDF test programs
# Tests create their own CDF file at runtime, no test data needed
check_PROGRAMS = tst_cdf_basic tst_cdf_self_load
tst_cdf_basic_SOURCES = tst_cdf_basic.c

# Test for UDF self-loading functionality (v1.5.4 Sprint 3)
tst_cdf_self_load_SOURCES = tst_cdf_self_load.c
tst_cdf_self_load_LDADD = $(top_builddir)/src/libnccdf.la $(NETCDF_LIBS) $(HDF5_LIBS) $(CDF_LIBS)

# Run the tests
TESTS = tst_cdf_basic tst_cdf_self_load

# Clean up test outputs (test creates and removes its own file)
CLEANFILES = tst_cdf_simple.cdf

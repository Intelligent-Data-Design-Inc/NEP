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

# Build the CDF test program
# Test creates its own CDF file at runtime, no test data needed
check_PROGRAMS = tst_cdf_basic
tst_cdf_basic_SOURCES = tst_cdf_basic.c

# Run the test
TESTS = tst_cdf_basic

# Clean up test outputs (test creates and removes its own file)
CLEANFILES = tst_cdf_simple.cdf

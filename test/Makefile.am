ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

check_SCRIPTS = test_vol_plugin.sh

check_PROGRAMS = vol_plugin
vol_plugin_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS)
vol_plugin_LDADD = $(HDF5_LIBS)

# Test data file - we'll handle it with custom rules, not with automake's installation mechanism
TEST_DATA_FILE = gdaswave.t00z.wcoast.0p16.f000.grib2

TESTS = test_vol_plugin.sh

DISTCLEANFILES = test_vol_plugin.sh

# Include test data file in distribution
dist_noinst_DATA = $(TEST_DATA_FILE)

# Install test data to the build directory during 'make all' or 'make check'
all-local:
	$(MKDIR_P) $(builddir)
	$(INSTALL_DATA) $(srcdir)/$(TEST_DATA_FILE) $(builddir)/$(TEST_DATA_FILE)

# Also copy during 'make check' in case user skipped 'make all'
check-local:
	$(MKDIR_P) $(builddir)
	$(INSTALL_DATA) $(srcdir)/$(TEST_DATA_FILE) $(builddir)/$(TEST_DATA_FILE)

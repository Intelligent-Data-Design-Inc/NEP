# This is the Makefile.am file for the test directory of the NFEP project.
# Edward Hartnett 8/27/25

ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

check_SCRIPTS = test_vol_plugin.sh

if HAVE_GRIB2
check_PROGRAMS = test_grib2_vol
test_grib2_vol_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS)
test_grib2_vol_LDADD = $(HDF5_LIBS)
test_grib2_vol_SOURCES = test_grib2_vol.c
# Test data file - declare it as noinst_HEADERS which won't be installed but will be distributed
TEST_DATA_FILE = gdaswave.t00z.wcoast.0p16.f000.grib2
noinst_DATA = $(TEST_DATA_FILE)
TESTS = test_vol_plugin.sh
DISTCLEANFILES = test_vol_plugin.sh

# Install test data to the build directory during 'make all' or 'make check'
# but only if we're doing an out-of-tree build
all-local:
	if test "$(srcdir)" != "$(builddir)" ; then \
		$(MKDIR_P) $(builddir); \
		$(INSTALL_DATA) $(srcdir)/$(TEST_DATA_FILE) $(builddir)/$(TEST_DATA_FILE); \
	fi

# Also copy during 'make check' in case user skipped 'make all'
check-local:
	if test "$(srcdir)" != "$(builddir)" ; then \
		$(MKDIR_P) $(builddir); \
		$(INSTALL_DATA) $(srcdir)/$(TEST_DATA_FILE) $(builddir)/$(TEST_DATA_FILE); \
	fi
endif

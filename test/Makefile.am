ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

check_SCRIPTS = test_vol_plugin.sh

check_PROGRAMS = vol_plugin
vol_plugin_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS)
vol_plugin_LDADD = $(HDF5_LIBS)

TESTS = test_vol_plugin.sh

DISTCLEANFILES = test_vol_plugin.sh

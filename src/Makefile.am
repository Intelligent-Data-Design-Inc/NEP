# This is the build file for the src directory of the NFEP project.
# Edward Hartnett 8/27/25

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra -fPIC $(HDF5_CFLAGS)

# Initialize variables
lib_LTLIBRARIES =
include_HEADERS =

# This is a libtool library.
lib_LTLIBRARIES += libncsqueeze.la
libncsqueeze_la_LDFLAGS = -version-info 1:1:0
libncsqueeze_la_SOURCES = ncsqueeze.c

# GRIB2 VOL connector
if HAVE_GRIB2
lib_LTLIBRARIES += libgrib2_vol_connector.la
include_HEADERS += $(top_srcdir)/include/grib2_vol_connector.h
libgrib2_vol_connector_la_SOURCES = grib2_vol_connector.c
libgrib2_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libgrib2_vol_connector_la_LIBADD = $(HDF5_LIBS) $(G2C_LIBS)
endif

# Uninstall target
uninstall-local:
	@echo "Uninstalling NFEP VOL connectors..."
if HAVE_GRIB2
	-rm -f $(DESTDIR)$(libdir)/libgrib2_vol_connector.*
	-rm -f $(DESTDIR)$(includedir)/grib2_vol_connector.h
endif
	@echo "NFEP uninstall complete."



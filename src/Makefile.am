ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra -fPIC $(HDF5_CFLAGS)

# Public header
include_HEADERS = template_vol_connector.h

# Template VOL connector
lib_LTLIBRARIES = libtemplate_vol_connector.la
libtemplate_vol_connector_la_SOURCES = template_vol_connector.c
libtemplate_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libtemplate_vol_connector_la_LIBADD = $(HDF5_LIBS)


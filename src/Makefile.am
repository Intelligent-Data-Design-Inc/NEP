ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra -fPIC $(HDF5_CFLAGS)

# Public header
include_HEADERS = grib2_vol_connector.h

# Template VOL connector
lib_LTLIBRARIES = libgrib2_vol_connector.la
libgrib2_vol_connector_la_SOURCES = grib2_vol_connector.c
libgrib2_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libgrib2_vol_connector_la_LIBADD = $(HDF5_LIBS) $(G2C_LIBS)


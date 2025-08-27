# This is the build file for the src directory of the NFEP project.
# Edward Hartnett 8/27/25

ACLOCAL_AMFLAGS = -I m4

# Adjust as needed
AM_CPPFLAGS = $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra -fPIC $(HDF5_CFLAGS)

# Initialize variables
lib_LTLIBRARIES =
include_HEADERS =

# GRIB2 VOL connector
if HAVE_GRIB2
lib_LTLIBRARIES += libgrib2_vol_connector.la
include_HEADERS += grib2_vol_connector.h
libgrib2_vol_connector_la_SOURCES = grib2_vol_connector.c
libgrib2_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libgrib2_vol_connector_la_LIBADD = $(HDF5_LIBS) $(G2C_LIBS)
endif

# BUFR VOL connector
if HAVE_BUFR
lib_LTLIBRARIES += libbufr_vol_connector.la
include_HEADERS += bufr_vol_connector.h
libbufr_vol_connector_la_SOURCES = bufr_vol_connector.c
libbufr_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libbufr_vol_connector_la_LIBADD = $(HDF5_LIBS) $(BUFR_LIBS)
endif

# GeoTIFF VOL connector
if HAVE_GEOTIFF
lib_LTLIBRARIES += libgeotiff_vol_connector.la
include_HEADERS += geotiff_vol_connector.h
libgeotiff_vol_connector_la_SOURCES = geotiff_vol_connector.c
libgeotiff_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libgeotiff_vol_connector_la_LIBADD = $(HDF5_LIBS) $(GEOTIFF_LIBS)
endif

# CDF VOL connector
if HAVE_CDF
lib_LTLIBRARIES += libcdf_vol_connector.la
include_HEADERS += cdf_vol_connector.h
libcdf_vol_connector_la_SOURCES = cdf_vol_connector.c
libcdf_vol_connector_la_LDFLAGS = $(AM_LDFLAGS) $(HDF5_LDFLAGS) -avoid-version -module -shared -export-dynamic
libcdf_vol_connector_la_LIBADD = $(HDF5_LIBS) $(CDF_LIBS)
endif


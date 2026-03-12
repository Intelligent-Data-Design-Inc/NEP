# This is the build file for the src directory of the NFEP project.
#
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra -fPIC $(HDF5_CFLAGS)

# Initialize variables
lib_LTLIBRARIES =
include_HEADERS =

# This is a libtool library.
lib_LTLIBRARIES += libnep.la
libnep_la_LDFLAGS = -version-info 1:0:0
libnep_la_SOURCES = nep.c

# CDF UDF Handler - v1.3.0 Sprint 3
# CDF library (netCDF UDF support)
if HAVE_CDF
lib_LTLIBRARIES += libnccdf.la
libnccdf_la_LDFLAGS = -version-info 1:0:0 $(CDF_LDFLAGS)
libnccdf_la_SOURCES = cdfdispatch.c cdffile.c cdffunc.c cdfvar.c
libnccdf_la_LIBADD = $(HDF5_LIBS) $(NETCDF_LIBS) $(CDF_LIBS)
endif

# GeoTIFF UDF Handler - v1.5.0
# GeoTIFF library (netCDF UDF support)
if HAVE_GEOTIFF
lib_LTLIBRARIES += libncgeotiff.la
libncgeotiff_la_LDFLAGS = -version-info 1:0:0
libncgeotiff_la_SOURCES = geotiffdispatch.c geotifffile.c
libncgeotiff_la_LIBADD = $(HDF5_LIBS) $(NETCDF_LIBS) $(GEOTIFF_LIBS)
endif

# GRIB2 UDF Handler - v1.7.0
# GRIB2 library (netCDF UDF support)
if HAVE_GRIB2
lib_LTLIBRARIES += libncgrib2.la
libncgrib2_la_LDFLAGS = -version-info 1:0:0 $(G2C_LDFLAGS)
libncgrib2_la_SOURCES = grib2dispatch.c grib2file.c
libncgrib2_la_LIBADD = $(HDF5_LIBS) $(NETCDF_LIBS) $(G2C_LIBS)
endif

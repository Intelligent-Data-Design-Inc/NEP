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
lib_LTLIBRARIES += libncsqueeze.la
libncsqueeze_la_LDFLAGS = -version-info 1:0:0
libncsqueeze_la_SOURCES = ncsqueeze.c

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# GRIB2 library (netCDF UDF support)
# if HAVE_GRIB2
# lib_LTLIBRARIES += libncgrib2.la
# libncgrib2_la_LDFLAGS = -version-info 1:0:0
# libncgrib2_la_SOURCES = grib2dispatch.c grib2attr.c grib2dim.c grib2file.c grib2grp.c	\
# grib2type.c grib2var.c grib2func.c grib2logging.c
# libncgrib2_la_LIBADD = $(HDF5_LIBS) $(G2C_LIBS)
# endif

# CDF UDF Handler - v1.3.0 Sprint 3
# CDF library (netCDF UDF support)
if HAVE_CDF
lib_LTLIBRARIES += libnccdf.la
libnccdf_la_LDFLAGS = -version-info 1:0:0
libnccdf_la_SOURCES = cdfdispatch.c cdffile.c cdffunc.c cdfvar.c
libnccdf_la_LIBADD = $(HDF5_LIBS) $(NETCDF_LIBS) $(CDF_LIBS)
endif

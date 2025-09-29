# This is the Makefile.am file for the test directory of the NEP project.

# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
#AM_LDFLAGS = ${top_builddir}/src/libncsqueeze.la
#LDADD = ${top_builddir}/src/libncsqueeze.la

# Test data file - declare it as noinst_DATA which won't be installed but will be distributed
TEST_DATA_FILE = data/gdaswave.t00z.wcoast.0p16.f000.grib2
noinst_DATA = $(TEST_DATA_FILE)

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# Run the test program.
check_PROGRAMS = tst_grib2
TESTS = tst_grib2
tst_grib2_LDADD = -lnetcdf ${top_builddir}/src/libncgrib2.la


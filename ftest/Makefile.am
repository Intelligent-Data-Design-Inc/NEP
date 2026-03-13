# This file builds the ftest directory for the NCSQUEEZE library. These
# tests depend on netCDF.

# Ed Hartnett 1/23/2020, Charlie Zender 9/20/2020

# Find the include files.
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/fsrc

# Fortran compile flags: pass build options from configure.ac
AM_FCFLAGS = -DBUILD_LZ4=$(BUILD_LZ4_DEF) -DBUILD_BZIP2=$(BUILD_BZIP2_DEF)

# Link to our assembled libraries.
LDADD = ${top_builddir}/fsrc/libnepf.la ${top_builddir}/src/libnep.la -lnetcdff

# Build our test programs.
check_PROGRAMS =

if BUILD_BZIP2
check_PROGRAMS += ftst_nep_bzip2
ftst_nep_bzip2_SOURCES = ftst_nep_bzip2.F90
endif

# Build the LZ4 tests?
if BUILD_LZ4
check_PROGRAMS += ftst_nep_lz4
ftst_nep_lz4_SOURCES = ftst_nep_lz4.F90
endif

# Run our test programs.
TESTS = run_tests.sh

# Delete files created in testing.
CLEANFILES = ftst_*.nc
DISTCLEANFILES = run_test.sh

AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

# Distrubute this file.
EXTRA_DIST = run_tests.sh.in

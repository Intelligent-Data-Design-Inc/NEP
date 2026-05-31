# Makefile.am for parallel I/O examples
# Only built when --enable-parallel-tests is specified.
# Edward Hartnett, Intelligent Data Design, Inc.
# v1.9.0 Sprint 3

AUTOMAKE_OPTIONS = parallel-tests

if HAVE_PARALLEL_TESTS

AM_CPPFLAGS = -I$(top_srcdir)/include

# C parallel example
check_PROGRAMS = square16_par

square16_par_SOURCES = square16_par.c
square16_par_LDADD =

if ENABLE_FORTRAN
check_PROGRAMS += f_square16_par

f_square16_par_SOURCES = f_square16_par.f90
f_square16_par_LDADD =
endif

# Test via the mpiexec-driven run script
TESTS = run_par_examples.sh

noinst_SCRIPTS = run_par_examples.sh

EXTRA_DIST = run_par_examples.sh.in

# Set LD_LIBRARY_PATH so test binaries can find HDF5/NetCDF shared libs at runtime
AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

check-local:
	chmod +x run_par_examples.sh

CLEANFILES = run_par_examples.sh

endif # HAVE_PARALLEL_TESTS

# Makefile.am for NcZarr examples
# Edward Hartnett, Intelligent Data Design, Inc.

AUTOMAKE_OPTIONS = parallel-tests

check_PROGRAMS = nczarr_simple

nczarr_simple_SOURCES = nczarr_simple.c

LDADD = -lnetcdf -lhdf5_hl -lhdf5

AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

TESTS = nczarr_simple test_nczarr_simple.sh

EXTRA_DIST = test_nczarr_simple.sh.in

# Ensure built binaries are executable before test scripts run them
check-local:
	chmod +x $(check_PROGRAMS)

# NcZarr produces a directory tree, not a single file
clean-local:
	rm -rf simple_nczarr.zarr

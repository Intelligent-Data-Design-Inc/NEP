# Makefile.am for NcZarr examples
# Edward Hartnett, Intelligent Data Design, Inc.
# 2026-06-26

AUTOMAKE_OPTIONS = parallel-tests

check_PROGRAMS = nczarr_simple nczarr_chunking nczarr_compression

nczarr_simple_SOURCES = nczarr_simple.c
nczarr_chunking_SOURCES = nczarr_chunking.c
nczarr_compression_SOURCES = nczarr_compression.c

LDADD = -lnetcdf -lhdf5_hl -lhdf5

AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

TESTS = nczarr_simple nczarr_chunking nczarr_compression test_nczarr_simple.sh

EXTRA_DIST = test_nczarr_simple.sh.in

# Fortran NcZarr example — only built when Fortran is enabled (v1.11.0 sprint 2)
if ENABLE_FORTRAN
AM_FCFLAGS = $(CPPFLAGS)

check_PROGRAMS += f_nczarr_simple f_nczarr_chunking f_nczarr_compression
f_nczarr_simple_SOURCES = f_nczarr_simple.f90
f_nczarr_simple_LDADD = -lnetcdff
f_nczarr_chunking_SOURCES = f_nczarr_chunking.f90
f_nczarr_chunking_LDADD = -lnetcdff
f_nczarr_compression_SOURCES = f_nczarr_compression.f90
f_nczarr_compression_LDADD = -lnetcdff

TESTS += f_nczarr_chunking f_nczarr_compression test_f_nczarr_simple.sh
EXTRA_DIST += test_f_nczarr_simple.sh.in
endif

# Ensure built binaries are executable before test scripts run them
check-local:
	chmod +x $(check_PROGRAMS)

# NcZarr produces directory trees, not single files
clean-local:
	rm -rf simple_nczarr.zarr f_simple_nczarr.zarr \
	       nczarr_chunking.zarr nczarr_compression.zarr \
	       f_nczarr_chunking.zarr f_nczarr_compression.zarr

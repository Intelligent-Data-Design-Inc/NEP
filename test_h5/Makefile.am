# This file builds the test_h5 directory for the ncsqueeze library. This
# directory has tests that depend only on HDF5, not on netCDF.

# Ed Hartnett 9/9/25

# Find the header files.
AM_CPPFLAGS = -I$(top_srcdir)/include

# Build the tests.
check_PROGRAMS =

# Build the LZ4 tests?
if BUILD_LZ4
check_PROGRAMS += tst_h_lz4 tst_lz4_size
endif

# Build the BZIP2 tests?
if BUILD_BZIP2
check_PROGRAMS += tst_h_bzip2
endif

# Run the tests. This script is built by configure from
# run_tests.sh.in.
TESTS = run_tests.sh

# Delete files created in testing.
CLEANFILES = tst_*.h5

EXTRA_DIST = run_tests.sh.in


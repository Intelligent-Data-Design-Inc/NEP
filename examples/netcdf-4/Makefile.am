# Makefile.am for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# List of NetCDF-4 example programs
check_PROGRAMS = simple_nc4 compression chunking_performance multi_unlimited user_types groups

# Source files
simple_nc4_SOURCES = simple_nc4.c
compression_SOURCES = compression.c
chunking_performance_SOURCES = chunking_performance.c
multi_unlimited_SOURCES = multi_unlimited.c
user_types_SOURCES = user_types.c
groups_SOURCES = groups.c

# Link math library for examples that use math functions
compression_LDADD = -lm
multi_unlimited_LDADD = -lm
user_types_LDADD = -lm

# All examples run as tests
# Note: user_types excluded from TESTS - it's a demonstration of upstream NetCDF-4
# features (compound, vlen, enum, opaque types) that triggers ASAN errors in NetCDF-C
# Note: groups excluded from TESTS - it's run by test_groups.sh to avoid parallel conflicts
TESTS = simple_nc4 compression chunking_performance multi_unlimited test_groups.sh

# Include test scripts in distribution
EXTRA_DIST = test_groups.sh

# Clean up NetCDF files created by examples
CLEANFILES = *.nc

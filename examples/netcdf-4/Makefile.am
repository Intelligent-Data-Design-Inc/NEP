# Makefile.am for NetCDF-4 C examples
# Edward Hartnett 1/25/26

# List of NetCDF-4 example programs
check_PROGRAMS = simple_nc4 format_variants compression chunking_performance multi_unlimited user_types groups dump_nc4_metadata

# Source files
simple_nc4_SOURCES = simple_nc4.c
format_variants_SOURCES = format_variants.c
compression_SOURCES = compression.c
chunking_performance_SOURCES = chunking_performance.c
multi_unlimited_SOURCES = multi_unlimited.c
user_types_SOURCES = user_types.c
groups_SOURCES = groups.c
dump_nc4_metadata_SOURCES = dump_nc4_metadata.c

# Link against netcdf and hdf5 (hdf5 needed to resolve symbols in libnetcdf.so)
LDADD = -lnetcdf -lhdf5_hl -lhdf5

# Link math library for examples that use math functions
# (per-target LDADD overrides global LDADD, so repeat base libs)
compression_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm
multi_unlimited_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm
user_types_LDADD = -lnetcdf -lhdf5_hl -lhdf5 -lm

# Set library path for tests so shared libs (netcdf, hdf5) are found at runtime
AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

# All examples run as tests
# Note: user_types excluded from TESTS - it's a demonstration of upstream NetCDF-4
# features (compound, vlen, enum, opaque types) that triggers ASAN errors in NetCDF-C
# Note: groups excluded from TESTS - it's run by test_groups.sh to avoid parallel conflicts
TESTS = simple_nc4 test_format_variants.sh compression chunking_performance multi_unlimited test_groups.sh \
        test_dump_nc4_metadata.sh

# Include test scripts in distribution
EXTRA_DIST = test_format_variants.sh test_groups.sh test_dump_nc4_metadata.sh

# Ensure built binaries are executable before test scripts run them
check-local:
	chmod +x $(check_PROGRAMS)

# Clean up NetCDF files created by examples
CLEANFILES = *.nc

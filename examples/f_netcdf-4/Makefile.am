# Makefile.am for NetCDF-4 Fortran examples
# Edward Hartnett 1/25/26

# Fortran module path - NetCDF-Fortran modules should be in CPPFLAGS from configure
AM_FCFLAGS = $(CPPFLAGS)

# List of Fortran NetCDF-4 example programs
check_PROGRAMS = f_simple_nc4 f_format_variants f_compression f_chunking_performance f_multi_unlimited f_user_types f_groups

# Source files
f_simple_nc4_SOURCES = f_simple_nc4.f90
f_format_variants_SOURCES = f_format_variants.f90
f_compression_SOURCES = f_compression.f90
f_chunking_performance_SOURCES = f_chunking_performance.f90
f_multi_unlimited_SOURCES = f_multi_unlimited.f90
f_user_types_SOURCES = f_user_types.f90
f_groups_SOURCES = f_groups.f90

# Link with NetCDF-Fortran library
LDADD = -lnetcdff

# Test wrapper scripts for CDL validation
TESTS = test_f_simple_nc4.sh test_f_format_variants.sh test_f_compression.sh test_f_chunking_performance.sh \
        test_f_multi_unlimited.sh test_f_user_types.sh test_f_groups.sh

# Include test scripts in distribution
EXTRA_DIST = test_f_simple_nc4.sh test_f_format_variants.sh test_f_compression.sh test_f_chunking_performance.sh \
             test_f_multi_unlimited.sh test_f_user_types.sh test_f_groups.sh

# Clean up NetCDF files and generated CDL files
CLEANFILES = *.nc *.cdl

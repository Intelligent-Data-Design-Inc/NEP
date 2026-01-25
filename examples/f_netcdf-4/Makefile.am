# Makefile.am for NetCDF-4 Fortran examples
# Edward Hartnett 1/25/26

# Fortran module path - NetCDF-Fortran modules should be in CPPFLAGS from configure
AM_FCFLAGS = $(CPPFLAGS)

# List of Fortran NetCDF-4 example programs
check_PROGRAMS = f_simple_nc4 f_compression f_chunking_performance f_multi_unlimited f_user_types

# Source files
f_simple_nc4_SOURCES = f_simple_nc4.f90
f_compression_SOURCES = f_compression.f90
f_chunking_performance_SOURCES = f_chunking_performance.f90
f_multi_unlimited_SOURCES = f_multi_unlimited.f90
f_user_types_SOURCES = f_user_types.f90

# Link with NetCDF-Fortran library
LDADD = -lnetcdff

# All examples run as tests
TESTS = $(check_PROGRAMS)

# Clean up NetCDF files created by examples
CLEANFILES = *.nc

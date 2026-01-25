# Makefile.am for classic NetCDF Fortran examples
# Edward Hartnett 1/25/26

# Fortran module path - NetCDF-Fortran modules should be in CPPFLAGS from configure
AM_FCFLAGS = $(CPPFLAGS)

# List of Fortran classic example programs
check_PROGRAMS = f_simple_2D f_coord_vars f_format_variants f_size_limits f_unlimited_dim f_var4d

# Source files
f_simple_2D_SOURCES = f_simple_2D.f90
f_coord_vars_SOURCES = f_coord_vars.f90
f_format_variants_SOURCES = f_format_variants.f90
f_size_limits_SOURCES = f_size_limits.f90
f_unlimited_dim_SOURCES = f_unlimited_dim.f90
f_var4d_SOURCES = f_var4d.f90

# Link with NetCDF-Fortran library
LDADD = -lnetcdff

# All examples run as tests
TESTS = $(check_PROGRAMS)

# Clean up NetCDF files created by examples
CLEANFILES = *.nc

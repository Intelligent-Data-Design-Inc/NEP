# Makefile.am for classic NetCDF Fortran examples
# Edward Hartnett 1/25/26

# Fortran module path - NetCDF-Fortran modules should be in CPPFLAGS from configure
AM_FCFLAGS = $(CPPFLAGS)

# List of Fortran classic example programs
check_PROGRAMS = f_quickstart f_simple_2D f_coord f_coord_vars f_size_limits f_unlimited_dim f_var4d

# Source files
f_quickstart_SOURCES = f_quickstart.f90
f_simple_2D_SOURCES = f_simple_2D.f90
f_coord_SOURCES = f_coord.f90
f_coord_vars_SOURCES = f_coord_vars.f90
f_size_limits_SOURCES = f_size_limits.f90
f_unlimited_dim_SOURCES = f_unlimited_dim.f90
f_var4d_SOURCES = f_var4d.f90

# Link with NetCDF-Fortran library
LDADD = -lnetcdff

# Test wrapper scripts for CDL validation
TESTS = test_f_quickstart.sh test_f_simple_2D.sh test_f_coord.sh test_f_coord_vars.sh \
        test_f_size_limits.sh test_f_unlimited_dim.sh test_f_var4d.sh

# Include test scripts in distribution
EXTRA_DIST = test_f_quickstart.sh test_f_simple_2D.sh test_f_coord.sh test_f_coord_vars.sh \
             test_f_size_limits.sh test_f_unlimited_dim.sh test_f_var4d.sh

# Clean up NetCDF files and generated CDL files
CLEANFILES = *.nc *.cdl

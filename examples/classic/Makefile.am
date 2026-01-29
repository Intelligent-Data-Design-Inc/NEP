# Makefile.am for classic NetCDF C examples
# Edward Hartnett 1/25/26

# List of classic example programs
check_PROGRAMS = quickstart simple_2D coord_vars format_variants size_limits unlimited_dim var4d

# Source files
quickstart_SOURCES = quickstart.c
simple_2D_SOURCES = simple_2D.c
coord_vars_SOURCES = coord_vars.c
format_variants_SOURCES = format_variants.c
size_limits_SOURCES = size_limits.c
unlimited_dim_SOURCES = unlimited_dim.c
var4d_SOURCES = var4d.c

# All examples run as tests
TESTS = $(check_PROGRAMS)

# Clean up NetCDF files created by examples
CLEANFILES = *.nc

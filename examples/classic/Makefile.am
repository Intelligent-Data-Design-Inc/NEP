# Makefile.am for classic NetCDF C examples
# Edward Hartnett 1/25/26

AUTOMAKE_OPTIONS = serial-tests

# List of classic example programs
check_PROGRAMS = quickstart simple_2D coord coord_vars size_limits unlimited_dim var4d dump_classic_metadata

# Source files
quickstart_SOURCES = quickstart.c
simple_2D_SOURCES = simple_2D.c
coord_SOURCES = coord.c
coord_vars_SOURCES = coord_vars.c
size_limits_SOURCES = size_limits.c
unlimited_dim_SOURCES = unlimited_dim.c
var4d_SOURCES = var4d.c
dump_classic_metadata_SOURCES = dump_classic_metadata.c

# Link against netcdf and hdf5 (hdf5 needed to resolve symbols in libnetcdf.so)
LDADD = -lnetcdf -lhdf5_hl -lhdf5

# Set library path for tests so shared libs (netcdf, hdf5) are found at runtime
AM_TESTS_ENVIRONMENT = LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH"

# Test wrapper scripts for CDL validation
TESTS = test_quickstart.sh test_coord.sh test_dump_classic_metadata.sh

# Include test scripts in distribution
EXTRA_DIST = test_quickstart.sh test_coord.sh test_dump_classic_metadata.sh

# Ensure built binaries are executable before test scripts run them
check-local:
	chmod +x $(check_PROGRAMS)

# Clean up NetCDF files and generated CDL files
CLEANFILES = *.nc *.cdl

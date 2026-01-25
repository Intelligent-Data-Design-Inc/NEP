# Makefile.am for NEP examples
# Edward Hartnett 1/25/26

# C examples are always built when BUILD_EXAMPLES is enabled
SUBDIRS = classic netcdf-4

# Fortran examples only built when Fortran is enabled
if ENABLE_FORTRAN
SUBDIRS += f_classic f_netcdf-4
endif

# Placeholder directories for future examples
# SUBDIRS += parallelIO performance

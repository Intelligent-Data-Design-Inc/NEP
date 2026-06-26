# Makefile.am for OPeNDAP examples

# C Example Programs
opendap_simple_SOURCES = opendap_simple.c
opendap_constraint_SOURCES = opendap_constraint.c
opendap_subset_SOURCES = opendap_subset.c

# Fortran Example Programs (if Fortran is enabled)
if ENABLE_FORTRAN
f_opendap_simple_SOURCES = f_opendap_simple.f90
f_opendap_simple_LDADD = $(FCLIBS) -lnetcdff

f_opendap_constraint_SOURCES = f_opendap_constraint.f90
f_opendap_constraint_LDADD = $(FCLIBS) -lnetcdff

f_opendap_subset_SOURCES = f_opendap_subset.f90
f_opendap_subset_LDADD = $(FCLIBS) -lnetcdff
endif

# Build rules
noinst_PROGRAMS = opendap_simple opendap_constraint opendap_subset

if ENABLE_FORTRAN
noinst_PROGRAMS += f_opendap_simple f_opendap_constraint f_opendap_subset
endif

LDADD = -lnetcdf
AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = $(CFLAGS)

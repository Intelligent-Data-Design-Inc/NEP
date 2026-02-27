# This file builds the fsrc directory for the NCSQUEEZE library.

# Ed Hartnett 1/23/2020

# Find the header files.
AM_CPPFLAGS = -I$(top_srcdir)/include

# Fortran compile flags: pass build options from configure.ac
AM_FCFLAGS = -DBUILD_LZ4=$(BUILD_LZ4_DEF) -DBUILD_BZIP2=$(BUILD_BZIP2_DEF)

#AM_LDFLAGS = ${top_builddir}/hdf5_plugins/BZIP2/src/libh5bz2.la ${top_builddir}/hdf5_plugins/LZ4/src/libh5lz4.la${top_builddir}/hdf5_plugins/LZ4/src/libh5lz4.la ${top_builddir}/hdf5_plugins/LZ4/src/libh5bgr.la ${top_builddir}/hdf5_plugins/LZ4/src/libh5gbr.la ${top_builddir}/hdf5_plugins/LZ4/src/libh5zstd.la
LIBADD = ${top_builddir}/src/libnep.la

# This is a libtool library.
lib_LTLIBRARIES = libnepf.la
libnepf_la_LDFLAGS = -version-info 1:1:0

# Each convenience library depends on its source.
libnepf_la_SOURCES = nep.F90

# Each mod file depends on the .o file.
nep.mod: nep.$(OBJEXT)

# Mod files are built and then installed as headers.
MODFILES = nep.mod
include_HEADERS = $(MODFILES)
BUILT_SOURCES = $(MODFILES)

CLEANFILES = *.mod

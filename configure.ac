# This is the configure.ac file for the NEP project.
# Edward Hartnett 8/27/25

AC_PREREQ([2.71])
AC_INIT([NEP],[0.1.1],[help@intelligentdatadesign.com])
AC_CONFIG_SRCDIR([src/grib2_vol_connector.c])
AC_CONFIG_HEADERS([config.h])

# Dump all the weird scripts in bin/
AC_CONFIG_AUX_DIR([bin])

# Script directory
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

MY_HDF5_PLUGIN_PATH=${HDF5_PLUGIN_PATH-.}
AC_MSG_CHECKING([where to put HDF5 plugins])
AC_ARG_WITH([hdf5-plugin-path],
            [AS_HELP_STRING([--with-hdf5-plugin-path=<directory>],
                            [specify HDF5 plugin path (defaults to /usr/local/hdf5/lib/plugin, or value of HDF5_PLUGIN_PATH, if set)])],
            [MY_HDF5_PLUGIN_PATH=$with_hdf5_plugin_path])
AC_MSG_RESULT($MY_HDF5_PLUGIN_PATH)
AC_DEFINE([MY_HDF5_PLUGIN_PATH], [$MY_HDF5_PLUGIN_PATH], [HDF5 plugin path])
AC_SUBST([MY_HDF5_PLUGIN_PATH], [$MY_HDF5_PLUGIN_PATH])

# Does the user want BZIP2?
AC_MSG_CHECKING([whether BZIP2 filter library should be built and installed])
AC_ARG_ENABLE([bzip2],
              [AS_HELP_STRING([--disable-bzip2],
                              [Disable the build and install of bzip2 filter library.])])
test "x$enable_bzip2" = xno || enable_bzip2=yes
AC_MSG_RESULT($enable_bzip2)
AM_CONDITIONAL(BUILD_BZIP2, [test "x$enable_bzip2" = xyes])
if test "x$enable_bzip2" = xyes; then
   AC_DEFINE([BUILD_BZIP2], 1, [If true, build with bzip2 filter.])
fi
AC_SUBST([BUILD_BZIP2], [$enable_bzip2])

# Does the user want LZ4?
AC_MSG_CHECKING([whether LZ4 filter library should be built and installed])
AC_ARG_ENABLE([lz4],
              [AS_HELP_STRING([--disable-lz4],
                              [Disable the build and install of lz4 filter library.])])
test "x$enable_lz4" = xno || enable_lz4=yes
AC_MSG_RESULT($enable_lz4)
AM_CONDITIONAL(BUILD_LZ4, [test "x$enable_lz4" = xyes])
if test "x$enable_lz4" = xyes; then
   AC_DEFINE([BUILD_LZ4], 1, [If true, build with lz4 filter.])
fi
AC_SUBST([BUILD_LZ4], [$enable_lz4])

# Does the user want JPEG?
AC_MSG_CHECKING([whether JPEG filter library should be built and installed])
AC_ARG_ENABLE([jpeg],
              [AS_HELP_STRING([--enable-jpeg],
                              [Enable the build and install of jpeg filter library.])])
test "x$enable_jpeg" = xyes || enable_jpeg=no
AC_MSG_RESULT($enable_jpeg)
AM_CONDITIONAL(BUILD_JPEG, [test "x$enable_jpeg" = xyes])
if test "x$enable_jpeg" = xyes; then
   AC_DEFINE([BUILD_JPEG], 1, [If true, build with jpeg filter.])
fi

# Does the user want LZF?
AC_MSG_CHECKING([whether LZF filter library should be built and installed])
AC_ARG_ENABLE([lzf],
              [AS_HELP_STRING([--disable-lzf],
                              [Disable the build and install of lzf filter library.])])
test "x$enable_lzf" = xno || enable_lzf=yes
AC_MSG_RESULT($enable_lzf)
AM_CONDITIONAL(BUILD_LZF, [test "x$enable_lzf" = xyes])
if test "x$enable_lzf" = xyes; then
   AC_DEFINE([BUILD_LZF], 1, [If true, build with lzf filter.])
fi
AC_SUBST([BUILD_LZF], [$enable_lzf])

dnl if test "x$enable_lz4" = xyes; then
dnl    AC_CONFIG_SUBDIRS([LZ4])
dnl fi

# VOL connector enable/disable options
AC_ARG_ENABLE([grib2],
    AS_HELP_STRING([--enable-grib2], [Enable GRIB2 VOL connector (default: yes)]),
    [enable_grib2=$enableval],
    [enable_grib2=yes])

AC_ARG_ENABLE([docs],
    AS_HELP_STRING([--enable-docs], [Enable documentation generation with Doxygen (default: yes if Doxygen found)]),
    [enable_docs=$enableval],
    [enable_docs=auto])

# Initialize variables
ENABLED_VOLS=""
HAVE_GRIB2=no
HAVE_BUFR=no
HAVE_GEOTIFF=no
HAVE_CDF=no
HAVE_DOXYGEN=no

# Check for GRIB2 dependencies if enabled
if test "x$enable_grib2" = "xyes"; then
    AC_CHECK_HEADERS([grib2.h], [grib2_header=yes], [grib2_header=no])
    AC_SEARCH_LIBS([g2c_inq], [g2c], [G2C_LIBS="-lg2c"; grib2_lib=yes], [grib2_lib=no], [-ljasper])
    
    if test "x$grib2_header" = "xyes" && test "x$grib2_lib" = "xyes"; then
        HAVE_GRIB2=yes
        ENABLED_VOLS="$ENABLED_VOLS GRIB2"
        AC_DEFINE([HAVE_GRIB2], [1], [Define if GRIB2 VOL connector is enabled])
        AC_MSG_NOTICE([GRIB2 VOL connector enabled])
    else
        AC_MSG_ERROR([GRIB2 VOL connector enabled but NCEPLIBS-g2c not found. Install NCEPLIBS-g2c or disable with --disable-grib2])
    fi
fi
AC_SUBST([G2C_LIBS])

# Check for Doxygen if documentation is enabled
if test "x$enable_docs" != "xno"; then
    AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])
    if test "x$DOXYGEN" = "x"; then
        if test "x$enable_docs" = "xyes"; then
            AC_MSG_ERROR([Documentation enabled but doxygen not found. Install doxygen or disable with --disable-docs])
        else
            AC_MSG_WARN([Doxygen not found - documentation will not be built])
            enable_docs=no
        fi
    else
        HAVE_DOXYGEN=yes
        enable_docs=yes
        AC_MSG_NOTICE([Documentation generation enabled with doxygen])
    fi
fi

# Check for netCDF Fortran library.
if test "x$enable_fortran" = xyes; then
   AC_LANG_PUSH(Fortran)
   AC_SEARCH_LIBS([nf_create], [netcdff], [], [AC_MSG_ERROR([Can't find or link to the netCDF Fortran library, set CPPFLAGS/LDFLAGS.])])
   AC_LANG_POP(Fortran)
fi

# Set up conditionals for Makefile.am
AM_CONDITIONAL([HAVE_GRIB2], [test "x$HAVE_GRIB2" = "xyes"])
AM_CONDITIONAL([HAVE_BUFR], [test "x$HAVE_BUFR" = "xyes"])
AM_CONDITIONAL([HAVE_GEOTIFF], [test "x$HAVE_GEOTIFF" = "xyes"])
AM_CONDITIONAL([HAVE_CDF], [test "x$HAVE_CDF" = "xyes"])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$HAVE_DOXYGEN" = "xyes"])

AC_MSG_NOTICE([Enabled VOL connectors:$ENABLED_VOLS])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

# Set prefix default (install directory) to a directory in the build area. 
AC_PREFIX_DEFAULT([`pwd`/plugin]) 

# Initialize Libtool
LT_INIT([shared disable-static])

# Check for netCDF C library.
AC_SEARCH_LIBS([nc_create], [netcdf], [],
                            [AC_MSG_ERROR([Can't find or link to the netCDF C library, set CPPFLAGS/LDFLAGS.])])
AC_CHECK_HEADERS([netcdf.h netcdf_meta.h])

# Do we have netCDF-4? If not, we're done.
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
[[#if !NC_HAS_HDF5
# error
#endif]
])], [have_netcdf4=yes], [have_netcdf4=no])
AC_MSG_CHECKING([whether netCDF provides netCDF/HDF5])
AC_MSG_RESULT([${have_netcdf4}])
if test "x$have_netcdf4" = xno; then
   AC_MSG_ERROR([NetCDF-4 is required])
fi

# Is this version 4.7.2, which does not work?
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
[[#if NC_VERSION_MAJOR == 4 && NC_VERSION_MINOR == 7 && NC_VERSION_PATCH == 2
#else
# error
#endif]
])], [have_472=yes], [have_472=no])
AC_MSG_CHECKING([whether this is netcdf-c-4.7.2])
AC_MSG_RESULT([${have_472}])
if test "x$have_472" = xyes; then
   AC_MSG_ERROR([cannot build with netcdf-c-4.7.2, please upgrade your netCDF version.])
fi

dnl # Do we have a parallel build of netCDF-4? (Really we should be
dnl # checking NC_HAS_PARALLEL4, but that was only recently introduced, so
dnl # we will go with NC_HAS_PARALLEL.)
dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
dnl [[#if !NC_HAS_PARALLEL
dnl # error
dnl #endif]
dnl ])], [have_netcdf_par=yes], [have_netcdf_par=no])
dnl AC_MSG_CHECKING([whether netCDF provides parallel I/O for netCDF/HDF5])
dnl AC_MSG_RESULT([${have_netcdf_par}])
dnl AC_SUBST(HAS_NETCDF_PAR,[$have_netcdf_par])
dnl AM_CONDITIONAL(HAVE_NETCDF_PAR, [test "x$have_netcdf_par" = xyes])

dnl # If user asked for parallel tests, and parallel is not available, error.
dnl if test "x$enable_parallel_tests" = xyes -a "x$have_netcdf_par" != xyes; then
dnl    AC_MSG_ERROR([parallel tests requested, but netcdf-c does not provide parallel I/O.])
dnl fi

# Do we have szip?
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
[[#if !NC_HAS_SZIP_WRITE
# error
#endif]
])], [have_szip_write=yes], [have_szip_write=no])
AC_MSG_CHECKING([whether netCDF provides szip write capability])
AC_MSG_RESULT([${have_szip_write}])

dnl # Do we have parallel filter support?
dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
dnl [[#if !NC_HAS_PAR_FILTERS
dnl # error
dnl #endif]
dnl ])], [have_par_filters=yes], [have_par_filters=no])
dnl AC_MSG_CHECKING([whether netCDF provides parallel filter support])
dnl AC_MSG_RESULT([${have_par_filters}])
dnl if test "x$have_par_filters" = xyes ; then
dnl    AC_DEFINE([HAVE_PAR_FILTERS], [1], [if true, netCDF-C supports filters with parallel I/O])
dnl fi
dnl AC_SUBST(HAS_PAR_FILTERS,[$have_par_filters])

# Does netcdf-c support zstd?
AC_MSG_CHECKING([if netCDF handles zstd])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[
#include <netcdf_meta.h>
#if !defined(NC_HAS_ZSTD) || NC_HAS_ZSTD == 0
      choke me
#endif]])], [have_zstd=yes], [have_zstd=no])
AC_MSG_RESULT([$have_zstd])
if test "x$have_zstd" = xno; then
   AC_MSG_ERROR([netcdf-c must be built with zstd and have version > 4.9.0.])
fi

# Find HDF5. This macro (from gnu.org) adds a --with-hdf5 option for setting
# a path to any repository.
AX_LIB_HDF5()
# If you want a particular build of the HDF5 library, you can use these
# instead.
# AX_LIB_HDF5([serial])
# AX_LIB_HDF5([parallel])
if test "$with_hdf5" = "no"; then
    AC_MSG_ERROR([Unable to find HDF5])
fi

# We need our own, fixed, copy of the LZ4 plugin.
if test "x$enable_lz4" = xyes; then
   AC_CONFIG_SUBDIRS([hdf5_plugins])
fi

AC_CONFIG_FILES([test/test_vol_plugin.sh], [chmod +x test/test_vol_plugin.sh])
AC_CONFIG_FILES([test/run_tests.sh], [chmod ugo+x test/run_tests.sh])
#AC_CONFIG_FILES([test/run_par_tests.sh], [chmod ugo+x test/run_par_tests.sh])
#AC_CONFIG_FILES([test/run_benchmarks.sh], [chmod ugo+x test/run_benchmarks.sh])
AC_CONFIG_FILES([ftest/run_tests.sh], [chmod ugo+x ftest/run_tests.sh])
AC_CONFIG_FILES([test_h5/run_tests.sh], [chmod ugo+x test_h5/run_tests.sh])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 docs/Doxyfile])

AC_OUTPUT

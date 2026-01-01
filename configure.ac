# This is the configure.ac file for the NEP project.
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

AC_PREREQ([2.71])
AC_INIT([NEP],[1.0.0],[help@intelligentdatadesign.com])
AC_CONFIG_SRCDIR([src/ncsqueeze.c])
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

# We are using HDF5.
AC_DEFINE([USE_HDF5], [yes], [Always needed])

MY_HDF5_PLUGIN_PATH=${HDF5_PLUGIN_PATH-.}
AC_MSG_CHECKING([where to put HDF5 plugins])
AC_ARG_WITH([hdf5-plugin-path],
            [AS_HELP_STRING([--with-hdf5-plugin-path=<directory>],
                            [specify HDF5 plugin path (defaults to /usr/local/hdf5/lib/plugin, or value of HDF5_PLUGIN_PATH, if set)])],
            [MY_HDF5_PLUGIN_PATH=$with_hdf5_plugin_path])
AC_MSG_RESULT($MY_HDF5_PLUGIN_PATH)
AC_DEFINE([MY_HDF5_PLUGIN_PATH], [$MY_HDF5_PLUGIN_PATH], [HDF5 plugin path])
AC_SUBST([MY_HDF5_PLUGIN_PATH], [$MY_HDF5_PLUGIN_PATH])

# Does the user want to enable netcdf-4 logging?
AC_MSG_CHECKING([whether logging is enabled])
AC_ARG_ENABLE([logging],
              [AS_HELP_STRING([--enable-logging],
                              [enable logging capability (only applies when netCDF-4 is built). \
			      This debugging features is only of interest to netCDF developers. \
			      Ignored if netCDF-4 is not enabled.])])
test "x$enable_logging" = xyes || enable_logging=no
AC_MSG_RESULT([$enable_logging])
if test "x$enable_logging" = xyes; then
   enable_set_log_level_func=yes
   enable_set_log_level=yes
   AC_DEFINE([NETCDF_ENABLE_SET_LOG_LEVEL], 1, [If true, enable nc_set_log_level function.])
else
   enable_set_log_level_func=no
   enable_set_log_level=no
fi
if test "x$enable_logging" = xyes; then
   AC_DEFINE([LOGGING], 1, [If true, turn on logging.])
   AC_DEFINE([GRIB2_LOGGING], 1, [If true, turn on logging.])
fi
AM_CONDITIONAL(USE_LOGGING, [test "x$enable_logging" = xyes])
AC_SUBST(HAS_LOGGING, [$enable_logging])

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

# Numeric build flags for Fortran compilation
AS_IF([test "x$enable_lz4" = "xyes"],  [BUILD_LZ4_DEF=1], [BUILD_LZ4_DEF=0])
AS_IF([test "x$enable_bzip2" = "xyes"], [BUILD_BZIP2_DEF=1], [BUILD_BZIP2_DEF=0])

AC_SUBST([BUILD_LZ4_DEF])
AC_SUBST([BUILD_BZIP2_DEF])

dnl if test "x$enable_lz4" = xyes; then
dnl    AC_CONFIG_SUBDIRS([LZ4])
dnl fi

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# GRIB2 UDF enable/disable option
# AC_ARG_ENABLE([grib2],
#     AS_HELP_STRING([--enable-grib2], [Enable GRIB2 UDF support (default: yes)]),
#     [enable_grib2=$enableval],
#     [enable_grib2=yes])

AC_ARG_ENABLE([docs],
    AS_HELP_STRING([--enable-docs], [Enable documentation generation with Doxygen (default: yes if Doxygen found)]),
    [enable_docs=$enableval],
    [enable_docs=auto])

# CDF library detection (v1.3.0) - library detection only, no UDF yet
AC_MSG_CHECKING([whether CDF library detection should be enabled])
AC_ARG_ENABLE([cdf],
              [AS_HELP_STRING([--enable-cdf],
                              [Enable CDF library detection (default: no)])])
test "x$enable_cdf" = xyes || enable_cdf=no
AC_MSG_RESULT([$enable_cdf])

# GeoTIFF library detection (v1.5.0) - library detection only, no UDF yet
AC_MSG_CHECKING([whether GeoTIFF library detection should be enabled])
AC_ARG_ENABLE([geotiff],
              [AS_HELP_STRING([--enable-geotiff],
                              [Enable GeoTIFF library detection (default: no)])])
test "x$enable_geotiff" = xyes || enable_geotiff=no
AC_MSG_RESULT([$enable_geotiff])

# Initialize variables
HAVE_GRIB2=no
HAVE_CDF=no
HAVE_GEOTIFF=no
HAVE_DOXYGEN=no

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# Check for GRIB2 dependencies if enabled
# if test "x$enable_grib2" = "xyes"; then
#     AC_CHECK_HEADERS([grib2.h], [grib2_header=yes], [grib2_header=no])
#     AC_SEARCH_LIBS([g2c_inq], [g2c], [G2C_LIBS="-lg2c"; grib2_lib=yes], [grib2_lib=no], [-ljasper])
#     
#     if test "x$grib2_header" = "xyes" && test "x$grib2_lib" = "xyes"; then
#         HAVE_GRIB2=yes
#         AC_DEFINE([HAVE_GRIB2], [1], [Define if GRIB2 UDF support is enabled])
#         AC_MSG_NOTICE([GRIB2 UDF support enabled])
#     else
#         AC_MSG_ERROR([GRIB2 UDF enabled but NCEPLIBS-g2c not found. Install NCEPLIBS-g2c or disable with --disable-grib2])
#     fi
# fi
# AC_SUBST([G2C_LIBS])

# CDF library detection (v1.3.0) - library detection only, no UDF implementation yet
if test "x$enable_cdf" = "xyes"; then
    AC_CHECK_HEADERS([cdf.h], [cdf_header=yes], [cdf_header=no])
    AC_CHECK_LIB([cdf], [CDFlib], [cdf_lib=yes; CDF_LIBS="-lcdf"], [cdf_lib=no])
    
    if test "x$cdf_header" = "xyes" && test "x$cdf_lib" = "xyes"; then
        HAVE_CDF=yes
        AC_DEFINE([HAVE_CDF], [1], [Define if CDF library is found])
        
        # Extract library directory from LDFLAGS for rpath
        # Use -R for libtool compatibility
        for flag in $LDFLAGS; do
            case $flag in
                -L*)
                    libdir=`echo $flag | sed 's/^-L//'`
                    # Check if this is the CDF library directory
                    if test -f "$libdir/libcdf.so" || test -f "$libdir/libcdf.a"; then
                        CDF_LDFLAGS="-R$libdir"
                        break
                    fi
                    ;;
            esac
        done
        
        AC_MSG_NOTICE([CDF library detection enabled (library location only, no UDF yet)])
    else
        AC_MSG_ERROR([CDF enabled but NASA CDF library not found. Install NASA CDF library or disable with --disable-cdf])
    fi
fi
AC_SUBST([CDF_LIBS])
AC_SUBST([CDF_LDFLAGS])

# GeoTIFF library detection (v1.5.0) - library detection only, no UDF implementation yet
if test "x$enable_geotiff" = "xyes"; then
    AC_CHECK_HEADERS([geotiff/geotiff.h], [geotiff_header=yes], [geotiff_header=no])
    AC_CHECK_HEADERS([tiff.h], [tiff_header=yes], [tiff_header=no])
    AC_CHECK_LIB([tiff], [TIFFOpen], [tiff_lib=yes; TIFF_LIBS="-ltiff"], [tiff_lib=no])
    AC_CHECK_LIB([geotiff], [GTIFNew], [geotiff_lib=yes; GEOTIFF_LIBS="-lgeotiff $TIFF_LIBS"], [geotiff_lib=no], [-ltiff])
    
    if test "x$geotiff_header" = "xyes" && test "x$tiff_header" = "xyes" && test "x$geotiff_lib" = "xyes" && test "x$tiff_lib" = "xyes"; then
        HAVE_GEOTIFF=yes
        AC_DEFINE([HAVE_GEOTIFF], [1], [Define if GeoTIFF library is found])
        AC_DEFINE([HAVE_GEOTIFF_H], [1], [Define if geotiff.h header is found])
        AC_DEFINE([HAVE_TIFF_H], [1], [Define if tiff.h header is found])
        AC_MSG_NOTICE([GeoTIFF library detection enabled (library location only, no UDF yet)])
    else
        AC_MSG_ERROR([GeoTIFF enabled but libgeotiff or libtiff not found. Install libgeotiff and libtiff or disable with --disable-geotiff])
    fi
fi
AC_SUBST([GEOTIFF_LIBS])
AC_SUBST([TIFF_LIBS])

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

# Fortran wrapper support (v1.1.0)
AC_MSG_CHECKING([whether Fortran wrappers should be built])
AC_ARG_ENABLE([fortran],
              [AS_HELP_STRING([--enable-fortran],
                              [Enable Fortran wrappers and tests (default: yes)])])
test "x$enable_fortran" = xno || enable_fortran=yes
AC_MSG_RESULT([$enable_fortran])
AM_CONDITIONAL([ENABLE_FORTRAN], [test "x$enable_fortran" = xyes])
AC_SUBST([ENABLE_FORTRAN], [$enable_fortran])

# Check for netCDF Fortran library when Fortran support is enabled.
if test "x$enable_fortran" = xyes; then
   AC_LANG_PUSH(Fortran)
   AC_SEARCH_LIBS([nf_create], [netcdff], [], [AC_MSG_ERROR([Can't find or link to the netCDF Fortran library, set CPPFLAGS/LDFLAGS.])])
   AC_LANG_POP(Fortran)
   
   # Add -Wall and -Werror for GNU Fortran compiler
   AC_PROG_FC
   if test "x$ac_cv_fc_compiler_gnu" = xyes; then
      FCFLAGS="$FCFLAGS -Wall -Werror"
      AC_MSG_NOTICE([GNU Fortran compiler detected: Adding -Wall -Werror flags])
   fi
fi

# Set up conditionals for Makefile.am
# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# AM_CONDITIONAL([HAVE_GRIB2], [test "x$HAVE_GRIB2" = "xyes"])
AM_CONDITIONAL([HAVE_CDF], [test "x$HAVE_CDF" = "xyes"])
AM_CONDITIONAL([HAVE_GEOTIFF], [test "x$HAVE_GEOTIFF" = "xyes"])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$HAVE_DOXYGEN" = "xyes"])

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

# Do we have szip?
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "netcdf_meta.h"],
[[#if !NC_HAS_SZIP_WRITE
# error
#endif]
])], [have_szip_write=yes], [have_szip_write=no])
AC_MSG_CHECKING([whether netCDF provides szip write capability])
AC_MSG_RESULT([${have_szip_write}])

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
# Only use AX_LIB_HDF5 if --with-hdf5 is explicitly specified
if test "x$with_hdf5" != "x"; then
    AX_LIB_HDF5()
    # If you want a particular build of the HDF5 library, you can use these
    # instead.
    # AX_LIB_HDF5([serial])
    # AX_LIB_HDF5([parallel])
    if test "$with_hdf5" = "no"; then
        AC_MSG_ERROR([Unable to find HDF5])
    fi
else
    # If HDF5 paths provided via CPPFLAGS/LDFLAGS, skip h5cc requirement
    AC_MSG_NOTICE([Skipping HDF5 autodetection - using provided CPPFLAGS/LDFLAGS])
fi

# We need our own, fixed, copy of the LZ4 plugin.
if test "x$enable_lz4" = xyes; then
   AC_CONFIG_SUBDIRS([hdf5_plugins])
fi

AC_CONFIG_FILES([test/run_tests.sh], [chmod ugo+x test/run_tests.sh])
AC_CONFIG_FILES([ftest/run_tests.sh], [chmod ugo+x ftest/run_tests.sh])
AC_CONFIG_FILES([test_h5/run_tests.sh], [chmod ugo+x test_h5/run_tests.sh])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 test/Makefile
                 fsrc/Makefile
                 ftest/Makefile
                 test_grib2/Makefile
                 test_cdf/Makefile
                 test_geotiff/Makefile
                 docs/Doxyfile])

AC_OUTPUT

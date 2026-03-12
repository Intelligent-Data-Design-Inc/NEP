# This is the configure.ac file for the NEP project.
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

AC_PREREQ([2.71])
AC_INIT([NEP],[1.0.0],[help@intelligentdatadesign.com])
AC_CONFIG_SRCDIR([src/nep.c])
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

AC_ARG_WITH([hdf5-plugin-path],
            [AS_HELP_STRING([--with-hdf5-plugin-path=<directory>],
                            [specify HDF5 plugin path (defaults to <netcdf-prefix>/hdf5/lib/plugin)])])

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

AC_ARG_ENABLE([docs],
    AS_HELP_STRING([--enable-docs], [Enable documentation generation with Doxygen (default: yes if Doxygen found)]),
    [enable_docs=$enableval],
    [enable_docs=auto])

# CDF library detection (v1.3.0) - default OFF; enable with --enable-cdf
# CDF and GRIB2 share UDF slot 2 and are mutually exclusive.
AC_MSG_CHECKING([whether CDF library detection should be enabled])
AC_ARG_ENABLE([cdf],
              [AS_HELP_STRING([--enable-cdf],
                              [Enable CDF library detection; mutually exclusive with --enable-grib2 (default: disabled)])])
test "x$enable_cdf" = xyes || enable_cdf=no
AC_MSG_RESULT([$enable_cdf])

# GeoTIFF library detection (v1.5.0) - default ON; disable with --disable-geotiff
AC_MSG_CHECKING([whether GeoTIFF library detection should be enabled])
AC_ARG_ENABLE([geotiff],
              [AS_HELP_STRING([--disable-geotiff],
                              [Disable GeoTIFF library detection (default: enabled)])])
test "x$enable_geotiff" = xno || enable_geotiff=yes
AC_MSG_RESULT([$enable_geotiff])

# Ensure PKG_CONFIG is set before any pkg-config queries below.
PKG_PROG_PKG_CONFIG

# Initialize variables
HAVE_CDF=no
HAVE_GEOTIFF=no
HAVE_GRIB2=no
HAVE_DOXYGEN=no

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
                    _ld_dir=`echo $flag | sed 's/^-L//'`
                    # Check if this is the CDF library directory
                    if test -f "$_ld_dir/libcdf.so" || test -f "$_ld_dir/libcdf.a"; then
                        CDF_LDFLAGS="-R$_ld_dir"
                        break
                    fi
                    ;;
            esac
        done
        
        AC_MSG_NOTICE([CDF library detection enabled (library location only, no UDF yet)])
    else
        AC_MSG_ERROR([libcdf is required for the CDF dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use --disable-cdf])
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
        AC_MSG_ERROR([libgeotiff is required for the GeoTIFF dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use --disable-geotiff])
    fi
fi
AC_SUBST([GEOTIFF_LIBS])
AC_SUBST([TIFF_LIBS])

# GRIB2 library detection (v1.7.0) - default ON; disable with --disable-grib2
# GRIB2 and CDF share UDF slot 2 and are mutually exclusive.
AC_MSG_CHECKING([whether GRIB2 library detection should be enabled])
AC_ARG_ENABLE([grib2],
              [AS_HELP_STRING([--disable-grib2],
                              [Disable GRIB2 library detection; mutually exclusive with --enable-cdf (default: enabled)])])
test "x$enable_grib2" = xno || enable_grib2=yes
AC_MSG_RESULT([$enable_grib2])

# Enforce mutual exclusion: GRIB2 and CDF both use UDF slot 2.
if test "x$enable_grib2" = xyes && test "x$enable_cdf" = xyes; then
    AC_MSG_ERROR([--enable-grib2 and --enable-cdf are mutually exclusive: both use UDF slot 2. Build with one or the other.])
fi

if test "x$enable_grib2" = "xyes"; then
    AC_CHECK_HEADERS([grib2.h], [g2c_header=yes], [g2c_header=no])
    AC_SEARCH_LIBS([g2c_open], [g2c], [g2c_lib=yes; G2C_LIBS="-lg2c"], [g2c_lib=no])

    if test "x$g2c_header" = "xyes" && test "x$g2c_lib" = "xyes"; then
        HAVE_GRIB2=yes
        AC_DEFINE([HAVE_GRIB2], [1], [Define if GRIB2 (g2c) library is found])
        G2C_LIBS="-lg2c -ljasper"
        # Embed rpaths so libncgrib2.so finds libg2c and libjasper at
        # runtime (dlopen) without requiring LD_LIBRARY_PATH.
        for flag in $LDFLAGS; do
            case $flag in
                -L*)
                    _ld_dir=`echo $flag | sed 's/^-L//'`
                    if test -f "$_ld_dir/libg2c.so" || test -f "$_ld_dir/libg2c.a"; then
                        G2C_LDFLAGS="$G2C_LDFLAGS -R$_ld_dir"
                    fi
                    if test -f "$_ld_dir/libjasper.so" || test -f "$_ld_dir/libjasper.a"; then
                        G2C_LDFLAGS="$G2C_LDFLAGS -R$_ld_dir"
                    fi
                    ;;
            esac
        done
        # Also try pkg-config to locate jasper (does not modify LIBS/LDFLAGS).
        # Add both -L (build-time) and -R (runtime rpath) for jasper.
        if test "x$PKG_CONFIG" != "x"; then
            _jasper_libdir=`$PKG_CONFIG --variable=libdir jasper 2>/dev/null`
            if test "x$_jasper_libdir" != "x"; then
                G2C_LDFLAGS="$G2C_LDFLAGS -L$_jasper_libdir -R$_jasper_libdir"
            fi
        fi
        AC_MSG_NOTICE([GRIB2 library detection enabled])
    else
        AC_MSG_ERROR([libg2c is required for the GRIB2 dispatch layer. Include the library path in CPPFLAGS/LDFLAGS, or use --disable-grib2])
    fi
fi
AC_SUBST([G2C_LIBS])
AC_SUBST([G2C_LDFLAGS])

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

# Examples support (v1.5.1)
AC_MSG_CHECKING([whether example programs should be built])
AC_ARG_ENABLE([examples],
              [AS_HELP_STRING([--enable-examples],
                              [Enable example programs (default: yes)])])
test "x$enable_examples" = xno || enable_examples=yes
AC_MSG_RESULT([$enable_examples])
AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$enable_examples" = xyes])
AC_SUBST([BUILD_EXAMPLES], [$enable_examples])

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
AM_CONDITIONAL([HAVE_CDF], [test "x$HAVE_CDF" = "xyes"])
AM_CONDITIONAL([HAVE_GEOTIFF], [test "x$HAVE_GEOTIFF" = "xyes"])
AM_CONDITIONAL([HAVE_GRIB2], [test "x$HAVE_GRIB2" = "xyes"])
AM_CONDITIONAL([BUILD_NCRC], [test "x$HAVE_GEOTIFF" = "xyes" || test "x$HAVE_CDF" = "xyes" || test "x$HAVE_GRIB2" = "xyes"])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$HAVE_DOXYGEN" = "xyes"])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

# Set prefix default (install directory) to a directory in the build area. 
AC_PREFIX_DEFAULT([`pwd`/plugin]) 

# Initialize Libtool
LT_INIT([shared disable-static])

# Check for netCDF C library (requires 4.10.0+ for UDF self-loading support).
AC_SEARCH_LIBS([nc_create], [netcdf], [],
                            [AC_MSG_ERROR([Can't find or link to the netCDF C library, set CPPFLAGS/LDFLAGS.])])
AC_CHECK_HEADERS([netcdf.h netcdf_meta.h])

# Verify netcdf-c >= 4.10.0 via pkg-config
# Auto-discover netcdf.pc by scanning LDFLAGS for the directory containing libnetcdf
for flag in $LDFLAGS; do
    case $flag in
        -L*)
            _ncdir=`echo $flag | sed 's/^-L//'`
            if test -f "$_ncdir/libnetcdf.so" || test -f "$_ncdir/libnetcdf.a" || test -f "$_ncdir/libnetcdf.dylib"; then
                _ncpc="$_ncdir/pkgconfig"
                if test -f "$_ncpc/netcdf.pc"; then
                    PKG_CONFIG_PATH="${_ncpc}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
                    export PKG_CONFIG_PATH
                    break
                fi
            fi
            ;;
    esac
done
PKG_CHECK_MODULES([NETCDF], [netcdf >= 4.10.0], [],
    [AC_MSG_ERROR([netcdf-c >= 4.10.0 is required. Found older version or netcdf not found via pkg-config.])])


# Find ncdump from the NetCDF-C installation
# Extract the library directory from LDFLAGS to find the bin directory
AC_MSG_CHECKING([for ncdump from NetCDF-C])
NCDUMP=""

# Extract -L paths from LDFLAGS to find NetCDF-C lib directory
for flag in $LDFLAGS; do
    case $flag in
        -L*)
            _ld_dir=`echo $flag | sed 's/^-L//'`
            # Check if this directory contains libnetcdf
            if test -f "$_ld_dir/libnetcdf.so" || test -f "$_ld_dir/libnetcdf.a" || test -f "$_ld_dir/libnetcdf.dylib"; then
                # Derive bin directory from lib directory (typically ../bin)
                NETCDF_PREFIX=`dirname "$_ld_dir"`
                bindir="${NETCDF_PREFIX}/bin"
                if test -x "$bindir/ncdump"; then
                    NCDUMP="$bindir/ncdump"
                    break
                fi
            fi
            ;;
    esac
done

# Fallback: try nc-config if we didn't find it via LDFLAGS
if test "x$NCDUMP" = "x"; then
    AC_CHECK_PROG([NC_CONFIG], [nc-config], [nc-config])
    if test "x$NC_CONFIG" != "x"; then
        NETCDF_PREFIX=`$NC_CONFIG --prefix`
        if test -x "${NETCDF_PREFIX}/bin/ncdump"; then
            NCDUMP="${NETCDF_PREFIX}/bin/ncdump"
        fi
    fi
fi

# Last resort: try to find ncdump in PATH
if test "x$NCDUMP" = "x"; then
    AC_PATH_PROG([NCDUMP], [ncdump])
fi

if test "x$NCDUMP" = "x"; then
    AC_MSG_ERROR([ncdump not found. Please ensure NetCDF-C tools are installed and in PATH.])
fi
AC_MSG_RESULT([$NCDUMP])
AC_SUBST([NCDUMP])

# Set HDF5 plugin path now that we know NETCDF_PREFIX.
if test "x$with_hdf5_plugin_path" != "x"; then
    MY_HDF5_PLUGIN_PATH=$with_hdf5_plugin_path
else
    MY_HDF5_PLUGIN_PATH="${NETCDF_PREFIX}/hdf5/lib/plugin"
fi
AC_MSG_CHECKING([where to find HDF5 plugins])
AC_MSG_RESULT([$MY_HDF5_PLUGIN_PATH])
AC_DEFINE([MY_HDF5_PLUGIN_PATH], [$MY_HDF5_PLUGIN_PATH], [HDF5 plugin path])
AC_SUBST([MY_HDF5_PLUGIN_PATH])

# Build MY_LD_LIBRARY_PATH from -L flags in LDFLAGS for use in run_tests.sh.
# Prepend the local src/.libs so the freshly built libnep takes priority
# over any system-installed version.
MY_LD_LIBRARY_PATH="${ac_pwd}/src/.libs"
for flag in $LDFLAGS; do
    case $flag in
        -L*)
            _ld_dir=`echo $flag | sed 's/^-L//'`
            MY_LD_LIBRARY_PATH="$MY_LD_LIBRARY_PATH:$_ld_dir"
            ;;
    esac
done
AC_MSG_CHECKING([runtime library path for tests])
AC_MSG_RESULT([$MY_LD_LIBRARY_PATH])
AC_SUBST([MY_LD_LIBRARY_PATH])

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
   MY_LZ4_PLUGIN_PATH="${ac_pwd}/hdf5_plugins/LZ4/src/.libs"
else
   MY_LZ4_PLUGIN_PATH=""
fi
AC_SUBST([MY_LZ4_PLUGIN_PATH])

AC_ARG_WITH([ncrc-dir],
    [AS_HELP_STRING([--with-ncrc-dir=DIR],
        [install .ncrc to DIR (default: ${datarootdir})])],
    [NEP_NCRC_DIR="$withval"],
    [NEP_NCRC_DIR="\${datarootdir}"])
AC_SUBST([NEP_NCRC_DIR])

if test "x$HAVE_GEOTIFF" = "xyes" || test "x$HAVE_CDF" = "xyes" || test "x$HAVE_GRIB2" = "xyes"; then
    # Expand libdir now so AC_CONFIG_COMMANDS receives a concrete path
    nep_expanded_prefix=$prefix
    test "x$nep_expanded_prefix" = xNONE && nep_expanded_prefix=$ac_default_prefix
    nep_expanded_exec_prefix=$exec_prefix
    test "x$nep_expanded_exec_prefix" = xNONE && nep_expanded_exec_prefix=$nep_expanded_prefix
    nep_expanded_libdir=$nep_expanded_exec_prefix/lib
    AC_CONFIG_COMMANDS([.ncrc],
        [
            cat "${srcdir}/.ncrc.in" > .ncrc
            if test "x$nep_have_geotiff" = "xyes"; then
                sed "s|@NEP_LIBDIR@|${nep_libdir}|g" "${srcdir}/.ncrc-geotiff.in" >> .ncrc
            fi
            if test "x$nep_have_cdf" = "xyes"; then
                sed "s|@NEP_LIBDIR@|${nep_libdir}|g" "${srcdir}/.ncrc-cdf.in" >> .ncrc
            fi
            if test "x$nep_have_grib2" = "xyes"; then
                sed "s|@NEP_LIBDIR@|${nep_libdir}|g" "${srcdir}/.ncrc-grib2.in" >> .ncrc
            fi
        ],
        [
            nep_have_geotiff="$HAVE_GEOTIFF"
            nep_have_cdf="$HAVE_CDF"
            nep_have_grib2="$HAVE_GRIB2"
            nep_libdir="$nep_expanded_libdir"
        ])
fi

AC_CONFIG_FILES([test/run_tests.sh], [chmod ugo+x test/run_tests.sh])
AC_CONFIG_FILES([ftest/run_tests.sh], [chmod ugo+x ftest/run_tests.sh])
AC_CONFIG_FILES([test_h5/run_tests.sh], [chmod ugo+x test_h5/run_tests.sh])
AC_CONFIG_FILES([examples/validate_cdl.sh], [chmod ugo+x examples/validate_cdl.sh])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 test/Makefile
                 fsrc/Makefile
                 ftest/Makefile
                 test_h5/Makefile
                 test_cdf/Makefile
                 test_geotiff/Makefile
                 examples/Makefile
                 examples/classic/Makefile
                 examples/netcdf-4/Makefile
                 examples/f_classic/Makefile
                 examples/f_netcdf-4/Makefile
                 docs/Doxyfile])

AC_OUTPUT

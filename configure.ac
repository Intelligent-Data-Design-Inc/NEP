# This is the configure.ac file for the NEP project.
# Edward Hartnett 8/27/25

AC_PREREQ([2.71])
AC_INIT([NEP],[1.0.0],[help@intelligentdatadesign.com])
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

# VOL connector enable/disable options
AC_ARG_ENABLE([grib2],
    AS_HELP_STRING([--enable-grib2], [Enable GRIB2 VOL connector (default: yes)]),
    [enable_grib2=$enableval],
    [enable_grib2=yes])

AC_ARG_ENABLE([bufr],
    AS_HELP_STRING([--enable-bufr], [Enable BUFR VOL connector (default: yes)]),
    [enable_bufr=$enableval],
    [enable_bufr=yes])

AC_ARG_ENABLE([geotiff],
    AS_HELP_STRING([--enable-geotiff], [Enable GeoTIFF VOL connector (default: yes)]),
    [enable_geotiff=$enableval],
    [enable_geotiff=yes])

AC_ARG_ENABLE([cdf],
    AS_HELP_STRING([--enable-cdf], [Enable CDF VOL connector (default: yes)]),
    [enable_cdf=$enableval],
    [enable_cdf=yes])

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

dnl # Check for BUFR dependencies if enabled
dnl if test "x$enable_bufr" = "xyes"; then
dnl     AC_CHECK_HEADERS([bufrlib.h], [bufr_header=yes], [bufr_header=no])
dnl     AC_SEARCH_LIBS([openbf_], [bufr_4], [BUFR_LIBS="-lbufr_4"; bufr_lib=yes], [bufr_lib=no], [-lgfortran -lm])
    
dnl     if test "x$bufr_header" = "xyes" && test "x$bufr_lib" = "xyes"; then
dnl         HAVE_BUFR=yes
dnl         ENABLED_VOLS="$ENABLED_VOLS BUFR"
dnl         AC_DEFINE([HAVE_BUFR], [1], [Define if BUFR VOL connector is enabled])
dnl         AC_MSG_NOTICE([BUFR VOL connector enabled])
dnl     else
dnl         AC_MSG_ERROR([BUFR VOL connector enabled but NCEPLIBS-bufr not found. Install NCEPLIBS-bufr or disable with --disable-bufr])
dnl     fi
dnl fi
dnl AC_SUBST([BUFR_LIBS])

dnl # Check for GeoTIFF dependencies if enabled
dnl if test "x$enable_geotiff" = "xyes"; then
dnl     AC_CHECK_HEADERS([geotiff.h], [geotiff_header=yes], [geotiff_header=no])
dnl     AC_SEARCH_LIBS([GTIFNew], [geotiff], [GEOTIFF_LIBS="-lgeotiff"; geotiff_lib=yes], [geotiff_lib=no])
    
dnl     if test "x$geotiff_header" = "xyes" && test "x$geotiff_lib" = "xyes"; then
dnl         HAVE_GEOTIFF=yes
dnl         ENABLED_VOLS="$ENABLED_VOLS GeoTIFF"
dnl         AC_DEFINE([HAVE_GEOTIFF], [1], [Define if GeoTIFF VOL connector is enabled])
dnl         AC_MSG_NOTICE([GeoTIFF VOL connector enabled])
dnl     else
dnl         AC_MSG_ERROR([GeoTIFF VOL connector enabled but libgeotiff not found. Install libgeotiff or disable with --disable-geotiff])
dnl     fi
dnl fi
dnl AC_SUBST([GEOTIFF_LIBS])

dnl # Check for CDF dependencies if enabled
dnl if test "x$enable_cdf" = "xyes"; then
dnl     AC_CHECK_HEADERS([cdf.h], [cdf_header=yes], [cdf_header=no])
dnl     AC_SEARCH_LIBS([CDFope], [cdf], [CDF_LIBS="-lcdf"; cdf_lib=yes], [cdf_lib=no])
    
dnl     if test "x$cdf_header" = "xyes" && test "x$cdf_lib" = "xyes"; then
dnl         HAVE_CDF=yes
dnl         ENABLED_VOLS="$ENABLED_VOLS CDF"
dnl         AC_DEFINE([HAVE_CDF], [1], [Define if CDF VOL connector is enabled])
dnl         AC_MSG_NOTICE([CDF VOL connector enabled])
dnl     else
dnl         AC_MSG_ERROR([CDF VOL connector enabled but NASA CDF library not found. Install NASA CDF library or disable with --disable-cdf])
dnl     fi
dnl fi
dnl AC_SUBST([CDF_LIBS])

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

# Check that at least one VOL connector is enabled
if test "x$HAVE_GRIB2" = "xno" && test "x$HAVE_BUFR" = "xno" && test "x$HAVE_GEOTIFF" = "xno" && test "x$HAVE_CDF" = "xno"; then
    AC_MSG_ERROR([No VOL connectors enabled. Enable at least one with --enable-<vol>])
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

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 docs/Doxyfile])

AC_CONFIG_FILES([test/test_vol_plugin.sh],
                [chmod +x test/test_vol_plugin.sh])

AC_OUTPUT

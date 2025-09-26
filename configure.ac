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

# If the env. variable HDF5_PLUGIN_PATH is set, or if
# --with-hdf5-plugin-dir=<directory>, use it.
AC_MSG_CHECKING([where to put HDF5 plugins])
AC_ARG_WITH([hdf5-plugin-path],
            [AS_HELP_STRING([--with-hdf5-plugin-path=<directory>],
                            [specify HDF5 plugin directory (defaults to /usr/local/hdf5/lib/plugin, or value of HDF5_PLUGIN_PATH, if set)])],
            [HDF5_PLUGIN_PATH=$with_hdf5_plugin_path])
HDF5_PLUGIN_PATH=${HDF5_PLUGIN_PATH-.}
AC_MSG_RESULT($HDF5_PLUGIN_PATH)

# Does the user want BZIP2?
AC_MSG_CHECKING([whether BZIP2 filter library should be built and installed])
AC_ARG_ENABLE([bzip2],
              [AS_HELP_STRING([--disable-bzip2],
                              [Disable the build and install of bzip2 filter library.])])
test "x$enable_bzip2" = xno || enable_bzip2=yes
AC_MSG_RESULT($enable_bzip2)
AM_CONDITIONAL(BUILD_BZIP2, [test "x$enable_bzip2" = xyes])

# Does the user want LZ4?
AC_MSG_CHECKING([whether LZ4 filter library should be built and installed])
AC_ARG_ENABLE([lz4],
              [AS_HELP_STRING([--disable-lz4],
                              [Disable the build and install of lz4 filter library.])])
test "x$enable_lz4" = xno || enable_lz4=yes
AC_MSG_RESULT($enable_lz4)
AM_CONDITIONAL(BUILD_LZ4, [test "x$enable_lz4" = xyes])

# Does the user want JPEG?
AC_MSG_CHECKING([whether JPEG filter library should be built and installed])
AC_ARG_ENABLE([jpeg],
              [AS_HELP_STRING([--disable-jpeg],
                              [Disable the build and install of jpeg filter library.])])
test "x$enable_jpeg" = xno || enable_jpeg=yes
AC_MSG_RESULT($enable_jpeg)
AM_CONDITIONAL(BUILD_JPEG, [test "x$enable_jpeg" = xyes])

# Does the user want LZF?
AC_MSG_CHECKING([whether LZF filter library should be built and installed])
AC_ARG_ENABLE([lzf],
              [AS_HELP_STRING([--disable-lzf],
                              [Disable the build and install of lzf filter library.])])
test "x$enable_lzf" = xno || enable_lzf=yes
AC_MSG_RESULT($enable_lzf)
AM_CONDITIONAL(BUILD_LZF, [test "x$enable_lzf" = xyes])

# Build the filter libraries as desired.
if test "x$enable_bzip2" = xyes; then
   AC_CONFIG_SUBDIRS([BZIP2])
fi
if test "x$enable_lz4" = xyes; then
   AC_CONFIG_SUBDIRS([LZ4])
fi
if test "x$enable_jpeg" = xyes; then
   AC_CONFIG_SUBDIRS([JPEG])
fi
if test "x$enable_lzf" = xyes; then
   AC_CONFIG_SUBDIRS([LZF])
fi

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

# We need our own, fixed, copy of the LZ4 plugin.
if test "x$enable_lz4" = xyes; then
   AC_CONFIG_SUBDIRS([hdf5_plugins])
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 docs/Doxyfile])

AC_CONFIG_FILES([test/test_vol_plugin.sh],
                [chmod +x test/test_vol_plugin.sh])

AC_OUTPUT

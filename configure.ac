## Process this file with autoconf to produce configure.
##

AC_PREREQ([2.69])
AC_INIT([TEMPLATE VOL], [1.0.0], [help@hdfgroup.org])
AC_CONFIG_SRCDIR([src/template_vol_connector.c])
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
                 test/Makefile])

AC_CONFIG_FILES([test/test_vol_plugin.sh],
                [chmod +x test/test_vol_plugin.sh])

AC_OUTPUT

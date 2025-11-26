# This is the Makefile.am file for the root directory of the NFEP project.
#
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4

# GRIB2 UDF Handler - Disabled for v1.0.0 (LZ4 compression release only)
# Will be re-enabled in v2.0.0 for GRIB2 UDF support
# If GRIB2 layer has been built, test it.
# if HAVE_GRIB2
# TEST_GRIB2 = test_grib2
# endif

# CDF test - v1.3.0 Sprint 2: CDF library integration test
if HAVE_CDF
TEST_CDF = test_cdf
endif

if ENABLE_FORTRAN
SUBDIRS = include src fsrc $(TEST_CDF) test ftest
else
SUBDIRS = include src $(TEST_CDF) test
endif
EXTRA_DIST = autogen.sh

# Documentation target
if HAVE_DOXYGEN
docs:
	$(DOXYGEN) docs/Doxyfile

clean-local:
	rm -rf docs/html

.PHONY: docs
else
docs:
	@echo "Doxygen not found. Documentation cannot be built."
	@echo "Install doxygen or configure with --disable-docs"
	@exit 1

.PHONY: docs
endif

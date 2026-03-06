# This is the Makefile.am file for the root directory of the NFEP project.
#
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4

# CDF test - v1.3.0 Sprint 2: CDF library integration test
if HAVE_CDF
TEST_CDF = test_cdf
endif

# GeoTIFF test - v1.5.0: GeoTIFF format detection test
if HAVE_GEOTIFF
TEST_GEOTIFF = test_geotiff
endif

# Examples subdirectory (v1.5.1)
if BUILD_EXAMPLES
EXAMPLES_DIR = examples
endif

if ENABLE_FORTRAN
SUBDIRS = include src fsrc $(TEST_CDF) $(TEST_GEOTIFF) test test_h5 ftest $(EXAMPLES_DIR)
else
SUBDIRS = include src $(TEST_CDF) $(TEST_GEOTIFF) test test_h5 $(EXAMPLES_DIR)
endif

if BUILD_LZ4
all-local:
	$(MAKE) -C hdf5_plugins all
endif
EXTRA_DIST = autogen.sh .ncrc.in .ncrc-geotiff.in .ncrc-cdf.in

if BUILD_NCRC
# Install .ncrc to $(NEP_NCRC_DIR) (default: $(datarootdir)/nep)
# .ncrc is assembled by configure (AC_CONFIG_COMMANDS) with the correct install libdir
ncrcdir = $(NEP_NCRC_DIR)
ncrc_DATA = .ncrc
endif

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

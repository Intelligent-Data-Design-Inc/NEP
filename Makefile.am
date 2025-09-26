# This is the Makefile.am file for the root directory of the NFEP project.
#
# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src test
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

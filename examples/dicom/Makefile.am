# Makefile.am for DICOM C example
# Edward Hartnett, Intelligent Data Design, Inc.

if HAVE_DICOM

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

dicom_read_SOURCES = dicom_read.c
dicom_read_LDADD = $(top_builddir)/src/libncdicom.la \
                   $(top_builddir)/src/libnep.la \
                   $(NETCDF_LIBS) $(HDF5_LIBS) -lm
check_PROGRAMS = dicom_read

AM_TESTS_ENVIRONMENT = NETCDF_RC=$(top_builddir); export NETCDF_RC; \
                       LD_LIBRARY_PATH="$(top_builddir)/src/.libs:$(LD_LIBRARY_PATH)"; export LD_LIBRARY_PATH;

check-local:
	@for p in $(check_PROGRAMS); do \
	    echo "Running $$p..."; \
	    $(AM_TESTS_ENVIRONMENT) ./$$p$(EXEEXT) $(top_srcdir)/test/data/DICOM/tst_dicom_uncompressed.dcm || exit 1; \
	done

distclean-local:
	rm -f dicom_read$(EXEEXT)

endif

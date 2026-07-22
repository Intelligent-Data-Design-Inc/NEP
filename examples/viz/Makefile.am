AUTOMAKE_OPTIONS = parallel-tests

TESTS = test_plot_common.py
if HAVE_FITS
TESTS += test_udf_open.py
endif

LOG_COMPILER = $(PYTHON3)
AM_TESTS_ENVIRONMENT = NCRCENV_RC="$(abs_top_builddir)/.ncrc" \
	NETCDF_RC="$(abs_top_builddir)" \
	LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH" \
	MPLCONFIGDIR="$(abs_builddir)/.matplotlib" \
	NEP_VIZ_UDF_FILE="$(abs_top_builddir)/test/data/WFPC2u5780205r_c0fx.fits"

EXTRA_DIST = README.md plot_common.py test_plot_common.py test_udf_open.py

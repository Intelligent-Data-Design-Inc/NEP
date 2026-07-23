AUTOMAKE_OPTIONS = parallel-tests

TESTS = test_plot_common.py
if HAVE_FITS
TESTS += test_udf_open.py plot_fits_image.py
endif
if HAVE_CDF
TESTS += plot_cdf_var.py
endif
if HAVE_GEOTIFF
TESTS += plot_geotiff_subset.py
endif
if HAVE_GRIB2
TESTS += plot_grib2_grid.py
endif

LOG_COMPILER = $(PYTHON3)
AM_TESTS_ENVIRONMENT = NCRCENV_RC="$(abs_top_builddir)/.ncrc" \
	NETCDF_RC="$(abs_top_builddir)" \
	LD_LIBRARY_PATH="@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH" \
	MPLCONFIGDIR="$(abs_builddir)/.matplotlib" \
	NEP_VIZ_UDF_FILE="$(abs_top_builddir)/test/data/WFPC2u5780205r_c0fx.fits" \
	NEP_VIZ_FITS_FILE="$(abs_top_builddir)/test/data/WFPC2u5780205r_c0fx.fits" \
	NEP_VIZ_CDF_FILE="$(abs_top_builddir)/test/data/tst_cdf_simple.cdf" \
	NEP_VIZ_GEOTIFF_FILE="$(abs_top_builddir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif" \
	NEP_VIZ_GRIB2_FILE="$(abs_top_builddir)/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2"

viz_test_data =
if HAVE_FITS
viz_test_data += $(abs_top_builddir)/test/data/WFPC2u5780205r_c0fx.fits
endif
if HAVE_CDF
viz_test_data += $(abs_top_builddir)/test/data/tst_cdf_simple.cdf
endif
if HAVE_GEOTIFF
viz_test_data += $(abs_top_builddir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif
endif
if HAVE_GRIB2
viz_test_data += $(abs_top_builddir)/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2
endif

$(abs_top_builddir)/test/data/WFPC2u5780205r_c0fx.fits: $(abs_top_srcdir)/test/data/WFPC2u5780205r_c0fx.fits
	$(MKDIR_P) $(abs_top_builddir)/test/data
	cp $< $@

$(abs_top_builddir)/test/data/tst_cdf_simple.cdf: $(abs_top_srcdir)/test/data/tst_cdf_simple.cdf
	$(MKDIR_P) $(abs_top_builddir)/test/data
	cp $< $@

$(abs_top_builddir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif: $(abs_top_srcdir)/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif
	$(MKDIR_P) $(abs_top_builddir)/test/data
	cp $< $@

$(abs_top_builddir)/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2: $(abs_top_srcdir)/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2
	$(MKDIR_P) $(abs_top_builddir)/test/data
	cp $< $@

$(TESTS): $(viz_test_data)

EXTRA_DIST = README.md plot_common.py test_plot_common.py test_udf_open.py plot_fits_image.py plot_cdf_var.py plot_geotiff_subset.py plot_grib2_grid.py

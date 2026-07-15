# This is the Makefile.am file for the test directory of the NEP project.

# Edward Hartnett 8/27/25
# Copyright Intelligent Data Design, Inc. All rights reserved.

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir) -I$(top_srcdir)
AM_LDFLAGS = ${top_builddir}/src/libnep.la
LDADD = ${top_builddir}/src/libnep.la
check_PROGRAMS =
TESTS =

# Adjust as needed
AM_CPPFLAGS += $(HDF5_CPPFLAGS)
AM_CFLAGS = -g -Wall -Wextra $(HDF5_CFLAGS)

# check_SCRIPTS = test_vol_plugin.sh

# Build GRIB2 UDF placeholder test, if GRIB2 is enabled.
if HAVE_GRIB2
check_PROGRAMS += tst_grib2_udf
tst_grib2_udf_LDADD = ${top_builddir}/src/libncgrib2.la ${top_builddir}/src/libnep.la
TESTS += tst_grib2_udf
endif

# Build LZ4 tests, if needed.
if BUILD_LZ4
check_PROGRAMS += tst_lz4
tst_lz4_LDADD = ${top_builddir}/src/libnep.la
endif

# Build BZIP2 tests, if needed.
if BUILD_BZIP2
check_PROGRAMS += tst_bzip2
endif

# Build the performance tests, if all filters have been built.
check_PROGRAMS += tst_perf
tst_perf_SOURCES = tst_perf.c tst_utils.c
tst_perf_LDADD = ${top_builddir}/src/libnep.la -lnetcdf -lhdf5_hl -lhdf5 -lz -ldl -lm

# Build CDF UDF tests, if CDF is enabled.
if HAVE_CDF
check_PROGRAMS += tst_cdf_udf tst_imap_mag
tst_cdf_udf_LDFLAGS = $(CDF_LDFLAGS)
tst_cdf_udf_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libnep.la -lcdf
tst_imap_mag_LDFLAGS = $(CDF_LDFLAGS)
tst_imap_mag_LDADD = ${top_builddir}/src/libnccdf.la ${top_builddir}/src/libnep.la -lcdf
TESTS += tst_cdf_udf tst_imap_mag
endif

# Build FITS UDF test, if FITS is enabled.
if HAVE_FITS
check_PROGRAMS += tst_fits_udf
tst_fits_udf_LDADD = ${top_builddir}/src/libncfits.la ${top_builddir}/src/libnep.la
TESTS += tst_fits_udf
endif

# Build PDS4 UDF test, if PDS4 is enabled.
if HAVE_PDS4
check_PROGRAMS += tst_pds4_udf
tst_pds4_udf_CFLAGS = $(XML2_CFLAGS)
tst_pds4_udf_LDADD = ${top_builddir}/src/libncpds4.la ${top_builddir}/src/libnep.la
check-TESTS: check-local
TESTS += tst_pds4_udf
endif

# Copy test data to build directory for out-of-tree builds.
check-local:
	if test "$(srcdir)" != "$(builddir)" ; then \
		$(MKDIR_P) $(builddir)/data; \
		cp $(srcdir)/data/gdaswave.t00z.wcoast.0p16.f000.grib2 $(builddir)/data/ 2>/dev/null || true; \
		cp $(srcdir)/data/imap_mag_l1b-calibration_20240229_v001.cdf $(builddir)/data/ 2>/dev/null || true; \
		cp $(srcdir)/data/WFPC2u5780205r_c0fx.fits $(builddir)/data/ 2>/dev/null || true; \
		$(MKDIR_P) $(builddir)/data/PDS4; \
		cp $(srcdir)/data/PDS4/test_image.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/test_image.img $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/test_table_binary.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/test_table_binary.dat $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/Table_Character_Example.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/Table_Character_Example.tab $(builddir)/data/PDS4/ 2>/dev/null || true; \
		$(MKDIR_P) $(builddir)/data/PDS4/cassini_hrd; \
		cp $(srcdir)/data/PDS4/cassini_hrd/hrd_2000_on_off.xml $(builddir)/data/PDS4/cassini_hrd/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/cassini_hrd/hrd_2000_on_off.tab $(builddir)/data/PDS4/cassini_hrd/ 2>/dev/null || true; \
		$(MKDIR_P) $(builddir)/data/PDS4/messenger_tnmap; \
		cp $(srcdir)/data/PDS4/messenger_tnmap/thermal_neutron_map.xml $(builddir)/data/PDS4/messenger_tnmap/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/messenger_tnmap/thermal_neutron_map.img $(builddir)/data/PDS4/messenger_tnmap/ 2>/dev/null || true; \
		$(MKDIR_P) $(builddir)/data/PDS4/lcs_9p; \
		cp $(srcdir)/data/PDS4/lcs_9p/20050706_000.xml $(builddir)/data/PDS4/lcs_9p/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/lcs_9p/20050706_000.tab $(builddir)/data/PDS4/lcs_9p/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.csv $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits $(builddir)/data/PDS4/ 2>/dev/null || true; \
		$(MKDIR_P) $(builddir)/data/PDS4/new_horizons; \
		cp $(srcdir)/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx $(builddir)/data/PDS4/new_horizons/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit $(builddir)/data/PDS4/new_horizons/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml $(builddir)/data/PDS4/ 2>/dev/null || true; \
		cp $(srcdir)/data/PDS4/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG $(builddir)/data/PDS4/ 2>/dev/null || true; \
	fi

TESTS_ENVIRONMENT = LD_LIBRARY_PATH=@MY_LD_LIBRARY_PATH@:$$LD_LIBRARY_PATH

# Run all the appropriate tests in this script.
TESTS += run_tests.sh

# Delete files created in testing.
CLEANFILES = tst_*.nc

EXTRA_DIST = run_tests.sh.in data/imap_mag_l1b-calibration_20240229_v001.cdf data/gdaswave.t00z.wcoast.0p16.f000.grib2 data/WFPC2u5780205r_c0fx.fits \
data/PDS4/test_image.xml data/PDS4/test_image.img data/PDS4/test_table_binary.xml data/PDS4/test_table_binary.dat data/PDS4/Table_Character_Example.xml data/PDS4/Table_Character_Example.tab \
data/PDS4/cassini_hrd/hrd_2000_on_off.xml data/PDS4/cassini_hrd/hrd_2000_on_off.tab \
data/PDS4/messenger_tnmap/thermal_neutron_map.xml data/PDS4/messenger_tnmap/thermal_neutron_map.img \
data/PDS4/lcs_9p/20050706_000.xml data/PDS4/lcs_9p/20050706_000.tab \
data/PDS4/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml data/PDS4/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.csv \
data/PDS4/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml data/PDS4/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv \
data/PDS4/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml data/PDS4/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits \
data/PDS4/mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml data/PDS4/mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits \
data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit \
data/PDS4/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml data/PDS4/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG \
data/PDS4/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml data/PDS4/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG

DISTCLEANFILES = run_tests.sh


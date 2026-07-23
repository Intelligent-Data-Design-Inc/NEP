set(_viz_python_files
    plot_common.py
    test_plot_common.py
    test_udf_open.py
    plot_fits_image.py
    plot_cdf_var.py
    plot_geotiff_subset.py
    plot_grib2_grid.py
    plot_pds4_messenger.py
    plot_pds4_perseverance.py
    plot_pds4_maven_l3.py
    plot_pds4_new_horizons.py
)

foreach(_viz_file ${_viz_python_files})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${_viz_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/${_viz_file}"
        COPYONLY
    )
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_BINARY_DIR}/README.md"
    COPYONLY
)

set(_viz_ld_path "${_nep_test_ld_path}")
if(HAVE_GRIB2)
    get_filename_component(_viz_g2c_library_dir "${G2C_LIBRARY}" DIRECTORY)
    get_filename_component(_viz_jasper_library_dir "${JASPER_LIBRARY}" DIRECTORY)
    string(APPEND _viz_ld_path ":${_viz_g2c_library_dir}:${_viz_jasper_library_dir}")
endif()
set(_viz_environment
    "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_viz_ld_path}:$ENV{LD_LIBRARY_PATH};MPLCONFIGDIR=${CMAKE_CURRENT_BINARY_DIR}/.matplotlib"
)

add_test(
    NAME viz_plot_common
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/test_plot_common.py"
)
set_tests_properties(viz_plot_common PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    ENVIRONMENT "${_viz_environment}"
)

if(HAVE_FITS)
    add_test(
        NAME viz_fits_udf_open
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/test_udf_open.py"
                "${CMAKE_BINARY_DIR}/test/data/WFPC2u5780205r_c0fx.fits"
    )
    set_tests_properties(viz_fits_udf_open PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )

    add_test(
        NAME viz_fits_image
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_fits_image.py"
                "${CMAKE_BINARY_DIR}/test/data/WFPC2u5780205r_c0fx.fits"
    )
    set_tests_properties(viz_fits_image PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )
endif()

if(HAVE_CDF)
    add_test(
        NAME viz_cdf_temperature
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_cdf_var.py"
                "${CMAKE_BINARY_DIR}/test/data/tst_cdf_simple.cdf"
    )
    set_tests_properties(viz_cdf_temperature PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )
endif()

if(HAVE_GEOTIFF)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif"
        "${CMAKE_BINARY_DIR}/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif"
        COPYONLY
    )
    add_test(
        NAME viz_geotiff_modis_flood
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_geotiff_subset.py"
                "${CMAKE_BINARY_DIR}/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif"
    )
    set_tests_properties(viz_geotiff_modis_flood PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )
endif()

if(HAVE_GRIB2)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2"
        "${CMAKE_BINARY_DIR}/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2"
        COPYONLY
    )
    add_test(
        NAME viz_grib2_wave
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_grib2_grid.py"
                "${CMAKE_BINARY_DIR}/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2"
    )
    set_tests_properties(viz_grib2_wave PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )
endif()

if(HAVE_PDS4)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data/PDS4/messenger_tnmap")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/messenger_tnmap/thermal_neutron_map.xml"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/messenger_tnmap/thermal_neutron_map.xml"
        COPYONLY
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/messenger_tnmap/thermal_neutron_map.img"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/messenger_tnmap/thermal_neutron_map.img"
        COPYONLY
    )
    add_test(
        NAME viz_pds4_messenger
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_pds4_messenger.py"
                "${CMAKE_BINARY_DIR}/test/data/PDS4/messenger_tnmap/thermal_neutron_map.xml"
    )
    set_tests_properties(viz_pds4_messenger PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data/PDS4/perseverance")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml"
        COPYONLY
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG"
        COPYONLY
    )
    add_test(
        NAME viz_pds4_perseverance
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_pds4_perseverance.py"
                "${CMAKE_BINARY_DIR}/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml"
    )
    set_tests_properties(viz_pds4_perseverance PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data/PDS4/maven")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml"
        COPYONLY
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv"
        COPYONLY
    )
    add_test(
        NAME viz_pds4_maven_l3
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_pds4_maven_l3.py"
                "${CMAKE_BINARY_DIR}/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml"
    )
    set_tests_properties(viz_pds4_maven_l3 PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test/data/PDS4/new_horizons")
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx"
        COPYONLY
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit"
        "${CMAKE_BINARY_DIR}/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit"
        COPYONLY
    )
    add_test(
        NAME viz_pds4_new_horizons
        COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/plot_pds4_new_horizons.py"
                "${CMAKE_BINARY_DIR}/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx"
    )
    set_tests_properties(viz_pds4_new_horizons PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "${_viz_environment}"
    )
endif()

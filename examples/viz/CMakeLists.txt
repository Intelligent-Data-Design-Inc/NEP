set(_viz_python_files
    plot_common.py
    test_plot_common.py
    test_udf_open.py
    plot_fits_image.py
    plot_cdf_var.py
    plot_geotiff_subset.py
    plot_grib2_grid.py
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

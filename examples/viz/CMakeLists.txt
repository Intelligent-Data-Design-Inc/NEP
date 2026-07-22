set(_viz_python_files
    plot_common.py
    test_plot_common.py
    test_udf_open.py
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

set(_viz_environment
    "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH};MPLCONFIGDIR=${CMAKE_CURRENT_BINARY_DIR}/.matplotlib"
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
endif()

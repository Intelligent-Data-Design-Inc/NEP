# CMakeLists.txt for NEXTCDF-4 examples
# Edward Hartnett, Intelligent Data Design, Inc.

foreach(_target bfloat16 bfloat16_perf bfloat16_io_perf make_bfloat16_sample)
    add_executable(${_target} ${_target}.c)
    target_include_directories(${_target} PRIVATE ${CMAKE_SOURCE_DIR}/include ${NETCDF_INCLUDE_DIRS})
    target_link_libraries(${_target} PRIVATE nep ${NETCDF_LIBRARIES} m)
    add_dependencies(${_target} ncnextcdf4)
    set_target_properties(${_target} PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/src;${NEP_HDF5_LIBRARY_DIR};${NETCDF_LIBRARY_DIRS}")
endforeach()

if(BUILD_TESTING)
    if(NEP_ZSTD_PLUGIN_DIR STREQUAL "")
        message(STATUS "NEP_ZSTD_PLUGIN_DIR not set; skipping NEXTCDF-4 bfloat16 example tests")
        return()
    endif()
    if(NOT IS_DIRECTORY "${NEP_ZSTD_PLUGIN_DIR}")
        message(FATAL_ERROR "NEP_ZSTD_PLUGIN_DIR (${NEP_ZSTD_PLUGIN_DIR}) is not a directory")
    endif()

    set(_bf16_sample "${CMAKE_SOURCE_DIR}/test/data/netCDF-4/gdas_sfc_sample.nc")
    set(_bf16_env "HDF5_PLUGIN_PATH=${NEP_ZSTD_PLUGIN_DIR};NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_test(NAME nxt4_bfloat16 COMMAND bfloat16 ${_bf16_sample} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nxt4_bfloat16 PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS nextcdf4)

    if(NEP_ENABLE_BENCHMARKS)
        add_test(NAME nxt4_bfloat16_perf COMMAND bfloat16_perf ${_bf16_sample} ${CMAKE_CURRENT_BINARY_DIR}/bfloat16_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_bfloat16_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)

        add_test(NAME nxt4_bfloat16_io_perf COMMAND bfloat16_io_perf ${_bf16_sample} ${CMAKE_SOURCE_DIR}/test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc ${CMAKE_CURRENT_BINARY_DIR}/bfloat16_io_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_bfloat16_io_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)
    endif()
endif()

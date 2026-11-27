# CMakeLists.txt for NEXTCDF-4 examples
# Edward Hartnett, Intelligent Data Design, Inc.

foreach(_target bfloat16 bfloat16_perf bfloat16_io_perf make_bfloat16_sample float16 float16_perf float16_io_perf make_float16_sample fun_float16 conversion_perf external_refs)
    add_executable(${_target} ${_target}.c)
    target_include_directories(${_target} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/examples ${NETCDF_INCLUDE_DIRS})
    target_link_libraries(${_target} PRIVATE nep ${NETCDF_LIBRARIES} m)
    add_dependencies(${_target} ncnextcdf4)
    set_target_properties(${_target} PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/src;${NEP_HDF5_LIBRARY_DIR};${NETCDF_LIBRARY_DIRS}")
endforeach()
target_include_directories(conversion_perf PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${CMAKE_BINARY_DIR} ${HDF5_INCLUDE_DIRS})
target_link_libraries(external_refs PRIVATE ncnextcdf4 ${NEP_HDF5_LIBRARIES})

# Parallel benchmark: needs MPI and the NEXTCDF-4 parallel build.
if(NEP_ENABLE_NEXTCDF4_PARALLEL)
    add_executable(parallel_perf parallel_perf.c)
    target_include_directories(parallel_perf PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/examples ${NETCDF_INCLUDE_DIRS})
    target_link_libraries(parallel_perf PRIVATE nep ${NETCDF_LIBRARIES} m)
    add_dependencies(parallel_perf ncnextcdf4)
    set_target_properties(parallel_perf PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/src;${NEP_HDF5_LIBRARY_DIR};${NETCDF_LIBRARY_DIRS}")
endif()

if(BUILD_TESTING)
    add_test(NAME nxt4_external_refs COMMAND external_refs WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nxt4_external_refs PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}"
        LABELS "nextcdf4;nextcdf4_ref")

    if(NEP_ENABLE_BENCHMARKS)
        add_test(NAME nxt4_conversion_perf COMMAND conversion_perf ${CMAKE_CURRENT_BINARY_DIR}/conversion_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_conversion_perf PROPERTIES
            ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}"
            LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)

        # Four-rank collective write+read benchmark; no plugin needed,
        # so it lives before the NEP_ZSTD_PLUGIN_DIR early return.
        if(NEP_ENABLE_NEXTCDF4_PARALLEL AND MPIEXEC_EXECUTABLE)
            set(_par_perf_env "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH};OMPI_MCA_rmaps_base_oversubscribe=1")
            add_test(NAME nxt4_parallel_perf COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4 $<TARGET_FILE:parallel_perf> ${CMAKE_CURRENT_BINARY_DIR}/parallel_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            set_tests_properties(nxt4_parallel_perf PROPERTIES ENVIRONMENT "${_par_perf_env}" PROCESSORS 4 LABELS "nextcdf4;nextcdf4_parallel;performance" TIMEOUT 900 RUN_SERIAL TRUE)
        endif()
    endif()
    if(NEP_ZSTD_PLUGIN_DIR STREQUAL "")
        message(STATUS "NEP_ZSTD_PLUGIN_DIR not set; skipping NEXTCDF-4 bfloat16/float16 example tests")
        return()
    endif()
    if(NOT IS_DIRECTORY "${NEP_ZSTD_PLUGIN_DIR}")
        message(FATAL_ERROR "NEP_ZSTD_PLUGIN_DIR (${NEP_ZSTD_PLUGIN_DIR}) is not a directory")
    endif()

    set(_bf16_sample "${CMAKE_SOURCE_DIR}/test/data/netCDF-4/gdas_sfc_sample.nc")
    set(_bf16_env "HDF5_PLUGIN_PATH=${NEP_ZSTD_PLUGIN_DIR};NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_test(NAME nxt4_bfloat16 COMMAND bfloat16 ${_bf16_sample} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nxt4_bfloat16 PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS nextcdf4)

    add_test(NAME nxt4_float16 COMMAND float16 ${_bf16_sample} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nxt4_float16 PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS nextcdf4)

    if(NEP_ENABLE_BENCHMARKS)
        add_test(NAME nxt4_bfloat16_perf COMMAND bfloat16_perf ${_bf16_sample} ${CMAKE_CURRENT_BINARY_DIR}/bfloat16_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_bfloat16_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)

        add_test(NAME nxt4_bfloat16_io_perf COMMAND bfloat16_io_perf ${_bf16_sample} ${CMAKE_SOURCE_DIR}/test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc ${CMAKE_CURRENT_BINARY_DIR}/bfloat16_io_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_bfloat16_io_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)

        add_test(NAME nxt4_float16_perf COMMAND float16_perf ${_bf16_sample} ${CMAKE_CURRENT_BINARY_DIR}/float16_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_float16_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)

        add_test(NAME nxt4_float16_io_perf COMMAND float16_io_perf ${_bf16_sample} ${CMAKE_SOURCE_DIR}/test/data/NEXTCDF4/gdas_sfc_sample_float16.nc ${CMAKE_CURRENT_BINARY_DIR}/float16_io_results.csv WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(nxt4_float16_io_perf PROPERTIES ENVIRONMENT "${_bf16_env}" LABELS "nextcdf4;performance" TIMEOUT 900 RUN_SERIAL TRUE)
    endif()

endif()

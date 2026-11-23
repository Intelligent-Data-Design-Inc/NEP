# Build file for the NEXTCDF-4 test directory.

if(HAVE_NEXTCDF4)
    add_executable(tst_nextcdf4_string tst_nextcdf4_string.c)
    target_link_libraries(tst_nextcdf4_string PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_string PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/string)
    add_test(NAME tst_nextcdf4_string COMMAND tst_nextcdf4_string)
    set_tests_properties(tst_nextcdf4_string PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/string
        LABELS nextcdf4_strings
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
    foreach(control IN ITEMS fill_mode cache)
        set(target tst_nextcdf4_${control})
        add_executable(${target} ${target}.c)
        target_link_libraries(${target} PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${control})
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${control}
            LABELS nextcdf4_controls
            ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
    endforeach()
    add_executable(tst_nextcdf4_range tst_nextcdf4_range.c)
    target_link_libraries(tst_nextcdf4_range PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES} m)
    target_include_directories(tst_nextcdf4_range PRIVATE ${NETCDF_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_range COMMAND tst_nextcdf4_range)
    set_tests_properties(tst_nextcdf4_range PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
    add_executable(tst_nextcdf4_internal_convert tst_nextcdf4_internal_convert.c)
    target_link_libraries(tst_nextcdf4_internal_convert PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES} m)
    target_include_directories(tst_nextcdf4_internal_convert PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_internal_convert COMMAND tst_nextcdf4_internal_convert)
    set_tests_properties(tst_nextcdf4_internal_convert PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
    add_executable(tst_nextcdf4_dispatch tst_nextcdf4_dispatch.c)
    target_link_libraries(tst_nextcdf4_dispatch PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_dispatch PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_dispatch COMMAND tst_nextcdf4_dispatch)
    set_tests_properties(tst_nextcdf4_dispatch PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_file tst_nextcdf4_file.c)
    target_link_libraries(tst_nextcdf4_file PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_file PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_file COMMAND tst_nextcdf4_file)
    set_tests_properties(tst_nextcdf4_file PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_autoload tst_nextcdf4_autoload.c)
    target_link_libraries(tst_nextcdf4_autoload PRIVATE ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_autoload PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_autoload COMMAND tst_nextcdf4_autoload)
    set_tests_properties(tst_nextcdf4_autoload PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_meta tst_nextcdf4_meta.c)
    target_link_libraries(tst_nextcdf4_meta PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_meta PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_meta COMMAND tst_nextcdf4_meta)
    set_tests_properties(tst_nextcdf4_meta PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_var tst_nextcdf4_var.c)
    target_link_libraries(tst_nextcdf4_var PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_var PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_var COMMAND tst_nextcdf4_var)
    set_tests_properties(tst_nextcdf4_var PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_group tst_nextcdf4_group.c)
    target_link_libraries(tst_nextcdf4_group PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_group PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_group COMMAND tst_nextcdf4_group)
    set_tests_properties(tst_nextcdf4_group PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_setters tst_nextcdf4_setters.c)
    target_link_libraries(tst_nextcdf4_setters PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_setters PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_setters COMMAND tst_nextcdf4_setters)
    set_tests_properties(tst_nextcdf4_setters PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_chunking tst_nextcdf4_chunking.c)
    target_link_libraries(tst_nextcdf4_chunking PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_chunking PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_chunking COMMAND tst_nextcdf4_chunking)
    set_tests_properties(tst_nextcdf4_chunking PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_filter tst_nextcdf4_filter.c)
    target_link_libraries(tst_nextcdf4_filter PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_filter PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_filter COMMAND tst_nextcdf4_filter)
    set_tests_properties(tst_nextcdf4_filter PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_open tst_nextcdf4_open.c)
    target_link_libraries(tst_nextcdf4_open PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_open PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_open COMMAND tst_nextcdf4_open)
    set_tests_properties(tst_nextcdf4_open PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_rename tst_nextcdf4_rename.c)
    target_link_libraries(tst_nextcdf4_rename PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_rename PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_rename COMMAND tst_nextcdf4_rename)
    set_tests_properties(tst_nextcdf4_rename PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_float16 tst_nextcdf4_float16.c)
    target_link_libraries(tst_nextcdf4_float16 PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_float16 PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_float16 COMMAND tst_nextcdf4_float16)
    # These tests aborted on 10-slot netcdf-c because released
    # FILEINFOFLAGS covered only UDF0/UDF1 (the USEFILEINFO assert in
    # NC_reclaim_data); fixed upstream in 5a81fb791, and NEXTCDF-4 now
    # requires that 64-slot scheme at configure time.
    set_tests_properties(tst_nextcdf4_float16 PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_conv tst_nextcdf4_conv.c)
    target_link_libraries(tst_nextcdf4_conv PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_conv PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_conv COMMAND tst_nextcdf4_conv)
    set_tests_properties(tst_nextcdf4_conv PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nep_bfloat16 tst_nep_bfloat16.c)
    target_link_libraries(tst_nep_bfloat16 PRIVATE nep ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nep_bfloat16 PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nep_bfloat16 COMMAND tst_nep_bfloat16)
    set_tests_properties(tst_nep_bfloat16 PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nep_float16 tst_nep_float16.c)
    target_link_libraries(tst_nep_float16 PRIVATE nep ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nep_float16 PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nep_float16 COMMAND tst_nep_float16)
    set_tests_properties(tst_nep_float16 PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_complex tst_nextcdf4_complex.c)
    target_link_libraries(tst_nextcdf4_complex PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_complex PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_complex COMMAND tst_nextcdf4_complex)
    set_tests_properties(tst_nextcdf4_complex PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_bitfield tst_nextcdf4_bitfield.c)
    target_link_libraries(tst_nextcdf4_bitfield PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_bitfield PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_bitfield COMMAND tst_nextcdf4_bitfield)
    set_tests_properties(tst_nextcdf4_bitfield PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_ref tst_nextcdf4_ref.c)
    target_link_libraries(tst_nextcdf4_ref PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_ref PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_ref COMMAND tst_nextcdf4_ref)
    set_tests_properties(tst_nextcdf4_ref PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_vlen tst_nextcdf4_vlen.c)
    target_link_libraries(tst_nextcdf4_vlen PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_vlen PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_vlen COMMAND tst_nextcdf4_vlen)
    set_tests_properties(tst_nextcdf4_vlen PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_compound_arrays tst_nextcdf4_compound_arrays.c)
    target_link_libraries(tst_nextcdf4_compound_arrays PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_compound_arrays PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_compound_arrays COMMAND tst_nextcdf4_compound_arrays)
    set_tests_properties(tst_nextcdf4_compound_arrays PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_user_types tst_nextcdf4_user_types.c)
    target_link_libraries(tst_nextcdf4_user_types PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_user_types PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_user_types COMMAND tst_nextcdf4_user_types)
    set_tests_properties(tst_nextcdf4_user_types PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_logging tst_nextcdf4_logging.c)
    target_link_libraries(tst_nextcdf4_logging PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_logging PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_logging COMMAND tst_nextcdf4_logging)
    set_tests_properties(tst_nextcdf4_logging PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_explore tst_explore.c)
    target_link_libraries(tst_explore PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_explore PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_explore COMMAND tst_explore)
    set_tests_properties(tst_explore PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_rename nc_test4/tst_rename.c)
    target_link_libraries(tst_rename PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(tst_rename PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nc_test4 ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_rename COMMAND tst_rename)
    set_tests_properties(tst_rename PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Ported upstream nc_test4 tests (Sprint 1L2). The nc_test4 include
    # directory supplies the local nc_tests.h/err_macros.h shims.
    foreach(_nxt4_port tst_camrun tst_atts3 tst_dims tst_vars tst_atts1)
        add_executable(${_nxt4_port} nc_test4/${_nxt4_port}.c)
        target_link_libraries(${_nxt4_port} PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
        target_include_directories(${_nxt4_port} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nc_test4 ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
        add_test(NAME ${_nxt4_port} COMMAND ${_nxt4_port})
        set_tests_properties(${_nxt4_port} PROPERTIES
            ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
    endforeach()

    # tst_dims reads the shipped upstream fixture ref_tst_dims.nc through
    # the srcdir environment variable, as automake does upstream.
    set_tests_properties(tst_dims PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};srcdir=${CMAKE_CURRENT_SOURCE_DIR}/nc_test4;LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Ported upstream user-type tests (Sprint 1N4). Each runs twice —
    # once in native mode and once in NC_NETCDF4_MODEL mode, selected by
    # NEP_NC_TEST4_MODEL via nep_nc_test4_create_mode() in nc_tests.h —
    # from a per-mode working directory so parallel runs never collide
    # on upstream file names. ref_tst_compounds.nc is a byte-for-byte
    # copy of netcdf-c nc_test4/ref_tst_compounds.nc at
    # 5baa62e6966d3e505b895574dafbd531a4aed360, sha256
    # ccf52cb2afefc3faca9d6e0e281f7d07a0297446efa3987fb2fcc592314cdddd,
    # read through srcdir by tst_compounds.
    foreach(_nxt4_utype tst_enums tst_opaques tst_compounds
            tst_compounds2 tst_vl tst_vlen_unlim
            tst_strings tst_strings2 tst_atts_string_rewrite)
        add_executable(${_nxt4_utype} nc_test4/${_nxt4_utype}.c)
        target_link_libraries(${_nxt4_utype} PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
        target_include_directories(${_nxt4_utype} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nc_test4 ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
        foreach(_nxt4_mode native netcdf4_model)
            file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nc_test4/${_nxt4_utype}/${_nxt4_mode})
            add_test(NAME ${_nxt4_utype}_${_nxt4_mode}
                COMMAND ${_nxt4_utype}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nc_test4/${_nxt4_utype}/${_nxt4_mode})
            if(_nxt4_mode STREQUAL native)
                set(_nxt4_selector native)
            else()
                set(_nxt4_selector netcdf4)
            endif()
            if(_nxt4_utype MATCHES "string")
                set(_nxt4_label nextcdf4_strings)
            else()
                set(_nxt4_label nextcdf4_user_types)
            endif()
            set_tests_properties(${_nxt4_utype}_${_nxt4_mode} PROPERTIES
                LABELS ${_nxt4_label}
                ENVIRONMENT "NEP_NC_TEST4_MODEL=${_nxt4_selector};NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};srcdir=${CMAKE_CURRENT_SOURCE_DIR}/nc_test4;LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
        endforeach()
    endforeach()
    set_tests_properties(tst_nextcdf4_group tst_nextcdf4_rename
        tst_nextcdf4_file tst_nextcdf4_open PROPERTIES LABELS nextcdf4_controls)
endif()

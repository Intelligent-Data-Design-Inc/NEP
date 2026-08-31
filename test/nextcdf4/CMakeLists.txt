# Build file for the NEXTCDF-4 test directory.

if(HAVE_NEXTCDF4)
    add_executable(tst_nextcdf4_dispatch tst_nextcdf4_dispatch.c)
    target_link_libraries(tst_nextcdf4_dispatch PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_dispatch PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_dispatch COMMAND tst_nextcdf4_dispatch)
    set_tests_properties(tst_nextcdf4_dispatch PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_file tst_nextcdf4_file.c)
    target_link_libraries(tst_nextcdf4_file PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_file PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_file COMMAND tst_nextcdf4_file)
    set_tests_properties(tst_nextcdf4_file PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_autoload tst_nextcdf4_autoload.c)
    target_link_libraries(tst_nextcdf4_autoload PRIVATE ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_autoload PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_autoload COMMAND tst_nextcdf4_autoload)
    set_tests_properties(tst_nextcdf4_autoload PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_meta tst_nextcdf4_meta.c)
    target_link_libraries(tst_nextcdf4_meta PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_meta PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_meta COMMAND tst_nextcdf4_meta)
    set_tests_properties(tst_nextcdf4_meta PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_var tst_nextcdf4_var.c)
    target_link_libraries(tst_nextcdf4_var PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_var PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_var COMMAND tst_nextcdf4_var)
    set_tests_properties(tst_nextcdf4_var PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_group tst_nextcdf4_group.c)
    target_link_libraries(tst_nextcdf4_group PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_group PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_group COMMAND tst_nextcdf4_group)
    set_tests_properties(tst_nextcdf4_group PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_chunking tst_nextcdf4_chunking.c)
    target_link_libraries(tst_nextcdf4_chunking PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_chunking PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_chunking COMMAND tst_nextcdf4_chunking)
    set_tests_properties(tst_nextcdf4_chunking PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_open tst_nextcdf4_open.c)
    target_link_libraries(tst_nextcdf4_open PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_open PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_open COMMAND tst_nextcdf4_open)
    set_tests_properties(tst_nextcdf4_open PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_rename tst_nextcdf4_rename.c)
    target_link_libraries(tst_nextcdf4_rename PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_rename PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_rename COMMAND tst_nextcdf4_rename)
    set_tests_properties(tst_nextcdf4_rename PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_float16 tst_nextcdf4_float16.c)
    target_link_libraries(tst_nextcdf4_float16 PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_float16 PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_float16 COMMAND tst_nextcdf4_float16)
    set_tests_properties(tst_nextcdf4_float16 PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_complex tst_nextcdf4_complex.c)
    target_link_libraries(tst_nextcdf4_complex PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_complex PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_complex COMMAND tst_nextcdf4_complex)
    set_tests_properties(tst_nextcdf4_complex PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_bitfield tst_nextcdf4_bitfield.c)
    target_link_libraries(tst_nextcdf4_bitfield PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_bitfield PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_bitfield COMMAND tst_nextcdf4_bitfield)
    set_tests_properties(tst_nextcdf4_bitfield PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_ref tst_nextcdf4_ref.c)
    target_link_libraries(tst_nextcdf4_ref PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_ref PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_ref COMMAND tst_nextcdf4_ref)
    set_tests_properties(tst_nextcdf4_ref PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_explore tst_explore.c)
    target_link_libraries(tst_explore PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_explore PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_explore COMMAND tst_explore)
    set_tests_properties(tst_explore PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_rename nc_test4/tst_rename.c)
    target_link_libraries(tst_rename PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_rename PRIVATE ${CMAKE_SOURCE_DIR}/src/nextcdf4 ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_rename COMMAND tst_rename)
    set_tests_properties(tst_rename PROPERTIES
        DISABLED TRUE
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

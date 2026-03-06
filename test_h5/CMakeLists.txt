# CMakeLists.txt for the test_h5 directory of the NEP project.
# These tests exercise the HDF5-level compression filter plugins directly.

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${HDF5_INCLUDE_DIR})

if(BUILD_LZ4)
    add_executable(tst_h_lz4 tst_h_lz4.c)
    target_link_libraries(tst_h_lz4 PRIVATE ${HDF5_LIBRARIES} m)
    add_executable(tst_lz4_size tst_lz4_size.c)
    target_link_libraries(tst_lz4_size PRIVATE ${HDF5_LIBRARIES} m)

    add_test(NAME tst_h_lz4 COMMAND tst_h_lz4)
    add_test(NAME tst_lz4_size COMMAND tst_lz4_size)

    set_tests_properties(tst_h_lz4 tst_lz4_size PROPERTIES
        ENVIRONMENT "HDF5_PLUGIN_PATH=${NEP_HDF5_PLUGIN_INSTALL_DIR}")
endif()

if(BUILD_BZIP2)
    add_executable(tst_h_bzip2 tst_h_bzip2.c)
    target_link_libraries(tst_h_bzip2 PRIVATE ${HDF5_C_${LIB_TYPE}_LIBRARY} m)

    add_test(NAME tst_h_bzip2 COMMAND tst_h_bzip2)

    set_tests_properties(tst_h_bzip2 PROPERTIES
        ENVIRONMENT "HDF5_PLUGIN_PATH=${NEP_HDF5_PLUGIN_INSTALL_DIR}")
endif()

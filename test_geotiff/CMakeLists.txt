# This is the build file for the test_geotiff directory of the NEP project.
# Edward Hartnett 12/26/25

# Only build tests if GeoTIFF is enabled
if(HAVE_GEOTIFF)
    # Copy test data files to build directory
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    
    # Include directories
    include_directories(${CMAKE_BINARY_DIR}/include)
    include_directories(${CMAKE_SOURCE_DIR}/include)
    include_directories(${CMAKE_BINARY_DIR})
    include_directories(${CMAKE_SOURCE_DIR})
    include_directories(${GEOTIFF_INCLUDE_DIR})
    include_directories(${TIFF_INCLUDE_DIR})

    # Test for GeoTIFF file handle management
    add_executable(tst_geotiff_handle tst_geotiff_handle.c)
    target_link_libraries(tst_geotiff_handle 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_geotiff_handle COMMAND tst_geotiff_handle)
    set_tests_properties(tst_geotiff_handle PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Test for GeoTIFF Phase 2: metadata extraction
    add_executable(tst_geotiff_metadata tst_geotiff_metadata.c)
    target_link_libraries(tst_geotiff_metadata 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_geotiff_metadata COMMAND tst_geotiff_metadata)
    set_tests_properties(tst_geotiff_metadata PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Test for GeoTIFF Phase 3.5a: error handling
    add_executable(tst_geotiff_errors tst_geotiff_errors.c)
    target_link_libraries(tst_geotiff_errors 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_geotiff_errors COMMAND tst_geotiff_errors)
    set_tests_properties(tst_geotiff_errors PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Test for GeoTIFF Phase 3.5a: edge cases
    add_executable(tst_geotiff_edge_cases tst_geotiff_edge_cases.c)
    target_link_libraries(tst_geotiff_edge_cases 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_geotiff_edge_cases COMMAND tst_geotiff_edge_cases)
    set_tests_properties(tst_geotiff_edge_cases PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Test for simple GeoTIFF value comparison (issue #84)
    add_executable(tst_simple_geotiff tst_simple_geotiff.c)
    target_link_libraries(tst_simple_geotiff 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_simple_geotiff COMMAND tst_simple_geotiff)
    set_tests_properties(tst_simple_geotiff PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # Benchmark for GeoTIFF Phase 3.5a: performance validation
    add_executable(bench_geotiff_performance bench_geotiff_performance.c)
    target_link_libraries(bench_geotiff_performance 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    # Note: benchmark is not added to tests - run manually for performance analysis

    message(STATUS "GeoTIFF tests enabled")
endif()

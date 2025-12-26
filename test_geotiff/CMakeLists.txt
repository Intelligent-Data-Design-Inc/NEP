# This is the build file for the test_geotiff directory of the NEP project.
# Edward Hartnett 12/26/25

# Only build tests if GeoTIFF is enabled
if(HAVE_GEOTIFF)
    # Include directories
    include_directories(${CMAKE_BINARY_DIR}/include)
    include_directories(${CMAKE_SOURCE_DIR}/include)
    include_directories(${CMAKE_BINARY_DIR})
    include_directories(${CMAKE_SOURCE_DIR})
    include_directories(${GEOTIFF_INCLUDE_DIR})
    include_directories(${TIFF_INCLUDE_DIR})

    # Test for GeoTIFF format detection
    add_executable(tst_geotiff_detect tst_geotiff_detect.c)
    target_link_libraries(tst_geotiff_detect 
        ncgeotiff
        ${NETCDF_LIBRARIES} 
        ${HDF5_LIBRARIES}
        ${GEOTIFF_LIBRARY} 
        ${TIFF_LIBRARY})
    add_test(NAME tst_geotiff_detect COMMAND tst_geotiff_detect)
    set_tests_properties(tst_geotiff_detect PROPERTIES 
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    message(STATUS "GeoTIFF tests enabled")
endif()

# CMakeLists.txt for DICOM C example
# Edward Hartnett, Intelligent Data Design, Inc.

if(NETCDF_LIBRARY_DIRS)
    set(_nc_config_hints ${NETCDF_LIBRARY_DIRS}/../bin)
elseif(NETCDF_PREFIX)
    set(_nc_config_hints ${NETCDF_PREFIX}/bin)
else()
    set(_nc_config_hints "")
endif()

find_program(NC_CONFIG nc-config
    HINTS ${_nc_config_hints}
    PATHS ${_nc_config_hints}
)

if(NOT NC_CONFIG)
    message(FATAL_ERROR "nc-config not found. Cannot build DICOM example.")
endif()

execute_process(
    COMMAND ${NC_CONFIG} --cflags
    OUTPUT_VARIABLE NC_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${NC_CONFIG} --libs
    OUTPUT_VARIABLE NC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(NC_CFLAGS_LIST UNIX_COMMAND "${NC_CFLAGS}")
separate_arguments(NC_LIBS_LIST UNIX_COMMAND "${NC_LIBS}")

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(dicom_read dicom_read.c)
target_compile_options(dicom_read PRIVATE ${NC_CFLAGS_LIST})
get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)
target_link_directories(dicom_read PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(dicom_read PRIVATE ${NC_LIBS_LIST} ncdicom nep)
set_target_properties(dicom_read PROPERTIES
    BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}"
    SKIP_BUILD_RPATH OFF
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}")

if(BUILD_TESTING)
    add_test(NAME dicom_read
             COMMAND dicom_read
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(dicom_read PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

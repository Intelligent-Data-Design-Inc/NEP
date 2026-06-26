# CMakeLists.txt for NcZarr examples
# Edward Hartnett, Intelligent Data Design, Inc.
# 2026-06-26

# Find nc-config from the NetCDF library directories or prefix
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
    message(FATAL_ERROR "nc-config not found. Cannot build examples.")
endif()

# Get NetCDF compile and link flags using nc-config
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

get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

execute_process(
    COMMAND ${NC_CONFIG} --libdir
    OUTPUT_VARIABLE NC_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_executable(nczarr_simple nczarr_simple.c)
target_compile_options(nczarr_simple PRIVATE ${NC_CFLAGS_LIST})
target_link_directories(nczarr_simple PRIVATE ${HDF5_LIB_DIR})
target_link_libraries(nczarr_simple PRIVATE ${NC_LIBS_LIST})
set_target_properties(nczarr_simple PROPERTIES
    BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}"
    SKIP_BUILD_RPATH OFF
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR}")

if(BUILD_TESTING)
    add_test(NAME nczarr_simple
             COMMAND nczarr_simple
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nczarr_simple PROPERTIES
        ENVIRONMENT_MODIFICATION
            "LD_LIBRARY_PATH=path_list_prepend:${NC_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${HDF5_LIB_DIR}")
endif()

# Configure validation script with ncdump path from top-level CMakeLists.txt
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh
               @ONLY)

if(BUILD_TESTING)
    add_test(NAME nczarr_simple_validate
             COMMAND bash -c "./nczarr_simple && bash validate_cdl.sh nczarr_simple 'file://simple_nczarr.zarr#mode=nczarr' ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/nczarr_simple_expected.cdl"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(nczarr_simple_validate PROPERTIES
                         DEPENDS nczarr_simple
                         ENVIRONMENT_MODIFICATION
                             "LD_LIBRARY_PATH=path_list_prepend:${NC_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${HDF5_LIB_DIR}")
endif()

# Fortran NcZarr example — only built when Fortran is enabled and NetCDF-Fortran is found (v1.11.0 sprint 2)
if(ENABLE_FORTRAN AND NetCDF_Fortran_FOUND)
    find_program(NF_CONFIG nf-config
        HINTS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
        PATHS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
    )

    if(NOT NF_CONFIG)
        message(STATUS "nf-config not found — skipping f_nczarr_simple")
    else()
        execute_process(
            COMMAND ${NF_CONFIG} --fflags
            OUTPUT_VARIABLE NF_FFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${NF_CONFIG} --flibs
            OUTPUT_VARIABLE NF_LIBS_F
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(NF_FFLAGS_LIST UNIX_COMMAND "${NF_FFLAGS}")
        separate_arguments(NF_LIBS_F_LIST UNIX_COMMAND "${NF_LIBS_F}")

        # Extract the NetCDF-Fortran lib dir from --flibs since --libdir is not
        # supported by all nf-config versions.
        set(NF_LIBDIR "")
        foreach(_flag IN LISTS NF_LIBS_F_LIST)
            if(_flag MATCHES "^-L(.+)")
                set(NF_LIBDIR "${CMAKE_MATCH_1}")
                break()
            endif()
        endforeach()

        add_executable(f_nczarr_simple f_nczarr_simple.f90)
        target_compile_options(f_nczarr_simple PRIVATE ${NF_FFLAGS_LIST})
        target_link_directories(f_nczarr_simple PRIVATE ${HDF5_LIB_DIR} ${NF_LIBDIR})
        target_link_libraries(f_nczarr_simple PRIVATE ${NF_LIBS_F_LIST} ${NETCDF_LIBRARIES} hdf5 hdf5_hl)
        set_target_properties(f_nczarr_simple PROPERTIES
            BUILD_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR};${NF_LIBDIR}"
            SKIP_BUILD_RPATH OFF
            BUILD_WITH_INSTALL_RPATH ON
            INSTALL_RPATH "${HDF5_LIB_DIR};${NC_LIBDIR};${NF_LIBDIR}")

        if(BUILD_TESTING)
            add_test(NAME f_nczarr_simple
                     COMMAND f_nczarr_simple
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            set_tests_properties(f_nczarr_simple PROPERTIES
                ENVIRONMENT_MODIFICATION
                    "LD_LIBRARY_PATH=path_list_prepend:${NF_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${NC_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${HDF5_LIB_DIR}")

            add_test(NAME f_nczarr_simple_validate
                     COMMAND bash -c "./f_nczarr_simple && bash validate_cdl.sh f_nczarr_simple 'file://f_simple_nczarr.zarr#mode=nczarr' ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/f_nczarr_simple_expected.cdl"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            set_tests_properties(f_nczarr_simple_validate PROPERTIES
                                 DEPENDS f_nczarr_simple
                                 ENVIRONMENT_MODIFICATION
                                     "LD_LIBRARY_PATH=path_list_prepend:${NF_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${NC_LIBDIR};LD_LIBRARY_PATH=path_list_prepend:${HDF5_LIB_DIR}")
        endif()
    endif()
endif()

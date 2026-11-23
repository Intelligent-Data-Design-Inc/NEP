# Build file for the ftest directory of the NEP project.
# Edward Hartnett 2026-06-28

# Include directories for format reader tests and netcdf-fortran modules.
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find NetCDF-Fortran for Fortran tests.
find_package(PkgConfig REQUIRED)
pkg_check_modules(NetCDF_Fortran netcdf-fortran)

if(NOT NetCDF_Fortran_FOUND)
    message(STATUS "NetCDF-Fortran not found, Fortran tests will be skipped")
    return()
endif()

include_directories(${NetCDF_Fortran_INCLUDE_DIRS})
link_directories(${NetCDF_Fortran_LIBRARY_DIRS})

if(TARGET nepnextcdf4f)
    set(_binding_dependency_dirs "${NEP_HDF5_LIBRARY_DIR};${NETCDF_LIBRARY_DIRS};${NetCDF_Fortran_LIBRARY_DIRS}")
    list(REMOVE_DUPLICATES _binding_dependency_dirs)
    string(REPLACE ";" ":" _binding_dependency_path "${_binding_dependency_dirs}")
    add_test(NAME ftst_nextcdf4_installed COMMAND ${CMAKE_COMMAND}
        "-DBINARY=${CMAKE_BINARY_DIR}" "-DSOURCE=${CMAKE_SOURCE_DIR}"
        "-DLIBDIR=${CMAKE_INSTALL_LIBDIR}" "-DINCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
        "-DFC=${CMAKE_Fortran_COMPILER}" "-DDEPENDENCY_PREFIXES=${CMAKE_PREFIX_PATH}"
        "-DDEPENDENCY_LIBS=${_binding_dependency_path}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_nextcdf4_install.cmake")
    set_tests_properties(ftst_nextcdf4_installed PROPERTIES LABELS nextcdf4_fortran RUN_SERIAL TRUE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS rank_test.F90.in)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/rank_test.F90.in" RANK_TEMPLATE)
    set(RANK_TESTS "")
    set(RANK_CALLS "")
    foreach(RANK RANGE 1 7)
        set(EXTENTS "2")
        set(FIRST "1:1")
        if(RANK GREATER 1)
            foreach(I RANGE 2 ${RANK})
                string(APPEND EXTENTS ",2")
                string(APPEND FIRST ",1:1")
            endforeach()
        endif()
        string(CONFIGURE "${RANK_TEMPLATE}" RANK_TEST @ONLY)
        string(APPEND RANK_TESTS "${RANK_TEST}\n")
        string(APPEND RANK_CALLS "call rank_${RANK}()\n")
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rank_tests.inc" "${RANK_TESTS}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rank_calls.inc" "${RANK_CALLS}")
    add_executable(ftst_nextcdf4_types ftst_nextcdf4_types.F90 nextcdf4_constants.c)
    target_include_directories(ftst_nextcdf4_types PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(ftst_nextcdf4_types PRIVATE nepnextcdf4f)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/types")
    add_test(NAME ftst_nextcdf4_types COMMAND ftst_nextcdf4_types)
    set_tests_properties(ftst_nextcdf4_types PROPERTIES
        LABELS nextcdf4_fortran WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/types")
    add_executable(ftst_nextcdf4_small_float ftst_nextcdf4_small_float.F90)
    target_link_libraries(ftst_nextcdf4_small_float PRIVATE nepnextcdf4f)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/small_float")
    add_test(NAME ftst_nextcdf4_small_float COMMAND ftst_nextcdf4_small_float)
    set_tests_properties(ftst_nextcdf4_small_float PROPERTIES
        LABELS nextcdf4_fortran WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/small_float")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/empty.ncrc" "")
    set_tests_properties(ftst_nextcdf4_types ftst_nextcdf4_small_float PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_CURRENT_BINARY_DIR}/empty.ncrc;NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR};NEP_LOG_LEVEL=-1")
endif()

# Build the FITS Fortran test if FITS is enabled.
if(HAVE_FITS)
    add_executable(ftst_fits_udf ftst_fits_udf.F90)
    target_link_libraries(ftst_fits_udf PRIVATE ncfits nep ${NetCDF_Fortran_LIBRARIES} ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(ftst_fits_udf PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS} ${CFITSIO_INCLUDE_DIR})
    add_dependencies(ftst_fits_udf ncfits)
    add_test(NAME ftst_fits_udf COMMAND ftst_fits_udf)
    set_tests_properties(ftst_fits_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Build the DICOM Fortran test if DICOM is enabled.
if(HAVE_DICOM)
    add_executable(ftst_dicom_udf ftst_dicom_udf.F90)
    target_link_libraries(ftst_dicom_udf PRIVATE ncdicom nep ${NetCDF_Fortran_LIBRARIES} ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(ftst_dicom_udf PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS} ${DICOM_INCLUDE_DIR})
    add_dependencies(ftst_dicom_udf ncdicom)
    add_test(NAME ftst_dicom_udf COMMAND ftst_dicom_udf)
    set_tests_properties(ftst_dicom_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Build the legacy PDB Fortran smoke test if PDB is enabled.
if(HAVE_PDB)
    add_executable(ftst_pdb_udf ftst_pdb_udf.F90)
    target_link_libraries(ftst_pdb_udf PRIVATE ncpdb nep ${NetCDF_Fortran_LIBRARIES} ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(ftst_pdb_udf PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
    add_dependencies(ftst_pdb_udf ncpdb)
    add_test(NAME ftst_pdb_udf COMMAND ftst_pdb_udf)
    set_tests_properties(ftst_pdb_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Build the PDBx/mmCIF Fortran smoke test if mmCIF is enabled.
if(HAVE_MMCIF)
    add_executable(ftst_mmcif_udf ftst_mmcif_udf.F90)
    target_link_libraries(ftst_mmcif_udf PRIVATE ncmmcif nep ${NetCDF_Fortran_LIBRARIES} ${NETCDF_LIBRARIES} ${NEP_HDF5_LIBRARIES})
    target_include_directories(ftst_mmcif_udf PRIVATE ${NetCDF_Fortran_INCLUDE_DIRS})
    add_dependencies(ftst_mmcif_udf ncmmcif)
    add_test(NAME ftst_mmcif_udf COMMAND ftst_mmcif_udf)
    set_tests_properties(ftst_mmcif_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Configure run_tests.sh script with format reader paths.
if(HAVE_FITS)
    set(BUILD_FITS "yes")
else()
    set(BUILD_FITS "no")
endif()
if(HAVE_DICOM)
    set(BUILD_DICOM "yes")
else()
    set(BUILD_DICOM "no")
endif()
if(HAVE_PDB)
    set(BUILD_PDB "yes")
else()
    set(BUILD_PDB "no")
endif()
if(HAVE_MMCIF)
    set(BUILD_MMCIF "yes")
else()
    set(BUILD_MMCIF "no")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh @ONLY)

file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Add the Fortran test script as a test.
add_test(NAME run_fortran_tests
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(run_fortran_tests PROPERTIES
    ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

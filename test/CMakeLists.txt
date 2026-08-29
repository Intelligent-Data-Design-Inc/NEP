# Build file for the test directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR})

# Enable testing
enable_testing()

# Find NetCDF library for tests
find_package(PkgConfig REQUIRED)
pkg_check_modules(NETCDF REQUIRED netcdf)
pkg_get_variable(NETCDF_PREFIX netcdf prefix)

# Build LZ4 test
if(NEP_BUILD_LZ4)
    add_executable(tst_lz4 tst_lz4.c)
    target_link_libraries(tst_lz4 PRIVATE nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_lz4 PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
endif()

# Build BZIP2 test
if(NEP_BUILD_BZIP2)
    add_executable(tst_bzip2 tst_bzip2.c)
    target_link_libraries(tst_bzip2 PRIVATE nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_bzip2 PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
endif()

# Build performance test
add_executable(tst_perf tst_perf.c tst_utils.c)
target_link_libraries(tst_perf PRIVATE nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
target_include_directories(tst_perf PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})

# Build build metadata header test
add_executable(tst_meta tst_meta.c)
add_test(NAME tst_meta COMMAND tst_meta)

if(HAVE_NEXTCDF4)
    add_executable(tst_nextcdf4_dispatch tst_nextcdf4_dispatch.c)
    target_link_libraries(tst_nextcdf4_dispatch PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_dispatch PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_dispatch COMMAND tst_nextcdf4_dispatch)
    set_tests_properties(tst_nextcdf4_dispatch PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_file tst_nextcdf4_file.c)
    target_link_libraries(tst_nextcdf4_file PRIVATE ncnextcdf4 ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_file PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_nextcdf4_file COMMAND tst_nextcdf4_file)
    set_tests_properties(tst_nextcdf4_file PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    add_executable(tst_nextcdf4_autoload tst_nextcdf4_autoload.c)
    target_link_libraries(tst_nextcdf4_autoload PRIVATE ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_nextcdf4_autoload PRIVATE ${NETCDF_INCLUDE_DIRS})
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
endif()

# Build CDF UDF test if CDF is enabled
if(HAVE_CDF)
    # Get HDF5 library directory
    get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)
    
    add_executable(tst_cdf_udf tst_cdf_udf.c)
    target_link_directories(tst_cdf_udf PRIVATE ${HDF5_LIB_DIR})
    target_link_libraries(tst_cdf_udf PRIVATE nccdf nep ${NETCDF_LIBRARIES} hdf5 hdf5_hl ${CDF_LIBRARY})
    target_include_directories(tst_cdf_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${CDF_INCLUDE_DIR})
    
    # Set rpath so the executable can find CDF library at runtime
    get_filename_component(CDF_LIB_DIR ${CDF_LIBRARY} DIRECTORY)
    set_target_properties(tst_cdf_udf PROPERTIES
        INSTALL_RPATH "${CDF_LIB_DIR}"
        BUILD_RPATH "${CDF_LIB_DIR}")
    
    # Add test - creates its own CDF file
    add_test(NAME tst_cdf_udf COMMAND tst_cdf_udf)
    set_tests_properties(tst_cdf_udf PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Build IMAP MAG CDF test
    add_executable(tst_imap_mag tst_imap_mag.c)
    target_link_directories(tst_imap_mag PRIVATE ${HDF5_LIB_DIR})
    target_link_libraries(tst_imap_mag PRIVATE nccdf nep ${NETCDF_LIBRARIES} hdf5 hdf5_hl ${CDF_LIBRARY})
    target_include_directories(tst_imap_mag PRIVATE ${NETCDF_INCLUDE_DIRS} ${CDF_INCLUDE_DIR})
    
    # Set rpath so the executable can find CDF library at runtime
    set_target_properties(tst_imap_mag PROPERTIES
        INSTALL_RPATH "${CDF_LIB_DIR}"
        BUILD_RPATH "${CDF_LIB_DIR}")
    
    # Copy test data files to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/imap_mag_l1b-calibration_20240229_v001.cdf
                   ${CMAKE_CURRENT_BINARY_DIR}/data/imap_mag_l1b-calibration_20240229_v001.cdf
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/tst_cdf_simple.cdf
                   ${CMAKE_CURRENT_BINARY_DIR}/data/tst_cdf_simple.cdf
                   COPYONLY)
    
    # Add test - uses existing IMAP MAG CDF file
    add_test(NAME tst_imap_mag COMMAND tst_imap_mag)
    set_tests_properties(tst_imap_mag PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")
endif()

# Build GRIB2 UDF placeholder test if GRIB2 is enabled
if(HAVE_GRIB2)
    add_executable(tst_grib2_udf tst_grib2_udf.c)
    target_link_libraries(tst_grib2_udf PRIVATE ncgrib2 nep ${NETCDF_LIBRARIES})
    target_include_directories(tst_grib2_udf PRIVATE ${NETCDF_INCLUDE_DIRS})
    add_test(NAME tst_grib2_udf COMMAND tst_grib2_udf)

    # Copy GRIB2 test data to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/gdaswave.t00z.wcoast.0p16.f000.grib2
                   ${CMAKE_CURRENT_BINARY_DIR}/data/gdaswave.t00z.wcoast.0p16.f000.grib2
                   COPYONLY)
endif()

# Build FITS UDF test if FITS is enabled
if(HAVE_FITS)
    # Get HDF5 library directory
    get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

    add_executable(tst_fits_udf tst_fits_udf.c)
    target_link_directories(tst_fits_udf PRIVATE ${HDF5_LIB_DIR})
    target_link_libraries(tst_fits_udf PRIVATE ncfits nep ${NETCDF_LIBRARIES} hdf5 hdf5_hl)
    target_include_directories(tst_fits_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${CFITSIO_INCLUDE_DIR})
    add_test(NAME tst_fits_udf COMMAND tst_fits_udf)
    set_tests_properties(tst_fits_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Copy FITS test data to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/WFPC2u5780205r_c0fx.fits
                   ${CMAKE_CURRENT_BINARY_DIR}/data/WFPC2u5780205r_c0fx.fits
                   COPYONLY)
endif()

# Build DICOM UDF test if DICOM is enabled
if(HAVE_DICOM)
    # Provide an empty .ncrc so NetCDF-C does not fall back to ~/.ncrc
    # and attempt to dlopen UDF handlers that are not built.
    file(TOUCH ${CMAKE_BINARY_DIR}/.ncrc)

    add_executable(tst_dicom_udf tst_dicom_udf.c)
    target_link_libraries(tst_dicom_udf PRIVATE ncdicom nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_dicom_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${DICOM_INCLUDE_DIR})
    add_test(NAME tst_dicom_udf COMMAND tst_dicom_udf)
    set_tests_properties(tst_dicom_udf PROPERTIES
        ENVIRONMENT "NCRCENV_RC=${CMAKE_BINARY_DIR}/.ncrc;NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Copy DICOM test data to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/tst_dicom_uncompressed.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/tst_dicom_uncompressed.dcm
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/MRBRAIN.DCM
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/MRBRAIN.DCM
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/0003.DCM
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/0003.DCM
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/CT-MONO2-16-chest.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/CT-MONO2-16-chest.dcm
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/CT-MONO2-16-brain.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/CT-MONO2-16-brain.dcm
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/MR-MONO2-16-head.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/MR-MONO2-16-head.dcm
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/CR-MONO1-10-chest.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/CR-MONO1-10-chest.dcm
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/DICOM/MR-MONO2-12-shoulder.dcm
                   ${CMAKE_CURRENT_BINARY_DIR}/data/DICOM/MR-MONO2-12-shoulder.dcm
                   COPYONLY)
endif()

# Build legacy PDB UDF test if PDB is enabled
if(HAVE_PDB)
    add_executable(tst_pdb_udf tst_pdb_udf.c)
    target_link_libraries(tst_pdb_udf PRIVATE ncpdb nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_pdb_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_pdb_udf COMMAND tst_pdb_udf)
    set_tests_properties(tst_pdb_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Copy legacy PDB test data (from https://www.rcsb.org) to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDB)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDB/1J7W.pdb
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDB/1J7W.pdb
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDB/4HHB.pdb
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDB/4HHB.pdb
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDB/1GAB.pdb
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDB/1GAB.pdb
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDB/no_cryst1.pdb
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDB/no_cryst1.pdb
                   COPYONLY)
endif()

# Build PDBx/mmCIF UDF test if mmCIF is enabled
if(HAVE_MMCIF)
    add_executable(tst_mmcif_udf tst_mmcif_udf.c)
    target_link_libraries(tst_mmcif_udf PRIVATE ncmmcif nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_mmcif_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    add_test(NAME tst_mmcif_udf COMMAND tst_mmcif_udf)
    set_tests_properties(tst_mmcif_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Copy PDBx/mmCIF test data (from https://www.rcsb.org) to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/mmCIF)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/mmCIF/1J7W.cif
                   ${CMAKE_CURRENT_BINARY_DIR}/data/mmCIF/1J7W.cif
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/mmCIF/2W6V.cif
                   ${CMAKE_CURRENT_BINARY_DIR}/data/mmCIF/2W6V.cif
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/mmCIF/4HHB.cif
                   ${CMAKE_CURRENT_BINARY_DIR}/data/mmCIF/4HHB.cif
                   COPYONLY)
endif()

# Build PDS4 UDF test if PDS4 is enabled
if(HAVE_PDS4)
    add_executable(tst_pds4_udf tst_pds4_udf.c)
    target_link_libraries(tst_pds4_udf PRIVATE ncpds4 nep ${NETCDF_LIBRARIES} ${HDF5_LIBRARIES})
    target_include_directories(tst_pds4_udf PRIVATE ${NETCDF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})
    add_test(NAME tst_pds4_udf COMMAND tst_pds4_udf)
    set_tests_properties(tst_pds4_udf PROPERTIES
        ENVIRONMENT "NETCDF_RC=${CMAKE_BINARY_DIR};LD_LIBRARY_PATH=${_nep_test_ld_path}:$ENV{LD_LIBRARY_PATH}")

    # Copy PDS4 test data to build directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4)

    # Generic PDS4 test data
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/test_image.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/test_image.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/test_image.img
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/test_image.img
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/test_table_binary.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/test_table_binary.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/test_table_binary.dat
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/test_table_binary.dat
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/Table_Character_Example.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/Table_Character_Example.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/general/Table_Character_Example.tab
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/general/Table_Character_Example.tab
                   COPYONLY)

    # Mission PDS4 test data
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/cassini_hrd)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/cassini_hrd/hrd_2000_on_off.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/cassini_hrd/hrd_2000_on_off.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/cassini_hrd/hrd_2000_on_off.tab
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/cassini_hrd/hrd_2000_on_off.tab
                   COPYONLY)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/messenger_tnmap)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/messenger_tnmap/thermal_neutron_map.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/messenger_tnmap/thermal_neutron_map.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/messenger_tnmap/thermal_neutron_map.img
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/messenger_tnmap/thermal_neutron_map.img
                   COPYONLY)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/lcs_9p)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/lcs_9p/20050706_000.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/lcs_9p/20050706_000.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/lcs_9p/20050706_000.tab
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/lcs_9p/20050706_000.tab
                   COPYONLY)

    # Maven mission data (NGIMS and IUVS)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.csv
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.csv
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758_v07_r01.fits
                   COPYONLY)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.fit
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.lblx
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.lblx
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.fit
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.fit
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.lblx
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.lblx
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.fit
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.fit
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0002845457_0x4b2_sci_1.lblx
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0002845457_0x4b2_sci_1.lblx
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/new_horizons/ali_0002845457_0x4b2_sci_1.fit
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/new_horizons/ali_0002845457_0x4b2_sci_1.fit
                   COPYONLY)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/maven/mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/maven/mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits
                   COPYONLY)

    # Perseverance Mastcam-Z mission data (Array_3D_Image, VICAR-prefixed binary)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/perseverance)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/perseverance/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/perseverance/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml
                   COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/PDS4/perseverance/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG
                   ${CMAKE_CURRENT_BINARY_DIR}/data/PDS4/perseverance/ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG
                   COPYONLY)
endif()

# Configure run_tests.sh script with proper paths
# For CMake builds, point to the installed plugin location
set(MY_HDF5_PLUGIN_PATH "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin:${NETCDF_PREFIX}/hdf5/lib/plugin:${NETCDF_PREFIX}/lib/plugin:${NETCDF_PREFIX}/lib/hdf5/plugin")
set(MY_LZ4_PLUGIN_PATH "${CMAKE_BINARY_DIR}/hdf5_plugins_install/lib/plugin")

# Provide a runtime library search path for the selected HDF5.
# FindHDF5.cmake does not always populate HDF5_LIBRARY_DIRS, so derive it.
set(_nep_hdf5_lib_dirs "")
if(HDF5_LIBRARY_DIRS)
    list(APPEND _nep_hdf5_lib_dirs ${HDF5_LIBRARY_DIRS})
else()
    foreach(_nep_lib ${HDF5_LIBRARIES})
        if(IS_ABSOLUTE "${_nep_lib}")
            get_filename_component(_nep_lib_dir "${_nep_lib}" DIRECTORY)
            list(APPEND _nep_hdf5_lib_dirs "${_nep_lib_dir}")
        endif()
    endforeach()
endif()
if(NOT _nep_hdf5_lib_dirs AND DEFINED HDF5_ROOT)
    list(APPEND _nep_hdf5_lib_dirs "${HDF5_ROOT}/lib" "${HDF5_ROOT}/lib64")
endif()
list(REMOVE_DUPLICATES _nep_hdf5_lib_dirs)
list(APPEND _nep_hdf5_lib_dirs "${NETCDF_PREFIX}/lib")
list(REMOVE_DUPLICATES _nep_hdf5_lib_dirs)
string(REPLACE ";" ":" MY_LD_LIBRARY_PATH "${_nep_hdf5_lib_dirs}")

if(NEP_BUILD_LZ4)
    set(BUILD_LZ4 "yes")
else()
    set(BUILD_LZ4 "no")
endif()
if(NEP_BUILD_BZIP2)
    set(BUILD_BZIP2 "yes")
else()
    set(BUILD_BZIP2 "no")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh.in 
               ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh @ONLY)

# Make the script executable
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh 
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Add single test that runs the test script (like autotools)
# The script sets HDF5_PLUGIN_PATH itself from MY_HDF5_PLUGIN_PATH
add_test(NAME run_tests 
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(run_tests PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${MY_LD_LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH}")

# Make test script depend on hdf5_plugins being built
set(_test_exe_deps tst_perf tst_meta)
if(NEP_BUILD_LZ4)
    list(APPEND _test_exe_deps tst_lz4)
endif()
if(NEP_BUILD_BZIP2)
    list(APPEND _test_exe_deps tst_bzip2)
endif()
if(HAVE_CDF)
    list(APPEND _test_exe_deps tst_cdf_udf tst_imap_mag)
endif()
if(HAVE_GRIB2)
    list(APPEND _test_exe_deps tst_grib2_udf)
endif()
if(HAVE_FITS)
    list(APPEND _test_exe_deps tst_fits_udf)
endif()
if(HAVE_DICOM)
    list(APPEND _test_exe_deps tst_dicom_udf)
endif()
if(HAVE_PDS4)
    list(APPEND _test_exe_deps tst_pds4_udf)
endif()
if(HAVE_PDB)
    list(APPEND _test_exe_deps tst_pdb_udf)
endif()
if(HAVE_MMCIF)
    list(APPEND _test_exe_deps tst_mmcif_udf)
endif()
add_custom_target(test_executables ALL DEPENDS ${_test_exe_deps})
add_dependencies(test_executables hdf5_plugins)

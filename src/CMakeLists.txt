# This is the build file for the src directory of the NEP project.
# Edward Hartnett 8/27/25

# Include directories to match Makefile.am AM_CPPFLAGS
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR})

# Build the main nep library
add_library(nep SHARED nep.c)
target_link_libraries(nep PRIVATE ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES})
set_target_properties(nep PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/nep.h"
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS nep
    EXPORT NEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${CMAKE_SOURCE_DIR}/include/grib2dispatch.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# CDF UDF Handler - v1.3.0 Sprint 3
# Build CDF library if enabled (netCDF UDF support)
if(HAVE_CDF)
    set(CDF_LIB_NAME nccdf)
    add_library(${CDF_LIB_NAME} SHARED 
        cdfdispatch.c cdffile.c cdffunc.c cdfvar.c)
    target_include_directories(${CDF_LIB_NAME} PRIVATE ${CDF_INCLUDE_DIR})
    target_link_libraries(${CDF_LIB_NAME} PRIVATE 
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${CDF_LIBRARY})
    set_target_properties(${CDF_LIB_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${CDF_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building CDF UDF library: ${CDF_LIB_NAME}")
endif()

# GeoTIFF UDF Handler - v1.5.0
# Build GeoTIFF library if enabled (netCDF UDF support)
if(HAVE_GEOTIFF)
    set(GEOTIFF_LIB_NAME ncgeotiff)
    add_library(${GEOTIFF_LIB_NAME} SHARED 
        geotiffdispatch.c geotifffile.c)
    target_include_directories(${GEOTIFF_LIB_NAME} PRIVATE ${GEOTIFF_INCLUDE_DIR} ${TIFF_INCLUDE_DIR})
    target_link_libraries(${GEOTIFF_LIB_NAME} PRIVATE 
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${GEOTIFF_LIBRARY} ${TIFF_LIBRARY})
    set_target_properties(${GEOTIFF_LIB_NAME} PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${GEOTIFF_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building GeoTIFF UDF library: ${GEOTIFF_LIB_NAME}")
endif()

# GRIB2 UDF Handler - v1.7.0
# Build GRIB2 library if enabled (netCDF UDF support)
if(HAVE_GRIB2)
    # Locate jasper (required by libg2c at runtime)
    find_library(JASPER_LIBRARY NAMES jasper)
    find_path(JASPER_INCLUDE_DIR jasper/jasper.h)
    if(NOT JASPER_LIBRARY)
        message(FATAL_ERROR "libjasper not found (required by libg2c). Add its path to CMAKE_PREFIX_PATH.")
    endif()
    # Extract the directory containing libjasper for rpath embedding
    get_filename_component(JASPER_LIBRARY_DIR "${JASPER_LIBRARY}" DIRECTORY)
    get_filename_component(G2C_LIBRARY_DIR "${G2C_LIBRARY}" DIRECTORY)

    set(GRIB2_LIB_NAME ncgrib2)
    add_library(${GRIB2_LIB_NAME} SHARED
        grib2dispatch.c grib2file.c)
    target_include_directories(${GRIB2_LIB_NAME} PRIVATE ${G2C_INCLUDE_DIR})
    target_link_libraries(${GRIB2_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${G2C_LIBRARY} ${JASPER_LIBRARY})
    set_target_properties(${GRIB2_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};${G2C_LIBRARY_DIR};${JASPER_LIBRARY_DIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH "${G2C_LIBRARY_DIR};${JASPER_LIBRARY_DIR}"
    )
    install(TARGETS ${GRIB2_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    message(STATUS "Building GRIB2 UDF library: ${GRIB2_LIB_NAME}")
    message(STATUS "  g2c: ${G2C_LIBRARY}")
    message(STATUS "  jasper: ${JASPER_LIBRARY}")
endif()

# FITS UDF Handler - v2.0.0
# Build FITS library if enabled (netCDF UDF support)
if(HAVE_FITS)
    get_filename_component(CFITSIO_LIBRARY_DIR "${CFITSIO_LIBRARY}" DIRECTORY)

    set(FITS_LIB_NAME ncfits)
    add_library(${FITS_LIB_NAME} SHARED
        fitsdispatch.c fitsfile.c)
    target_include_directories(${FITS_LIB_NAME} PRIVATE ${CFITSIO_INCLUDE_DIR})
    target_link_libraries(${FITS_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${CFITSIO_LIBRARY})
    set_target_properties(${FITS_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};${CFITSIO_LIBRARY_DIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH "${CFITSIO_LIBRARY_DIR}"
    )
    install(TARGETS ${FITS_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/include/fitsdispatch.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building FITS UDF library: ${FITS_LIB_NAME}")
    message(STATUS "  cfitsio: ${CFITSIO_LIBRARY}")
endif()

# PDS4 UDF Handler - v2.2.0
# Build PDS4 library if enabled (netCDF UDF support)
if(HAVE_PDS4)
    set(PDS4_LIB_NAME ncpds4)
    add_library(${PDS4_LIB_NAME} SHARED
        pds4dispatch.c pds4file.c)
    target_include_directories(${PDS4_LIB_NAME} PRIVATE ${LIBXML2_INCLUDE_DIR})
    target_link_libraries(${PDS4_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${LIBXML2_LIBRARIES})
    set_target_properties(${PDS4_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${PDS4_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/include/pds4dispatch.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building PDS4 UDF library: ${PDS4_LIB_NAME}")
    message(STATUS "  libxml2: ${LIBXML2_LIBRARIES}")
endif()

# DICOM UDF Handler - v3.0.0 Sprint 1
# Build DICOM library if enabled (netCDF UDF support)
if(HAVE_DICOM)
    get_filename_component(DICOM_LIBRARY_DIR "${DICOM_LIBRARY}" DIRECTORY)

    set(DICOM_LIB_NAME ncdicom)
    add_library(${DICOM_LIB_NAME} SHARED
        dicomdispatch.c dicomfile.c
        dicomjpeglossless8.c dicomjpeglossless12.c dicomjpeglossless16.c
        dicomjpeglosslessprecision.c)
    target_include_directories(${DICOM_LIB_NAME} PRIVATE ${DICOM_INCLUDE_DIR})
    target_link_libraries(${DICOM_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES} ${DICOM_LIBRARY} ${JPEG_LIBRARIES})
    set(DICOM_RPATH_DIRS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};${DICOM_LIBRARY_DIR};${JPEG_LIBRARY_DIR}")
    set(DICOM_BUILD_RPATH_DIRS "${DICOM_LIBRARY_DIR};${JPEG_LIBRARY_DIR}")

    # JPEG Lossless (Process 14) decode wrappers each need their own
    # precision-specific GDCM include directory (gdcmjpeg/8, /12, /16) set
    # per-source-file, not on the target, so the mutually-incompatible
    # 8/12/16-bit jpeglib.h variants do not collide within the same build.
    if(HAVE_DICOM_LOSSLESS)
        set_source_files_properties(dicomjpeglossless8.c PROPERTIES
            INCLUDE_DIRECTORIES "${GDCMJPEG_INCLUDE_DIR}/gdcmjpeg/8")
        set_source_files_properties(dicomjpeglossless12.c PROPERTIES
            INCLUDE_DIRECTORIES "${GDCMJPEG_INCLUDE_DIR}/gdcmjpeg/12")
        set_source_files_properties(dicomjpeglossless16.c PROPERTIES
            INCLUDE_DIRECTORIES "${GDCMJPEG_INCLUDE_DIR}/gdcmjpeg/16")
        target_link_libraries(${DICOM_LIB_NAME} PRIVATE
            ${GDCMJPEG8_LIBRARY} ${GDCMJPEG12_LIBRARY} ${GDCMJPEG16_LIBRARY})
        get_filename_component(GDCMJPEG_LIBRARY_DIR "${GDCMJPEG8_LIBRARY}" DIRECTORY)
        string(APPEND DICOM_RPATH_DIRS ";${GDCMJPEG_LIBRARY_DIR}")
        string(APPEND DICOM_BUILD_RPATH_DIRS ";${GDCMJPEG_LIBRARY_DIR}")
    endif()

    set_target_properties(${DICOM_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${DICOM_RPATH_DIRS}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH "${DICOM_BUILD_RPATH_DIRS}"
    )
    install(TARGETS ${DICOM_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/include/dicomdispatch.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building DICOM UDF library: ${DICOM_LIB_NAME}")
    message(STATUS "  libdicom: ${DICOM_LIBRARY}")
    if(HAVE_DICOM_LOSSLESS)
        message(STATUS "  JPEG Lossless support: enabled (gdcmjpeg8/12/16)")
    else()
        message(STATUS "  JPEG Lossless support: disabled (libgdcm-dev not found)")
    endif()
endif()

# Legacy PDB UDF Handler - v3.3.0 Sprint 1
# Build PDB library if enabled (netCDF UDF support). No external parsing
# library is required; Sprint 1 is a no-op dispatch skeleton only.
if(HAVE_PDB)
    set(PDB_LIB_NAME ncpdb)
    add_library(${PDB_LIB_NAME} SHARED
        pdbdispatch.c pdbfile.c)
    target_link_libraries(${PDB_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES})
    set_target_properties(${PDB_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${PDB_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/include/pdbdispatch.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building legacy PDB UDF library: ${PDB_LIB_NAME}")
endif()

# PDBx/mmCIF UDF Handler - v3.4.0 Sprint 1
# Build mmCIF library if enabled (netCDF UDF support). No external parsing
# library is required; a custom STAR/CIF tokenizer will be added in Sprint 2.
if(HAVE_MMCIF)
    set(MMCIF_LIB_NAME ncmmcif)
    add_library(${MMCIF_LIB_NAME} SHARED
        mmcifdispatch.c mmciffile.c)
    target_link_libraries(${MMCIF_LIB_NAME} PRIVATE
        ${HDF5_LIBRARIES} ${NETCDF_LIBRARIES})
    set_target_properties(${MMCIF_LIB_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    install(TARGETS ${MMCIF_LIB_NAME}
        EXPORT NEPTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/include/mmcifdispatch.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    message(STATUS "Building PDBx/mmCIF UDF library: ${MMCIF_LIB_NAME}")
endif()

if(HAVE_NEXTCDF4)
    add_subdirectory(nextcdf4)
endif()

# Install the generated build metadata header.
install(FILES "${CMAKE_BINARY_DIR}/include/nep_meta.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

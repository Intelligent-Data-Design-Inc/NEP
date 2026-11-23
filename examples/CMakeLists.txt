# CMakeLists.txt for NEP examples
# Edward Hartnett 1/25/26

# Performance benchmarks (disk-bound timing via perf_disk.h)
add_subdirectory(performance)

if(HAVE_NEXTCDF4)
    add_subdirectory(nextcdf4)
endif()

if(HAVE_DICOM)
    add_subdirectory(dicom)
endif()

if(HAVE_PDB)
    add_subdirectory(pdb)
endif()

if(NEP_ENABLE_VIZ_EXAMPLES)
    add_subdirectory(viz)
endif()

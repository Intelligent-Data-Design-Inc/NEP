# CMakeLists.txt for OPeNDAP examples
#
# These examples demonstrate accessing remote datasets via OPeNDAP.
# They require network access and are not built by default.

# C Examples
add_executable(opendap_simple opendap_simple.c)
target_link_libraries(opendap_simple netcdf ${HDF5_LIBRARIES})

add_executable(opendap_constraint opendap_constraint.c)
target_link_libraries(opendap_constraint netcdf ${HDF5_LIBRARIES})

add_executable(opendap_subset opendap_subset.c)
target_link_libraries(opendap_subset netcdf ${HDF5_LIBRARIES})

# Group all OPeNDAP examples
set(OPENDAP_EXAMPLE_TARGETS
  opendap_simple
  opendap_constraint
  opendap_subset
)

# Fortran Examples (if Fortran is enabled)
if(ENABLE_FORTRAN)
  # Find nf-config from the NetCDF-Fortran library directories
  find_program(NF_CONFIG nf-config
      HINTS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
      PATHS ${NetCDF_Fortran_LIBRARY_DIRS}/../bin
  )

  if(NOT NF_CONFIG)
      message(WARNING "nf-config not found. Cannot build Fortran OPeNDAP examples.")
  else()
      # Get NetCDF-Fortran compile and link flags using nf-config
      execute_process(
          COMMAND ${NF_CONFIG} --fflags
          OUTPUT_VARIABLE NF_FFLAGS
          OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      execute_process(
          COMMAND ${NF_CONFIG} --flibs
          OUTPUT_VARIABLE NF_LIBS
          OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      # Convert space-separated strings to CMake lists
      separate_arguments(NF_FFLAGS_LIST UNIX_COMMAND "${NF_FFLAGS}")
      separate_arguments(NF_LIBS_LIST UNIX_COMMAND "${NF_LIBS}")

      # Get HDF5 library directory from HDF5_INCLUDE_DIR (parent scope)
      get_filename_component(HDF5_LIB_DIR "${HDF5_INCLUDE_DIR}/../lib" ABSOLUTE)

      # Build each Fortran example
      set(FORTRAN_OPENDAP_EXAMPLES
          f_opendap_simple
          f_opendap_constraint
          f_opendap_subset
      )

      foreach(example ${FORTRAN_OPENDAP_EXAMPLES})
          add_executable(${example} ${example}.f90)
          target_compile_options(${example} PRIVATE ${NF_FFLAGS_LIST})
          target_link_directories(${example} PRIVATE ${HDF5_LIB_DIR})
          target_link_libraries(${example} PRIVATE ${NF_LIBS_LIST} ${NETCDF_LIBRARIES} hdf5 hdf5_hl)
          list(APPEND OPENDAP_EXAMPLE_TARGETS ${example})
      endforeach()
  endif()
endif()

# Create custom target that depends on all examples
add_custom_target(opendap_examples DEPENDS ${OPENDAP_EXAMPLE_TARGETS})

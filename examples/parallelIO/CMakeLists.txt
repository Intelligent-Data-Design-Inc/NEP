# CMakeLists.txt for parallel I/O examples
# Only built when NEP_ENABLE_PARALLEL_TESTS is ON (Sprint 2 detection).
# Edward Hartnett, Intelligent Data Design, Inc.
# v1.9.0 Sprint 4

find_package(MPI REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(NETCDF REQUIRED netcdf)
pkg_check_modules(NETCDFF REQUIRED netcdf-fortran)

# C parallel example
add_executable(square16_par square16_par.c)
target_include_directories(square16_par PRIVATE ${MPI_C_INCLUDE_PATH} ${NETCDF_INCLUDE_DIRS})
target_link_libraries(square16_par PRIVATE ${MPI_C_LIBRARIES} ${NETCDF_LDFLAGS})
target_compile_options(square16_par PRIVATE ${NETCDF_CFLAGS_OTHER})

# Fortran parallel example (only when Fortran is enabled)
if(NEP_ENABLE_FORTRAN)
    add_executable(f_square16_par f_square16_par.f90)
    target_include_directories(f_square16_par PRIVATE ${MPI_Fortran_INCLUDE_PATH} ${NETCDF_INCLUDE_DIRS} ${NETCDFF_INCLUDE_DIRS})
    target_link_libraries(f_square16_par PRIVATE ${MPI_Fortran_LIBRARIES} ${NETCDFF_LDFLAGS} ${NETCDF_LDFLAGS})
    target_compile_options(f_square16_par PRIVATE ${NETCDFF_CFLAGS_OTHER})
endif()

# Configure the run script: substitute @MPIEXEC@ with the discovered mpiexec path.
# MPIEXEC_EXECUTABLE is set by Sprint 2 detection; expose it as MPIEXEC for configure_file.
set(MPIEXEC "${MPIEXEC_EXECUTABLE}")
configure_file(run_par_examples.sh.in run_par_examples.sh @ONLY)

# Register the shell script as a CTest test
if(BUILD_TESTING)
    add_test(
        NAME parallel_examples
        COMMAND bash run_par_examples.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    # Allow OpenMPI to run as root (GitHub Actions CI runners run as root)
    # Get NetCDF bin directory for ncdump
    get_filename_component(NETCDF_BIN ${NETCDF_INCLUDE_DIRS} DIRECTORY)
    set_tests_properties(parallel_examples PROPERTIES
        ENVIRONMENT "OMPI_ALLOW_RUN_AS_ROOT=1;OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1;PATH=${NETCDF_BIN}/bin:$ENV{PATH}"
    )
endif()

# CMakeLists.txt for parallel I/O examples
# Only built when ENABLE_PARALLEL_TESTS is ON (Sprint 2 detection).
# Edward Hartnett, Intelligent Data Design, Inc.
# v1.9.0 Sprint 3

find_package(MPI REQUIRED)

# C parallel example
add_executable(square16_par square16_par.c)
target_include_directories(square16_par PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(square16_par PRIVATE ${MPI_C_LIBRARIES})

# Fortran parallel example (only when Fortran is enabled)
if(ENABLE_FORTRAN)
    add_executable(f_square16_par f_square16_par.f90)
    target_include_directories(f_square16_par PRIVATE ${MPI_Fortran_INCLUDE_PATH})
    target_link_libraries(f_square16_par PRIVATE ${MPI_Fortran_LIBRARIES})
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
    set_tests_properties(parallel_examples PROPERTIES
        ENVIRONMENT "OMPI_ALLOW_RUN_AS_ROOT=1;OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1"
    )
endif()

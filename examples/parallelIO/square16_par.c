/**
 * @file square16_par.c
 * @brief Demonstrates parallel NetCDF-4 I/O with MPI using a 2x2 rank decomposition
 *
 * This example shows how to use NetCDF-4's parallel I/O capabilities with MPI
 * to write and read a shared dataset concurrently from multiple processes. Four
 * MPI ranks cooperate to write a single 16x16 integer array, each contributing
 * an 8x8 quadrant filled with its own rank number.
 *
 * The program demonstrates the fundamental parallel I/O pattern: open a file
 * collectively, define the full dataset shape once, then have each rank write
 * only its local portion using hyperslab selections (start/count). After writing,
 * it performs a parallel read-back to verify correctness.
 *
 * **Learning Objectives:**
 * - Open/create NetCDF files in parallel with nc_create_par() and nc_open_par()
 * - Use nc_var_par_access() to enable collective I/O mode
 * - Compute per-rank hyperslab offsets for a 2D domain decomposition
 * - Write and read variable subsets with nc_put_vara_int() / nc_get_vara_int()
 * - Verify parallel write correctness with a coordinated read-back
 *
 * **Key Concepts:**
 * - **Collective I/O**: All ranks participate in every I/O call (NC_COLLECTIVE),
 *   which allows the MPI-IO layer to optimize access patterns
 * - **Independent I/O**: Alternative mode (NC_INDEPENDENT) where each rank
 *   calls I/O functions independently; less common for performance-critical code
 * - **Domain Decomposition**: Dividing a global array among MPI ranks; here a
 *   simple 2x2 block decomposition maps rank → quadrant
 * - **Hyperslab**: A rectangular sub-region of a variable selected by start[]
 *   and count[] arrays passed to nc_put_vara / nc_get_vara functions
 * - **MPI_INFO_NULL**: Passed when no MPI-IO hints are needed; production code
 *   may supply hints (e.g., striping) via MPI_Info objects for better performance
 *
 * **Rank Layout (2×2 grid):**
 * @code
 *   col 0-7    col 8-15
 *  +----------+----------+
 *  | rank 0   | rank 2   |  rows 0-7
 *  +----------+----------+
 *  | rank 1   | rank 3   |  rows 8-15
 *  +----------+----------+
 * @endcode
 *
 * **Prerequisites:**
 * - simple_nc4.c - NetCDF-4 format basics
 * - An MPI installation and a NetCDF-C library built with parallel support
 *
 * **Related Examples:**
 * - f_square16_par.f90 - Fortran equivalent
 *
 * **Compilation:**
 * @code
 * mpicc -o square16_par square16_par.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * mpirun -n 4 ./square16_par
 * @endcode
 *
 * **Expected Output:**
 * @code
 * Parallel I/O write and read-back successful. File: square16_par.nc
 * @endcode
 *
 * @note Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to Writing
 * High-Performance Programs for Scientific Data Management, Second Edition"
 * (https://www.amazon.com/dp/B0H7Q1Z75L)
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date June 3, 2026
 */
#include <netcdf.h>
#include <netcdf_par.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define NDIMS 2
#define QUAD_SIZE 8
#define DIM_SIZE 16

/* Check NetCDF call result - standard NEP pattern */
#define ERR(e) do { \
    fprintf(stderr, "Error at line %d: %s\n", __LINE__, nc_strerror(e)); \
    MPI_Abort(MPI_COMM_WORLD, 1); \
} while(0)

int main(int argc, char **argv)
{
    int rank, size;
    int ncid, varid, dimids[NDIMS];
    int ret;
    int data[QUAD_SIZE][QUAD_SIZE];
    size_t start[NDIMS], count[NDIMS];
    int read_data[QUAD_SIZE][QUAD_SIZE];
    int i, j;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Require exactly 4 processes for 2x2 grid */
    if (size != 4) {
        if (rank == 0) {
            fprintf(stderr, "Error: This program requires exactly 4 MPI processes, got %d\n", size);
        }
        MPI_Finalize();
        return 1;
    }

    /* Fill 8x8 array with rank number */
    for (i = 0; i < QUAD_SIZE; i++) {
        for (j = 0; j < QUAD_SIZE; j++) {
            data[i][j] = rank;
        }
    }

    /* Create file with parallel NetCDF-4 */
    if ((ret = nc_create_par("square16_par.nc", NC_NETCDF4 | NC_MPIIO,
			     MPI_COMM_WORLD, MPI_INFO_NULL, &ncid)))
	ERR(ret);

    /* Define dimensions: i (rows) and j (cols), each length 16 */
    if ((ret = nc_def_dim(ncid, "i", DIM_SIZE, &dimids[0])))
	ERR(ret);
    if ((ret = nc_def_dim(ncid, "j", DIM_SIZE, &dimids[1])))
	ERR(ret);

    /* Define variable sample_data(i,j) as integer */
    if ((ret = nc_def_var(ncid, "sample_data", NC_INT, NDIMS, dimids, &varid)))
	ERR(ret);

    /* End define mode */
    if ((ret = nc_enddef(ncid)))
	ERR(ret);

    /* Enable collective I/O for this variable */
    if ((ret = nc_var_par_access(ncid, varid, NC_COLLECTIVE)))
	ERR(ret);

    /* Calculate start position based on rank:
     * rank 0: (0, 0)   rank 1: (0, 8)
     * rank 2: (8, 0)   rank 3: (8, 8) */
    start[0] = (rank / 2) * QUAD_SIZE;  /* row offset */
    start[1] = (rank % 2) * QUAD_SIZE;  /* column offset */
    count[0] = QUAD_SIZE;
    count[1] = QUAD_SIZE;

    /* Collective write - all ranks must participate */
    if ((ret = nc_put_vara_int(ncid, varid, start, count, &data[0][0])))
	ERR(ret);

    /* Close file */
    if ((ret = nc_close(ncid)))
	ERR(ret);

    /* Parallel read-back verification */
    if ((ret = nc_open_par("square16_par.nc", NC_MPIIO,
			   MPI_COMM_WORLD, MPI_INFO_NULL, &ncid)))
	ERR(ret);
    if ((ret = nc_inq_varid(ncid, "sample_data", &varid)))
	ERR(ret);
    if ((ret = nc_var_par_access(ncid, varid, NC_COLLECTIVE)))
	ERR(ret);
    if ((ret = nc_get_vara_int(ncid, varid, start, count, &read_data[0][0])))
	ERR(ret);
    if ((ret = nc_close(ncid)))
	ERR(ret);

    /* Verify data */
    for (i = 0; i < QUAD_SIZE; i++) {
        for (j = 0; j < QUAD_SIZE; j++) {
            if (read_data[i][j] != rank) {
                fprintf(stderr, "Rank %d: Data mismatch at (%zu,%zu): expected %d, got %d\n",
                        rank, start[0] + i, start[1] + j, rank, read_data[i][j]);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }

    if (rank == 0) {
        printf("Parallel I/O write and read-back successful. File: square16_par.nc\n");
    }

    MPI_Finalize();
    return 0;
}

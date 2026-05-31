/* square16_par.c - Parallel NetCDF I/O example: 4 ranks write 16x16 dataset.
 * Each rank writes an 8x8 quadrant filled with its rank number.
 * Edward Hartnett, Intelligent Data Design, Inc.
 * v1.9.0 Sprint 4 */
#include <netcdf.h>
#include <netcdf_par.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define NDIMS 2
#define QUAD_SIZE 8
#define DIM_SIZE 16

/* Check NetCDF call result - standard NEP pattern */
#define NC_CHK(err) do { \
    if ((err) != NC_NOERR) { \
        fprintf(stderr, "Error at line %d: %s\n", __LINE__, nc_strerror(err)); \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    } \
} while(0)

int main(int argc, char **argv)
{
    int rank, size;
    int ncid, varid, dimids[NDIMS];
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
    NC_CHK(nc_create_par("square16_par.nc", NC_NETCDF4 | NC_MPIIO,
                         MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));

    /* Define dimensions: i (rows) and j (cols), each length 16 */
    NC_CHK(nc_def_dim(ncid, "i", DIM_SIZE, &dimids[0]));
    NC_CHK(nc_def_dim(ncid, "j", DIM_SIZE, &dimids[1]));

    /* Define variable sample_data(i,j) as integer */
    NC_CHK(nc_def_var(ncid, "sample_data", NC_INT, NDIMS, dimids, &varid));

    /* End define mode */
    NC_CHK(nc_enddef(ncid));

    /* Enable collective I/O for this variable */
    NC_CHK(nc_var_par_access(ncid, varid, NC_COLLECTIVE));

    /* Calculate start position based on rank:
     * rank 0: (0, 0)   rank 1: (8, 0)
     * rank 2: (0, 8)   rank 3: (8, 8) */
    start[0] = (rank / 2) * QUAD_SIZE;  /* row offset */
    start[1] = (rank % 2) * QUAD_SIZE;  /* column offset */
    count[0] = QUAD_SIZE;
    count[1] = QUAD_SIZE;

    /* Collective write - all ranks must participate */
    NC_CHK(nc_put_vara_int(ncid, varid, start, count, &data[0][0]));

    /* Close file */
    NC_CHK(nc_close(ncid));

    /* Parallel read-back verification */
    NC_CHK(nc_open_par("square16_par.nc", NC_MPIIO,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &ncid));
    NC_CHK(nc_inq_varid(ncid, "sample_data", &varid));
    NC_CHK(nc_var_par_access(ncid, varid, NC_COLLECTIVE));
    NC_CHK(nc_get_vara_int(ncid, varid, start, count, &read_data[0][0]));
    NC_CHK(nc_close(ncid));

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

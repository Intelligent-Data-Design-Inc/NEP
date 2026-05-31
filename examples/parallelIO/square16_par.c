/* square16_par.c - Parallel I/O hello: initialize MPI, print rank and size, exit 0.
 * Edward Hartnett, Intelligent Data Design, Inc.
 * v1.9.0 Sprint 3 */
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d\n", rank, size);

    MPI_Finalize();
    return 0;
}

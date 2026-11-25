/* tst_nextcdf4_parallel.c - parallel I/O tests for NEXTCDF-4.
 *
 * Runs under mpiexec (see test/nextcdf4/CMakeLists.txt): all ranks
 * create a file collectively with nc_create_par, write one element
 * each, then reopen with nc_open_par and verify every rank's data.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define FILE_NAME "tst_nextcdf4_parallel.nc"
#define DIM_NAME "d"
#define VAR_NAME "v"

int
main(int argc, char **argv)
{
    int rank, nprocs;
    int ncid, dimid, varid;
    int formatx, mode;
    int i, ret;
    int *values;
    size_t start, count;

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
        return 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (!NC_NEXTCDF4_initialize()) {
        fprintf(stderr, "rank %d: NC_NEXTCDF4_initialize failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Collective create: nc_create_par requires NC_NETCDF4 in the
     * mode (upstream rejects other creates without PnetCDF); the UDF
     * flag still selects the NEXTCDF-4 dispatcher. */
    if (!rank) unlink(FILE_NAME);
    MPI_Barrier(MPI_COMM_WORLD);
    ret = nc_create_par(FILE_NAME, NC_NEXTCDF4 | NC_NETCDF4 | NC_CLOBBER,
                        MPI_COMM_WORLD, MPI_INFO_NULL, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "rank %d: nc_create_par: %s\n", rank, nc_strerror(ret));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (nc_def_dim(ncid, DIM_NAME, (size_t)nprocs, &dimid) ||
        nc_def_var(ncid, VAR_NAME, NC_INT, 1, &dimid, &varid) ||
        nc_enddef(ncid)) {
        fprintf(stderr, "rank %d: metadata definition failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Each rank writes its rank number at its own index; the default
     * transfer mode is independent I/O. */
    start = (size_t)rank;
    count = 1;
    if (nc_put_vara_int(ncid, varid, &start, &count, &rank) ||
        nc_close(ncid)) {
        fprintf(stderr, "rank %d: parallel write/close failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Reopen collectively and have every rank verify the full record. */
    ret = nc_open_par(FILE_NAME, NC_NEXTCDF4,
                      MPI_COMM_WORLD, MPI_INFO_NULL, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "rank %d: nc_open_par: %s\n", rank, nc_strerror(ret));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (nc_inq_format_extended(ncid, &formatx, &mode) ||
        formatx != NC_FORMATX_UDF9 || !(mode & NC_NEXTCDF4) ||
        nc_inq_varid(ncid, VAR_NAME, &varid)) {
        fprintf(stderr, "rank %d: reopened file is not NEXTCDF-4\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (!(values = malloc((size_t)nprocs * sizeof(int))))
        MPI_Abort(MPI_COMM_WORLD, 1);
    start = 0;
    count = (size_t)nprocs;
    if (nc_get_vara_int(ncid, varid, &start, &count, values)) {
        fprintf(stderr, "rank %d: parallel read failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < nprocs; i++)
        if (values[i] != i) {
            fprintf(stderr, "rank %d: values[%d] = %d, expected %d\n",
                    rank, i, values[i], i);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

    if (nc_close(ncid))
        MPI_Abort(MPI_COMM_WORLD, 1);

    /* Read/write reopen: each rank multiplies its slot by 10, syncs,
     * and verifies on a second open in the same session. */
    ret = nc_open_par(FILE_NAME, NC_NEXTCDF4 | NC_WRITE,
                      MPI_COMM_WORLD, MPI_INFO_NULL, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "rank %d: nc_open_par NC_WRITE: %s\n", rank,
                nc_strerror(ret));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (nc_inq_varid(ncid, VAR_NAME, &varid)) {
        fprintf(stderr, "rank %d: varid lookup failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    values[0] = rank * 10;
    start = (size_t)rank;
    count = 1;
    if (nc_put_vara_int(ncid, varid, &start, &count, values) ||
        nc_sync(ncid) || nc_close(ncid)) {
        fprintf(stderr, "rank %d: parallel rewrite failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    ret = nc_open_par(FILE_NAME, NC_NEXTCDF4,
                      MPI_COMM_WORLD, MPI_INFO_NULL, &ncid);
    if (ret != NC_NOERR ||
        nc_inq_varid(ncid, VAR_NAME, &varid)) {
        fprintf(stderr, "rank %d: second reopen failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    start = 0;
    count = (size_t)nprocs;
    if (nc_get_vara_int(ncid, varid, &start, &count, values)) {
        fprintf(stderr, "rank %d: verification read failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < nprocs; i++)
        if (values[i] != i * 10) {
            fprintf(stderr, "rank %d: values[%d] = %d, expected %d\n",
                    rank, i, values[i], i * 10);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    if (nc_close(ncid)) {
        fprintf(stderr, "rank %d: final close failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    free(values);

    if (!rank)
        printf("OK: NEXTCDF-4 parallel create/open with %d ranks\n", nprocs);
    MPI_Barrier(MPI_COMM_WORLD);
    if (!rank) unlink(FILE_NAME);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}

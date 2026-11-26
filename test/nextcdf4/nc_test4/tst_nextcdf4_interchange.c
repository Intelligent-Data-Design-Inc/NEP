/* Copyright 2026, Intelligent Data Design, Inc.
 *
 * This program tests bidirectional parallel interchange between the
 * NEXTCDF-4 backend and upstream netcdf-c's built-in netCDF-4/HDF5
 * dispatcher, on the same libnetcdf in one MPI program:
 *
 *   - NEXTCDF-4 writes a file collectively; all ranks then read and
 *     verify it through the upstream dispatcher (no NC_NEXTCDF4 flag).
 *   - Upstream netcdf-c writes a file collectively; all ranks then
 *     read and verify it through NEXTCDF-4 (marker-less open path).
 *   - Rank 0 additionally verifies both files through serial opens in
 *     each direction.
 *
 * Only standard netCDF-4 types are exchanged; native-only NEXTCDF-4
 * types are not claimed interchangeable.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <nc_tests.h>
#include "err_macros.h"
#include <mpi.h>
#include <netcdf_par.h>

#define FILE_NEP "tst_nextcdf4_interchange_nep.nc"
#define FILE_UP "tst_nextcdf4_interchange_up.nc"
#define NDIMS 3
#define DIMSIZE 24
#define NUM_SLABS 4
#define NUM_PROC 4
#define FILLER -777

int
main(int argc, char **argv)
{
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    int ncid, v1id, v2id, dimids[NDIMS];
    size_t start[NDIMS], count[NDIMS];
    int i, s, res;
    int slab_data[DIMSIZE * DIMSIZE / NUM_PROC];
    int slab_in[DIMSIZE * DIMSIZE / NUM_PROC];
    float fdata[DIMSIZE];
    float fdata_in[DIMSIZE];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_size != NUM_PROC) ERR;

    if (!mpi_rank)
        printf("\n*** Testing NEXTCDF-4 <-> upstream parallel interchange.\n");

    for (i = 0; i < DIMSIZE * DIMSIZE / NUM_PROC; i++)
        slab_data[i] = mpi_rank;
    for (i = 0; i < DIMSIZE; i++)
        fdata[i] = mpi_rank + i * 0.5f;

    /* ==== Round 1: NEXTCDF-4 writes, upstream reads ==== */
    if (!mpi_rank)
        printf("*** NEXTCDF-4 collective write, upstream parallel read...");

    if ((res = nc_create_par(FILE_NEP, nep_nc_test4_par_create_mode(),
                             comm, info, &ncid))) ERR;
    if (nc_def_dim(ncid, "d1", DIMSIZE, dimids)) ERR;
    if (nc_def_dim(ncid, "d2", DIMSIZE, &dimids[1])) ERR;
    if (nc_def_dim(ncid, "d3", NUM_SLABS, &dimids[2])) ERR;
    if ((res = nc_def_var(ncid, "v1", NC_INT, NDIMS, dimids, &v1id))) ERR;
    if ((res = nc_def_var(ncid, "v2", NC_FLOAT, 1, dimids, &v2id))) ERR;
    /* A built-in filtered variable exercises interchange of the HDF5
     * filter pipeline; defining a filter forces collective access on
     * the variable in a parallel session. */
    if (nc_def_var_deflate(ncid, v2id, 1, 1, 1)) ERR;
    if ((res = nc_enddef(ncid))) ERR;

    start[0] = mpi_rank * DIMSIZE / mpi_size;
    start[1] = 0;
    count[0] = DIMSIZE / mpi_size;
    count[1] = DIMSIZE;
    count[2] = 1;

    if (nc_var_par_access(ncid, v1id, NC_COLLECTIVE)) ERR;
    for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
        if (nc_put_vara_int(ncid, v1id, start, count, slab_data)) ERR;

    /* v2 is filtered, so its access preference was forced collective
     * at define time. Each rank writes its own row. */
    start[0] = mpi_rank * DIMSIZE / mpi_size;
    count[0] = DIMSIZE / mpi_size;
    count[1] = 1;
    if (nc_put_vara_float(ncid, v2id, start, count, fdata)) ERR;

    if ((res = nc_close(ncid))) ERR;
    MPI_Barrier(comm);

    /* All ranks reopen through the upstream built-in netCDF-4/HDF5
     * dispatcher (no NC_NEXTCDF4 flag) and verify the data. */
    if ((res = nc_open_par(FILE_NEP, NC_NOWRITE, comm, info, &ncid))) ERR;
    if (nc_inq_varid(ncid, "v1", &v1id)) ERR;
    if (nc_inq_varid(ncid, "v2", &v2id)) ERR;

    start[0] = mpi_rank * DIMSIZE / mpi_size;
    start[1] = 0;
    count[0] = DIMSIZE / mpi_size;
    count[1] = DIMSIZE;
    count[2] = 1;
    for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
    {
        if (nc_get_vara_int(ncid, v1id, start, count, slab_in)) ERR;
        for (i = 0; i < DIMSIZE * DIMSIZE / NUM_PROC; i++)
            if (slab_in[i] != mpi_rank) ERR;
    }

    /* Every rank reads the whole filtered v2 and checks every row. */
    for (s = 0; s < mpi_size; s++)
    {
        start[0] = s * DIMSIZE / mpi_size;
        if (nc_get_vara_float(ncid, v2id, start, count, fdata_in)) ERR;
        for (i = 0; i < DIMSIZE / mpi_size; i++)
            if (fdata_in[i] != s + i * 0.5f) ERR;
    }
    if ((res = nc_close(ncid))) ERR;

    if (!mpi_rank)
    {
        /* Serial upstream read on rank 0. */
        if ((res = nc_open(FILE_NEP, NC_NOWRITE, &ncid))) ERR;
        if (nc_inq_varid(ncid, "v1", &v1id)) ERR;
        start[0] = 0; start[1] = 0; start[2] = 0;
        count[0] = DIMSIZE; count[1] = DIMSIZE; count[2] = 1;
        {
            int *whole = malloc(DIMSIZE * DIMSIZE * sizeof(int));
            if (!whole) ERR;
            for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
            {
                if (nc_get_vara_int(ncid, v1id, start, count, whole)) ERR;
                for (s = 0; s < mpi_size; s++)
                    for (i = 0; i < DIMSIZE * DIMSIZE / mpi_size; i++)
                        if (whole[s * DIMSIZE * DIMSIZE / mpi_size + i] != s) ERR;
            }
            free(whole);
        }
        if (nc_close(ncid)) ERR;
    }
    MPI_Barrier(comm);
    if (!mpi_rank)
        SUMMARIZE_ERR;

    /* ==== Round 2: upstream writes, NEXTCDF-4 reads ==== */
    if (!mpi_rank)
        printf("*** upstream collective write, NEXTCDF-4 parallel read...");

    if ((res = nc_create_par(FILE_UP, NC_NETCDF4|NC_CLOBBER,
                             comm, info, &ncid))) ERR;
    if (nc_def_dim(ncid, "d1", DIMSIZE, dimids)) ERR;
    if (nc_def_dim(ncid, "d2", DIMSIZE, &dimids[1])) ERR;
    if (nc_def_dim(ncid, "d3", NUM_SLABS, &dimids[2])) ERR;
    if ((res = nc_def_var(ncid, "v1", NC_INT, NDIMS, dimids, &v1id))) ERR;
    if ((res = nc_def_var(ncid, "v2", NC_FLOAT, 1, dimids, &v2id))) ERR;
    if (nc_def_var_deflate(ncid, v2id, 1, 1, 1)) ERR;
    if ((res = nc_enddef(ncid))) ERR;

    start[0] = mpi_rank * DIMSIZE / mpi_size;
    start[1] = 0;
    count[0] = DIMSIZE / mpi_size;
    count[1] = DIMSIZE;
    count[2] = 1;
    if (nc_var_par_access(ncid, v1id, NC_COLLECTIVE)) ERR;
    if (nc_var_par_access(ncid, v2id, NC_COLLECTIVE)) ERR;
    for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
        if (nc_put_vara_int(ncid, v1id, start, count, slab_data)) ERR;
    count[1] = 1;
    if (nc_put_vara_float(ncid, v2id, start, count, fdata)) ERR;
    if ((res = nc_close(ncid))) ERR;
    MPI_Barrier(comm);

    /* Reopen through NEXTCDF-4: the file carries no _Nextcdf4Backend
     * marker and must open as a plain upstream netCDF-4 file. */
    if ((res = nc_open_par(FILE_UP, nep_nc_test4_par_open_mode()|NC_NOWRITE,
                           comm, info, &ncid))) ERR;
    if (nc_inq_varid(ncid, "v1", &v1id)) ERR;
    if (nc_inq_varid(ncid, "v2", &v2id)) ERR;

    /* The filter pipeline was defined by upstream; the reopened
     * variable defaults to independent, so select collective
     * explicitly per the NEXTCDF-4 filtered-read contract. */
    if (nc_var_par_access(ncid, v1id, NC_COLLECTIVE)) ERR;
    if (nc_var_par_access(ncid, v2id, NC_COLLECTIVE)) ERR;

    start[0] = mpi_rank * DIMSIZE / mpi_size;
    start[1] = 0;
    count[0] = DIMSIZE / mpi_size;
    count[1] = DIMSIZE;
    count[2] = 1;
    for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
    {
        if (nc_get_vara_int(ncid, v1id, start, count, slab_in)) ERR;
        for (i = 0; i < DIMSIZE * DIMSIZE / NUM_PROC; i++)
            if (slab_in[i] != mpi_rank) ERR;
    }
    for (s = 0; s < mpi_size; s++)
    {
        start[0] = s * DIMSIZE / mpi_size;
        if (nc_get_vara_float(ncid, v2id, start, count, fdata_in)) ERR;
        for (i = 0; i < DIMSIZE / mpi_size; i++)
            if (fdata_in[i] != s + i * 0.5f) ERR;
    }
    if ((res = nc_close(ncid))) ERR;

    if (!mpi_rank)
    {
        /* Serial NEXTCDF-4 read on rank 0 of the upstream file. */
        if ((res = nc_open(FILE_UP, NC_NEXTCDF4|NC_NOWRITE, &ncid))) ERR;
        if (nc_inq_varid(ncid, "v1", &v1id)) ERR;
        {
            int *whole = malloc(DIMSIZE * DIMSIZE * sizeof(int));
            if (!whole) ERR;
            start[0] = 0; start[1] = 0;
            count[0] = DIMSIZE; count[1] = DIMSIZE; count[2] = 1;
            for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
            {
                if (nc_get_vara_int(ncid, v1id, start, count, whole)) ERR;
                for (s = 0; s < mpi_size; s++)
                    for (i = 0; i < DIMSIZE * DIMSIZE / mpi_size; i++)
                        if (whole[s * DIMSIZE * DIMSIZE / mpi_size + i] != s) ERR;
            }
            free(whole);
        }
        if (nc_close(ncid)) ERR;
    }
    MPI_Barrier(comm);
    if (!mpi_rank)
        SUMMARIZE_ERR;

    /* Clean up on rank 0 only, after all ranks have closed. */
    if (!mpi_rank)
    {
        remove(FILE_NEP);
        remove(FILE_UP);
    }
    MPI_Barrier(comm);
    MPI_Finalize();

    if (!mpi_rank)
        FINAL_RESULTS;
    return 0;
}

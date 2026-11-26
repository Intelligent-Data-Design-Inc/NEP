/* Copyright 2026, Intelligent Data Design, Inc.
 *
 * This program tests NEXTCDF-4 parallel I/O with external HDF5 filter
 * plugins. HDF5_PLUGIN_PATH must point at a directory holding the
 * zstd (32015) plugin; its absence is a test failure because the
 * directory is explicitly configured. The bzip2 (307) case is skipped
 * with a notice if that plugin is not found, since it is not
 * guaranteed by the CI dependency stack.
 *
 * For each plugin: a filtered variable is defined (which forces
 * collective access in a parallel session), written and read back on
 * four ranks, then verified after a collective reopen. Independent
 * access on a plugin-filtered variable must be rejected.
 *
 * Author: Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <nc_tests.h>
#include "err_macros.h"
#include <mpi.h>
#include <netcdf_par.h>
#include <netcdf_filter.h>
#include <hdf5.h>

#define FILE_NAME "tst_nextcdf4_parallel_plugin.nc"
#define DIM_NAME "d1"
#define DIMSIZE 24
#define NUM_PROC 4
#define ZSTD_LEVEL 4
#define BZIP2_LEVEL 4

/* True when HDF5 reports the filter available (plugin found or
 * built in). */
static int
filter_avail(unsigned int id)
{
    return H5Zfilter_avail((H5Z_filter_t)id) > 0;
}

int
main(int argc, char **argv)
{
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;
    int ncid, dimid, varid, varid2;
    size_t start[1], count[1];
    int i, res;
    float data[DIMSIZE / NUM_PROC];
    float data_in[DIMSIZE / NUM_PROC];
    int filters[] = {H5Z_FILTER_ZSTD, H5Z_FILTER_BZIP2};
    const char *names[] = {"zstd", "bzip2"};
    int f;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_size != NUM_PROC) ERR;

    if (!mpi_rank)
        printf("\n*** Testing NEXTCDF-4 parallel I/O with external filter plugins.\n");

    for (i = 0; i < DIMSIZE / NUM_PROC; i++)
        data[i] = mpi_rank + i * 0.1f;

    for (f = 0; f < 2; f++)
    {
        int avail;

        /* All ranks must agree on availability; H5Zfilter_avail
         * depends on each rank's plugin search path. */
        avail = filter_avail(filters[f]);
        if (!avail)
        {
            if (filters[f] == H5Z_FILTER_ZSTD)
            {
                /* zstd is required: the plugin directory was
                 * explicitly configured for this test. */
                if (!mpi_rank)
                    printf("zstd plugin not found in HDF5_PLUGIN_PATH - failing\n");
                ERR;
            }
            if (!mpi_rank)
                printf("*** skipping %s: plugin not present\n", names[f]);
            continue;
        }

        if (!mpi_rank)
            printf("*** testing %s plugin with parallel I/O...", names[f]);

        /* Create with a plugin-filtered var and an unfiltered var in
         * the same file. */
        if ((res = nc_create_par(FILE_NAME, nep_nc_test4_par_create_mode()|NC_CLOBBER,
                                 comm, info, &ncid))) ERR;
        if (nc_def_dim(ncid, DIM_NAME, DIMSIZE, &dimid)) ERR;
        if ((res = nc_def_var(ncid, "filtered", NC_FLOAT, 1, &dimid, &varid))) ERR;
        if ((res = nc_def_var(ncid, "plain", NC_INT, 1, &dimid, &varid2))) ERR;
        if (filters[f] == H5Z_FILTER_ZSTD)
        {
            if (nc_def_var_zstandard(ncid, varid, ZSTD_LEVEL)) ERR;
        }
        else
        {
            if (nc_def_var_bzip2(ncid, varid, BZIP2_LEVEL)) ERR;
        }

        /* Defining the filter forced collective access; independent
         * selection on the filtered var must be rejected. */
        if (nc_var_par_access(ncid, varid, NC_INDEPENDENT) != NC_EINVAL) ERR;

        if (nc_enddef(ncid)) ERR;

        /* Collective write of each rank's row. */
        start[0] = mpi_rank * DIMSIZE / mpi_size;
        count[0] = DIMSIZE / mpi_size;
        if (nc_put_vara_float(ncid, varid, start, count, data)) ERR;
        {
            int idata[DIMSIZE / NUM_PROC];
            for (i = 0; i < DIMSIZE / NUM_PROC; i++)
                idata[i] = mpi_rank * 100 + i;
            if (nc_put_vara_int(ncid, varid2, start, count, idata)) ERR;
        }
        if ((res = nc_close(ncid))) ERR;
        MPI_Barrier(comm);

        /* Reopen in parallel; the plugin-filtered var loads its filter
         * metadata, defaults to independent, so collective must be
         * selected explicitly. */
        if ((res = nc_open_par(FILE_NAME, nep_nc_test4_par_open_mode()|NC_NOWRITE,
                               comm, info, &ncid))) ERR;
        if (nc_inq_varid(ncid, "filtered", &varid)) ERR;
        if (nc_var_par_access(ncid, varid, NC_INDEPENDENT) != NC_EINVAL) ERR;
        if (nc_var_par_access(ncid, varid, NC_COLLECTIVE)) ERR;

        start[0] = mpi_rank * DIMSIZE / mpi_size;
        count[0] = DIMSIZE / mpi_size;
        if (nc_get_vara_float(ncid, varid, start, count, data_in)) ERR;
        for (i = 0; i < DIMSIZE / NUM_PROC; i++)
            if (data_in[i] != mpi_rank + i * 0.1f) ERR;
        if ((res = nc_close(ncid))) ERR;
        MPI_Barrier(comm);

        if (!mpi_rank)
        {
            remove(FILE_NAME);
            SUMMARIZE_ERR;
        }
    }

    MPI_Finalize();

    if (!mpi_rank)
        FINAL_RESULTS;
    return 0;
}

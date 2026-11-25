/**
 * @file parallel_perf.c
 * @brief Benchmark four-rank collective NEXTCDF-4 write and read
 *        throughput with disk-bound timing.
 *
 * Each rank owns one contiguous slab of a two-dimensional
 * @c NC_FLOAT variable and transfers it with
 * nc_put_vara_float()/nc_get_vara_float() under the default
 * (collective for filtered variables, independent otherwise)
 * parallel access. Two storage configurations are timed: contiguous
 * and chunked+shuffle+deflate level 1 (the deflate rows force
 * NC_COLLECTIVE). Every read is checked bit-for-bit against the data
 * written.
 *
 * Write time covers nc_create_par() through nc_close() plus a
 * rank-0 fsync() of the file (nep_perf_sync_file()), so data has
 * reached the storage device; before each read rank 0 drops the file
 * from the page cache (nep_perf_drop_cache(), untimed) and read time
 * covers nc_open_par() through nc_close(). Each phase ends with an
 * MPI_Allreduce so the reported number is the slowest rank's wall
 * time (MPI_Wtime). @c NREPS (10) repetitions run per configuration
 * and the minimum times are kept.
 *
 * **Output:** CSV written to the path given on the command line, with
 * columns:
 *   @code
 *   ranks,storage,slab_rows,cols,uncompressed_bytes,file_bytes,write_s,read_s,write_MBps,read_MBps
 *   @endcode
 *   Each row is also printed to stdout on rank 0.
 *
 * **Typical workflow:**
 * @code
 *   mpiexec -n 4 ./parallel_perf parallel_results.csv
 *   ./record_parallel_machine.sh build parallel_machine.txt
 * @endcode
 *
 * @note The program is intended for local performance profiling. It is
 *       registered as a ctest test only when NEP_ENABLE_BENCHMARKS=ON
 *       together with NEP_ENABLE_NEXTCDF4_PARALLEL.
 *       Reference results are measured with
 *       `-DCMAKE_BUILD_TYPE=RelWithDebInfo` (-O2).
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>
#include <nep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "perf_disk.h"

/** Slab rows written by each rank. */
#define ROWS 512
/** Columns in the variable. */
#define NX 1536
/** Repetitions per configuration; the minimum time is kept. */
#define NREPS 10
/** Temporary NetCDF file created for each measurement. */
#define TMP_FILE "parallel_perf_tmp.nc"

#define ERRCODE 2
#define ERR(e) {fprintf(stderr, "rank %d: %s\n", rank, nc_strerror(e)); \
                MPI_Abort(MPI_COMM_WORLD, ERRCODE);}

/** Number of storage configurations: contiguous, deflate1. */
#define NCONFIGS 2

/**
 * @brief One benchmark row for one storage configuration.
 */
typedef struct {
    const char *storage;   /**< "contiguous" or "deflate1". */
    long long file_bytes;  /**< File size from the last repetition. */
    double write_s;        /**< Minimum collective write time. */
    double read_s;         /**< Minimum collective read time. */
} row_t;

/**
 * @brief Write and read the slab in one configuration NREPS times.
 *
 * For each repetition all ranks create @c TMP_FILE with
 * nc_create_par(NC_NEXTCDF4 | NC_NETCDF4 | NC_CLOBBER) — NC_NETCDF4 is
 * required because upstream nc_create_par() gates on it — define
 * dims @c row = mpi_size*ROWS and @c col = NX and one
 * NC_FLOAT variable, collectively write each rank's slab, close and
 * fsync (timed), then drop the page cache (untimed) and collectively
 * read the slab back (timed). The deflate1 configuration is chunked
 * ROWSxNX with shuffle+deflate level 1, which forces collective
 * access.
 *
 * @param comm  MPI communicator.
 * @param rank  This rank.
 * @param size  Number of ranks.
 * @param deflate Non-zero for shuffle+deflate level 1.
 * @param data  This rank's slab (ROWS*NX floats).
 * @param rbuf  Read-back slab (ROWS*NX floats).
 * @param row   Result row to fill in.
 * @return 0 on success, 1 on verification failure.
 */
static int
run_config(MPI_Comm comm, int rank, int size, int deflate,
           const float *data, float *rbuf, row_t *row)
{
    int rep;
    struct stat st;

    row->storage = deflate ? "deflate1" : "contiguous";
    row->write_s = 1e30;
    row->read_s = 1e30;
    row->file_bytes = 0;

    for (rep = 0; rep < NREPS; rep++)
    {
        int ncid, varid, dimids[2];
        size_t chunksizes[2] = {ROWS, NX};
        size_t start[2] = {(size_t)rank * ROWS, 0};
        size_t count[2] = {ROWS, NX};
        double t0, local_s, write_s, read_s;
        int ret;

        /* --- Write pass, collective. ------------------------------ */
        MPI_Barrier(comm);
        t0 = MPI_Wtime();

        if ((ret = nc_create_par(TMP_FILE, NC_NEXTCDF4 | NC_NETCDF4 | NC_CLOBBER,
                                 comm, MPI_INFO_NULL, &ncid))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "row", (size_t)size * ROWS, &dimids[0]))) ERR(ret);
        if ((ret = nc_def_dim(ncid, "col", NX, &dimids[1]))) ERR(ret);
        if ((ret = nc_def_var(ncid, "v", NC_FLOAT, 2, dimids, &varid))) ERR(ret);
        if ((ret = nc_def_var_fill(ncid, varid, NC_NOFILL, NULL))) ERR(ret);
        if (deflate)
        {
            if ((ret = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizes))) ERR(ret);
            if ((ret = nc_def_var_deflate(ncid, varid, 1, 1, 1))) ERR(ret);
        }
        if ((ret = nc_enddef(ncid))) ERR(ret);
        if ((ret = nc_put_vara_float(ncid, varid, start, count, data))) ERR(ret);
        if ((ret = nc_close(ncid))) ERR(ret);

        /* Rank 0 pushes the file to storage inside the timed region;
         * the barrier keeps every rank's timer honest. */
        if (!rank && nep_perf_sync_file(TMP_FILE))
        {
            perror("fsync");
            return 1;
        }
        MPI_Barrier(comm);
        local_s = MPI_Wtime() - t0;
        MPI_Allreduce(&local_s, &write_s, 1, MPI_DOUBLE, MPI_MAX, comm);
        if (write_s < row->write_s)
            row->write_s = write_s;

        if (!rank)
        {
            if (stat(TMP_FILE, &st) != 0)
            {
                perror("stat");
                return 1;
            }
            row->file_bytes = (long long)st.st_size;
            /* Start the read from the device, not the page cache. */
            if (nep_perf_drop_cache(TMP_FILE))
            {
                perror("drop cache");
                return 1;
            }
        }
        MPI_Barrier(comm);

        /* --- Read pass, collective. ------------------------------- */
        t0 = MPI_Wtime();

        if ((ret = nc_open_par(TMP_FILE, NC_NEXTCDF4 | NC_NETCDF4 | NC_NOWRITE,
                               comm, MPI_INFO_NULL, &ncid))) ERR(ret);
        if ((ret = nc_inq_varid(ncid, "v", &varid))) ERR(ret);
        if ((ret = nc_get_vara_float(ncid, varid, start, count, rbuf))) ERR(ret);
        if ((ret = nc_close(ncid))) ERR(ret);

        MPI_Barrier(comm);
        local_s = MPI_Wtime() - t0;
        MPI_Allreduce(&local_s, &read_s, 1, MPI_DOUBLE, MPI_MAX, comm);
        if (read_s < row->read_s)
            row->read_s = read_s;

        /* Check the bits read against the bits written. */
        if (memcmp(data, rbuf, (size_t)ROWS * NX * sizeof(float)))
        {
            fprintf(stderr, "rank %d: %s data did not round-trip\n",
                    rank, row->storage);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Main entry point.
 *
 * Fills each rank's slab with a rank-identifying pattern, runs the
 * contiguous and deflate1 configurations and writes one CSV row per
 * configuration on rank 0.
 *
 * @param argc Argument count; must be 2.
 * @param argv argv[1] is the output CSV path.
 * @return 0 on success, 1 on verification failure, 2 on usage or
 *         NetCDF error.
 */
int
main(int argc, char **argv)
{
    float *data, *rbuf;
    row_t rows[NCONFIGS];
    FILE *fp;
    int c, rank, size, fail = 0;
    size_t i;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2)
    {
        if (!rank)
            fprintf(stderr, "usage: parallel_perf <results.csv>\n");
        MPI_Finalize();
        return 2;
    }
    if (!rank && size != 4)
        fprintf(stderr, "warning: reference results use 4 ranks; "
                "this run uses %d\n", size);

#ifndef __OPTIMIZE__
    if (!rank)
        fprintf(stderr, "warning: built without compiler optimization; "
                "timings are not representative (use -DCMAKE_BUILD_TYPE=RelWithDebInfo)\n");
#endif

    if (!(data = malloc((size_t)ROWS * NX * sizeof(float))) ||
        !(rbuf = malloc((size_t)ROWS * NX * sizeof(float))))
    {
        fprintf(stderr, "rank %d: memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < (size_t)ROWS * NX; i++)
        data[i] = (float)(rank * 1000000 + (int)i % 7919) * 0.25f;

    /* Silence NEXTCDF-4 lifecycle messages on stderr. */
    nep_set_log_level(-1);

    for (c = 0; c < NCONFIGS; c++)
        if (run_config(MPI_COMM_WORLD, rank, size, c, data, rbuf, &rows[c]))
            fail = 1;

    if (!rank)
    {
        double uncompressed = (double)size * ROWS * NX * sizeof(float);

        if (!(fp = fopen(argv[1], "w")))
        {
            fprintf(stderr, "cannot open %s for writing\n", argv[1]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fprintf(fp, "ranks,storage,slab_rows,cols,uncompressed_bytes,file_bytes,write_s,read_s,write_MBps,read_MBps\n");
        printf("ranks,storage,slab_rows,cols,uncompressed_bytes,file_bytes,write_s,read_s,write_MBps,read_MBps\n");
        for (c = 0; c < NCONFIGS; c++)
        {
            char line[256];

            snprintf(line, sizeof(line), "%d,%s,%d,%d,%.0f,%lld,%.6f,%.6f,%.3f,%.3f",
                     size, rows[c].storage, ROWS, NX, uncompressed,
                     rows[c].file_bytes, rows[c].write_s, rows[c].read_s,
                     uncompressed / 1e6 / rows[c].write_s,
                     uncompressed / 1e6 / rows[c].read_s);
            fprintf(fp, "%s\n", line);
            printf("%s\n", line);
        }
        fclose(fp);
        remove(TMP_FILE);
        if (fail)
            fprintf(stderr, "parallel_perf: round-trip verification failed\n");
    }

    free(data);
    free(rbuf);
    MPI_Finalize();
    return fail;
}

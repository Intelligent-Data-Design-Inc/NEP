/**
 * @file conversion_perf.c
 * @brief Compare standard numeric NEXTCDF-4 conversion and quantized I/O.
 *
 * Usage: conversion_perf RESULTS.csv. Each case uses 768*1536 signed
 * integer-valued inputs, ten copies, ten repetitions and minimum times.
 * Writes include fsync; reads follow an untimed cache-eviction request.
 * Output columns are engine,case,storage,copies,nvalues,memory_bytes,
 * file_bytes,write_s,read_s,write_MBps,read_MBps. Throughput counts memory
 * bytes, not the potentially narrower on-disk representation. Benchmark
 * verification is outside timed regions. No external filter is needed.
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "nep.h"
#include "nep_meta.h"
#include "nxt4internal.h"
#include "perf_disk.h"

/** Values in each time step. */
#define NVALS ((size_t)768 * 1536)
/** Time steps per file. */
#define NCOPIES 10
/** Repetitions per case. */
#define NREPS 10
/** File owned by this program. */
#define TMP_FILE "conversion_perf_tmp.nc"

/** Abort on an unexpected NetCDF status. */
#define CHECK(call) do { int e = (call); if (e) { fprintf(stderr, "%s: %s\n", #call, nc_strerror(e)); exit(2); } } while (0)

/** Return monotonic time in seconds.
 * @return Time in seconds.
 */
static double
seconds(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) { perror("clock_gettime"); exit(2); }
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/** Run one case and write its verified minimum times.
 * @param csv Output stream.
 * @param which Case index.
 * @param compressed Nonzero for shuffle/deflate1.
 * @param ints Integer input array.
 * @param floats Float input array.
 * @param doubles Double input array.
 * @return Zero on success, one on verification or storage failure.
 */
static int
run_case(FILE *csv, int which, int compressed, const int *ints,
         const float *floats, const double *doubles)
{
    static const char *names[] = {"int_short", "double_float", "float_double",
        "double_float_bitgroom", "double_float_granularbr", "double_float_bitround"};
    nc_type memtype = which == 0 ? NC_INT : which == 2 ? NC_FLOAT : NC_DOUBLE;
    nc_type xtype = which == 0 ? NC_SHORT : which == 2 ? NC_DOUBLE : NC_FLOAT;
    size_t memsize = memtype == NC_INT ? sizeof(int) : memtype == NC_FLOAT ? sizeof(float) : sizeof(double);
    size_t bytes = NVALS * NCOPIES * memsize, i;
    void *output = malloc(bytes);
    float *expected = NULL;
    double write_min = HUGE_VAL, read_min = HUGE_VAL;
    int rep, mode = which >= 3 ? which - 2 : NC_NOQUANTIZE;
    struct stat st;
    if (!output) return 1;
    if (mode) {
        int range;
        expected = malloc(NVALS * sizeof(float));
        if (!expected) { free(output); return 1; }
        CHECK(nc4_convert_type(doubles, expected, NC_DOUBLE, NC_FLOAT, NVALS,
                                &range, NULL, 0, mode, 3));
        if (range) { free(expected); free(output); return 1; }
    }
    for (rep = 0; rep < NREPS; rep++) {
        int ncid, dims[2], var, step;
        size_t start[2] = {0, 0}, count[2] = {1, NVALS}, chunks[2] = {1, NVALS};
        double t = seconds(), elapsed;
        CHECK(nc_create(TMP_FILE, NC_NEXTCDF4 | NC_CLOBBER, &ncid));
        CHECK(nc_def_dim(ncid, "time", NCOPIES, &dims[0]));
        CHECK(nc_def_dim(ncid, "values", NVALS, &dims[1]));
        CHECK(nc_def_var(ncid, "v", xtype, 2, dims, &var));
        CHECK(nc_def_var_fill(ncid, var, NC_NOFILL, NULL));
        if (compressed) {
            CHECK(nc_def_var_chunking(ncid, var, NC_CHUNKED, chunks));
            CHECK(nc_def_var_deflate(ncid, var, 1, 1, 1));
        }
        if (mode) CHECK(nc_def_var_quantize(ncid, var, mode, 3));
        CHECK(nc_enddef(ncid));
        for (step = 0; step < NCOPIES; step++) {
            start[0] = (size_t)step;
            if (memtype == NC_INT) CHECK(nc_put_vara_int(ncid, var, start, count, ints));
            else if (memtype == NC_FLOAT) CHECK(nc_put_vara_float(ncid, var, start, count, floats));
            else CHECK(nc_put_vara_double(ncid, var, start, count, doubles));
        }
        CHECK(nc_close(ncid));
        if (nep_perf_sync_file(TMP_FILE)) { perror("fsync"); free(expected); free(output); return 1; }
        elapsed = seconds() - t;
        if (elapsed < write_min) write_min = elapsed;
        if (stat(TMP_FILE, &st) || nep_perf_drop_cache(TMP_FILE)) {
            perror("stat/cache eviction"); free(expected); free(output); return 1;
        }
        t = seconds();
        CHECK(nc_open(TMP_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid));
        CHECK(nc_inq_varid(ncid, "v", &var));
        for (step = 0; step < NCOPIES; step++) {
            start[0] = (size_t)step;
            if (memtype == NC_INT) CHECK(nc_get_vara_int(ncid, var, start, count, (int *)output + step * NVALS));
            else if (memtype == NC_FLOAT) CHECK(nc_get_vara_float(ncid, var, start, count, (float *)output + step * NVALS));
            else CHECK(nc_get_vara_double(ncid, var, start, count, (double *)output + step * NVALS));
        }
        CHECK(nc_close(ncid));
        elapsed = seconds() - t;
        if (elapsed < read_min) read_min = elapsed;
        for (i = 0; i < NVALS * NCOPIES; i++) {
            double got = memtype == NC_INT ? ((int *)output)[i] :
                memtype == NC_FLOAT ? ((float *)output)[i] : ((double *)output)[i];
            double want = expected ? expected[i % NVALS] : doubles[i % NVALS];
            if (got != want) {
                fprintf(stderr, "%s repetition %d element %zu: %.17g != %.17g\n", names[which], rep, i, got, want);
                free(expected); free(output); return 1;
            }
        }
    }
    fprintf(csv, "%s,%s,%s,%d,%zu,%zu,%lld,%.9f,%.9f,%.6f,%.6f\n",
            "internal", names[which],
            compressed ? "deflate1" : "contiguous", NCOPIES, NVALS * NCOPIES,
            bytes, (long long)st.st_size, write_min, read_min,
            bytes / write_min / 1e6, bytes / read_min / 1e6);
    fflush(csv);
    free(expected);
    free(output);
    unlink(TMP_FILE);
    return 0;
}

/** Run the six cases with both storage layouts.
 * @param argc Argument count; must be two.
 * @param argv Output CSV path in argv[1].
 * @return Zero on success, one on failure, two on usage or NetCDF error.
 */
int
main(int argc, char **argv)
{
    int *ints, which, compressed, failed = 0;
    float *floats;
    double *doubles;
    size_t i;
    FILE *csv;
    if (argc != 2) { fprintf(stderr, "usage: conversion_perf RESULTS.csv\n"); return 2; }
    nep_set_log_level(-1);
    ints = malloc(NVALS * sizeof(int));
    floats = malloc(NVALS * sizeof(float));
    doubles = malloc(NVALS * sizeof(double));
    if (!ints || !floats || !doubles) { free(ints); free(floats); free(doubles); return 1; }
    for (i = 0; i < NVALS; i++) {
        ints[i] = (int)(i % 1024) - 512;
        floats[i] = (float)ints[i]; doubles[i] = ints[i];
    }
    csv = fopen(argv[1], "w");
    if (!csv) { perror(argv[1]); free(ints); free(floats); free(doubles); return 1; }
    fprintf(csv, "engine,case,storage,copies,nvalues,memory_bytes,file_bytes,write_s,read_s,write_MBps,read_MBps\n");
    for (which = 0; which < 6 && !failed; which++)
        for (compressed = 0; compressed < 2 && !failed; compressed++)
            failed = run_case(csv, which, compressed, ints, floats, doubles);
    if (fclose(csv)) failed = 1;
    free(ints); free(floats); free(doubles);
    return failed;
}

/**
 * @file tst_nextcdf4_logging.c
 * @brief Tests that NEXTCDF-4 emits the expected diagnostics when
 * NetCDF-C logging is enabled.
 * @author Edward Hartnett
 * @date 2026-08-31
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netcdf.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

/** NEXTCDF-4 file created by the test. */
#define FILE_NAME "tst_nextcdf4_logging.h5"
/** Captured stderr log. */
#define LOG_FILE "tst_nextcdf4_logging.log"
/** Truncated HDF5 file used to force an open-time HDF5 failure. */
#define BAD_FILE "tst_nextcdf4_logging_bad.h5"

/**
 * Restore the original stderr file descriptor after capturing.
 * @param oldfd Saved stderr descriptor from dup().
 */
static void
restore_stderr(int oldfd)
{
    fflush(stderr);
    if (oldfd >= 0) {
        dup2(oldfd, STDERR_FILENO);
        close(oldfd);
    }
}

/**
 * Read the captured log file into a buffer.
 * @param path Path to the log file.
 * @param buf Destination buffer.
 * @param bufsize Size of the destination buffer.
 * @return Number of bytes read, or -1 on failure.
 */
static ssize_t
read_log(const char *path, char *buf, size_t bufsize)
{
    FILE *f = fopen(path, "r");
    size_t n;

    if (!f)
        return -1;
    n = fread(buf, 1, bufsize - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (ssize_t)n;
}

/**
 * Copy the first @p bytes from @p src to @p dst. This produces a
 * truncated HDF5 file that passes H5Fis_hdf5() but fails H5Fopen().
 */
static int
copy_truncated_file(const char *src, const char *dst, size_t bytes)
{
    FILE *in = fopen(src, "rb");
    FILE *out = fopen(dst, "wb");
    char buf[64];
    size_t copied = 0;
    size_t n;

    if (!in || !out) {
        if (in)
            fclose(in);
        if (out)
            fclose(out);
        return 1;
    }
    while (copied < bytes && (n = fread(buf, 1,
                                         (bytes - copied) < sizeof(buf) ?
                                         (bytes - copied) : sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return 1;
        }
        copied += n;
    }
    fclose(in);
    fclose(out);
    return copied < bytes ? 1 : 0;
}

/** @return Zero when all logging checks pass. */
int
main(void)
{
#ifndef LOGGING
    /* NetCDF-C was built without logging; there is nothing to test. */
    return 0;
#endif

    int ncid;
    int oldfd;
    char buf[4096];
    ssize_t n;

    unlink(FILE_NAME);
    unlink(LOG_FILE);
    unlink(BAD_FILE);

    /* Phase 1: enable logging and create/close a NEXTCDF-4 file. */
    oldfd = dup(STDERR_FILENO);
    if (!freopen(LOG_FILE, "w", stderr)) {
        restore_stderr(oldfd);
        return 1;
    }

    nep_set_log_level(3);

    if (!NC_NEXTCDF4_initialize()) {
        restore_stderr(oldfd);
        return 1;
    }

    if (nc_create(FILE_NAME, NC_NEXTCDF4 | NC_CLOBBER, &ncid) != NC_NOERR) {
        restore_stderr(oldfd);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR) {
        restore_stderr(oldfd);
        return 1;
    }

    restore_stderr(oldfd);

    n = read_log(LOG_FILE, buf, sizeof(buf));
    if (n < 0)
        return 1;

    /* With level 3 we should see lifecycle entry points and the create
     * success message from the NEXTCDF-4 backend. */
    if (!strstr(buf, "NEXTCDF4_create") ||
        !strstr(buf, "H5Fcreate succeeded") ||
        !strstr(buf, "NEXTCDF4_close")) {
        fprintf(stderr, "Missing expected log output\n");
        return 1;
    }

    /* Phase 2: turn logging off and confirm no further output. */
    unlink(LOG_FILE);
    oldfd = dup(STDERR_FILENO);
    if (!freopen(LOG_FILE, "w", stderr)) {
        restore_stderr(oldfd);
        return 1;
    }

    nep_set_log_level(-1);

    if (nc_open(FILE_NAME, NC_NEXTCDF4 | NC_NOWRITE, &ncid) != NC_NOERR) {
        restore_stderr(oldfd);
        return 1;
    }
    if (nc_close(ncid) != NC_NOERR) {
        restore_stderr(oldfd);
        return 1;
    }

    restore_stderr(oldfd);

    n = read_log(LOG_FILE, buf, sizeof(buf));
    if (n < 0)
        return 1;
    if (n != 0) {
        fprintf(stderr, "Unexpected log output after disabling logging\n");
        return 1;
    }

    /* Phase 3: re-enable logging and exercise an HDF5 failure path.
       Copy the first 64 bytes of the valid NEXTCDF-4 file so that
       H5Fis_hdf5() succeeds but H5Fopen() fails. */
    if (copy_truncated_file(FILE_NAME, BAD_FILE, 64))
        return 1;

    unlink(LOG_FILE);
    oldfd = dup(STDERR_FILENO);
    if (!freopen(LOG_FILE, "w", stderr)) {
        restore_stderr(oldfd);
        return 1;
    }

    nep_set_log_level(1);

    /* This open must fail because the file is not a valid HDF5 file. */
    if (nc_open(BAD_FILE, NC_NEXTCDF4 | NC_NOWRITE, &ncid) == NC_NOERR) {
        restore_stderr(oldfd);
        return 1;
    }

    restore_stderr(oldfd);

    n = read_log(LOG_FILE, buf, sizeof(buf));
    if (n < 0)
        return 1;

    /* A severity-0 ("ERROR:") line should have been emitted. */
    if (!strstr(buf, "ERROR:")) {
        fprintf(stderr, "Missing ERROR line for HDF5 failure\n");
        return 1;
    }

    /* Cleanup. */
    unlink(FILE_NAME);
    unlink(LOG_FILE);
    unlink(BAD_FILE);

    return 0;
}

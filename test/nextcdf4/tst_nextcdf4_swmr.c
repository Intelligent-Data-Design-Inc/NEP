/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
#include <netcdf.h>
#include <netcdf_filter.h>
#include <hdf5.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

#define CHECK(call) do { int status_ = (call); if (status_) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(status_)); return 1; } } while (0)
#define EXPECT(call, expected) do { int status_ = (call); if (status_ != (expected)) { \
    fprintf(stderr, "%d: %s: got %d expected %d\n", __LINE__, #call, status_, (expected)); return 1; } } while (0)
#define REQUIRE(value) do { if (!(value)) { fprintf(stderr, "%d: %s\n", __LINE__, #value); return 1; } } while (0)

static int
controls(void)
{
    int ncid, dim, var, group, mode, format, other;
    size_t one = 1, zero = 0, length;
    int value = 7;
    nc_type type;
    nc_std_ref_t ref = {0};

    EXPECT(nc_start_swmr_write(-1), NC_EBADID);
    EXPECT(nc_swmr_flush(-1), NC_EBADID);
    EXPECT(nc_swmr_refresh(-1), NC_EBADID);
    CHECK(nc_create("classic.nc", NC_CLOBBER, &ncid));
    EXPECT(nc_start_swmr_write(ncid), NC_ENOTNC4);
    EXPECT(nc_swmr_flush(ncid), NC_ENOTNC4);
    EXPECT(nc_swmr_refresh(ncid), NC_ENOTNC4);
    CHECK(nc_close(ncid));
    for (int i = 0; i < 2; i++) {
        int model = i ? NC_NETCDF4_MODEL : NC_CLASSIC_MODEL;
        CHECK(nc_create("model.nc", NC_NEXTCDF4 | model, &ncid));
        EXPECT(nc_start_swmr_write(ncid), NC_EINVAL);
        CHECK(nc_close(ncid));
        EXPECT(nc_create("model.nc", NC_NEXTCDF4 | model | NC_SWMR_WRITE, &other), NC_EINVAL);
        EXPECT(nc_open("model.nc", NC_NEXTCDF4 | NC_WRITE | NC_SWMR_WRITE, &other), NC_EINVAL);
        CHECK(nc_open("model.nc", NC_NEXTCDF4, &ncid));
        CHECK(nc_close(ncid));
    }
    for (int i = 0; i < 5; i++) {
        CHECK(nc_create("unsupported.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &ncid));
        if (i < 2)
            CHECK(nc_def_var(ncid, "v", i ? NC_STD_REF : NC_STRING, 0, NULL, &var));
        else {
            CHECK(nc_def_opaque(ncid, sizeof(int), "opaque", &type));
            if (i == 3)
                CHECK(nc_def_var(ncid, "v", type, 0, NULL, &var));
            if (i == 4)
                CHECK(nc_put_att(ncid, NC_GLOBAL, "opaque", type, 1, &value));
        }
        EXPECT(nc_start_swmr_write(ncid), NC_ENOTBUILT);
        EXPECT(nc_enddef(ncid), NC_ENOTBUILT);
        CHECK(nc_close(ncid));
        EXPECT(nc_open("unsupported.nc", NC_NEXTCDF4 | NC_WRITE | NC_SWMR_WRITE, &other), NC_ENOTBUILT);
        CHECK(nc_open("unsupported.nc", NC_NEXTCDF4 | NC_WRITE, &ncid));
        CHECK(nc_close(ncid));
    }
    {
        CHECK(nc_create("old.h5", NC_NETCDF4, &ncid));
        CHECK(nc_def_var(ncid, "old", NC_INT, 0, NULL, &var));
        CHECK(nc_close(ncid));
        CHECK(nc_open("old.h5", NC_NEXTCDF4 | NC_WRITE, &ncid));
        EXPECT(nc_start_swmr_write(ncid), NC_EINVAL);
        CHECK(nc_close(ncid));
        /* A pre-v3-superblock file cannot be a SWMR reader either. */
        EXPECT(nc_open("old.h5", NC_NEXTCDF4 | NC_SWMR_READ, &ncid), NC_EINVAL);
    }
    CHECK(nc_create("control.nc", NC_NEXTCDF4, &ncid));
    CHECK(nc_def_dim(ncid, "t", NC_UNLIMITED, &dim));
    CHECK(nc_def_var(ncid, "t", NC_INT, 1, &dim, &var));
    CHECK(nc_def_grp(ncid, "g", &group));
    CHECK(nc_put_att_int(ncid, NC_GLOBAL, "a", NC_INT, 1, &value));
    EXPECT(nc_swmr_flush(ncid), NC_EINVAL);
    CHECK(nc_start_swmr_write(group));
    CHECK(nc_start_swmr_write(ncid));
    CHECK(nc_inq_format_extended(ncid, &format, &mode));
    REQUIRE(mode & NC_SWMR_WRITE);
    EXPECT(nc_swmr_refresh(ncid), NC_EINVAL);
    EXPECT(nc_redef(ncid), NC_EPERM);
    EXPECT(nc_def_dim(ncid, "bad", 1, &other), NC_EPERM);
    EXPECT(nc_def_var(ncid, "bad", NC_INT, 0, NULL, &other), NC_EPERM);
    EXPECT(nc_def_grp(ncid, "bad", &other), NC_EPERM);
    EXPECT(nc_def_opaque(ncid, 4, "bad", &type), NC_EPERM);
    EXPECT(nc_def_enum(ncid, NC_INT, "bad", &type), NC_EPERM);
    EXPECT(nc_def_vlen(ncid, "bad", NC_INT, &type), NC_EPERM);
    EXPECT(nc_def_compound(ncid, 4, "bad", &type), NC_EPERM);
    EXPECT(nc_rename_dim(ncid, dim, "bad"), NC_EPERM);
    EXPECT(nc_rename_var(ncid, var, "bad"), NC_EPERM);
    EXPECT(nc_rename_grp(group, "bad"), NC_EPERM);
    EXPECT(nc_put_att_int(ncid, NC_GLOBAL, "a", NC_INT, 1, &value), NC_EPERM);
    EXPECT(nc_put_att_int(ncid, var, "new", NC_INT, 1, &value), NC_EPERM);
    EXPECT(nc_rename_att(ncid, NC_GLOBAL, "a", "bad"), NC_EPERM);
    EXPECT(nc_del_att(ncid, NC_GLOBAL, "a"), NC_EPERM);
    EXPECT(nc_set_fill(ncid, NC_NOFILL, NULL), NC_EPERM);
    EXPECT(nc_def_var_chunking(ncid, var, NC_CHUNKED, &one), NC_ELATEDEF);
    EXPECT(nc_def_var_deflate(ncid, var, 0, 1, 1), NC_ELATEDEF);
    EXPECT(nc_def_var_fletcher32(ncid, var, 1), NC_ELATEDEF);
    EXPECT(nc_def_var_fill(ncid, var, 1, NULL), NC_ELATEDEF);
    EXPECT(nc_def_var_endian(ncid, var, NC_ENDIAN_LITTLE), NC_ELATEDEF);
    EXPECT(nc_ref_object(ncid, NC_REF_OBJ_VAR, var, &ref), NC_EPERM);
    CHECK(nc_put_vara_int(ncid, var, &zero, &zero, NULL));
    CHECK(nc_inq_dimlen(ncid, dim, &length));
    REQUIRE(length == 0);
    CHECK(nc_put_vara_int(ncid, var, &zero, &one, &value));
    CHECK(nc_set_var_chunk_cache(ncid, var, 1048576, 521, 0.75f));
    CHECK(nc_get_vara_int(ncid, var, &zero, &one, &other));
    REQUIRE(other == value);
    CHECK(nc_swmr_flush(group));
    CHECK(nc_sync(ncid));
    CHECK(nc_close(ncid));
    EXPECT(nc_start_swmr_write(ncid), NC_EBADID);
    CHECK(nc_open("control.nc", NC_NEXTCDF4, &ncid));
    EXPECT(nc_start_swmr_write(ncid), NC_EPERM);
    EXPECT(nc_swmr_flush(ncid), NC_EPERM);
    CHECK(nc_get_att_int(ncid, NC_GLOBAL, "a", &other));
    REQUIRE(other == value);
    CHECK(nc_close(ncid));
    EXPECT(nc_open("control.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &other), NC_EINVAL);
    CHECK(nc_open("control.nc", NC_NEXTCDF4 | NC_WRITE | NC_SWMR_WRITE, &ncid));
    EXPECT(nc_redef(ncid), NC_EPERM);
    CHECK(nc_close(ncid));
    CHECK(nc_open("control.nc", NC_NEXTCDF4 | NC_WRITE, &ncid));
    CHECK(nc_inq_format_extended(ncid, &format, &mode));
    REQUIRE(!(mode & NC_SWMR_WRITE));
    CHECK(nc_redef(ncid));
    CHECK(nc_enddef(ncid));
    CHECK(nc_start_swmr_write(ncid));
    CHECK(nc_close(ncid));
    CHECK(nc_create("automatic.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &ncid));
    CHECK(nc_def_var(ncid, "v", NC_INT, 0, NULL, &var));
    CHECK(nc_put_var_int(ncid, var, &value));
    EXPECT(nc_redef(ncid), NC_EPERM);
    CHECK(nc_close(ncid));
    /* Reader-side flag and refresh rejections. */
    EXPECT(nc_create("reader.nc", NC_NEXTCDF4 | NC_SWMR_READ, &ncid), NC_EINVAL);
    EXPECT(nc_open("control.nc", NC_NEXTCDF4 | NC_SWMR_READ | NC_WRITE, &ncid), NC_EINVAL);
    EXPECT(nc_open("control.nc", NC_NEXTCDF4 | NC_SWMR_READ | NC_SWMR_WRITE, &ncid), NC_EINVAL);
    EXPECT(nc_open("control.nc", NC_NEXTCDF4 | NC_SWMR_READ | NC_CLASSIC_MODEL, &ncid), NC_EINVAL);
    /* model.nc is NC_NETCDF4_MODEL but HDF5 2.1.1 gives it a v3
     * superblock, so a SWMR reader is accepted (decision 3c). */
    CHECK(nc_open("model.nc", NC_NEXTCDF4 | NC_SWMR_READ, &ncid));
    CHECK(nc_swmr_refresh(ncid));
    CHECK(nc_close(ncid));
    /* An ordinary read-only session is not a refreshable SWMR session. */
    CHECK(nc_open("control.nc", NC_NEXTCDF4, &ncid));
    EXPECT(nc_swmr_refresh(ncid), NC_EINVAL);
    CHECK(nc_close(ncid));
    /* A quiescent v3 file accepts a SWMR reader; it sees final data. */
    CHECK(nc_open("control.nc", NC_NEXTCDF4 | NC_SWMR_READ, &ncid));
    CHECK(nc_inq_format_extended(ncid, &format, &mode));
    REQUIRE(mode & NC_SWMR_READ);
    CHECK(nc_swmr_refresh(ncid));
    EXPECT(nc_swmr_flush(ncid), NC_EPERM);
    EXPECT(nc_start_swmr_write(ncid), NC_EPERM);
    CHECK(nc_inq_dimlen(ncid, dim, &length));
    REQUIRE(length == 1);
    CHECK(nc_get_var1_int(ncid, var, &zero, &other));
    REQUIRE(other == value);
    CHECK(nc_close(ncid));
    return 0;
}

static int
atomic_types(void)
{
    const nc_type types[] = {NC_BYTE, NC_UBYTE, NC_CHAR, NC_SHORT, NC_USHORT,
        NC_INT, NC_UINT, NC_INT64, NC_UINT64, NC_FLOAT, NC_DOUBLE,
        NC_FLOAT16, NC_BFLOAT16, NC_FLOAT8_E4M3, NC_FLOAT8_E5M2,
        NC_FLOAT6_E2M3, NC_FLOAT6_E3M2, NC_FLOAT4_E2M1,
        NC_COMPLEX, NC_DOUBLECOMPLEX, NC_BITFIELD8, NC_BITFIELD16,
        NC_BITFIELD32, NC_BITFIELD64};
    const size_t sizes[] = {1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8,
        2, 2, 1, 1, 1, 1, 1, 8, 16, 1, 2, 4, 8};
    int ncid, vars[sizeof(types) / sizeof(types[0])];
    unsigned char zero[32] = {0}, value[32];
    CHECK(nc_create("atomic.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &ncid));
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        char name[32];
        snprintf(name, sizeof(name), "v%zu", i);
        CHECK(nc_def_var(ncid, name, types[i], 0, NULL, &vars[i]));
        CHECK(nc_def_var_fill(ncid, vars[i], 1, NULL));
    }
    CHECK(nc_enddef(ncid));
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        size_t size = sizes[i];
        REQUIRE(size <= sizeof(zero));
        memset(value, 0xff, sizeof(value));
        CHECK(nc_put_var(ncid, vars[i], zero));
        CHECK(nc_get_var(ncid, vars[i], value));
        REQUIRE(!memcmp(value, zero, size));
    }
    EXPECT(nc_put_var_double(ncid, vars[0], (double[]){1e20}), NC_ERANGE);
    CHECK(nc_swmr_flush(ncid));
    CHECK(nc_close(ncid));
    return 0;
}

static int
reply(int fd, char expected)
{
    char value;
    struct pollfd pfd = {fd, POLLIN, 0};
    return poll(&pfd, 1, 30000) == 1 && read(fd, &value, 1) == 1 && value == expected;
}

static pid_t
launch(const char *reader, const char *file, int commands[2], int replies[2])
{
    pid_t child;
    if (pipe(commands) || pipe(replies))
        return -1;
    child = fork();
    if (child == 0) {
        if (dup2(commands[0], STDIN_FILENO) < 0 || dup2(replies[1], STDOUT_FILENO) < 0)
            _exit(127);
        close(commands[0]); close(commands[1]); close(replies[0]); close(replies[1]);
        execl(reader, reader, file, (char *)NULL);
        _exit(127);
    }
    if (child > 0) {
        close(commands[0]); commands[0] = -1;
        close(replies[1]); replies[1] = -1;
    }
    return child;
}

static int
wait_reader(pid_t child, int *status)
{
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start))
        return 0;
    for (;;) {
        pid_t result = waitpid(child, status, WNOHANG);
        if (result == child)
            return 1;
        if (result < 0 || clock_gettime(CLOCK_MONOTONIC, &now) ||
            now.tv_sec - start.tv_sec >= 30)
            return 0;
        poll(NULL, 0, 10);
    }
}

static int
visibility(const char *reader)
{
    int ncid = -1, time, record, vars[4], coord, group, shorter;
    int commands[2] = {-1, -1}, replies[2] = {-1, -1}, status, ret = 1;
    pid_t child = -1;
    size_t chunk = 2;
#define TRY(call) do { int status_ = (call); if (status_) { \
    fprintf(stderr, "%d: %s: %s\n", __LINE__, #call, nc_strerror(status_)); goto done; } } while (0)
    TRY(nc_create("stream.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &ncid));
    TRY(nc_def_dim(ncid, "time", NC_UNLIMITED, &time));
    TRY(nc_def_dim(ncid, "record", NC_UNLIMITED, &record));
    TRY(nc_def_var(ncid, "time", NC_INT, 1, &time, &coord));
    TRY(nc_def_var(ncid, "v0", NC_INT, 1, &time, &vars[0]));
    TRY(nc_def_var(ncid, "v1", NC_FLOAT16, 1, &time, &vars[1]));
    TRY(nc_def_grp(ncid, "g", &group));
    TRY(nc_def_var(group, "v2", NC_FLOAT, 1, &time, &vars[2]));
    TRY(nc_def_var(ncid, "v3", NC_INT, 1, &record, &vars[3]));
    TRY(nc_def_var(ncid, "shorter", NC_INT, 1, &time, &shorter));
    int fill = -99;
    TRY(nc_def_var_fill(ncid, shorter, 0, &fill));
    for (int i = 0; i < 4; i++)
        TRY(nc_def_var_chunking(i == 2 ? group : ncid, vars[i], NC_CHUNKED, &chunk));
    TRY(nc_def_var_deflate(ncid, vars[1], 0, 1, 1));
    TRY(nc_def_var_deflate(group, vars[2], 1, 1, 1));
    TRY(nc_def_var_fletcher32(ncid, vars[3], 1));
    TRY(nc_enddef(ncid));
    child = launch(reader, "stream.nc", commands, replies);
    if (child < 0)
        goto done;
    if (!reply(replies[0], 'R'))
        goto done;
    for (int round = 1; round <= 3; round++) {
        size_t start = (size_t)round - 1, count = 1;
        float value = (float)round;
        TRY(nc_put_vara_int(ncid, coord, &start, &count, &round));
        for (int i = 0; i < 4; i++)
            TRY(nc_put_vars_float(i == 2 ? group : ncid, vars[i], &start, &count,
                                  (ptrdiff_t[]){1}, &value));
        if (round == 1)
            TRY(nc_put_vara_int(ncid, shorter, &start, &count, &round));
        if (round == 2) {
            TRY(nc_set_var_chunk_cache(ncid, coord, 1048576, 521, 0.5f));
            TRY(nc_set_var_chunk_cache(group, vars[2], 1048576, 521, 0.5f));
        }
        TRY(round % 2 ? nc_swmr_flush(ncid) : nc_sync(ncid));
        char command = (char)('0' + round);
        if (write(commands[1], &command, 1) != 1 || !reply(replies[0], 'A'))
            goto done;
    }
    if (write(commands[1], "Q", 1) != 1)
        goto done;
    if (!wait_reader(child, &status))
        goto done;
    child = -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status))
        goto done;
    TRY(nc_close(ncid)); ncid = -1;
    TRY(nc_open("stream.nc", NC_NEXTCDF4, &ncid));
    TRY(nc_inq_varid(ncid, "shorter", &shorter));
    int values[3];
    TRY(nc_get_vara_int(ncid, shorter, (size_t[]){0}, (size_t[]){3}, values));
    if (values[0] != 1 || values[1] != fill || values[2] != fill)
        goto done;
    ret = 0;
done:
    if (child > 0) {
        kill(child, SIGKILL);
        waitpid(child, &status, 0);
    }
    for (int i = 0; i < 2; i++) {
        if (commands[i] >= 0) close(commands[i]);
        if (replies[i] >= 0) close(replies[i]);
    }
    if (ncid >= 0 && nc_close(ncid))
        ret = 1;
    if (ret)
        fprintf(stderr, "SWMR visibility test failed\n");
    return ret;
}

/* Single writer, two simultaneous NEP-API readers, plus both close
 * orders: reader 2 quits mid-stream and reader 1 outlives the writer's
 * close. argv reader is tst_nextcdf4_swmr_nep_reader. */
static int
nep_readers(const char *reader)
{
    int ncid = -1, time, record, vars[4], coord, group, shorter;
    int ret = 1, status;
    size_t chunk = 2;
    struct {
        pid_t pid;
        int cmd[2];
        int rep[2];
    } r[2] = {{-1, {-1, -1}, {-1, -1}}, {-1, {-1, -1}, {-1, -1}}};
    TRY(nc_create("stream_nep.nc", NC_NEXTCDF4 | NC_SWMR_WRITE, &ncid));
    TRY(nc_def_dim(ncid, "time", NC_UNLIMITED, &time));
    TRY(nc_def_dim(ncid, "record", NC_UNLIMITED, &record));
    TRY(nc_def_var(ncid, "time", NC_INT, 1, &time, &coord));
    TRY(nc_def_var(ncid, "v0", NC_INT, 1, &time, &vars[0]));
    TRY(nc_def_var(ncid, "v1", NC_FLOAT16, 1, &time, &vars[1]));
    TRY(nc_def_grp(ncid, "g", &group));
    TRY(nc_def_var(group, "v2", NC_FLOAT, 1, &time, &vars[2]));
    TRY(nc_def_var(ncid, "v3", NC_INT, 1, &record, &vars[3]));
    TRY(nc_def_var(ncid, "shorter", NC_INT, 1, &time, &shorter));
    int fill = -99;
    TRY(nc_def_var_fill(ncid, shorter, 0, &fill));
    for (int i = 0; i < 4; i++)
        TRY(nc_def_var_chunking(i == 2 ? group : ncid, vars[i], NC_CHUNKED, &chunk));
    TRY(nc_def_var_deflate(ncid, vars[1], 0, 1, 1));
    TRY(nc_def_var_deflate(group, vars[2], 1, 1, 1));
    TRY(nc_def_var_fletcher32(ncid, vars[3], 1));
    TRY(nc_enddef(ncid));
    for (int i = 0; i < 2; i++) {
        r[i].pid = launch(reader, "stream_nep.nc", r[i].cmd, r[i].rep);
        if (r[i].pid < 0)
            goto done;
    }
    if (!reply(r[0].rep[0], 'R') || !reply(r[1].rep[0], 'R'))
        goto done;
    for (int round = 1; round <= 4; round++) {
        size_t start = (size_t)round - 1, count = 1;
        float value = (float)round;
        int nreaders = round < 4 ? 2 : 1;
        TRY(nc_put_vara_int(ncid, coord, &start, &count, &round));
        for (int i = 0; i < 4; i++)
            TRY(nc_put_vars_float(i == 2 ? group : ncid, vars[i], &start, &count,
                                  (ptrdiff_t[]){1}, &value));
        if (round == 1)
            TRY(nc_put_vara_int(ncid, shorter, &start, &count, &round));
        TRY(round % 2 ? nc_swmr_flush(ncid) : nc_sync(ncid));
        for (int i = 0; i < nreaders; i++) {
            char command = (char)('0' + round);
            if (write(r[i].cmd[1], &command, 1) != 1 || !reply(r[i].rep[0], 'A'))
                goto done;
        }
        /* After round 3 reader 2 quits first: the writer keeps going. */
        if (round == 3) {
            if (write(r[1].cmd[1], "Q", 1) != 1 ||
                !wait_reader(r[1].pid, &status))
                goto done;
            r[1].pid = -1;
            if (!WIFEXITED(status) || WEXITSTATUS(status))
                goto done;
        }
    }
    /* The writer closes while reader 1 is still open. */
    TRY(nc_close(ncid));
    ncid = -1;
    if (write(r[0].cmd[1], "Q", 1) != 1 || !wait_reader(r[0].pid, &status))
        goto done;
    r[0].pid = -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status))
        goto done;
    /* An ordinary open sees the four published rounds. */
    TRY(nc_open("stream_nep.nc", NC_NEXTCDF4, &ncid));
    {
        size_t length;
        TRY(nc_inq_dimid(ncid, "time", &time));
        TRY(nc_inq_dimlen(ncid, time, &length));
        if (length != 4)
            goto done;
    }
    TRY(nc_close(ncid));
    ncid = -1;
    ret = 0;
done:
    for (int i = 0; i < 2; i++) {
        if (r[i].pid > 0) {
            kill(r[i].pid, SIGKILL);
            waitpid(r[i].pid, &status, 0);
        }
        for (int j = 0; j < 2; j++) {
            if (r[i].cmd[j] >= 0) close(r[i].cmd[j]);
            if (r[i].rep[j] >= 0) close(r[i].rep[j]);
        }
    }
    if (ncid >= 0 && nc_close(ncid))
        ret = 1;
    if (ret)
        fprintf(stderr, "NEP SWMR reader test failed\n");
    return ret;
}

int
main(int argc, char **argv)
{
    REQUIRE(argc == 3);
    signal(SIGPIPE, SIG_IGN);
    REQUIRE(NC_NEXTCDF4_initialize());
    CHECK(nep_set_log_level(-1));
    if (controls() || atomic_types() || visibility(argv[1]) ||
        nep_readers(argv[2]))
        return 1;
    puts("SWMR writer tests passed");
    return 0;
}

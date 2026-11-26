/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
#include <netcdf.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include "nep.h"
#include "nextcdf4dispatch.h"

static int
message(char *value)
{
    struct pollfd fd = {STDIN_FILENO, POLLIN, 0};
    return poll(&fd, 1, 30000) == 1 && read(STDIN_FILENO, value, 1) == 1;
}

/* Read a 1-D int variable fully and expect values 1..round. */
static int
check_int(int ncid, int var, int round)
{
    int values[16] = {0};
    int rc;
    if ((rc = nc_get_vara_int(ncid, var, (size_t[]){0}, (size_t[]){(size_t)round}, values))) {
        fprintf(stderr, "get_vara_int var %d rc %d %s\n", var, rc, nc_strerror(rc));
        return 1;
    }
    for (int j = 0; j < round; j++)
        if (values[j] != j + 1) {
            fprintf(stderr, "var %d idx %d got %d want %d\n", var, j, values[j], j + 1);
            return 1;
        }
    return 0;
}

/* Read a 1-D variable as float and expect values 1..round. */
static int
check_float(int ncid, int var, int round)
{
    float values[16] = {0};
    if (nc_get_vara_float(ncid, var, (size_t[]){0}, (size_t[]){(size_t)round}, values))
        return 1;
    for (int j = 0; j < round; j++)
        if (values[j] != (float)(j + 1))
            return 1;
    return 0;
}

int
main(int argc, char **argv)
{
    int ncid = -1, group = -1;
    int v0, v1, v2, v3, coord, shorter, time_dim, record_dim;
    int ret = 1;
    char command;
    const char *step = "init";

    if (argc != 2 || !NC_NEXTCDF4_initialize())
        goto done;
    step = "open";
    if (nc_open(argv[1], NC_NEXTCDF4 | NC_SWMR_READ, &ncid))
        goto done;
    step = "inq_format_extended";
    {
        int mode;
        if (nc_inq_format_extended(ncid, NULL, &mode) || !(mode & NC_SWMR_READ))
            goto done;
    }
    step = "inquiries";
    if (nc_inq_dimid(ncid, "time", &time_dim) ||
        nc_inq_dimid(ncid, "record", &record_dim) ||
        nc_inq_grp_ncid(ncid, "g", &group) ||
        nc_inq_varid(ncid, "time", &coord) ||
        nc_inq_varid(ncid, "v0", &v0) ||
        nc_inq_varid(ncid, "v1", &v1) ||
        nc_inq_varid(group, "v2", &v2) ||
        nc_inq_varid(ncid, "v3", &v3) ||
        nc_inq_varid(ncid, "shorter", &shorter))
        goto done;
    /* Refresh on a session that just loaded metadata is a no-op. */
    step = "first refresh";
    if (nc_swmr_refresh(ncid))
        goto done;
    step = "write R";
    if (write(STDOUT_FILENO, "R", 1) != 1)
        goto done;
    while (message(&command)) {
        size_t length;
        int svalues[16] = {0};
        if (command == 'Q') {
            ret = 0;
            break;
        }
        int round = command - '0';
        if (round < 1 || round > 16)
            goto done;
        step = "refresh";
        if (nc_swmr_refresh(ncid))
            goto done;
        /* Refreshed dim lengths must reflect the writer's growth. */
        step = "inq_dimlen";
        if (nc_inq_dimlen(ncid, time_dim, &length) || length != (size_t)round)
            goto done;
        if (nc_inq_dimlen(ncid, record_dim, &length) || length != (size_t)round)
            goto done;
        /* The writer publishes 1..round for every variable. */
        step = "check coord";
        if (check_int(ncid, coord, round)) goto done;
        step = "check v0";
        if (check_int(ncid, v0, round)) goto done;
        step = "check v1";
        if (check_float(ncid, v1, round)) goto done;
        step = "check v2";
        if (check_float(group, v2, round)) goto done;
        step = "check v3";
        if (check_int(ncid, v3, round)) goto done;
        /* "shorter" holds 1 at index 0 and fill (-99) afterwards. */
        step = "shorter";
        if (nc_get_vara_int(ncid, shorter, (size_t[]){0},
                            (size_t[]){(size_t)round}, svalues))
            goto done;
        for (int j = 0; j < round; j++)
            if (svalues[j] != (j == 0 ? 1 : -99))
                goto done;
        step = "write A";
        if (write(STDOUT_FILENO, "A", 1) != 1)
            goto done;
    }
done:
    if (ncid >= 0 && nc_close(ncid))
        ret = 1;
    if (ret)
        fprintf(stderr, "NEP SWMR reader failed at %s\n", step);
    return ret;
}

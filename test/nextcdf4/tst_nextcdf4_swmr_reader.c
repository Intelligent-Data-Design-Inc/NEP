/* Author: Edward Hartnett, Intelligent Data Design, Inc. */
#include <hdf5.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

static int
message(char *value)
{
    struct pollfd fd = {STDIN_FILENO, POLLIN, 0};
    return poll(&fd, 1, 30000) == 1 && read(STDIN_FILENO, value, 1) == 1;
}

int
main(int argc, char **argv)
{
    const char *names[] = {"time", "v0", "v1", "g/v2", "v3", "record"};
    hid_t file = -1, datasets[6] = {-1, -1, -1, -1, -1, -1};
    int ret = 1;
    char command;

    if (argc != 2 || (file = H5Fopen(argv[1], H5F_ACC_RDONLY | H5F_ACC_SWMR_READ,
                                    H5P_DEFAULT)) < 0)
        goto done;
    for (int i = 0; i < 6; i++)
        if ((datasets[i] = H5Dopen2(file, names[i], H5P_DEFAULT)) < 0)
            goto done;
    if (write(STDOUT_FILENO, "R", 1) != 1)
        goto done;
    while (message(&command)) {
        if (command == 'Q') {
            ret = 0;
            break;
        }
        int round = command - '0';
        if (round < 1 || round > 3)
            goto done;
        for (int i = 0; i < 6; i++) {
            hsize_t length = 0;
            hid_t space;
            if (H5Drefresh(datasets[i]) < 0 ||
                (space = H5Dget_space(datasets[i])) < 0)
                goto done;
            int rank = H5Sget_simple_extent_dims(space, &length, NULL);
            H5Sclose(space);
            if (rank != 1 || length != (hsize_t)round)
                goto done;
            if (i < 5) {
                float values[3] = {0};
                if (H5Dread(datasets[i], H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL,
                            H5P_DEFAULT, values) < 0)
                    goto done;
                for (int j = 0; j < round; j++)
                    if (values[j] != (float)(j + 1))
                        goto done;
            }
        }
        if (write(STDOUT_FILENO, "A", 1) != 1)
            goto done;
    }
done:
    for (int i = 0; i < 6; i++)
        if (datasets[i] >= 0 && H5Dclose(datasets[i]) < 0)
            ret = 1;
    if (file >= 0 && H5Fclose(file) < 0)
        ret = 1;
    if (ret)
        fprintf(stderr, "SWMR reader failed\n");
    return ret;
}

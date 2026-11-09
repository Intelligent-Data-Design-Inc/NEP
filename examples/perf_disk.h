/**
 * @file perf_disk.h
 * @brief Helpers that make example benchmarks measure disk-bound I/O.
 *
 * NetCDF/HDF5 writes return once the data is in the operating
 * system's page cache, and a file read right after it was written is
 * served from that cache. To time I/O against the storage device, a
 * benchmark calls nep_perf_sync_file() after nc_close() inside the
 * write timer, and nep_perf_drop_cache() before starting the read
 * timer. Neither function needs root privileges.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 */

#ifndef NEP_PERF_DISK_H
#define NEP_PERF_DISK_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Flush a closed file's data to the storage device.
 *
 * Opens @p path read-only, calls fsync() on it and closes it.
 *
 * @param path Path of the file to flush.
 * @return 0 on success, or -1 with errno set.
 */
static inline int
nep_perf_sync_file(const char *path)
{
    int fd, ret;

    if ((fd = open(path, O_RDONLY)) < 0)
        return -1;
    ret = fsync(fd);
    close(fd);
    return ret;
}

/**
 * @brief Drop a file's pages from the page cache.
 *
 * Opens @p path read-only, calls fsync() so no dirty pages remain,
 * then posix_fadvise(POSIX_FADV_DONTNEED) on the whole file, so the
 * next read comes from the storage device.
 *
 * @param path Path of the file to drop from the cache.
 * @return 0 on success, or -1 with errno set (to the posix_fadvise()
 *         return value if that call fails).
 */
static inline int
nep_perf_drop_cache(const char *path)
{
    int fd, ret;

    if ((fd = open(path, O_RDONLY)) < 0)
        return -1;
    if (fsync(fd) < 0)
    {
        close(fd);
        return -1;
    }
    ret = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    close(fd);
    if (ret)
    {
        errno = ret;
        return -1;
    }
    return 0;
}

#endif /* NEP_PERF_DISK_H */

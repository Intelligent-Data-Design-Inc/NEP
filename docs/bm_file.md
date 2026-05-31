# Benchmarking NetCDF Files with bm_file

## Overview

The bm_file program copies a netCDF file from one format to another while measuring read and write performance for both metadata and data. During the copy it applies whatever chunking, compression, endianness, and parallel I/O settings the user specifies. By running bm_file repeatedly with different settings, a data producer can identify the storage parameters that best serve a given user community for a particular data file.

The program reads every variable in the input file one slice at a time along the slowest-varying dimension, writes that slice to the output file with the requested storage parameters, and then optionally rereads the output file to confirm correctness and measure the reread rate. Timing is captured at microsecond resolution using gettimeofday (or MPI wall-clock time for parallel runs), and the results are printed as a single comma-separated line suitable for import into a spreadsheet.

bm_file works only on classic-model netCDF files. The input may be in any of the classic formats (classic, 64-bit offset, CDF-5, or netCDF-4/classic), but files that use netCDF-4 extended features such as groups or user-defined types are not supported.

## Building

NetCDF must be configured with benchmarks enabled before bm_file will be compiled. For Autotools builds, pass --enable-benchmarks to the configure script. For CMake builds, set -DNETCDF_ENABLE_BENCHMARKS=ON on the cmake command line. When that option is present the program is built as part of the normal build and installed under nc_perf/. The standard make check run will also exercise bm_file on a set of synthetic test files.

## Command Line

```
bm_file -v [-s N]|[-t V:S:S:S -u V:C:C:C -r V:I:I:I] \
        -o file_out -f N -h \
        -c V:Z:S:C:C:C[,V:Z:S:C:C:C,...] \
        -d -m -p -i -e 1|2 -l -y -q N file
```

The trailing positional argument is the input netCDF file. All other arguments are optional switches described below.

### Options

-v turns on verbose output that prints the time taken for each individual read and write step. This is most useful when running in batch mode on a cluster where there is no other way to observe progress.

-o file sets the name of the output file. If this option is omitted, bm_file still performs a timed read of the input but does not write any output.

-f N selects the output format. The argument is an integer: 1 for classic, 2 for 64-bit offset, 3 for netCDF-4, and 4 for netCDF-4 classic model. If the format supports chunking and compression (formats 3 and 4) those settings can be supplied through the -c option.

-h prints a column header line before the data line. Use this on the first call in a script and omit it for subsequent calls so that the header appears only once above all the data rows.

-c V:Z:S:C:C:C[,...] specifies deflate compression level, shuffle, and chunk sizes for one or more variables. The fields, separated by colons, are: variable index V, deflate level Z (0 means no compression, 1-9 are zlib levels, -1 leaves the variable contiguous), shuffle flag S (0 or 1), and then one chunk size per dimension. Multiple variable specifications are separated by commas. For example, 0:-1:0:1024:256:256 means variable 0, no compression, no shuffle, chunks of 1024 by 256 by 256.

-t V:S:S:S[,...], -u V:C:C:C[,...], and -r V:I:I:I[,...] together specify explicit start, count, and increment arrays for reading and writing. V is the variable index and the remaining colon-separated values are the per-dimension start positions, counts, and increments respectively. These three options must be used together and cannot be combined with the -s option or with parallel I/O.

-d enables a doublecheck pass. After writing the output file, bm_file copies it to a second temporary file and rereads it, reporting the reread time alongside the original read and write times. The reread also verifies that the output file contains the same metadata as the input.

-m enables a full value-by-value comparison during the doublecheck pass. This is much slower than -d alone for large files and should be reserved for validation rather than benchmarking.

-p activates parallel I/O using MPI. The program must have been built with parallel support for this to work.

-i selects MPI-IO as the parallel I/O transport instead of the default parallel HDF5 path. This option is only meaningful in parallel builds.

-s N sets the denominator for the fraction of the slowest-varying dimension read in each step. The default is 10, meaning each step reads one-tenth of the records at a time. For files with a small number of records along the unlimited dimension a larger value may be needed. This option cannot be combined with -t/-u/-r.

-e 1|2 sets the endianness of all variables in the output file. 1 requests little-endian storage and 2 requests big-endian storage. Omitting this option leaves the library to choose the native byte order.

-l converts any unlimited (record) dimension in the input file to a fixed-length dimension in the output file.

-y applies Zstandard compression instead of zlib deflate when a compression level is specified through -c.

-q N quantizes floating-point data to N significant decimal digits using the GranularBR algorithm before writing. This can substantially reduce file size with minimal loss of precision for geophysical data.

## Output Format

Each run appends one line of comma-separated values to standard output. The columns are: input format number, output format number, input file size in bytes, output file size in bytes, metadata read time in microseconds, metadata write time in microseconds, data read time in microseconds, data write time in microseconds, endianness setting, and then, when -d was given, metadata reread time, data reread time, read rate, write rate, and reread rate in MB/s. When -p is given a processor count column follows. The line ends with deflate level, shuffle flag, and up to four chunk sizes per variable option, plus the zstandard and nsd fields.

A header line produced by -h names each column:

```
input format, output_format, input size, output size, meta read time, meta write time,
data read time, data write time, enddianness, metadata reread time, data reread time,
read rate, write rate, reread rate, deflate, shuffle, chunksize[0], chunksize[1],
chunksize[2], chunksize[3], zstandard, nsd
```

## Using bm_file for Benchmarking

A single run tells you the performance for one combination of settings. To find the best settings for a particular file you need to sweep across a range of chunk shapes, compression levels, and possibly formats. The standard approach is a shell script that calls bm_file in a loop, printing the header once and then adding one data row per parameter combination. The resulting table can be pasted directly into a spreadsheet for graphing.

The following example from run_bm_elena.sh benchmarks a 3D integer file across six chunk-shape choices, all writing to netCDF-4 format with no compression and with a doublecheck of the output:

```sh
#!/bin/sh
echo "*** Testing the benchmarking program bm_file for simple float file, no compression..."
./bm_file -h -d -f 3 -o tst_elena_out.nc -c 0:-1:0:1024:16:256  tst_elena_int_3D.nc
./bm_file    -d -f 3 -o tst_elena_out.nc -c 0:-1:0:1024:256:256 tst_elena_int_3D.nc
./bm_file    -d -f 3 -o tst_elena_out.nc -c 0:-1:0:512:64:256   tst_elena_int_3D.nc
./bm_file    -d -f 3 -o tst_elena_out.nc -c 0:-1:0:512:256:256  tst_elena_int_3D.nc
./bm_file    -d -f 3 -o tst_elena_out.nc -c 0:-1:0:256:64:256   tst_elena_int_3D.nc
./bm_file    -d -f 3 -o tst_elena_out.nc -c 0:-1:0:256:256:256  tst_elena_int_3D.nc
echo '*** SUCCESS!!!'
```

The -h flag appears only on the first call so the header is printed once. Subsequent calls omit it, producing a table where each row corresponds to one chunk shape.

The output from a loop like this looks like the following:

```
*** Running benchmarking program bm_file for simple shorts test files, 1D to 6D...
input format, output_format, input size, output size, meta read time, meta write time, data read time, data write time, enddianness, metadata reread time, data reread time, read rate, write rate, reread rate, deflate, shuffle, chunksize[0], chunksize[1], chunksize[2], chunksize[3]
1, 4, 200092, 207283, 1613, 1054, 409, 312, 0, 1208, 1551, 488.998, 641.026, 128.949, 0, 0, 100000, 0, 0, 0
1, 4, 199824, 208093, 1545, 1293, 397, 284, 0, 1382, 1563, 503.053, 703.211, 127.775, 0, 0, 316, 316, 0, 0
1, 4, 194804, 204260, 1562, 1611, 390, 10704, 0, 1627, 2578, 499.159, 18.1868, 75.5128, 0, 0, 46, 46, 46, 0
1, 4, 167196, 177744, 1531, 1888, 330, 265, 0, 12888, 1301, 506.188, 630.347, 128.395, 0, 0, 17, 17, 17, 17
1, 4, 200172, 211821, 1509, 2065, 422, 308, 0, 1979, 1550, 473.934, 649.351, 129.032, 0, 0, 10, 10, 10, 10
1, 4, 93504, 106272, 1496, 2467, 191, 214, 0, 32208, 809, 488.544, 436.037, 115.342, 0, 0, 6, 6, 6, 6
*** SUCCESS!!!
```

The read rate, write rate, and reread rate columns in MB/s are the most directly useful for comparing parameter choices. A chunk shape that gives high read rates but mediocre write rates is appropriate when data is written once and read many times, which is the common case for scientific archives.

## Controlling the Read Pattern

By default bm_file reads each variable one step at a time along the slowest-varying dimension, where each step covers 1/slow_count of the total records. The -s N option adjusts slow_count, so -s 1 reads the entire variable in a single step while larger values break it into more, smaller reads.

When the expected access pattern for a variable is known in advance, the -t, -u, and -r options give full control over the start index, element count, and step increment for each dimension of each variable. This is useful for simulating the specific hyperslab reads that a downstream application is likely to perform. The following example from run_bm_test2.sh reads a single element at a time, stepping by one across variable 0:

```sh
./bm_file -h -f 3 -o tst_simple_3.nc -c 0:-1:0:10 \
          -t 0:0 -u 0:1 -r 0:1 tst_simple.nc
```

## Compression and Quantization

Deflate compression is selected by giving a compression level of 1 through 9 in the -c argument. Level 1 is fastest with the least compression; level 9 is slowest with the most. Shuffle byte reordering, enabled by setting the shuffle field in -c to 1, almost always improves compression ratios for numerical data and adds negligible overhead, so it is worth testing alongside any deflate level.

Zstandard compression, enabled by -y, can achieve better compression ratios than zlib at similar or faster speeds for many scientific datasets. When -y is given the compression level in the -c argument is passed to the Zstandard library instead of zlib.

The -q N option applies lossy quantization before compression. It rounds each floating-point value to N significant decimal digits, which dramatically increases the compressibility of the data because the trailing bits that carry no real information become zero. For many geophysical variables, 3 to 5 significant digits is sufficient to preserve all scientifically meaningful precision. Quantization is most effective when combined with deflate or Zstandard compression.

## Parallel I/O

When netCDF has been built with parallel HDF5 support, bm_file can perform parallel reads and writes using MPI. Run it under mpirun or mpiexec with the -p option to enable parallel I/O. Each MPI process handles a contiguous block of the slowest-varying dimension, and timing is collected with MPI_Wtime and reduced across all ranks with MPI_Reduce so that the output reflects the maximum time seen by any rank. The -i option switches the parallel transport from the default parallel HDF5 path to MPI-IO.

Note that the -t/-u/-r start/count/increment options are not available in parallel mode.

## Interpreting Results

After importing the CSV output into a spreadsheet, plot read rate and write rate against the chunk sizes or compression levels you tested. Look for the parameter combination that maximizes the rate relevant to your use case. If data consumers read the file in sequential record order, a chunk shape that aligns with that order and fits comfortably in the HDF5 chunk cache will typically give the best read performance. If consumers read spatial subsets that cross many records, a chunk shape with smaller extent along the record dimension and larger extent along the spatial dimensions may be preferable.

The metadata read and write times report how long it takes to open the file and read or write the header. For files with many variables or attributes this can be a significant fraction of total access time, and choosing a more compact format or reducing unnecessary metadata can help.

The doublecheck reread rate reflects cold-cache performance on a file that was just written, which is a reasonable lower bound on what users will experience. When the reread rate is much lower than the initial read rate, the chunk cache is likely being thrashed and a larger or differently shaped chunk may help.

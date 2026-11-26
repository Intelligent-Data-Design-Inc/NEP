# NEP Example Programs

This directory contains example programs exercising NEP features: the NEXTCDF-4 backend, the UDF format readers, and disk-bound performance benchmarks.

The generic NetCDF teaching examples (classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and all Fortran versions) moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0. The mission examples moved to [Earth Observation in Practice](https://github.com/captainkirk99/Earth_Observation_in_Practice) in v4.1.0.

## Example Categories

### NEXTCDF-4 Examples (C)
Located in `nextcdf4/` (built when `-DNEP_ENABLE_NEXTCDF4=ON`):
- **bfloat16.c** - Store real GDAS temperature data as NC_BFLOAT16 with the nc_*_bfloat16() functions
- **bfloat16_perf.c** - float32 vs bfloat16 (user-code and HDF5 conversion) size and I/O benchmark, CSV output
- **bfloat16_io_perf.c** - float32 vs bfloat16 write/read benchmark with data already in bfloat16 (no conversion), CSV output
- **make_bfloat16_sample.c** - Convert the GDAS sample to a NEXTCDF-4 NC_BFLOAT16 file (used with make_bfloat16_sample.sh)
- **make_bfloat16_sample.sh** - Run make_bfloat16_sample, then h5repack to apply shuffle+zstd chunking
- **plot_bfloat16.py** - Plot the benchmark CSV (size, speed and HDF5-conversion PNGs)
- **plot_bfloat16_io.py** - Plot the no-conversion I/O benchmark CSV (bfloat16_io.png)
- **float16.c** - Store real GDAS temperature data as NC_FLOAT16 with the nc_*_float16() functions
- **float16_perf.c** - float32 vs float16 (user-code and internal conversion) size and I/O benchmark, CSV output
- **float16_io_perf.c** - float32 vs float16 write/read benchmark with data already in float16 (no conversion), CSV output
- **make_float16_sample.c** - Convert the GDAS sample to a NEXTCDF-4 NC_FLOAT16 file (used with make_float16_sample.sh)
- **make_float16_sample.sh** - Run make_float16_sample, then h5repack to apply shuffle+zstd chunking
- **plot_float16.py** - Plot the benchmark CSV (size, speed and typed-conversion PNGs)
- **plot_float16_io.py** - Plot the no-conversion I/O benchmark CSV (float16_io.png)
- **conversion_perf.c** - `conversion_perf RESULTS.csv`: representative standard numeric and quantized-write cases, without an external plugin; 10 copies, minimum of 10 repetitions, both storage layouts
- **plot_conversion_compare.py** - Plot paired engine-labeled GDAS measurements or `--numeric` standard-conversion measurements; validates complete, unique matrices and never overwrites historical charts

Typed bfloat16 benchmark rows were `bfloat16_hdf5` with internal conversion OFF and `bfloat16_internal` with ON; the internal engine is now permanent, so new runs are labeled `bfloat16_internal`. The legacy plot scripts describe historical OFF results; use the comparison plotter for paired runs:

```sh
python3 examples/nextcdf4/plot_conversion_compare.py OFF.csv ON.csv OFF_IO.csv ON_IO.csv PREFIX
python3 examples/nextcdf4/plot_conversion_compare.py --numeric OFF_NUMERIC.csv ON_NUMERIC.csv PREFIX
```

The first form writes `PREFIX_typed.png` and `PREFIX_io.png`; the second writes `PREFIX_numeric.png`. The synthetic CSV columns are `engine,case,storage,copies,nvalues,memory_bytes,file_bytes,write_s,read_s,write_MBps,read_MBps`. Its throughput counts memory bytes per case; the GDAS programs count float32-equivalent bytes. Raw-I/O rows are controls, not converting-path measurements.

Build with `-DNEP_ENABLE_NEXTCDF4=ON -DNEP_BUILD_EXAMPLES=ON -DNEP_ZSTD_PLUGIN_DIR=<dir>`;
add `-DNEP_ENABLE_BENCHMARKS=ON` to register the benchmark tests.
Benchmarks should be built with `-DCMAKE_BUILD_TYPE=RelWithDebInfo` for
representative timings; the benchmark programs warn when built without
optimization. The bfloat16, float16 and `performance/` benchmarks use
`examples/perf_disk.h` to `fsync()` each file inside the write time and
drop it from the page cache before each read, so their times are
disk-bound.

### Performance Benchmarks (C)
Located in `performance/`:
- **cache_tuning.c** - Chunk cache configuration: default vs enlarged cache, file-level vs variable-level settings
- **chunking.c** - Chunk shape selection across access patterns, CSV output
- **chunking_performance.c** - Chunking strategies across contiguous, time-optimized, spatial-optimized, and balanced layouts (moved from `netcdf-4/` in v4.3.0)
- **deflate.c** - Deflate levels 0–9 with and without shuffle
- **zstandard.c** - Zstandard level sweep via the HDF5 plugin
- **szip.c** - SZIP coding method sweep (built only when an SZIP library is found)
- **endianness.c** - Byte-order effects on I/O performance
- **fill_values.c** - Fill mode NC_FILL vs NC_NOFILL cost

Benchmarks run as tests only with `-DNEP_ENABLE_BENCHMARKS=ON`. LZ4/BZIP2 benchmarks are maintained by [nep-compress](https://github.com/captainkirk99/nep-compress).

### Format Reader Examples (C)
- **dicom_read.c** (`dicom/`) - Opens a DICOM image via `NC_DICOM_initialize()`/`NC_UDF6`, prints metadata, and reads a slice of pixel data. Built when `-DNEP_ENABLE_DICOM=ON`.
- **pdb_read.c** (`pdb/`) - Opens a legacy PDB structure via `NC_PDB_initialize()`/`NC_UDF7`, prints metadata, and reads a slice of atom coordinates. Built when `-DNEP_ENABLE_PDB=ON` (default OFF).

### Visualization Examples (Python)
Located in `viz/` (built when `-DNEP_ENABLE_VIZ_EXAMPLES=ON`): open enabled NEP UDF files through `netCDF4.Dataset` and write static grayscale PNG plots with companion metadata files. See `viz/README.md`.

## Building Examples

### CMake
```bash
# Build with examples enabled
cmake -B build -DNEP_BUILD_EXAMPLES=ON
cmake --build build

# Disable examples
cmake -B build -DNEP_BUILD_EXAMPLES=OFF
cmake --build build
```

## Running Examples

Examples are automatically run as tests when built:

```bash
ctest --test-dir build
```

Each example creates NetCDF output files demonstrating the features being illustrated.

## Output Validation

The `cache_tuning` and `chunking_performance` tests are validated against expected CDL (Common Data Language) files stored in `expected_output/` using `validate_cdl.sh` (generated from `validate_cdl.sh.in`). This ensures examples continue producing correct output across code changes.

## Documentation

All examples are documented with Doxygen comments explaining:
- What the example demonstrates
- Compilation and usage instructions
- Expected output

### Viewing Documentation

The examples are integrated into the main NEP Doxygen documentation. After building the documentation:

```bash
# Build documentation
cmake -B build -DNEP_BUILD_DOCUMENTATION=ON
cmake --build build --target docs

# View in browser
open build/docs/html/examples.html
```

Or view the online documentation at: https://intelligent-data-design-inc.github.io/NEP/

## Dependencies

- NetCDF-C library (required)
- C99 compiler
- Python 3 virtual environment with NumPy, Matplotlib, and source-built `netCDF4` (for visualization examples)

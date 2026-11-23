# NEP Example Programs

NEP's example programs exercise NEP features: the NEXTCDF-4 backend, the UDF format readers, and the disk-bound performance benchmarks.

The generic NetCDF teaching examples — classic NetCDF, NetCDF-4, NcZarr, OPeNDAP remote access, parallel I/O, and all Fortran versions — moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0, where they are maintained as companion code for *[The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

See `examples/README.md` for build instructions and usage details.

## NEXTCDF-4 (`examples/nextcdf4/`)

Built when NEXTCDF-4 is enabled (`-DNEP_ENABLE_NEXTCDF4=ON`).

- `bfloat16.c` — Store real GDAS temperature data as `NC_BFLOAT16` with the `nc_*_bfloat16()` functions
- `float16.c` — Store real GDAS temperature data as `NC_FLOAT16` with the `nc_*_float16()` functions
- `make_bfloat16_sample.c` / `make_float16_sample.c` — Convert the GDAS sample to NEXTCDF-4 small-float files (driven by `make_*_sample.sh`)
- `bfloat16_perf.c` / `float16_perf.c` — float32 vs 16-bit float size and I/O benchmarks, CSV output
- `bfloat16_io_perf.c` / `float16_io_perf.c` — Write/read benchmarks with data already in 16-bit floats (no conversion)
- `conversion_perf.c` — Standard numeric conversion and quantized writes benchmark, CSV output
- `plot_*.py` — Plot scripts for the benchmark CSVs

## Performance Benchmarks (`examples/performance/`)

C only. Benchmarks run as tests with `-DNEP_ENABLE_BENCHMARKS=ON` (CMake).
All benchmark numbers are disk-bound: write time includes `fsync`, and
each read starts with the file dropped from the page cache (see
`examples/perf_disk.h`). Reference results are measured with
`-DCMAKE_BUILD_TYPE=RelWithDebInfo` (-O2).

- `cache_tuning.c` — Chunk cache size and slots effect on read performance
- `chunking.c` — Row-optimized, column-optimized, and balanced chunk shape comparison
- `chunking_performance.c` — Chunking strategies across contiguous, time-optimized, spatial-optimized, and balanced layouts (moved from `examples/netcdf-4/` in v4.3.0)
- `deflate.c` — Deflate compression levels 1–9 speed and ratio benchmarking
- `zstandard.c` — Zstandard (ZSTD) compression benchmarking
- `szip.c` — SZIP compression benchmarking (built only when an SZIP library is found)
- `endianness.c` — Byte order effects on I/O performance
- `fill_values.c` — Fill value storage and read performance

LZ4/BZIP2-specific benchmark programs, CSVs and plotters are maintained by
[nep-compress](https://github.com/captainkirk99/nep-compress).

## UDF Reader Examples (`examples/dicom/`, `examples/pdb/`)

- `dicom/dicom_read.c` — Read a DICOM file through the NEP DICOM UDF handler (built when `-DNEP_ENABLE_DICOM=ON`)
- `pdb/pdb_read.c` — Read a PDB file through the NEP PDB UDF handler (built when `-DNEP_ENABLE_PDB=ON`)

## Visualization Examples (`examples/viz/`)

Python visualization examples open NEP UDF files directly with `netCDF4.Dataset` and write static Matplotlib PNGs. They are optional and disabled by default. The examples cover FITS images, CDF temperature data, GeoTIFF rasters, GRIB2 wind grids, and PDS4 MESSENGER, Perseverance, MAVEN, and New Horizons products.

Enable visualizations with `-DNEP_ENABLE_VIZ_EXAMPLES=ON` for CMake or `--enable-viz-examples` for Autotools. Normal examples and at least one UDF reader must also be enabled. A source-root `.venv` must provide NumPy, Matplotlib, and `netCDF4` built from source against the exact NetCDF-C installation used by NEP; wheels or system packages using another `libnetcdf` cannot load NEP UDF handlers.

Tests use the generated build-tree `.ncrc`: `NCRCENV_RC` selects the file, `NETCDF_RC` points to its directory, and `LD_LIBRARY_PATH` includes the NEP UDF libraries. Generated PNGs and `<basename>_metadata.txt` files remain in the visualization build directory and are neither installed nor written to the source tree. Each plot is grayscale, no caption appears inside its PNG, figures are limited to 8.0 by 6.1 inches at 150 DPI, and metadata contains ordered non-empty `title`, `caption`, and `alt_text` fields; captions are limited to 75 words.

See `examples/viz/README.md` for setup, build, test, and direct-execution instructions.

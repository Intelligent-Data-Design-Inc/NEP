# NEP Example Programs

NEP includes over 55 C and Fortran example programs covering classic NetCDF, NetCDF-4, NcZarr, OPeNDAP remote access, performance tuning, and parallel I/O.

They are companion code for *[The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

See `examples/README.md` for build instructions and usage details.

## Classic NetCDF (`examples/classic/`, `examples/f_classic/`)

Both C and Fortran versions provided for all programs.

- `quickstart.c` / `f_quickstart.f90` — Minimal introduction; the simplest starting point
- `simple_2D.c` / `f_simple_2D.f90` — Basic 2D arrays with dimensions and variables
- `coord_vars.c` / `f_coord_vars.f90` — Coordinate variables and named dimensions
- `coord.c` / `f_coord.f90` — Multi-dimensional coordinate systems
- `dump_classic_metadata.c` / `f_dump_classic_metadata.f90` — Reading and printing all metadata
- `size_limits.c` / `f_size_limits.f90` — Classic format size limits and 64-bit offset format
- `unlimited_dim.c` / `f_unlimited_dim.f90` — Unlimited dimensions and record variables
- `var4d.c` / `f_var4d.f90` — 4-dimensional variables

## NetCDF-4 (`examples/netcdf-4/`, `examples/f_netcdf-4/`)

Both C and Fortran versions provided for all programs.

- `simple_nc4.c` / `f_simple_nc4.f90` — NetCDF-4 file creation basics
- `format_variants.c` / `f_format_variants.f90` — Classic, 64-bit, CDF-5, and NetCDF-4 formats
- `compression.c` / `f_compression.f90` — Deflate and shuffle compression filters
- `chunking_performance.c` / `f_chunking_performance.f90` — Chunking strategies and performance impact
- `multi_unlimited.c` / `f_multi_unlimited.f90` — Multiple independent unlimited dimensions
- `user_types.c` / `f_user_types.f90` — Compound and enum user-defined types
- `groups.c` / `f_groups.f90` — Hierarchical groups and dimension visibility
- `dump_nc4_metadata.c` / `f_dump_nc4_metadata.f90` — Reading and printing NetCDF-4 metadata

## NcZarr (`examples/nczarr/`)

Both C and Fortran versions provided for all programs.

- `nczarr_simple.c` / `f_nczarr_simple.f90` — Local NcZarr create/read/write with `file://...#mode=nczarr` URLs
- `nczarr_chunking.c` / `f_nczarr_chunking.f90` — Explicit chunk shape with `nc_def_var_chunking()`
- `nczarr_compression.c` / `f_nczarr_compression.f90` — Deflate + shuffle compression in NcZarr stores
- `nczarr_enhanced.c` / `f_nczarr_enhanced.f90` — Hierarchical groups and multiple unlimited dimensions

## OPeNDAP Remote Access (`examples/opendap/`)

Both C and Fortran versions provided for all programs.

- `opendap_simple.c` / `f_opendap_simple.f90` — Open and read a remote dataset via OPeNDAP URL
- `opendap_subset.c` / `f_opendap_subset.f90` — Read a hyperslab from a remote variable
- `opendap_constraint.c` / `f_opendap_constraint.f90` — Apply constraint expressions to filter remote data

## Visualization Examples (`examples/viz/`)

Python visualization examples open NEP UDF files directly with `netCDF4.Dataset` and write static Matplotlib PNGs. They are optional and disabled by default. The examples cover FITS images, CDF temperature data, GeoTIFF rasters, GRIB2 wind grids, and PDS4 MESSENGER, Perseverance, MAVEN, and New Horizons products.

Enable visualizations with `-DNEP_ENABLE_VIZ_EXAMPLES=ON` for CMake or `--enable-viz-examples` for Autotools. Normal examples and at least one UDF reader must also be enabled. A source-root `.venv` must provide NumPy, Matplotlib, and `netCDF4` built from source against the exact NetCDF-C installation used by NEP; wheels or system packages using another `libnetcdf` cannot load NEP UDF handlers.

Tests use the generated build-tree `.ncrc`: `NCRCENV_RC` selects the file, `NETCDF_RC` points to its directory, and `LD_LIBRARY_PATH` includes the NEP UDF libraries. Generated PNGs and `<basename>_metadata.txt` files remain in the visualization build directory and are neither installed nor written to the source tree. Each plot is grayscale, no caption appears inside its PNG, figures are limited to 8.0 by 6.1 inches at 150 DPI, and metadata contains ordered non-empty `title`, `caption`, and `alt_text` fields; captions are limited to 75 words.

See `examples/viz/README.md` for setup, build, test, and direct-execution instructions.

## Performance and Compression Benchmarking (`examples/performance/`)

C only. Built with `-DENABLE_BENCHMARKS=ON` (CMake) or `--enable-benchmarks` (Autotools).

- `cache_tuning.c` — Chunk cache size and slots effect on read performance
- `chunking.c` — Row-optimized, column-optimized, and balanced chunk shape comparison
- `deflate.c` — Deflate compression levels 1–9 speed and ratio benchmarking
- `lz4.c` — LZ4 compression performance vs deflate
- `bzip2.c` — BZIP2 compression performance vs deflate
- `zstandard.c` — Zstandard (ZSTD) compression benchmarking
- `szip.c` — SZIP compression benchmarking
- `lossless.c` — Lossless compression algorithm comparison
- `quantize.c` — Quantization for lossy compression and storage reduction
- `endianness.c` — Byte order effects on I/O performance
- `fill_values.c` — Fill value storage and read performance

## Parallel I/O (`examples/parallelIO/`)

Built only when parallel tests are enabled (`--enable-parallel-tests` / `-DENABLE_PARALLEL_TESTS=ON`).

- `square16_par.c` / `f_square16_par.f90` — MPI-based parallel NetCDF-4 write and read

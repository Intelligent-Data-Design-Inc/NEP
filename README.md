![NEP Logo](docs/images/logo_small.png)

# NEP (NetCDF Expansion Pack)

**[📚 Full Documentation](https://intelligent-data-design-inc.github.io/NEP/)**

---

## Summary

NEP extends NetCDF-4 with powerful new capabilities:
* more compression filters
* read layers for different data formats
* extra examples covering advanced netCDF use

### Compression

Additional compression options for netCDF data.

| Algorithm | Description | Best For |
|---|---|---|
| **LZ4** | Faster than zstandard | Real-time processing, HPC I/O |
| **BZIP2** | Very slow writes but best compression | Long-term archives, bandwidth-limited |

### Multi-Format Readers

Read other formats with the netcdf-c library, as if they were netCDF files.

| Format | Domain |
|---|---|
| **NASA CDF** | Space physics (IMAP, MMS, Van Allen) |
| **GeoTIFF** | Geospatial rasters, CF-1.8 CRS metadata |
| **GRIB2** | NWP model output (GFS, NAM, HRRR, wave) |
| **FITS** | Astronomical images and tables (HST, JWST) |
| **PDS4** | NASA/ESA planetary science (Mars, Moon, etc.) |

All readers use the standard NetCDF UDF system. Once enabled, **existing C and Fortran programs require no code modification** — an existing Fortran program that calls `nf90_open()` and `nf90_get_var()` can read a FITS, CDF, GeoTIFF, GRIB2, or PDS4 file without changing code.

Enable one or more readers at configure time:

```bash
# CMake
cmake -B build \
  -DENABLE_GEOTIFF=ON \
  -DENABLE_GRIB2=ON \
  -DENABLE_CDF=ON \
  -DENABLE_FITS=ON \
  -DENABLE_PDS4=ON

# Autotools
./configure \
  --enable-geotiff \
  --enable-grib2 \
  --enable-cdf \
  --enable-fits \
  --enable-pds4
```

### Example Programs

NEP includes over 26 C and Fortran example programs, organized by topic. They are companion code for *[The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

| Category | Examples |
|---|---|
| **Classic NetCDF** | Quickstart, 2D arrays, coord vars, format variants, unlimited dims |
| **NetCDF-4** | Compression, chunking, groups, compound/enum types, multiple unlimited dims |
| **NcZarr** | Local store create/read/write, chunking, deflate compression |
| **Performance** | Chunk cache tuning, chunking strategy comparison, compression benchmarking |
| **Parallel I/O** | MPI-based parallel write and read |

Both C and Fortran versions provided for Classic, NetCDF-4, and NcZarr categories.

---

## Compression

NEP adds LZ4 and BZIP2 HDF5 filter plugins for NetCDF-4 files, enabled by default. LZ4 favors speed (2–3× faster than DEFLATE); BZIP2 favors maximum compression ratio (up to 6.7× on typical scientific datasets). See [docs/compression.md](docs/compression.md) for algorithm details, C/Fortran API reference, and performance benchmarks.

---

## Installation

### Build Requirements

- **NetCDF-C >= 4.10.1** (required for UDF slots > 2, which NEP uses for FITS, CDF, and PDS4)
- **HDF5 >= 1.10** (required by NetCDF-4)
- C compiler and, optionally, a Fortran 90+ compiler
- Doxygen and Graphviz (only if building documentation)

### CMake Build and Installation

```bash
# Configure
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
cmake --build build

# Build documentation (optional)
cmake --build build --target docs

# Install
cmake --install build

# Uninstall (if needed)
cmake --build build --target uninstall
```

**Note:** If HDF5 is installed in a non-standard location, you may need to specify `HDF5_ROOT`:

```bash
cmake -B build -DHDF5_ROOT=/path/to/hdf5 -DCMAKE_INSTALL_PREFIX=/usr/local
```

For example, if HDF5 is installed in `/usr/local/hdf5-1.14.6`:

```bash
cmake -B build -DHDF5_ROOT=/usr/local/hdf5-1.14.6 -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Autotools Build and Installation

```bash
# Bootstrap and configure
./autogen.sh
./configure --prefix=/usr/local --enable-lz4 --enable-bzip2

# Build
make

# Build documentation (optional)
make docs

# Install
make install

# Uninstall (if needed)
make uninstall
```

### Configuration Options

#### Quick Reference

| Option (CMake / Autotools) | Default | Purpose |
|---|---|---|
| `-DBUILD_LZ4` / `--enable-lz4` | ON | LZ4 compression filter |
| `-DBUILD_BZIP2` / `--enable-bzip2` | ON | BZIP2 compression filter |
| `-DENABLE_FORTRAN` / `--enable-fortran` | ON | Fortran wrappers and tests |
| `-DENABLE_GEOTIFF` / `--enable-geotiff` | **OFF** | GeoTIFF UDF handler (UDF0/UDF1) |
| `-DENABLE_GRIB2` / `--enable-grib2` | **OFF** | GRIB2 UDF handler (UDF2) |
| `-DENABLE_FITS` / `--enable-fits` | **OFF** | FITS UDF handler (UDF3) |
| `-DENABLE_CDF` / `--enable-cdf` | **OFF** | NASA CDF UDF handler (UDF4) |
| `-DENABLE_PDS4` / `--enable-pds4` | **OFF** | NASA/ESA PDS4 UDF handler (UDF5) |
| `-DBUILD_EXAMPLES` / `--enable-examples` | ON | Example programs |
| `-DENABLE_BENCHMARKS` / `--enable-benchmarks` | OFF | Performance benchmark examples |
| `-DENABLE_PARALLEL_TESTS` / `--enable-parallel-tests` | OFF | MPI parallel I/O tests |
| `-DBUILD_DOCUMENTATION` / `--enable-docs` | ON | Doxygen API docs |

#### Compression

- **LZ4** (`-DBUILD_LZ4`, default ON): builds `libh5lz4.so` HDF5 filter plugin; provides `nc_def_var_lz4()` / `nf90_def_var_lz4()`. Requires liblz4.
- **BZIP2** (`-DBUILD_BZIP2`, default ON): builds `libh5bzip2.so` HDF5 filter plugin; provides `nc_def_var_bzip2()` / `nf90_def_var_bzip2()`. Requires libbz2.

#### Fortran

`-DENABLE_FORTRAN` / `--enable-fortran` (default ON): builds Fortran wrappers in `fsrc/`, Fortran tests in `ftest/`, and Fortran examples in `examples/f_*/`. Requires NetCDF-Fortran and a Fortran 90+ compiler.

#### Format Readers

All five format readers default to **OFF** and are independent — any combination can be enabled simultaneously.

| Format | CMake / Autotools | UDF Slot | Dependencies |
|--------|-------------------|----------|--------------|
| GeoTIFF | `-DENABLE_GEOTIFF` / `--enable-geotiff` | UDF0, UDF1 | libgeotiff, libtiff |
| GRIB2 | `-DENABLE_GRIB2` / `--enable-grib2` | UDF2 | NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0 |
| FITS | `-DENABLE_FITS` / `--enable-fits` | UDF3 | CFITSIO ≥ 3.0 |
| NASA CDF | `-DENABLE_CDF` / `--enable-cdf` | UDF4 | NASA CDF library v3.9.x |
| PDS4 | `-DENABLE_PDS4` / `--enable-pds4` | UDF5 | libxml2 ≥ 2.9 |

- NASA CDF library: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/ (or `spack install cdf`)
- CDF moved from UDF2 to UDF4 in v2.2.0; GRIB2 and CDF can now be enabled together.

#### Examples and Benchmarks

- **`-DBUILD_EXAMPLES`** / `--enable-examples` (default ON): builds all example programs and registers them as tests.
- **`-DENABLE_BENCHMARKS`** / `--enable-benchmarks` (default OFF): builds compression benchmark programs in `examples/performance/`.
- **`-DENABLE_PARALLEL_TESTS`** / `--enable-parallel-tests` (default OFF): builds `examples/parallelIO/` and runs tests via `mpiexec -n 4`. Requires MPI and NetCDF-C with `NC_HAS_PARALLEL4`.

#### Documentation

`-DBUILD_DOCUMENTATION` / `--enable-docs` (default ON): generates Doxygen API documentation from C and Fortran sources, published to GitHub Pages. Requires Doxygen and Graphviz.

### Using NEP in Your Project

LZ4 and BZIP2 compression are provided as HDF5 filter plugins. Simply set the `HDF5_PLUGIN_PATH` environment variable to the NEP installation directory, and use standard NetCDF-4 compression APIs.

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd.md)** - Detailed product requirements and specifications
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

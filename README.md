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
  -DNEP_ENABLE_GEOTIFF=ON \
  -DNEP_ENABLE_GRIB2=ON \
  -DNEP_ENABLE_CDF=ON \
  -DNEP_ENABLE_FITS=ON \
  -DNEP_ENABLE_PDS4=ON

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
| `-DNEP_BUILD_LZ4` / `--enable-lz4` | ON | LZ4 compression filter |
| `-DNEP_BUILD_BZIP2` / `--enable-bzip2` | ON | BZIP2 compression filter |
| `-DNEP_ENABLE_FORTRAN` / `--enable-fortran` | ON | Fortran wrappers and tests |
| `-DNEP_ENABLE_GEOTIFF` / `--enable-geotiff` | **OFF** | GeoTIFF UDF handler (UDF0/UDF1) |
| `-DNEP_ENABLE_GRIB2` / `--enable-grib2` | **OFF** | GRIB2 UDF handler (UDF2) |
| `-DNEP_ENABLE_FITS` / `--enable-fits` | **OFF** | FITS UDF handler (UDF3) |
| `-DNEP_ENABLE_CDF` / `--enable-cdf` | **OFF** | NASA CDF UDF handler (UDF4) |
| `-DNEP_ENABLE_PDS4` / `--enable-pds4` | **OFF** | NASA/ESA PDS4 UDF handler (UDF5) |
| `-DNEP_BUILD_EXAMPLES` / `--enable-examples` | ON | Example programs |
| `-DNEP_ENABLE_BENCHMARKS` / `--enable-benchmarks` | OFF | Performance benchmark examples |
| `-DNEP_ENABLE_PARALLEL_TESTS` / `--enable-parallel-tests` | OFF | MPI parallel I/O tests |
| `-DNEP_BUILD_DOCUMENTATION` / `--enable-docs` | ON | Doxygen API docs |

#### Compression

- **LZ4** (`-DNEP_BUILD_LZ4`, default ON): builds `libh5lz4.so` HDF5 filter plugin; provides `nc_def_var_lz4()` / `nf90_def_var_lz4()`. Requires liblz4.
- **BZIP2** (`-DNEP_BUILD_BZIP2`, default ON): builds `libh5bzip2.so` HDF5 filter plugin; provides `nc_def_var_bzip2()` / `nf90_def_var_bzip2()`. Requires libbz2.

#### Fortran

`-DNEP_ENABLE_FORTRAN` / `--enable-fortran` (default ON): builds Fortran wrappers in `fsrc/`, Fortran tests in `ftest/`, and Fortran examples in `examples/f_*/`. Requires NetCDF-Fortran and a Fortran 90+ compiler.

#### Format Readers

All five format readers default to **OFF** and are independent — any combination can be enabled simultaneously.

| Format | CMake / Autotools | UDF Slot | Dependencies |
|--------|-------------------|----------|--------------|
| GeoTIFF | `-DNEP_ENABLE_GEOTIFF` / `--enable-geotiff` | UDF0, UDF1 | libgeotiff, libtiff |
| GRIB2 | `-DNEP_ENABLE_GRIB2` / `--enable-grib2` | UDF2 | NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0 |
| FITS | `-DNEP_ENABLE_FITS` / `--enable-fits` | UDF3 | CFITSIO ≥ 3.0 |
| NASA CDF | `-DNEP_ENABLE_CDF` / `--enable-cdf` | UDF4 | NASA CDF library v3.9.x |
| PDS4 | `-DNEP_ENABLE_PDS4` / `--enable-pds4` | UDF5 | libxml2 ≥ 2.9 |

- NASA CDF library: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/ (or `spack install cdf`)
- CDF moved from UDF2 to UDF4 in v2.2.0; GRIB2 and CDF can now be enabled together.

#### Examples and Benchmarks

- **`-DNEP_BUILD_EXAMPLES`** / `--enable-examples` (default ON): builds all example programs and registers them as tests.
- **`-DNEP_ENABLE_BENCHMARKS`** / `--enable-benchmarks` (default OFF): builds compression benchmark programs in `examples/performance/`.
- **`-DNEP_ENABLE_PARALLEL_TESTS`** / `--enable-parallel-tests` (default OFF): builds `examples/parallelIO/` and runs tests via `mpiexec -n 4`. Requires MPI and NetCDF-C with `NC_HAS_PARALLEL4`.

#### Documentation

`-DNEP_BUILD_DOCUMENTATION` / `--enable-docs` (default ON): generates Doxygen API documentation from C and Fortran sources, published to GitHub Pages. Requires Doxygen and Graphviz.

### Using NEP in Your Project

LZ4 and BZIP2 compression are provided as HDF5 filter plugins. Simply set the `HDF5_PLUGIN_PATH` environment variable to the NEP installation directory, and use standard NetCDF-4 compression APIs.

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

### Spack Installation

NEP can be installed using the [Spack](https://spack.io) package manager.

#### Default install

```bash
# Latest stable release
spack install nep@2.5.0

# Or install from the main branch
spack install nep
```

This builds NEP with LZ4, BZIP2, Fortran wrappers, and documentation — no format readers.

#### Variant reference

| Variant | Default | Description |
|---------|---------|-------------|
| `docs` | ON | Build Doxygen documentation (requires Doxygen) |
| `lz4` | ON | LZ4 compression filter (requires liblz4) |
| `bzip2` | ON | BZIP2 compression filter (requires libbz2) |
| `fortran` | ON | Fortran wrappers and tests (requires NetCDF-Fortran) |
| `fits` | OFF | FITS reader — HST, JWST, Chandra (requires CFITSIO) |
| `geotiff` | OFF | GeoTIFF reader — CF-1.8 CRS metadata (requires libgeotiff) |
| `grib2` | OFF | GRIB2 reader — NWP model output (requires NCEPLIBS-g2c) |
| `cdf` | OFF | NASA CDF reader — space physics (requires local Spack repo, see below) |
| `pds4` | OFF | PDS4 reader — planetary science (requires libxml2) |
| `parallel` | OFF | Parallel I/O tests (requires MPI, HDF5+mpi, netcdf-c+mpi) |
| `examples` | OFF | Example programs |
| `benchmarks` | OFF | Performance benchmark programs |

#### Common combinations

```bash
# GeoTIFF + GRIB2 (geospatial and NWP workflows)
spack install nep+geotiff+grib2

# All format readers
spack install nep+geotiff+grib2+cdf+fits+pds4

# Minimal build (no docs, no Fortran) for CI/containers
spack install nep~docs~fortran

# Parallel I/O (requires MPI stack)
spack install nep+parallel
```

#### Installing the NASA CDF variant (`+cdf`)

The NASA CDF library is not a Spack builtin. The in-repo `spack/cdf/package.py` recipe must be registered in a local Spack repository before concretizing `+cdf`:

```bash
# 1. Create a local Spack repository containing both NEP and CDF recipes
mkdir -p $HOME/nep-repo/packages/nep
mkdir -p $HOME/nep-repo/packages/cdf
cp spack/NEP/package.py $HOME/nep-repo/packages/nep/
cp spack/cdf/package.py $HOME/nep-repo/packages/cdf/
cat > $HOME/nep-repo/repo.yaml << 'EOF'
repo:
  namespace: nep-local
EOF

# 2. Register the repository with Spack
spack repo add $HOME/nep-repo

# 3. Install with +cdf
spack install nep+cdf
```

---

## Testing

Run the test suite after building:

```bash
# CMake
ctest --test-dir build

# Autotools
make check
```

### PDS4 Tests

`test/tst_pds4_udf.c` contains all PDS4 tests. Requires `--enable-pds4` / `-DNEP_ENABLE_PDS4=ON`.

| Test Name | Description | Mission / Source | Data Type |
|---|---|---|---|
| `test_table_binary()` | Verifies metadata for a synthetic binary table: 5-record dimension, three variables (`Timestamp`, `Detector_Counts`, `Temperature`) with units | Synthetic test data | `Table_Binary` |
| `test_table_character()` | Verifies metadata for a fixed-width character table: 224-record dimension, three `NC_DOUBLE` variables (`Wavelength`, `Reflectance`, `Error`) | Synthetic test data | `Table_Character` |
| `test_array_data_read()` | Reads a 4×4 float32 image array and verifies all 16 values and a sub-hyperslab `[1:2,1:2]` | Synthetic test data | Array (`Array_2D_Image`) |
| `test_table_binary_data_read()` | Reads binary table field values: `Timestamp[0..4]`, `Detector_Counts[0..4]`, `Temperature[0..4]` | Synthetic test data | `Table_Binary` |
| `test_table_character_data_read()` | Reads fixed-width character table values: `Wavelength[0..2]`, `Reflectance[0..2]`, `Error[0..2]` | Synthetic test data | `Table_Character` |
| `test_mission_cassini_hrd()` | Opens real HRD engineering table; verifies 11 records, two `NC_CHAR` fields (`ON_OFF_TIME`, `ON_OFF_FLAG`) | Cassini-Huygens HRD dust detector | `Table_Character` |
| `test_mission_messenger_tnmap()` | Opens real thermal neutron map; verifies 360×720 `NC_UBYTE` array and reads a hyperslab | MESSENGER Mercury orbiter | Binary array (`Array_2D_Map`) |
| `test_mission_lcs_9p()` | Opens real comet photometry table; verifies 8 variables (`ASCII_Integer`, `ASCII_Real`) and first-record values | Deep Impact LCS, 9P/Tempel comet | `Table_Character` |
| `test_mission_maven_l1b()` | Opens real L1B housekeeping table; verifies 324 variables, 26121 records, `TIME` is `NC_DOUBLE`, and `TIME[0..1] > 0` | MAVEN NGIMS, Mars atmosphere | `Table_Delimited` (324 fields, 9.3 MB CSV) |
| `test_mission_maven_l3()` | Opens real L3 science table; verifies 15 variables, 2 records, `T_UTC` is `NC_CHAR` (`ASCII_Date_Time`), and known values for `T_UNIX[0]`, `SCALE_HEIGHT[0]`, `TEMPERATURE[0]` | MAVEN NGIMS, Mars atmosphere | `Table_Delimited` (15 fields, 446 B CSV) |
| `test_mission_maven_iuvs_metadata()` | Opens FITS-backed PDS4 file; verifies 116 variables (scalar + `Group_Field_Binary`), `COLUMN` is 2D with trailing dim=2, `V_TANGENT` trailing dim=3, `TANGENT_ALT` is 1D `NC_FLOAT` | MAVEN IUVS, Mars corona | `Table_Binary` (8 tables, FITS container, `Group_Field_Binary`) |
| `test_mission_maven_iuvs_data()` | Reads scalar field `TANGENT_ALT[0]` (finite > 0 km) and group field `RADIANCE[50,0..1]` (finite > 0 kR), confirming FITS-as-container I/O and 2D group field reading | MAVEN IUVS, Mars corona | `Table_Binary` (FITS container, 181 KB) |
| `test_mission_maven_periapse_metadata()` | Opens FITS-backed PDS4 file; verifies nested `Group_Field_Binary` (depth-2), table-prefixed field names, `DENSITY_ALT` dims `[12,19,3]`, and `GEOMETRY_RETRIEVAL` scalar doubles | MAVEN IUVS, Mars periapse | `Table_Binary` (FITS container, nested groups) |
| `test_mission_maven_periapse_data()` | Reads scalar `LAT[0]` from `GEOMETRY_RETRIEVAL` and 3D nested group field `DENSITY_ALT[0,0,0]` (finite > 0) | MAVEN IUVS, Mars periapse | `Table_Binary` (nested groups) |
| `test_mission_perseverance_mastcamz_metadata()` | Opens Perseverance Sol 1738 `Array_3D_Image`; verifies `data` is `NC_SHORT`, dims `[3,1200,1648]`, and `scaling_factor="5.0e-06"` | Mars 2020 / Perseverance | `Array_3D_Image` (VICAR/ODL `.IMG`) |
| `test_mission_perseverance_mastcamz_data()` | Reads pixel `[0,0,0]` as `NC_SHORT` and checks valid `SignedMSB2` range | Mars 2020 / Perseverance | `Array_3D_Image` |
| `test_mission_perseverance_mastcamz_1737_metadata()` | Same metadata checks for the second Perseverance Sol 1737 `Array_3D_Image` product | Mars 2020 / Perseverance | `Array_3D_Image` |
| `test_mission_perseverance_mastcamz_1737_data()` | Reads pixel `[0,0,0]` from Sol 1737 and checks valid `SignedMSB2` range | Mars 2020 / Perseverance | `Array_3D_Image` |

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd.md)** - Detailed product requirements and specifications
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

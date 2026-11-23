![NEP Logo](docs/images/logo_small.png)

# NEP (NetCDF Expansion Pack)

**[📚 Full Documentation](https://intelligent-data-design-inc.github.io/NEP/)**

**[NEXTCDF-4 Documentation](docs/nextcdf4.md)**

[NEXTCDF-4 Fortran binding](docs/nextcdf4_fortran.md): enable NEXTCDF-4 and Fortran
to install `nep_nextcdf4.mod` / `libnepnextcdf4f` and use
`NEP::nepnextcdf4f`. Requires NetCDF-Fortran >=4.6.2 with compatible compiler
modules. Includes mode/type constants and scalar–rank-7 float16/bfloat16 bit-buffer
I/O; reference wrappers and general enhanced-type value interfaces remain deferred.

**[📄 NEP Abstract](docs/NEP_abstract.md)**

<a href="https://www.amazon.com/NetCDF-Developers-Handbook-Authoritative-High-Performance/dp/B0H7Q49MPS/">
  <img src="docs/images/netcdf_developers_handbook_second_edition.png" alt="Cover of The NetCDF Developer's Handbook, Second Edition" width="240">
</a>

[*The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition*](https://www.amazon.com/NetCDF-Developers-Handbook-Authoritative-High-Performance/dp/B0H7Q49MPS/) provides practical guidance and complete examples for NetCDF programming in C and Fortran.

---

## Summary

NEP extends NetCDF-C with scientific format readers, the NEXTCDF-4 backend,
and examples covering advanced NetCDF use. LZ4/BZIP2 compression lives in
[nep-compress](https://github.com/captainkirk99/nep-compress).

### Multi-Format Readers

Read other formats with the netcdf-c library, as if they were netCDF files.

| Format | Domain |
|---|---|
| **NASA CDF** | Space physics (IMAP, MMS, Van Allen) |
| **FITS** | Astronomical images and tables (HST, JWST) |
| **PDS4** | NASA/ESA planetary science (Mars, Moon, etc.) |
| **GeoTIFF** | Geospatial rasters, CF-1.8 CRS metadata |
| **GRIB2** | NWP model output (GFS, NAM, HRRR, wave) |
| **DICOM** | Medical imaging (CT, MR, XA, US) |
| **Legacy PDB / mmCIF** | Protein/macromolecular structures (RCSB) |

All readers use the standard NetCDF UDF system. Once enabled, **existing C and Fortran programs require no code modification** — an existing Fortran program that calls `nf90_open()` and `nf90_get_var()` can read a FITS, CDF, GeoTIFF, GRIB2, or PDS4 file without changing code.

Enable one or more readers at configure time:

```bash
# CMake
cmake -B build \
  -DNEP_ENABLE_GEOTIFF=ON \
  -DNEP_ENABLE_GRIB2=ON \
  -DNEP_ENABLE_CDF=ON \
  -DNEP_ENABLE_FITS=ON \
  -DNEP_ENABLE_PDS4=ON \
  -DNEP_ENABLE_DICOM=ON \
  -DNEP_ENABLE_PDB=ON \
  -DNEP_ENABLE_MMCIF=ON
```

### NEXTCDF-4

NEXTCDF-4 is NEP's clean-room, drop-in replacement backend for
netCDF-4/HDF5, delivered as a UDF expansion pack (`-DNEP_ENABLE_NEXTCDF4=ON`,
selected at run time with `NC_NEXTCDF4`). It is **experimental** in v4.3.0.

- Full netCDF-4 data model: groups, unlimited dimensions, attributes,
  user types (compound, enum, opaque, vlen), `NC_STRING`, filters
  (`nc_def_var_filter` with deflate/fletcher32/szip/bzip2/zstd plugins),
  chunking and quantize writes.
- New atomic types: 16-bit floats (`NC_FLOAT16`, `NC_BFLOAT16`) with
  NEP round-to-nearest-even conversion, FP8/FP6/FP4 storage types,
  complex and bitfield types, and `NC_STD_REF` HDF5 reference tokens
  with a `nc_ref_*`/`nc_deref_*` API.
- Compatibility modes write files readable by upstream netCDF-c:
  `NC_NETCDF4_MODEL` (Superblock v1) and `NC_CLASSIC_MODEL`.
- Built on the latest HDF5: Superblock v3 in native mode, requiring
  HDF5 2.1.1 and netcdf-c with the 64-slot UDF scheme.
- An optional Fortran module `nep_nextcdf4` provides mode/type
  constants and float16/bfloat16 I/O (see
  [nextcdf4_fortran.md](docs/nextcdf4_fortran.md)).

See [docs/nextcdf4.md](docs/nextcdf4.md) for the full backend reference
and [docs/nextcdf4_architecture.md](docs/nextcdf4_architecture.md) for
how it works internally.

### Example Programs

NEP's example programs exercise NEP features: NEXTCDF-4 small-float and conversion demos (`examples/nextcdf4/`), disk-bound performance benchmarks (`examples/performance/`), UDF reader examples (`examples/dicom/`, `examples/pdb/`), and optional Python UDF visualizations (`examples/viz/`).

The generic NetCDF teaching examples — classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and all Fortran versions — moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0, where they are maintained as companion code for the book.

### Earth Observation Examples

Mission-by-mission satellite-data examples, data-access instructions, and Python visualizations formerly hosted in NEP are maintained in the [Earth Observation in Practice](https://github.com/captainkirk99/Earth_Observation_in_Practice) repository.

The companion book, [*Earth Observation in Practice: A Mission-by-Mission Guide to Reading Satellite Data with NetCDF and Python*](https://www.amazon.com/Earth-Observation-Practice-Mission-Mission/dp/B0HJRZ9F7L/), provides additional background and worked examples. These external resources are informational and are not build-time, run-time, test-time, or packaging dependencies of NEP.

---

## Compression

LZ4/BZIP2 plugins, C/Fortran APIs, tests, and documentation moved to
[nep-compress](https://github.com/captainkirk99/nep-compress). See its
[compression guide](https://github.com/captainkirk99/nep-compress/blob/main/docs/compression.md)
when upgrading an application that previously used NEP's `<nep.h>`,
`libnep`, or Fortran `use nep`.

---

## Installation

### Build Requirements

- **CMake >= 3.19**
- **NetCDF-C >= 4.10.1** (required for UDF slots > 2, which NEP uses for FITS, CDF, and PDS4); NEXTCDF-4 additionally requires the 64-slot UDF scheme (netcdf-c >= 4.10.2 or current main)
- **HDF5 >= 1.14.0** for the format readers; **HDF5 2.1.1** required by NEXTCDF-4
- C compiler and, optionally, a Fortran 90+ compiler
- Doxygen and Graphviz (only if building documentation)

### Ubuntu / Debian Dependencies

The core build tools and optional format/reader packages can be installed with `apt`.
NetCDF-C and HDF5 should usually be built from source to guarantee matching versions (see below).

| Feature | Install command | Notes |
|---|---|---|
| Core build tools | `sudo apt-get install build-essential cmake pkg-config` | Required for every build |
| Fortran format tests / examples | `sudo apt-get install gfortran` | Also requires a matching NetCDF-Fortran installation |
| Documentation | `sudo apt-get install doxygen graphviz` | Optional |
| GeoTIFF reader | `sudo apt-get install libgeotiff-dev libtiff-dev` | |
| FITS reader | `sudo apt-get install libcfitsio-dev` | |
| GRIB2 reader | `sudo apt-get install libjasper-dev` | NCEPLIBS-g2c must be built separately |
| NASA CDF reader | — | Not packaged; build from [SPDF CDF](https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/) or use `spack install cdf` |
| PDS4 reader | `sudo apt-get install libxml2-dev` | |
| DICOM reader | `sudo apt-get install libdicom-dev libjpeg-dev libgdcm-dev` | `libgdcm-dev` is required because most DICOM images use JPEG Lossless transfer syntaxes |
| Legacy PDB / mmCIF reader | — | No external packages needed |

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

For example, if HDF5 2.1.1 is installed in `/usr/local/hdf5-2.1.1`:

```bash
cmake -B build -DHDF5_ROOT=/usr/local/hdf5-2.1.1 -DCMAKE_INSTALL_PREFIX=/usr/local
```

NEP must use the same HDF5 as NetCDF-C. Configure stops if the
`libhdf5.so` version that `libnetcdf.so` needs differs from the HDF5
that CMake found; set `HDF5_ROOT` to NetCDF-C's HDF5 to fix it.

### Configuration Options

#### Quick Reference

| CMake Option | Default | Purpose |
|---|---|---|
| `-DNEP_ENABLE_FORTRAN` | ON | Fortran format-reader tests |
| `-DNEP_ENABLE_GEOTIFF` | **OFF** | GeoTIFF UDF handler (UDF0/UDF1) |
| `-DNEP_ENABLE_GRIB2` | **OFF** | GRIB2 UDF handler (UDF2) |
| `-DNEP_ENABLE_FITS` | **OFF** | FITS UDF handler (UDF3) |
| `-DNEP_ENABLE_CDF` | **OFF** | NASA CDF UDF handler (UDF4) |
| `-DNEP_ENABLE_PDS4` | **OFF** | NASA/ESA PDS4 UDF handler (UDF5) |
| `-DNEP_ENABLE_DICOM` | **OFF** | DICOM UDF handler (UDF6) |
| `-DNEP_ENABLE_PDB` | ON | Legacy PDB UDF handler (UDF7) |
| `-DNEP_ENABLE_MMCIF` | **OFF** | PDBx/mmCIF UDF handler (UDF8) |
| `-DNEP_ENABLE_NEXTCDF4` | **OFF** | NEXTCDF-4 backend (UDF9) |
| `-DNEP_BUILD_EXAMPLES` | ON | Example programs |
| `-DNEP_ENABLE_BENCHMARKS` | OFF | Performance benchmark examples |
| `-DNEP_ZSTD_PLUGIN_DIR` | (empty) | Directory with one HDF5 zstd plugin; enables the NEXTCDF-4 bfloat16 example tests |
| `-DNEP_ENABLE_PARALLEL_TESTS` | OFF | MPI parallel I/O tests |
| `-DNEP_BUILD_DOCUMENTATION` | ON | Doxygen API docs |

#### Compression

Use [nep-compress](https://github.com/captainkirk99/nep-compress) for
LZ4/BZIP2. The old `NEP_BUILD_LZ4` / `NEP_BUILD_BZIP2` CMake options
no longer build filters in NEP and produce a migration warning.

#### Fortran

`-DNEP_ENABLE_FORTRAN` (default ON): enables Fortran format-reader tests
in `ftest/`. NEP no longer
installs `nepf` or `nep.mod`. Requires NetCDF-Fortran and a Fortran compiler.
When combined with `-DNEP_ENABLE_NEXTCDF4=ON` it also builds and installs
the `nep_nextcdf4` module and `libnepnextcdf4f` (`NEP::nepnextcdf4f`),
which requires NetCDF-Fortran >= 4.6.2.

#### Format Readers

All format readers are independent — any combination can be enabled simultaneously. Legacy PDB defaults to **ON**; the rest default to **OFF**.

| Format | CMake Option | UDF Slot | Dependencies |
|--------|--------------|----------|--------------|
| GeoTIFF | `-DNEP_ENABLE_GEOTIFF` | UDF0, UDF1 | libgeotiff, libtiff |
| GRIB2 | `-DNEP_ENABLE_GRIB2` | UDF2 | NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0 |
| FITS | `-DNEP_ENABLE_FITS` | UDF3 | CFITSIO ≥ 3.0 |
| NASA CDF | `-DNEP_ENABLE_CDF` | UDF4 | NASA CDF library v3.9.x |
| PDS4 | `-DNEP_ENABLE_PDS4` | UDF5 | libxml2 ≥ 2.9 |
| DICOM | `-DNEP_ENABLE_DICOM` | UDF6 | libdicom, libjpeg / libjpeg-turbo |
| Legacy PDB | `-DNEP_ENABLE_PDB` | UDF7 | None (built-in fixed-column parser) |
| PDBx/mmCIF | `-DNEP_ENABLE_MMCIF` | UDF8 | None (built-in CIF parser) |
| NEXTCDF-4 | `-DNEP_ENABLE_NEXTCDF4` | UDF9 | HDF5 2.1.1, netcdf-c 64-slot UDF |

- NASA CDF library: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/ (or `spack install cdf`)
- CDF moved from UDF2 to UDF4 in v2.2.0; GRIB2 and CDF can now be enabled together.

#### Examples and Benchmarks

- **`-DNEP_BUILD_EXAMPLES`** (default OFF): builds all example programs and registers them as tests.
- **`-DNEP_ENABLE_BENCHMARKS`** (default OFF): runs general NetCDF performance benchmarks in `examples/performance/`.
- **`-DNEP_ENABLE_PARALLEL_TESTS`** (default OFF): builds the parallel test programs and runs tests via `mpiexec -n 4`. Requires MPI and NetCDF-C with `NC_HAS_PARALLEL4`.

#### Documentation

`-DNEP_BUILD_DOCUMENTATION` (default ON): generates Doxygen API documentation from C and Fortran sources, published to GitHub Pages. Requires Doxygen and Graphviz.

### Using NEP in Your Project

For LZ4/BZIP2, use nep-compress's installation and
[`HDF5_PLUGIN_PATH` instructions](https://github.com/captainkirk99/nep-compress#build-and-test).
NEP format readers do not require these compression plugins.

### Spack Installation

NEP can be installed using the [Spack](https://spack.io) package manager.

#### Default install

```bash
# Latest stable release
spack install nep@4.2.0

# Or install from the main branch
spack install nep
```

This installs NEP's enabled format readers and optional documentation;
legacy PDB support is enabled by default. Compression requires nep-compress.

#### Variant reference

| Variant | Default | Description |
|---------|---------|-------------|
| `docs` | ON | Build Doxygen documentation (requires Doxygen) |
| `fortran` | ON | Fortran format-reader tests (requires NetCDF-Fortran) |
| `fits` | OFF | FITS reader — HST, JWST, Chandra (requires CFITSIO) |
| `geotiff` | OFF | GeoTIFF reader — CF-1.8 CRS metadata (requires libgeotiff); needs `nep@main` or a release after 4.2.0 |
| `grib2` | OFF | GRIB2 reader — NWP model output (requires NCEPLIBS-g2c) |
| `pds4` | OFF | PDS4 reader — planetary science (requires libxml2) |
| `pdb` | ON | Legacy PDB reader — protein structures (no external library required) |
| `mmcif` | OFF | PDBx/mmCIF reader — macromolecular structures (no external library required) |
| `nextcdf4` | OFF | NEXTCDF-4 UDF backend (requires HDF5 2.1.1+) |
| `parallel` | OFF | Parallel I/O tests (requires MPI, HDF5+mpi, netcdf-c+mpi) |
| `examples` | OFF | Example programs |
| `benchmarks` | OFF | Performance benchmark programs |

The CDF and DICOM readers are not available through Spack: there is no
`cdf` package yet ([spack/spack-packages#2797](https://github.com/spack/spack-packages/pull/2797))
and no `gdcm` package for the DICOM dependencies.

Enabled readers are registered by the `share/.ncrc` file the package installs.

#### Common combinations

```bash
# GeoTIFF + GRIB2 (geospatial and NWP workflows)
spack install nep+geotiff+grib2

# All format readers
spack install nep+geotiff+grib2+fits+pds4+pdb+mmcif

# Minimal build (no docs, no Fortran) for CI/containers
spack install nep~docs~fortran

# Parallel I/O (requires MPI stack)
spack install nep+parallel
```

### Conda Installation

Install the 4.2.0 package from conda-forge (Linux only):

```bash
conda install -c conda-forge nep
```

The package includes the PDB, mmCIF, FITS and PDS4 readers; GeoTIFF is
enabled starting with the 4.3.0 package. DICOM, GRIB2, CDF and NEXTCDF-4
are excluded because they need packages or versions conda-forge does not
provide.

Register the readers with NetCDF-C by appending the installed `.ncrc`
to your own:

```bash
cat "$CONDA_PREFIX/share/.ncrc" >> ~/.ncrc
```

Or, for a per-session override, set `NCRCENV_RC` — note this makes
NetCDF-C ignore `~/.ncrc` entirely:

```bash
export NCRCENV_RC="$CONDA_PREFIX/share/.ncrc"
```

For LZ4/BZIP2 compression, install
[nep-compress](https://github.com/captainkirk99/nep-compress)
separately.

The package is pending until
[staged-recipes#34309](https://github.com/conda-forge/staged-recipes/pull/34309)
merges.

---

## Testing

Run the test suite after building:

```bash
ctest --test-dir build
```

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd.md)** - Detailed product requirements and specifications
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

# NEP Build and Configuration Options

This page documents all build-time options for NEP v2.2.0.

## Minimum Requirements

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| NetCDF-C   | 4.9+           | 4.10+ recommended for `.ncrc` UDF autoload |
| HDF5       | 1.12+          | |
| CMake      | 3.9+           | |

## Quick Reference

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

## Compression Options

### LZ4
- **CMake**: `-DBUILD_LZ4=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-lz4` / `--disable-lz4`
- **Dependencies**: liblz4
- Builds `libh5lz4.so` HDF5 filter plugin. Provides `nc_def_var_lz4()` / `nf90_def_var_lz4()`.

### BZIP2
- **CMake**: `-DBUILD_BZIP2=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-bzip2` / `--disable-bzip2`
- **Dependencies**: libbz2
- Builds `libh5bzip2.so` HDF5 filter plugin. Provides `nc_def_var_bzip2()` / `nf90_def_var_bzip2()`.

## Fortran Support

- **CMake**: `-DENABLE_FORTRAN=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-fortran` / `--disable-fortran`
- **Dependencies**: NetCDF-Fortran library and a Fortran 90+ compiler
- When enabled: builds Fortran wrappers in `fsrc/`, Fortran tests in `ftest/`, and Fortran examples in `examples/f_*/`.

## Format Reader Options

All five format readers default to **OFF**. They are independent — any combination can be enabled simultaneously.

### GeoTIFF (UDF0 / UDF1)

- **CMake**: `-DENABLE_GEOTIFF=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-geotiff` / `--disable-geotiff`
- **Dependencies**: libgeotiff, libtiff
- **UDF slots**: UDF0 (BigTIFF), UDF1 (standard TIFF/GeoTIFF)
- Reads GeoTIFF raster files via `nc_open()`; emits CF-1.8 `crs` grid-mapping variable, coordinate variables, and optional bounds.

### GRIB2 (UDF2)

- **CMake**: `-DENABLE_GRIB2=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-grib2` / `--disable-grib2`
- **Dependencies**: NOAA NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0
- **UDF slot**: UDF2
- Reads GRIB2 NWP model output via `nc_open()`; each product exposed as a named `NC_FLOAT` variable with shared `[y, x]` dimensions. Bitmap-masked points become `_FillValue`.

### FITS (UDF3)

- **CMake**: `-DENABLE_FITS=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-fits` / `--disable-fits`
- **Dependencies**: CFITSIO ≥ 3.0
- **UDF slot**: UDF3
- Reads FITS files via `nc_open()`; primary HDU maps to root group, each extension HDU becomes a child group. Provides `NC_FITS_initialize()` for explicit handler registration.

### NASA CDF (UDF4)

- **CMake**: `-DENABLE_CDF=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-cdf` / `--disable-cdf`
- **Dependencies**: NASA CDF library v3.9.x (https://spdf.gsfc.nasa.gov/pub/software/cdf/)
- **UDF slot**: UDF4 (moved from UDF2 in v2.2.0; no longer mutually exclusive with GRIB2)
- Reads CDF space physics files via `nc_open()`; CDF types and attributes mapped to NetCDF equivalents.

### NASA/ESA PDS4 (UDF5)

- **CMake**: `-DENABLE_PDS4=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-pds4` / `--disable-pds4`
- **Dependencies**: libxml2 ≥ 2.9 (`libxml2-dev` on Ubuntu/Debian)
- **UDF slot**: UDF5
- Reads PDS4 XML-label products via `nc_open()`; arrays and tables mapped to netCDF dimensions and variables. Provides `NC_PDS4_initialize()` for explicit handler registration.

## UDF Slot Assignments

| Slot | Format | Magic / Detection |
|------|--------|-------------------|
| UDF0 | GeoTIFF BigTIFF | `II+` (8-byte TIFF header) |
| UDF1 | GeoTIFF standard TIFF | `II*` or `MM\x00*` |
| UDF2 | GRIB2 | `GRIB` |
| UDF3 | FITS | `SIMPLE` |
| UDF4 | NASA CDF | `\xCD\xF3\x00\x01` |
| UDF5 | NASA/ESA PDS4 | XML root element `Product_Observational` |

## UDF Autoloading via `.ncrc`

NEP installs a `.ncrc` file that configures NetCDF-C's UDF self-loading mechanism. When present, `nc_open()` and `ncdump` automatically load the enabled format handlers without any explicit `NC_*_initialize()` call in application code.

### Installed Location

| Build system | Default install path |
|---|---|
| CMake | `${CMAKE_INSTALL_PREFIX}/share/nep/.ncrc` |
| Autotools | `${datarootdir}/nep/.ncrc` |

### Enabling Autoloading

```bash
cat /usr/local/share/nep/.ncrc >> ~/.ncrc
```

Or point `NETCDF_RC` to the directory containing the `.ncrc`:

```bash
export NETCDF_RC=/usr/local/share/nep
```

**Note**: `.ncrc` autoload requires NetCDF-C built from the main branch. With NetCDF-C 4.10.0, call `NC_*_initialize()` explicitly before `nc_open()`.

## Examples and Benchmarks

### Example Programs
- **CMake**: `-DBUILD_EXAMPLES=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-examples` / `--disable-examples`
- Builds all example programs in `examples/` and registers them as tests.

### Performance Benchmarks
- **CMake**: `-DENABLE_BENCHMARKS=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-benchmarks` / `--disable-benchmarks`
- Builds compression benchmark programs in `examples/performance/`.

### Parallel I/O Tests
- **CMake**: `-DENABLE_PARALLEL_TESTS=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-parallel-tests` / `--disable-parallel-tests`
- **Dependencies**: MPI (OpenMPI or MPICH), NetCDF-C with `NC_HAS_PARALLEL4`
- Builds `examples/parallelIO/` and runs tests via `mpiexec -n 4`.

## Documentation

- **CMake**: `-DBUILD_DOCUMENTATION=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-docs` / `--disable-docs`
- **Dependencies**: Doxygen, Graphviz
- Generates API documentation from C and Fortran sources; published to GitHub Pages.

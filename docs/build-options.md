# NEP Build and Configuration Options

This page documents the primary build-time options for NEP, including compression and Fortran options referenced from the main page.

## Minimum Version Requirements

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| NetCDF-C   | 4.10.0         | Required for UDF self-loading support (`NC_HAS_UDF_SELF_LOAD`) |
| HDF5       | 1.10.0         | |
| CMake      | 3.9            | |

## Compression Options

### LZ4 Compression
- **CMake**: `-DBUILD_LZ4=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-lz4/--disable-lz4` (if provided by configure script)
- **Behavior**: Controls whether the LZ4 compression filter support is built and installed.

### BZIP2 Compression
- **CMake**: `-DBUILD_BZIP2=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-bzip2/--disable-bzip2` (if provided by configure script)
- **Behavior**: Controls whether the BZIP2 compression filter support is built and installed.

## Fortran Support

### Enable or Disable Fortran
- **CMake**: `-DENABLE_FORTRAN=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-fortran/--disable-fortran`

When Fortran support is **enabled**:
- NEP builds Fortran wrappers in `fsrc/` and Fortran tests in `ftest/`.
- **Dependencies**:
  - A Fortran compiler supported by your toolchain.
  - `netcdf-fortran` library and headers.

When Fortran support is **disabled**:
- No Fortran libraries or modules are built or installed.
- Fortran tests are not built or run.

## Library Detection Options

### CDF Library Detection (v1.3.0)
- **CMake**: `-DENABLE_CDF=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-cdf/--disable-cdf`
- **Behavior**: Enables detection of NASA CDF library for future CDF UDF handler support.
- **Dependencies**: NASA CDF library v3.9.x or later
- **Note**: Library detection only - no UDF implementation yet.

### GeoTIFF Support (v1.5.0)
- **CMake**: `-DENABLE_GEOTIFF=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-geotiff/--disable-geotiff`
- **Behavior**: Enables GeoTIFF file format support through NetCDF User Defined Format (UDF) handler.
- **Dependencies**: 
  - libgeotiff (latest stable version recommended)
  - libtiff (dependency of libgeotiff)
  - NetCDF-C v4.10.0 or later (required for UDF self-loading support)
- **Status**: ✅ Fully implemented (Phase 1 & 2 complete)

When GeoTIFF support is **enabled**:
- Build system verifies presence of libgeotiff and libtiff libraries.
- GeoTIFF dispatch layer is compiled and linked.
- `HAVE_GEOTIFF` macro is defined in configuration headers.
- GeoTIFF test suite is built and can be run.
- Build fails with clear error message if required libraries are not found.

When GeoTIFF support is **disabled** (default):
- No GeoTIFF library checks are performed.
- GeoTIFF source files are not compiled.
- Build proceeds without GeoTIFF support.

**GeoTIFF Features (Phase 1 & 2):**
- ✅ Automatic GeoTIFF file format detection via magic number
- ✅ Open GeoTIFF files through standard `nc_open()` API
- ✅ Close GeoTIFF files through standard `nc_close()` API
- ✅ Extract image dimensions (width, height, bands)
- ✅ Extract data type information (byte, short, int, float, double)
- ✅ Extract coordinate reference system (CRS) metadata
- ✅ Support for both little-endian and big-endian TIFF files
- ✅ Robust error handling for malformed files
- ⏳ Raster data reading (Phase 3 - planned)
- ⏳ Coordinate system transformations (Phase 4 - planned)

**Build Examples:**

CMake:
```bash
mkdir build && cd build
cmake .. -DENABLE_GEOTIFF=ON
make
make test  # Runs GeoTIFF test suite
```

Autotools:
```bash
./configure --enable-geotiff
make
make check  # Runs GeoTIFF test suite
```

**Testing:**
The GeoTIFF test suite includes:
- Format detection tests (valid/invalid files)
- File operations tests (open/close/error handling)
- Metadata extraction tests (dimensions, types, CRS)
- Memory leak detection (valgrind)
- Code coverage reporting (>80% target)

## UDF Autoloading via .ncrc (v1.5.5)

NEP installs a `nep.ncrc` file that configures NetCDF-C's UDF self-loading mechanism.
When present, NetCDF-C automatically loads the NEP shared library when opening any
GeoTIFF or CDF file, without requiring applications to call `NC_GEOTIFF_initialize()`
or `NC_CDF_initialize()` explicitly.

### Installed Location

| Build system | Default install path | Override option |
|---|---|---|
| CMake | `${CMAKE_INSTALL_PREFIX}/share/nep/nep.ncrc` | `-DNEP_NCRC_INSTALL_DIR=<path>` |
| Autotools | `${datarootdir}/nep/nep.ncrc` | `--with-ncrc-dir=<path>` |

### Enabling Autoloading

Append the contents of the installed `nep.ncrc` to your `~/.ncrc`:

```bash
cat $(nep-config --datadir)/nep/nep.ncrc >> ~/.ncrc
```

Or point `NETCDF_RC` to the directory containing `nep.ncrc`:

```bash
export NETCDF_RC=/usr/local/share/nep
nc_open("myfile.tif", NC_NOWRITE, &ncid);   # works without any init call
```

### UDF Slot Assignments

| Slot | Format | Magic bytes |
|---|---|---|
| UDF0 | GeoTIFF BigTIFF (little-endian) | `II+` |
| UDF1 | GeoTIFF standard TIFF (little-endian) | `II*` |
| UDF2 | NASA CDF | `\xCD\xF3\x00\x01` |

See [NetCDF UDF documentation](https://docs.unidata.ucar.edu/netcdf/NUG/user_defined_formats.html)
for full details on the RC file format.

## Documentation Build

- **CMake**: `-DBUILD_DOCUMENTATION=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-docs/--disable-docs` (if provided)

When documentation is enabled:
- Doxygen-based API documentation is generated from the C/Fortran sources.
- The docs deployment workflow publishes these docs to GitHub Pages.

## Notes

- The exact set of Autotools options may vary depending on how `configure.ac` is structured, but they are intended to mirror the CMake options listed above.
- For full usage and installation instructions, see the main page and README.

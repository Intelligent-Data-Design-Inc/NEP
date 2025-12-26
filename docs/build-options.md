# NEP Build and Configuration Options

This page documents the primary build-time options for NEP, including compression and Fortran options referenced from the main page.

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

### GeoTIFF Library Detection (v1.5.0)
- **CMake**: `-DENABLE_GEOTIFF=ON/OFF` (default: `OFF`)
- **Autotools**: `--enable-geotiff/--disable-geotiff`
- **Behavior**: Enables detection of libgeotiff and libtiff libraries for future GeoTIFF UDF handler support.
- **Dependencies**: 
  - libgeotiff (latest stable version)
  - libtiff (dependency of libgeotiff)
- **Note**: Library detection only - no UDF implementation yet.

When GeoTIFF detection is **enabled**:
- Build system verifies presence of libgeotiff and libtiff libraries.
- `HAVE_GEOTIFF` macro is defined in configuration headers.
- Build fails with clear error message if libraries are not found.

When GeoTIFF detection is **disabled** (default):
- No GeoTIFF library checks are performed.
- Build proceeds without GeoTIFF support.

## Documentation Build

- **CMake**: `-DBUILD_DOCUMENTATION=ON/OFF` (default: `ON`)
- **Autotools**: `--enable-docs/--disable-docs` (if provided)

When documentation is enabled:
- Doxygen-based API documentation is generated from the C/Fortran sources.
- The docs deployment workflow publishes these docs to GitHub Pages.

## Notes

- The exact set of Autotools options may vary depending on how `configure.ac` is structured, but they are intended to mirror the CMake options listed above.
- For full usage and installation instructions, see the main page and README.

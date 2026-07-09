# NEP (NetCDF Extension Pack)

## Overview

NEP (NetCDF Extension Pack) extends NetCDF-4 with high-performance compression and multi-format data access.

### Compression Filters

LZ4 and BZIP2 HDF5 filter plugins for NetCDF-4 files, both enabled by default. LZ4 targets speed (2–3× faster than DEFLATE); BZIP2 targets ratio (ideal for archival).

See **[Compression Filters](compression.md)** — algorithm details, C/Fortran API reference, and performance benchmarks.

### Format Readers

Five NetCDF UDF handlers that expose external scientific formats through the standard `nc_open()` API, all disabled by default:
- **GeoTIFF** (UDF0/UDF1): Geospatial raster files with CF-1.8 CRS metadata
- **GRIB2** (UDF2): Meteorological and oceanographic NWP model output (NOAA GFS, NAM, HRRR)
- **FITS** (UDF3): Astronomical images and tables from HST, JWST, Chandra, and other observatories
- **CDF** (UDF4): NASA space physics and heliophysics time-series data
- **PDS4** (UDF5): NASA/ESA planetary science archives (Curiosity, Perseverance, Cassini, and others)

See **[Format Readers](formats.md)** — usage, code examples, and dependency information for each reader.

### Example Programs

Over 55 C and Fortran programs covering classic NetCDF, NetCDF-4, NcZarr, OPeNDAP remote access, performance tuning, and parallel I/O. Companion code for *[The NetCDF Developer's Handbook, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

See **[Example Programs](examples.md)** — complete program listing by category.

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+) and its dependencies
- LZ4 library
- BZIP2 library
- NetCDF-Fortran library (optional, for Fortran support)
- NASA CDF library v3.9+ (optional, for CDF file reading)
- libgeotiff and libtiff (optional, for GeoTIFF file reading; `--enable-geotiff`)
- NOAA NCEPLIBS-g2c >= 2.1.0 and libjasper >= 3.0.0 (optional, for GRIB2 file reading; `--enable-grib2`)
- CFITSIO >= 3.0 (optional, for FITS file reading; `--enable-fits`)
- libxml2 >= 2.9 (optional, for PDS4 file reading; `--enable-pds4`)

Build with CMake or Autotools:

```bash
# CMake
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build

# Autotools
./configure --prefix=/usr/local
make
make install
```

## Build Options

See the [Configuration Options](https://github.com/Intelligent-Data-Design-Inc/NEP#configuration-options) section of the README for the full CMake and Autotools option reference.

## API Documentation

Key API reference pages:

- [C API (nep.c)](nep_8c.html)
- [Fortran API (nep.F90)](nep_8F90.html)

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

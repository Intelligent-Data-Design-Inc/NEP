# NEP (NetCDF Extension Pack)

## Overview

NEP (NetCDF Extension Pack) extends NetCDF-4 with high-performance compression and multi-format data access.

**Compression Filters** (HDF5 filter plugins for NetCDF-4 files):
- **LZ4**: Speed-optimized lossless compression (2-3x faster than DEFLATE)
- **BZIP2**: High-ratio lossless compression for archival storage

See `docs/compression.md` for algorithm details, API reference, and performance benchmarks.

**Format Readers** (NetCDF UDF handlers; all disabled by default):
- **GeoTIFF** (UDF0/UDF1): Geospatial raster files
- **GRIB2** (UDF2): Meteorological and oceanographic NWP model output
- **FITS** (UDF3): Astronomical images and tables (HST, JWST, Chandra)
- **CDF** (UDF4): NASA space physics data
- **PDS4** (UDF5): NASA/ESA planetary science archives

See `docs/formats.md` for usage, API examples, and dependency information for each reader.

Over 55 C and Fortran **example programs** cover classic NetCDF, NetCDF-4, NcZarr, OPeNDAP remote access, performance tuning, and parallel I/O. They are companion code for *[The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

See `docs/examples.md` for the full program listing.

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

See `docs/build-options.md` for the full CMake and Autotools option reference.

## API Documentation

Key API reference pages:

- [C API (nep.c)](nep_8c.html)
- [Fortran API (nep.F90)](nep_8F90.html)

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

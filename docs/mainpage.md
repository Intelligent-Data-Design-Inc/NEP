# NEP (NetCDF Expansion Pack)

## Overview

NEP (NetCDF Expansion Pack) provides scientific format readers and the NEXTCDF-4 backend through the NetCDF API.

### Compression Filters

LZ4 and BZIP2 HDF5 filter plugins and their C/Fortran wrappers are provided by the separate [nep-compress](https://github.com/captainkirk99/nep-compress) project; NEP no longer ships them.

See the **[nep-compress compression guide](https://github.com/captainkirk99/nep-compress/blob/main/docs/compression.md)** for the package/API move.

### Format Readers

Five NetCDF UDF handlers that expose external scientific formats through the standard `nc_open()` API, all disabled by default:
- **GeoTIFF** (UDF0/UDF1): Geospatial raster files with CF-1.8 CRS metadata
- **GRIB2** (UDF2): Meteorological and oceanographic NWP model output (NOAA GFS, NAM, HRRR)
- **FITS** (UDF3): Astronomical images and tables from HST, JWST, Chandra, and other observatories
- **CDF** (UDF4): NASA space physics and heliophysics time-series data
- **PDS4** (UDF5): NASA/ESA planetary science archives (Curiosity, Perseverance, Cassini, and others)

See **[Format Readers](formats.md)** — usage, code examples, and dependency information for each reader.

### NEXTCDF-4 Backend

Optional alternative NetCDF-4/HDF5 backend selected explicitly with `NC_NEXTCDF4` (UDF9). It supports the full enhanced NetCDF-4 data model, variable I/O, chunking, compression, and new atomic types (`NC_FLOAT16`, small floating-point types, complex numbers, bitfields, and HDF5 references) while coexisting with netcdf-c's built-in HDF5 backend. Files written in `NC_NETCDF4_MODEL` compatibility mode remain readable by upstream netcdf-c.

See **[NEXTCDF-4](nextcdf4.md)** — selection flags, compatibility modes, supported types, and examples — and **[NEXTCDF-4 float16 storage](NEXTCDF4_float16.md)** — the `NC_FLOAT16`/`NC_BFLOAT16` user guide with examples and benchmark results.

### Example Programs

Over 55 C and Fortran programs covering classic NetCDF, NetCDF-4, NcZarr, OPeNDAP remote access, performance tuning, and parallel I/O. Companion code for *[The NetCDF Developer's Handbook, Second Edition](https://www.amazon.com/dp/B0H7Q1Z75L)*.

See **[Example Programs](examples.md)** — complete program listing by category.

## Installation

NEP requires:
- NetCDF-C library (v4.10.1+) (required for UDF slots > 2 used by FITS, CDF, and PDS4)
- HDF5 library (v1.10+) and its dependencies
- NetCDF-Fortran library (optional, for Fortran support)
- NASA CDF library v3.9+ (optional, for CDF file reading)
- libgeotiff and libtiff (optional, for GeoTIFF file reading; `--enable-geotiff`)
- NOAA NCEPLIBS-g2c >= 2.1.0 and libjasper >= 3.0.0 (optional, for GRIB2 file reading; `--enable-grib2`)
- CFITSIO >= 3.0 (optional, for FITS file reading; `--enable-fits`)
- libxml2 >= 2.9 (optional, for PDS4 file reading; `--enable-pds4`)

Build with CMake:

```bash
# CMake
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build

```

## Build Options

See the [Configuration Options](https://github.com/Intelligent-Data-Design-Inc/NEP#configuration-options) section of the README for the CMake option reference.

## API Documentation

Key API reference pages:

- [C API (nep.c)](nep_8c.html)

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

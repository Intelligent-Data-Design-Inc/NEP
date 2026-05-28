# NEP (NetCDF Expansion Pack) - Skill File

## Overview

NEP extends NetCDF-4 with high-performance compression filters and User Defined Format (UDF) handlers for accessing diverse scientific data formats through the standard NetCDF API. It provides:

- **LZ4 Compression**: 2-3x faster than DEFLATE, ideal for real-time processing and HPC
- **BZIP2 Compression**: 6.7x compression ratios for archival storage
- **CDF File Reader**: Read NASA Common Data Format files via NetCDF API
- **GeoTIFF Reader**: Read geospatial raster files with CF-1.8 compliant CRS metadata
- **GRIB2 Reader**: Read meteorological data through NetCDF API

## Architecture

### Dual Architecture

NEP has two primary components:

1. **HDF5 Filter Plugin Architecture**: Compression filters (LZ4, BZIP2) integrate into HDF5 I/O
2. **NetCDF UDF System**: Format handlers (CDF, GeoTIFF, GRIB2) provide transparent multi-format access

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴────────────┬──────────────┬──────────────┐
│                   │              │              │
[HDF5 Backend]  [CDF Handler]  [GeoTIFF Handler]  [GRIB2 Handler]
│                   │              │              │
│               [CDF Library]  [libgeotiff]  [NCEPLIBS-g2c]
│                   │              │              │
└───────┬───────────┼──────────────┴──────────────┘
        │           │              │              │
[NetCDF-4 Files]  [CDF Files]  [GeoTIFF Files]  [GRIB2 Files]
```

### UDF Slot Allocation

NetCDF-C 4.10.0+ provides 10 UDF slots:
- **UDF0**: GeoTIFF BigTIFF (magic: "II+")
- **UDF1**: GeoTIFF standard TIFF (magic: "II*")
- **UDF2**: GRIB2 (magic: "GRIB") [default] OR NASA CDF (magic: 0xCDF30001) [mutually exclusive]
- **UDF3-UDF9**: Reserved for future use

## Directory Structure

```
/home/ed/NEP/
├── src/                    # Core C source code
│   ├── nep.c               # Main NEP library implementation
│   ├── cdfdispatch.c       # CDF dispatch table implementation
│   ├── cdffile.c           # CDF file operations
│   ├── geotiffdispatch.c   # GeoTIFF dispatch table
│   ├── geotifffile.c       # GeoTIFF file operations
│   ├── grib2dispatch.c     # GRIB2 dispatch table
│   └── grib2file.c         # GRIB2 file operations
├── include/                # Public header files
│   ├── nep.h               # Main public API header
│   ├── cdfdispatch.h       # CDF dispatch definitions
│   ├── geotiffdispatch.h   # GeoTIFF dispatch definitions
│   └── grib2dispatch.h     # GRIB2 dispatch definitions
├── hdf5_plugins/            # HDF5 compression filter plugins
│   └── LZ4/                 # LZ4 filter implementation
├── fsrc/                    # Fortran wrapper source
├── ftest/                   # Fortran unit tests
├── test/                    # C unit tests
│   ├── tst_lz4.c            # LZ4 compression tests
│   ├── tst_bzip2.c          # BZIP2 compression tests
│   ├── tst_cdf_udf.c        # CDF UDF handler tests
│   ├── tst_imap_mag.c       # NASA IMAP CDF file tests
│   └── data/                # Test data files
├── test_cdf/                # CDF-specific tests
├── test_geotiff/            # GeoTIFF-specific tests
├── test_h5/                 # HDF5-specific tests
├── examples/                # Example programs
│   ├── netcdf-4/            # NetCDF-4 examples
│   └── classic/             # Classic NetCDF examples
├── docs/                    # Documentation
│   ├── design.md            # Architecture design document
│   ├── prd.md               # Product Requirements Document
│   ├── roadmap.md           # Development roadmap
│   ├── cf-compliance.md     # CF conventions compliance
│   └── releases/              # Release notes
└── spack/                   # Spack package recipes
```

## Key Files

### Core Implementation
- `@/home/ed/NEP/src/nep.c` - Main library with compression API
- `@/home/ed/NEP/src/cdfdispatch.c` - CDF dispatch table (read-only CDF access)
- `@/home/ed/NEP/src/cdffile.c` - CDF file operations and metadata extraction
- `@/home/ed/NEP/src/geotiffdispatch.c` - GeoTIFF dispatch table
- `@/home/ed/NEP/src/geotifffile.c` - GeoTIFF reading and CF metadata generation
- `@/home/ed/NEP/src/grib2dispatch.c` - GRIB2 dispatch table
- `@/home/ed/NEP/src/grib2file.c` - GRIB2 file operations

### Headers
- `@/home/ed/NEP/include/nep.h` - Public API with filter IDs and UDF slots
- `@/home/ed/NEP/include/cdfdispatch.h` - CDF dispatch interface
- `@/home/ed/NEP/include/geotiffdispatch.h` - GeoTIFF dispatch interface
- `@/home/ed/NEP/include/grib2dispatch.h` - GRIB2 dispatch interface

### Build System
- `@/home/ed/NEP/CMakeLists.txt` - CMake build configuration
- `@/home/ed/NEP/configure.ac` - Autotools configuration

## Filter IDs

```c
#define BZIP2_ID 307        // BZIP2 compression filter
#define LZ4_ID 32004        // LZ4 compression filter
#define LZF_ID 32000        // LZF compression filter
```

## Build Commands

### CMake (Recommended)
```bash
# Configure with all features
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DENABLE_CDF=ON -DENABLE_GEOTIFF=ON -DENABLE_GRIB2=OFF \
    -DBUILD_LZ4=ON -DBUILD_BZIP2=ON

# Build
cmake --build build

# Test
ctest --test-dir build

# Install
cmake --install build

# Documentation
cmake --build build --target docs
```

### Autotools (Legacy support)
```bash
# Bootstrap
./autogen.sh

# Configure
./configure --prefix=/usr/local \
    --enable-cdf --enable-lz4 --enable-bzip2

# Build
make

# Test
make check

# Install
make install
```

## Test Commands

```bash
# Run all tests
ctest

# Run specific test
ctest -R tst_lz4

# Run with verbose output
ctest -V

# Run test executable directly
./test/tst_lz4
./test/tst_cdf_udf
./test/tst_imap_mag
```

## Configuration Options

| CMake Option | Autotools Option | Description |
|--------------|------------------|-------------|
| `-DENABLE_CDF=ON` | `--enable-cdf` | Enable CDF file support |
| `-DENABLE_GEOTIFF=ON` | `--enable-geotiff` | Enable GeoTIFF support |
| `-DENABLE_GRIB2=ON` | `--enable-grib2` | Enable GRIB2 support (mutually exclusive with CDF) |
| `-DBUILD_LZ4=ON` | `--enable-lz4` | Build LZ4 filter |
| `-DBUILD_BZIP2=ON` | `--enable-bzip2` | Build BZIP2 filter |
| `-DENABLE_FORTRAN=ON` | `--enable-fortran` | Build Fortran wrappers |
| `-DBUILD_DOCUMENTATION=ON` | `--enable-docs` | Build Doxygen documentation |
| `-DBUILD_EXAMPLES=ON` | `--enable-examples` | Build example programs |
| `-DBUILD_TESTING=ON` | - | Build test suite |

## Common Tasks

### Adding a New UDF Handler

1. Create dispatch table in `src/{format}dispatch.c`
2. Create file operations in `src/{format}file.c`
3. Create header in `include/{format}dispatch.h`
4. Add to `CMakeLists.txt` and `configure.ac`
5. Add tests in `test_{format}/`

### Adding a New Compression Filter

1. Implement filter in `hdf5_plugins/{FILTER}/src/H5Z{filter}.c`
2. Add filter ID to `include/nep.h`
3. Add API functions to `src/nep.c`
4. Update build system configurations
5. Add tests in `test/`

### Running Examples

```bash
# After building with -DBUILD_EXAMPLES=ON
./examples/netcdf-4/compression
./examples/netcdf-4/simple_nc4
```

## Dependencies

**Required:**
- NetCDF-C (v4.10.0+ for UDF self-loading)
- HDF5 (v1.12+)
- CMake (v3.9+) or Autotools

**Optional:**
- LZ4 library (for LZ4 compression)
- BZIP2 library (for BZIP2 compression)
- NASA CDF library (v3.9+ for CDF support)
- libgeotiff + libtiff (for GeoTIFF support)
- NCEPLIBS-g2c (for GRIB2 support)
- NetCDF-Fortran (for Fortran wrappers)
- Doxygen (for documentation)

## Environment Variables

```bash
# Required for HDF5 filter plugins
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin

# For UDF autoloading via .ncrc
export NETCDF_RC=/path/to/.ncrc
```

## Testing

Test data is in `test/data/`:
- `imap_mag_l1b-calibration_20240229_v001.cdf` - NASA IMAP magnetometer data
- `MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif` - MODIS flood product

## Documentation

- Full docs: https://intelligent-data-design-inc.github.io/NEP/
- Design doc: `@/home/ed/NEP/docs/design.md`
- PRD: `@/home/ed/NEP/docs/prd.md`
- Roadmap: `@/home/ed/NEP/docs/roadmap.md`

## Code Style Notes

- C code follows NetCDF-C conventions
- Doxygen documentation for all public APIs
- Error handling: return `NC_NOERR` (0) on success, error code on failure
- Use `ERR(retval)` macro for error checking
- Read-only UDF handlers use `NC_RO_*` stubs for write operations

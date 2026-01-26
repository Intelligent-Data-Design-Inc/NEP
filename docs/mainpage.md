# NEP (NetCDF Extension Pack)

## Overview

NEP (NetCDF Extension Pack) extends NetCDF-4 with high-performance compression and multi-format data access:

- **LZ4 Compression**: Speed-optimized lossless compression (2-3x faster than DEFLATE)
- **BZIP2 Compression**: High-ratio lossless compression for archival storage
- **CDF File Reader**: Read NASA Common Data Format files through the NetCDF API
- **GeoTIFF File Reader**: Read GeoTIFF geospatial raster files through the NetCDF API

NEP provides flexible compression options and unified data access for diverse scientific data workflows.

## NetCDF Example Programs

NEP includes 22 comprehensive example programs demonstrating NetCDF API usage in both C and Fortran. These examples cover classic NetCDF and NetCDF-4 features, providing practical learning resources for users at all levels.

### Example Categories

- **C Classic Examples** (6 programs): Basic 2D arrays, coordinate variables, format variants, size limits, unlimited dimensions, and 4D variables
- **C NetCDF-4 Examples** (5 programs): NetCDF-4 basics, compression, chunking performance, multiple unlimited dimensions, and user-defined types
- **Fortran Classic Examples** (6 programs): Fortran equivalents of all classic examples
- **Fortran NetCDF-4 Examples** (5 programs): Fortran equivalents of all NetCDF-4 examples

### Key Features

- **Automated Testing**: All examples run as automated tests with CDL-based output validation
- **C/Fortran Equivalence**: Parallel implementations produce identical output, enabling cross-language learning
- **Comprehensive Documentation**: Doxygen-integrated with learning paths from beginner to advanced
- **Build Integration**: Examples build by default in both CMake and Autotools, can be disabled with `-DBUILD_EXAMPLES=OFF` or `--disable-examples`

### Learning Paths

**Beginners**: Start with `simple_2D` → `coord_vars` → `unlimited_dim`

**Intermediate**: Progress to `format_variants` → `size_limits` → `var4d`

**Advanced (NetCDF-4)**: Master `simple_nc4` → `compression` → `chunking_performance` → `multi_unlimited` → `user_types`

See the [Examples Documentation](@ref examples) for complete details, build instructions, and usage guides.

## Compression Algorithms

### LZ4 Compression

LZ4 is a lossless data compression algorithm from the LZ77 family of byte-oriented compression schemes. It prioritizes compression and decompression speed while maintaining good compression ratios.

**Key Features:**
- **High-Speed Compression**: 2-3x faster compression/decompression than DEFLATE
- **Lossless**: Guarantees data integrity with no data loss
- **Optimized for HPC**: Designed for speed-critical workflows in high-performance computing environments
- **Transparent Integration**: Works seamlessly with existing NetCDF-4 applications

**Performance Characteristics:**
- **Compression Speed**: Similar to LZO, several times faster than DEFLATE
- **Decompression Speed**: Significantly faster than LZO
- **Compression Ratio**: Good trade-off between speed and size reduction
- **Use Case**: Ideal for I/O-bound scientific workflows where speed matters more than maximum compression

### BZIP2 Compression

BZIP2 is a lossless data compression algorithm using the Burrows-Wheeler block sorting algorithm. It prioritizes achieving high compression ratios, making it ideal for archival storage and bandwidth-constrained scenarios.

**Key Features:**
- **Superior Compression Ratios**: Better compression than DEFLATE and significantly better than LZ4
- **Lossless**: Guarantees data integrity with no data loss
- **Block-Sorting Algorithm**: Particularly effective for repetitive patterns in scientific data
- **Transparent Integration**: Works seamlessly with existing NetCDF-4 applications

**Performance Characteristics:**
- **Compression Ratio**: Exceeds DEFLATE, ideal for storage optimization
- **Compression Speed**: Slower than LZ4 but faster than most high-ratio algorithms
- **Decompression Speed**: Moderate, suitable for archival workflows
- **Use Case**: Ideal for long-term archival storage, bandwidth-constrained transfers, and storage-limited environments

## Choosing the Right Algorithm

- **Use LZ4 when**: I/O speed is critical, working with real-time data, running on HPC systems, need fast decompression
- **Use BZIP2 when**: Storage space is limited, archiving data long-term, transferring over slow networks, compression ratio matters most

## CDF File Reader

NEP provides a User-Defined Format (UDF) handler that enables reading NASA Common Data Format (CDF) files using the familiar NetCDF API.

### What is CDF?

The Common Data Format (CDF) is a self-describing data format developed by NASA's Space Physics Data Facility (SPDF). CDF is widely used in space physics and heliophysics research for storing time-series and multi-dimensional scientific data from space missions.

**Key Characteristics:**
- Self-describing format with embedded metadata
- Support for multiple data types and dimensions
- Platform-independent binary format
- Optimized for space physics data
- Used by NASA missions: IMAP, MMS, Van Allen Probes, and many others

### CDF Support in NEP

**Transparent Access**: Read CDF files using standard NetCDF functions (`nc_open()`, `nc_get_var()`, `nc_get_att()`) without file conversion.

**Automatic Type Mapping**: CDF data types are automatically mapped to NetCDF equivalents:
- CDF_INT4 → NC_INT
- CDF_REAL8 → NC_DOUBLE
- CDF_TIME_TT2000 → NC_INT64 (for time variables)
- And more...

**Attribute Conventions**: CDF attributes are automatically converted to NetCDF conventions (e.g., FILLVAL → _FillValue).

**Use Cases:**
- Space physics and heliophysics research
- NASA mission data analysis
- Cross-format data integration (CDF + NetCDF)
- Legacy CDF data access in modern workflows

### Enabling CDF Support

CDF support is optional and disabled by default. To enable:

```bash
# CMake
cmake -B build -DENABLE_CDF=ON

# Autotools
./configure --enable-cdf
```

**Requirements**: NASA CDF library v3.9+ must be installed. Download from: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/

**Spack Users**: Install CDF separately with `spack install cdf`.

## GeoTIFF File Reader

NEP provides a User-Defined Format (UDF) handler that enables reading GeoTIFF files using the familiar NetCDF API.

### What is GeoTIFF?

GeoTIFF is a public domain metadata standard that allows georeferencing information to be embedded within a TIFF (Tagged Image File Format) file. GeoTIFF is widely used in geographic information systems (GIS), remote sensing, and earth science applications for storing geospatial raster data.

**Key Characteristics:**
- Standard TIFF format with geospatial metadata tags
- Support for coordinate reference systems (CRS)
- Multi-band raster data support
- Platform-independent format
- Used by NASA, USGS, and earth observation missions

### GeoTIFF Support in NEP

**Transparent Access**: Read GeoTIFF files using standard NetCDF functions (`nc_open()`, `nc_get_var()`, `nc_inq_dim()`) without file conversion.

**Automatic Dimension Mapping**: GeoTIFF structure is automatically mapped to NetCDF dimensions:
- Bands → NetCDF dimension (if multi-band)
- Rows (height) → NetCDF dimension
- Columns (width) → NetCDF dimension

**Metadata Preservation**: GeoTIFF tags and geospatial metadata are accessible as NetCDF attributes, including coordinate reference system information.

**Use Cases:**
- Remote sensing and satellite imagery analysis
- GIS data integration
- Earth observation data processing
- Cross-format geospatial workflows (GeoTIFF + NetCDF)

### Enabling GeoTIFF Support

GeoTIFF support is optional and disabled by default. To enable:

```bash
# CMake
cmake -B build -DENABLE_GEOTIFF=ON

# Autotools
./configure --enable-geotiff
```

**Requirements**: libgeotiff and libtiff libraries must be installed.

**Spack Users**: Install with `spack install nep +geotiff`.

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+) and its dependencies
- LZ4 library
- BZIP2 library
- NetCDF-Fortran library (optional, for Fortran support)
- NASA CDF library v3.9+ (optional, for CDF file reading)
- libgeotiff and libtiff (optional, for GeoTIFF file reading)

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

## Usage

### Compression

Once installed, both LZ4 and BZIP2 compression filters are available to NetCDF-4 applications through HDF5 filter plugins. Set the `HDF5_PLUGIN_PATH` environment variable and use standard NetCDF-4 compression APIs:

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

Choose the algorithm that best fits your workflow requirements—LZ4 for speed or BZIP2 for compression ratio.

### CDF File Reading

When CDF support is enabled, open and read CDF files using standard NetCDF functions:

```c
int ncid;
nc_open("data.cdf", NC_NOWRITE, &ncid);
// Use standard NetCDF API calls
nc_close(ncid);
```

No special code is needed—the CDF UDF handler automatically detects and processes CDF files.

### GeoTIFF File Reading

When GeoTIFF support is enabled, open and read GeoTIFF files using standard NetCDF functions:

```c
int ncid, varid;
size_t width, height, bands;
float *data;

// Open GeoTIFF file using NetCDF API
nc_open("satellite_image.tif", NC_NOWRITE, &ncid);

// Query dimensions
nc_inq_dimlen(ncid, 0, &bands);
nc_inq_dimlen(ncid, 1, &height);
nc_inq_dimlen(ncid, 2, &width);

// Read raster data
data = malloc(width * height * sizeof(float));
nc_inq_varid(ncid, "raster", &varid);
nc_get_var_float(ncid, varid, data);

// Close file
nc_close(ncid);
```

No special code is needed—the GeoTIFF UDF handler automatically detects and processes GeoTIFF files.

## Features and Options

- **Compression Algorithms**:
  - LZ4: speed-optimized lossless compression
  - BZIP2: high-ratio lossless compression
- **CDF File Reader**:
  - Read NASA CDF files via NetCDF API
  - Automatic type and attribute mapping
  - Support for TT2000 time variables
- **GeoTIFF File Reader**:
  - Read GeoTIFF geospatial raster files via NetCDF API
  - Automatic dimension mapping (bands, rows, columns)
  - Metadata and coordinate reference system preservation
- **Configurable Build Options**:
  - LZ4 and BZIP2 support can be enabled or disabled at build time
  - CDF support is optional (disabled by default)
  - GeoTIFF support is optional (disabled by default)
- **Fortran Support**:
  - Optional Fortran wrappers for NEP functions
  - Requires `netcdf-fortran` library when enabled

See the dedicated build and configuration page for full details on CMake and Autotools options:

- `docs/build-options.md`

## API Documentation

The API documentation is generated directly from the C and Fortran source code.

- **Versioned API docs**: Documentation is published per release (for example, `/NEP/v1.2.0/api/`).
- **Latest API docs**: A `latest` alias points to the most recent release documentation.

Key API reference pages:

- [C API (ncsqueeze.c)](ncsqueeze_8c.html)
- [Fortran API (ncsqueeze.F90)](ncsqueeze_8F90.html)

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

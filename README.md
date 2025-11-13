# NEP (NetCDF Extension Pack)

## Overview

NEP is a unified framework that provides high-performance LZ4 compression and enables seamless access to diverse scientific data formats through the standard NetCDF interface using User Defined Formats (UDF). It eliminates the need for data conversion or format-specific processing pipelines while preserving the optimizations of specialized formats.

## The Problem

Large-scale scientific data producers face significant challenges with data management:

- **Massive data volumes**: Unprecedented amounts of observational and simulation data in diverse formats
- **Complex integration**: Need to combine datasets from multiple sources across heterogeneous computing environments
- **Inefficient workflows**: Current approaches require:
  - Data conversion between formats
  - Separate processing pipelines for each format
  - Specialized custom solutions for each data type
  - Significant storage and computational overhead

## The Solution

NEP provides a transparent, runtime-pluggable solution:

### Core Features
- **LZ4 Compression** (v1.0.0): High-performance lossless compression for HDF5/NetCDF-4 files with superior speed characteristics
- **Automatic format detection**: NetCDF automatically identifies file formats at read time
- **Dynamic format loading**: Appropriate User Defined Format handlers are loaded on-demand
- **Transparent access**: Users can work with any supported format using the familiar NetCDF API
- **No conversion required**: Direct access to original data without preprocessing

### Supported Features by Release

| Feature | Version | Library | Status | Use Case |
|---------|---------|---------|--------|----------|
| LZ4 Compression | v1.0.0 | [LZ4](https://github.com/lz4/lz4) | Released | Fast compression for all NetCDF-4/HDF5 files |
| GRIB2 UDF | v2.0.0 | [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2) | In Development | Meteorological and weather data |
| CDF UDF | v3.0 | [NASA CDF](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html) | Planned | Space physics and satellite data |
| GeoTIFF UDF | v4.0 | [libgeotiff](https://github.com/OSGeo/libgeotiff) | Planned | Geospatial and remote sensing data |

## Key Benefits

- **High-Speed Compression**: LZ4 provides several times faster compression/decompression than DEFLATE while maintaining good compression ratios
- **Unified Interface**: Use existing NetCDF code with any supported format
- **Performance**: Preserves format-specific optimizations while providing standardized access
- **Efficiency**: Reduces storage requirements and computational overhead
- **Compatibility**: Works with existing scientific codebases without modification
- **Scalability**: Designed for exascale computing environments

---

## Installation

### Prerequisites

NEP requires the following dependencies:

- **NetCDF-C library** (v4.9+)
- **HDF5 library** (v1.12+)
- **CMake** (v3.9+) or **Autotools** for building
- **LZ4 library** for compression support
- **Format-specific libraries** (optional, based on desired UDF handlers):
  - GRIB2: [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2)
  - GeoTIFF: [libgeotiff](https://github.com/OSGeo/libgeotiff)
  - CDF: [NASA CDF library](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html)

### Test Data

NEP includes test data files in the `test/data/` directory for validation:
- **GRIB2**: `gdaswave.t00z.wcoast.0p16.f000.grib2` - Sample meteorological data file (v2.0.0+)
- Additional test files will be added for CDF and GeoTIFF formats in future releases

The build system automatically copies test data files to the build directory so tests can access them during both in-tree and out-of-tree builds.

### CMake Build and Installation

```bash
# Configure with desired UDF handlers
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DENABLE_GRIB2=ON \
    -DENABLE_GEOTIFF=ON \
    -DENABLE_CDF=ON

# Build
cmake --build build

# Install
cmake --install build

# Uninstall (if needed)
cmake --build build --target uninstall
```

### Autotools Build and Installation

```bash
# Configure with desired UDF handlers
./configure --prefix=/usr/local \
    --enable-grib2 \
    --enable-geotiff \
    --enable-cdf

# Build
make

# Install
make install

# Uninstall (if needed)
make uninstall
```

### Configuration Options

Both build systems support enable/disable options for each UDF handler:

| CMake Option | Autotools Option | Default | Description | Version |
|--------------|------------------|---------|-------------|---------||
| `-DENABLE_GRIB2=ON/OFF` | `--enable-grib2/--disable-grib2` | ON/enabled | GRIB2 UDF handler | v2.0.0+ |
| `-DENABLE_GEOTIFF=ON/OFF` | `--enable-geotiff/--disable-geotiff` | ON/enabled | GeoTIFF UDF handler | v4.0+ |
| `-DENABLE_CDF=ON/OFF` | `--enable-cdf/--disable-cdf` | ON/enabled | CDF UDF handler | v3.0+ |

### Using NEP in Your Project

After installation, you can use NEP in your CMake projects:

```cmake
find_package(NEP REQUIRED)
target_link_libraries(your_target PRIVATE NEP::grib2_udf)
```

Or link directly with the installed shared libraries:

```bash
gcc -o myapp myapp.c -L/usr/local/lib -lgrib2_udf
```

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd.md)** - Detailed product requirements and specifications
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

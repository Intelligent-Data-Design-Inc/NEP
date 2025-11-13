# NEP (NetCDF Extension Pack)

## v1.0.0: High-Performance Compression

NEP v1.0.0 provides high-performance compression for HDF5/NetCDF-4 files with two complementary algorithms:
- **LZ4**: Fast, lossless compression optimized for speed-critical workflows
- **BZIP2**: Higher compression ratios for storage-optimized workflows

**Future Versions**: NEP will expand to enable seamless access to diverse scientific data formats (GRIB2, CDF, GeoTIFF) through the standard NetCDF interface using User Defined Formats (UDF) in v2.0.0 and beyond.

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

### v1.0.0 Features
- **LZ4 Compression**: High-performance lossless compression for speed-critical workflows
  - >2x faster compression/decompression than DEFLATE
  - Optimized for real-time data processing
- **BZIP2 Compression**: Higher compression ratios for storage optimization
  - Better compression ratios than DEFLATE for archival storage
  - Block-sorting algorithm ideal for repetitive scientific data
- **Transparent Integration**: Works seamlessly with standard NetCDF-4 API
- **HDF5 Filter Plugins**: Automatic discovery and loading
- **Dual Build Systems**: Full CMake and Autotools support

### Future Features (v2.0.0+)
- **Automatic format detection**: NetCDF will automatically identify file formats at read time
- **Dynamic format loading**: UDF handlers will be loaded on-demand
- **Transparent access**: Users will work with any supported format using the familiar NetCDF API
- **No conversion required**: Direct access to original data without preprocessing

### Supported Features by Release

| Feature | Version | Library | Status | Use Case |
|---------|---------|---------|--------|----------|
| LZ4 Compression | v1.0.0 | [LZ4](https://github.com/lz4/lz4) | Released | Fast compression for real-time workflows |
| BZIP2 Compression | v1.0.0 | [BZIP2](https://sourceware.org/bzip2/) | Released | High compression ratios for archival storage |
| GRIB2 UDF | v2.0.0 | [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2) | In Development | Meteorological and weather data |
| CDF UDF | v3.0 | [NASA CDF](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html) | Planned | Space physics and satellite data |
| GeoTIFF UDF | v4.0 | [libgeotiff](https://github.com/OSGeo/libgeotiff) | Planned | Geospatial and remote sensing data |

## Key Benefits

- **Flexible Compression Options**: Choose between LZ4 for speed or BZIP2 for compression ratio based on your workflow needs
- **High-Speed Compression**: LZ4 provides several times faster compression/decompression than DEFLATE
- **Superior Compression Ratios**: BZIP2 provides better compression ratios than DEFLATE for archival storage
- **Unified Interface**: Use existing NetCDF code with any supported format
- **Performance**: Preserves format-specific optimizations while providing standardized access
- **Efficiency**: Reduces storage requirements and computational overhead
- **Compatibility**: Works with existing scientific codebases without modification
- **Scalability**: Designed for exascale computing environments

---

## Installation

### Prerequisites

NEP v1.0.0 requires the following dependencies:

- **NetCDF-C library** (v4.9+)
- **HDF5 library** (v1.12+)
- **CMake** (v3.9+) or **Autotools** for building
- **LZ4 library** for LZ4 compression support
- **BZIP2 library** for BZIP2 compression support
- **Doxygen** (optional, for building documentation)

**Future versions** (v2.0.0+) will add format-specific libraries:
  - GRIB2: [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2) (v2.0.0)
  - CDF: [NASA CDF library](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html) (v3.0)
  - GeoTIFF: [libgeotiff](https://github.com/OSGeo/libgeotiff) (v4.0)

### Test Data

NEP v1.0.0 includes comprehensive LZ4 and BZIP2 compression tests. Test data for UDF handlers will be added in future releases:
- **GRIB2**: Sample meteorological data file (v2.0.0+)
- **CDF**: Space physics test data (v3.0+)
- **GeoTIFF**: Geospatial test data (v4.0+)

### CMake Build and Installation

```bash
# Configure (v1.0.0 - LZ4 and BZIP2 compression)
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

### Autotools Build and Installation

```bash
# Bootstrap and configure (v1.0.0 - LZ4 and BZIP2 compression)
./autogen.sh
./configure --prefix=/usr/local --enable-lz4 --enable-bzip2

# Build
make

# Build documentation (optional)
make docs

# Install
make install

# Uninstall (if needed)
make uninstall
```

### Configuration Options

**v1.0.0 Options:**

| CMake Option | Autotools Option | Default | Description |
|--------------|------------------|---------|-------------|
| `-DBUILD_DOCUMENTATION=ON/OFF` | `--enable-docs/--disable-docs` | ON/enabled | Build API documentation |
| N/A | `--enable-lz4/--disable-lz4` | enabled | LZ4 compression support |
| N/A | `--enable-bzip2/--disable-bzip2` | enabled | BZIP2 compression support |

**Future Options** (v2.0.0+):

| CMake Option | Autotools Option | Version | Description |
|--------------|------------------|---------|-------------|
| `-DENABLE_GRIB2=ON/OFF` | `--enable-grib2/--disable-grib2` | v2.0.0+ | GRIB2 UDF handler |
| `-DENABLE_CDF=ON/OFF` | `--enable-cdf/--disable-cdf` | v3.0+ | CDF UDF handler |
| `-DENABLE_GEOTIFF=ON/OFF` | `--enable-geotiff/--disable-geotiff` | v4.0+ | GeoTIFF UDF handler |

### Using NEP in Your Project

**v1.0.0**: LZ4 and BZIP2 compression are provided as HDF5 filter plugins. Simply set the `HDF5_PLUGIN_PATH` environment variable to the NEP installation directory, and use standard NetCDF-4 compression APIs.

**Future versions** (v2.0.0+) will support CMake integration:

```cmake
find_package(NEP REQUIRED)
target_link_libraries(your_target PRIVATE NEP::grib2_udf)
```

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd.md)** - Detailed product requirements and specifications
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

# NEP (NetCDF Extension Pack)

## v1.0.0: High-Performance Compression

NEP v1.0.0 provides high-performance compression for HDF5/NetCDF-4 files with two complementary algorithms:
- **LZ4**: Fast, lossless compression optimized for speed-critical workflows
- **BZIP2**: Higher compression ratios for storage-optimized workflows

## The Problem

Large-scale scientific data producers face significant challenges with data storage:

- **Massive data volumes**: Unprecedented amounts of observational and simulation data
- **Storage costs**: Need to balance compression ratios with processing performance
- **Performance constraints**: Current compression options require trade-offs:
  - DEFLATE (zlib) is slow for large datasets
  - Limited compression algorithm choices in NetCDF-4
  - No easy way to select optimal compression for specific workflows

## The Solution

NEP provides high-performance compression options as HDF5 filter plugins:

### Features
- **LZ4 Compression**: High-performance lossless compression for speed-critical workflows
  - >2x faster compression/decompression than DEFLATE
  - Optimized for real-time data processing
- **BZIP2 Compression**: Higher compression ratios for storage optimization
  - Better compression ratios than DEFLATE for archival storage
  - Block-sorting algorithm ideal for repetitive scientific data
- **Transparent Integration**: Works seamlessly with standard NetCDF-4 API
- **HDF5 Filter Plugins**: Automatic discovery and loading
- **Dual Build Systems**: Full CMake and Autotools support

## Key Benefits

- **Flexible Compression Options**: Choose between LZ4 for speed or BZIP2 for compression ratio based on your workflow needs
- **High-Speed Compression**: LZ4 provides several times faster compression/decompression than DEFLATE
- **Superior Compression Ratios**: BZIP2 provides better compression ratios than DEFLATE for archival storage
- **Easy Integration**: Works seamlessly with standard NetCDF-4 API
- **Efficiency**: Reduces storage requirements with minimal performance overhead
- **Compatibility**: Works with existing NetCDF-4 applications without code changes
- **Scalability**: Designed for large-scale scientific computing environments

## Compression Performance

The following benchmarks compare compression methods on a 150 MB NetCDF-4 dataset:

### All Compression Methods

![Compression Performance](docs/compression_performance.svg)

### Fast Compression Methods (Excluding BZIP2)

For better visualization of the faster compression methods:

![Fast Compression Performance](docs/compression_performance_fast.svg)

| Method | Write Time (s) | File Size (MB) | Read Time (s) | Compression Ratio | Write Speed | Read Speed |
|--------|----------------|----------------|---------------|-------------------|-------------|------------|
| none   | 0.27          | 150.01         | 0.14          | 1.0×              | 1.0×        | 1.0×       |
| lz4    | 0.34          | 68.95          | 0.16          | 2.2×              | 0.79×       | 0.88×      |
| zstd   | 0.66          | 34.94          | 0.26          | 4.3×              | 0.41×       | 0.54×      |
| zlib   | 1.83          | 41.78          | 0.59          | 3.6×              | 0.15×       | 0.24×      |
| bzip2  | 22.14         | 22.39          | 5.90          | 6.7×              | 0.01×       | 0.02×      |

**Key Insights:**
- **LZ4** offers the best balance: 2.2× compression with minimal performance impact (79% write speed, 88% read speed)
- **BZIP2** achieves the highest compression ratio (6.7×) but is significantly slower (1% write speed, 2% read speed)
- **ZSTD** provides excellent compression (4.3×) with moderate performance impact (41% write speed, 54% read speed)
- **Read performance** generally mirrors write performance, with LZ4 being fastest and BZIP2 slowest

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

### Test Data

NEP v1.0.0 includes comprehensive LZ4 and BZIP2 compression tests with sample NetCDF-4 datasets.

### CMake Build and Installation

```bash
# Configure
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
# Bootstrap and configure
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

| CMake Option | Autotools Option | Default | Description |
|--------------|------------------|---------|-------------|
| `-DBUILD_DOCUMENTATION=ON/OFF` | `--enable-docs/--disable-docs` | ON/enabled | Build API documentation |
| N/A | `--enable-lz4/--disable-lz4` | enabled | LZ4 compression support |
| N/A | `--enable-bzip2/--disable-bzip2` | enabled | BZIP2 compression support |

### Using NEP in Your Project

LZ4 and BZIP2 compression are provided as HDF5 filter plugins. Simply set the `HDF5_PLUGIN_PATH` environment variable to the NEP installation directory, and use standard NetCDF-4 compression APIs.

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

---

## Documentation

For more detailed information about the project:

- **[PR/FAQ](docs/prfaq.md)** - Press release and frequently asked questions
- **[Roadmap](docs/roadmap.md)** - Development roadmap and release schedule
- **[Product Requirements](docs/prd_1.md)** - Detailed product requirements and specifications (v1.0.0)
- **[Design Document](docs/design.md)** - Technical architecture and design details

---

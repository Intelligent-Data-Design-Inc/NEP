![NEP Logo](docs/images/logo_small.png)

# NEP (NetCDF Expansion Pack)

**[📚 Full Documentation](https://intelligent-data-design-inc.github.io/NEP/)**

## High-Performance Compression + Multi-Format Data Access

NEP extends NetCDF-4 with powerful new capabilities for scientific data workflows:

- **Ultra-Fast LZ4 Compression**: 2-3x faster than DEFLATE with excellent compression ratios - ideal for real-time data processing and HPC workflows
- **High-Ratio BZIP2 Compression**: Superior compression for archival storage - reduce storage costs while maintaining data integrity
- **NASA CDF File Reader**: Access Common Data Format files directly through the familiar NetCDF API - no conversion needed
- **Drop-In Compatibility**: Works with existing NetCDF-4 applications without code changes

## Why NEP?

Scientific data producers need better tools to handle growing data volumes:

- **Storage Costs**: Petabyte-scale datasets require efficient compression without sacrificing performance
- **Processing Speed**: DEFLATE compression creates bottlenecks in data pipelines and analysis workflows
- **Data Format Silos**: CDF and NetCDF communities use different tools despite similar data structures
- **Limited Options**: NetCDF-4 needs more compression algorithms optimized for different use cases

## What NEP Delivers

### LZ4 Compression: Speed Without Compromise

**Performance**: 2-3x faster compression and decompression than DEFLATE while achieving 2.2x compression ratios on typical scientific datasets.

**Use Cases**:
- Real-time satellite data processing
- High-throughput simulation output
- Interactive data analysis workflows
- Cloud-based data pipelines

**How It Works**: LZ4 compression is provided as an HDF5 filter plugin. Simply set `HDF5_PLUGIN_PATH` and use standard NetCDF-4 compression APIs - no code changes required.

### BZIP2 Compression: Maximum Storage Efficiency

**Performance**: 6.7x compression ratios on scientific datasets - significantly better than DEFLATE for long-term archival storage.

**Use Cases**:
- Long-term data archives
- Reducing cloud storage costs
- Datasets with repetitive patterns
- Bandwidth-constrained data transfers

**How It Works**: Like LZ4, BZIP2 integrates as an HDF5 filter plugin with zero code changes to existing applications.

### CDF File Reader: Unified Data Access

**Capability**: Read NASA Common Data Format files using the NetCDF API you already know.

**Benefits**:
- No file conversion required - access CDF data directly
- Automatic type mapping (CDF types → NetCDF types)
- Support for TT2000 time variables and multi-dimensional arrays
- Unified analysis tools for both NetCDF and CDF datasets

**Use Cases**:
- Space physics and heliophysics research
- NASA mission data analysis (IMAP, MMS, Van Allen Probes)
- Cross-format data integration
- Legacy CDF data access in modern workflows

**How It Works**: NEP provides a User-Defined Format (UDF) handler that transparently reads CDF files through standard NetCDF functions like `nc_open()`, `nc_get_var()`, and `nc_get_att()`.

## Key Benefits

- **Choose Your Trade-Off**: Select LZ4 for speed or BZIP2 for compression ratio - optimize for your specific workflow
- **No Code Changes**: Drop-in replacement for existing NetCDF-4 applications via HDF5 filter plugins
- **Multi-Format Support**: Work with both NetCDF and CDF files using a single API
- **Production Ready**: Full CMake and Autotools build support, comprehensive test suites, CI validation
- **HPC Optimized**: Designed for large-scale scientific computing with Spack package manager support
- **Cost Savings**: Reduce storage and bandwidth costs without sacrificing data access performance

## Compression Performance

The following benchmarks compare compression methods on a 150 MB NetCDF-4 dataset. For comparison, I include ZSTD, which was recently added to NetCDF and provides better performance than ZLIB, though not as fast as LZ4:

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
- **ZSTD** (recently added to NetCDF) provides excellent compression (4.3×) with moderate performance impact (41% write speed, 54% read speed)
- **ZLIB** (standard DEFLATE) shows 3.6× compression but is slower than both LZ4 and ZSTD
- **BZIP2** achieves the highest compression ratio (6.7×) but is significantly slower (1% write speed, 2% read speed)
- **Read performance** generally mirrors write performance, with LZ4 being fastest and BZIP2 slowest

---

## CDF Reader

NEP includes support for reading NASA Common Data Format (CDF) files through a NetCDF-like API. CDF is a self-describing data format designed for the storage and manipulation of multi-dimensional data sets, widely used in space physics and solar research communities.

### What is CDF?

The Common Data Format (CDF) is a conceptually similar format to NetCDF, developed and maintained by NASA's Space Physics Data Facility (SPDF). CDF files are commonly used for storing time-series and multi-dimensional scientific data from space missions and ground-based observations.

**Key characteristics:**
- Self-describing format with metadata
- Support for multiple data types and dimensions
- Platform-independent binary format
- Optimized for space physics data

### Resources

- **[NASA CDF Homepage](https://cdf.gsfc.nasa.gov/)** - Official CDF library and documentation
- **[CDF C Reference Manual](https://spdf.gsfc.nasa.gov/pub/software/cdf/doc/cdf_C_RefManual.pdf)** - Complete C API reference

### CDF Support in NEP

NEP provides a User-Defined Format (UDF) handler that allows reading CDF files using NetCDF-style API calls. This enables applications to work with both NetCDF and CDF files through a unified interface.

To enable CDF support during build, use the `--enable-cdf` (Autotools) or `-DENABLE_CDF=ON` (CMake) configuration option. You must have the NASA CDF library installed on your system.

---

## Installation

### Prerequisites

NEP v1.4.0 requires the following dependencies:

- **NetCDF-C library** (v4.9+)
- **HDF5 library** (v1.12+)
- **CMake** (v3.9+) or **Autotools** for building
- **LZ4 library** for LZ4 compression support
- **BZIP2 library** for BZIP2 compression support
- **NetCDF-Fortran** (optional, for Fortran wrappers)
- **NASA CDF library** (v3.9+, optional, for CDF file support)
- **Doxygen** (optional, for building documentation)

### Spack Installation (Recommended for HPC)

NEP and CDF can be installed using Spack for simplified dependency management:

```bash
# Install NEP with all features
spack install nep

# Install NEP with minimal features
spack install nep~docs~fortran

# Install CDF library separately
spack install cdf

# Load packages
spack load nep
spack load cdf
```

**Status**: NEP and CDF packages submitted to spack/spack-packages repository (PR pending approval).

For more details on Spack installation options and variants, see **[Spack Installation Guide](docs/spack.md)**.

### Test Data

NEP v1.4.0 includes comprehensive test suites:
- LZ4 and BZIP2 compression tests with sample NetCDF-4 datasets
- CDF file reading tests with NASA IMAP MAG L1B calibration data

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

**Note:** If HDF5 is installed in a non-standard location, you may need to specify `HDF5_ROOT`:

```bash
cmake -B build -DHDF5_ROOT=/path/to/hdf5 -DCMAKE_INSTALL_PREFIX=/usr/local
```

For example, if HDF5 is installed in `/usr/local/hdf5-1.14.6`:

```bash
cmake -B build -DHDF5_ROOT=/usr/local/hdf5-1.14.6 -DCMAKE_INSTALL_PREFIX=/usr/local
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
| `-DENABLE_FORTRAN=ON/OFF` | `--enable-fortran/--disable-fortran` | ON/enabled | Fortran wrappers and tests |
| `-DENABLE_CDF=ON/OFF` | `--enable-cdf/--disable-cdf` | OFF/disabled | CDF UDF handler build (v1.3.0 Sprint 3+) |
| N/A | `--enable-lz4/--disable-lz4` | enabled | LZ4 compression support |
| N/A | `--enable-bzip2/--disable-bzip2` | enabled | BZIP2 compression support |

**Note on CDF Support (v1.3.0):** The `--enable-cdf` option enables building the CDF UDF handler library (`libnccdf`) with full read support for CDF files. To use this option, you must have the NASA CDF library installed. Download from: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/

The CDF UDF handler library is installed to `${prefix}/lib/libnccdf.so` (CMake) or `${prefix}/lib/libnccdf.la` (Autotools) when CDF support is enabled.

**Spack Users:** Install CDF separately with `spack install cdf` (v1.4.0+). The CDF variant will be added back to the NEP Spack package once the CDF package is accepted into the main Spack repository.

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
- **[Design Document](docs/design_1.md)** - Technical architecture and design details (v1.0.0)

---

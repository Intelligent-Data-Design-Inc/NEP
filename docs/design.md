# NEP – NetCDF Extension Pack v1.0.0
## Project Overview

The NetCDF Extension Pack (NEP) v1.0.0 provides high-performance compression for HDF5/NetCDF-4 files through HDF5 filter plugins. NEP enables flexible lossless compression with two complementary algorithms: LZ4 (optimized for speed) and BZIP2 (optimized for compression ratio).

## Architecture

NEP uses the HDF5 filter plugin architecture to provide transparent compression for NetCDF-4 files. The architecture consists of:

1. **Core NetCDF API**: Standard API used by applications
2. **HDF5 Filter Pipeline**: Compression filters integrated into HDF5 I/O operations
3. **LZ4 Filter Plugin**: High-speed compression filter
4. **BZIP2 Filter Plugin**: High-ratio compression filter
5. **NetCDF-4/HDF5 Storage**: Compressed data files

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[HDF5 Library]
       │
[HDF5 Filter Pipeline]
       │
┌──────┴──────┐
│             │
[LZ4 Filter]  [BZIP2 Filter]
│             │
└──────┬──────┘
       │
[Compressed NetCDF-4/HDF5 Files]
```

## Project Structure

The project is structured as follows:
- `/hdf5_plugins` - HDF5 filter plugins (LZ4 and BZIP2 compression)
- `/test` - Unit tests for compression filters
- `/test_h5` - HDF5-specific compression tests
- `/docs` - Project documentation
- `/.github/workflows` - CI/CD pipeline configurations

## Compression Filter Design

### LZ4 Compression Filter

**Purpose**: High-speed lossless compression for real-time and HPC workflows

**Technical Details**:
- LZ4 is a lossless data compression algorithm from the LZ77 family
- Focused on compression and decompression speed rather than maximum compression ratio
- Provides excellent trade-off between speed and compression ratio for scientific data

**Performance Characteristics**:
- Compression speed 2-3x faster than DEFLATE
- Decompression speed significantly faster than DEFLATE
- Compression ratio typically smaller than DEFLATE but sufficient for most scientific datasets
- Optimized for speed-critical workflows in HPC environments

**Implementation**:
- HDF5 filter plugin with unique filter ID
- Transparent integration with NetCDF-4 API
- Automatic discovery via HDF5_PLUGIN_PATH
- Lossless compression guarantee

### BZIP2 Compression Filter

**Purpose**: High-ratio lossless compression for archival storage

**Technical Details**:
- BZIP2 uses the Burrows-Wheeler block sorting algorithm
- Focused on achieving high compression ratios for storage optimization
- Block-sorting algorithm particularly effective for repetitive scientific data patterns

**Performance Characteristics**:
- Compression ratios typically better than DEFLATE and significantly better than LZ4
- Slower compression/decompression than LZ4 but better than most high-ratio algorithms
- Ideal for archival storage where compression ratio is prioritized over speed
- Effective for datasets with repetitive patterns common in scientific data

**Implementation**:
- HDF5 filter plugin with unique filter ID
- Transparent integration with NetCDF-4 API
- Automatic discovery via HDF5_PLUGIN_PATH
- Lossless compression guarantee

## Continuous Integration and Documentation

The project uses GitHub Actions for CI/CD with comprehensive documentation integration:

### Build and Test Pipeline
- Automatic builds on Ubuntu for both CMake and Autotools
- Installation of dependencies (HDF5, NetCDF-C, LZ4, BZIP2, Doxygen)
- Custom dependency builds with caching for performance
- Test execution via CMake (CTest) and Autotools
- Compression performance benchmarking

### Documentation Integration
- **Doxygen Build System Integration**: Complete documentation generation integrated into both build systems
  - CMake: `make docs` target using `find_package(Doxygen)` with conditional building
  - Autotools: `make docs` target with Doxygen detection in configure.ac
  - Template configuration: `Doxyfile.in` with automatic variable substitution
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
- **CI Documentation Builds**: Integrated documentation generation in CI pipeline
  - Documentation build matrix for both CMake and Autotools build systems
  - Zero-warning enforcement (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - GitHub Pages deployment for API documentation

## Build System Architecture

### Dual Build System Support

NEP provides comprehensive support for both CMake and Autotools build systems to ensure maximum compatibility across diverse computing environments:

#### CMake Build System
- **Modern CMake 3.9+**: Leverages contemporary CMake features and best practices
- **Modular Compilation**: Each filter builds as an independent shared library
- **Advanced Dependency Detection**: Automatic discovery with comprehensive Find modules
- **Configurable Build Options**: Fine-grained control over compression filter inclusion
- **Cross-Platform Support**: Unified build configuration across Linux and Unix
- **Installation Integration**: Complete install/uninstall target implementation

#### Autotools Build System
- **Traditional Configure/Make Workflow**: Standard GNU autotools compatibility
- **Legacy System Support**: Ensures compatibility with older computing environments
- **Parallel Configuration**: Maintains feature parity with CMake system
- **Portable Build Scripts**: Cross-platform shell script compatibility
- **Standard Installation Paths**: Follows GNU coding standards for installation

### HDF5 Filter Plugin Integration

Each compression filter is compiled as a separate HDF5 filter plugin:

```
hdf5_plugins/
├── LZ4/
│   └── src/
│       └── H5Zlz4.c          # LZ4 filter plugin → libh5lz4.so
└── BZIP2/
    └── src/
        └── H5Zbzip2.c        # BZIP2 filter plugin → libh5bzip2.so
```

### HDF5 Filter Plugin Architecture

#### Dynamic Loading Framework
- **Runtime Discovery**: Filters discovered via HDF5_PLUGIN_PATH environment variable
- **Separate Compilation Units**: Each filter compiles to independent `.so` files
- **Automatic Registration**: HDF5 automatically registers filters when needed
- **Filter ID Management**: Unique filter IDs for each compression algorithm
- **Memory Management**: Proper cleanup and resource management

#### Build Configuration Options

**CMake Configuration:**
```bash
# Installation path configuration
cmake -DCMAKE_INSTALL_PREFIX=/usr/local

# Documentation control
cmake -DBUILD_DOCUMENTATION=ON
```

**Autotools Configuration:**
```bash
# Configure with compression filters (default: enabled)
./configure --prefix=/usr/local --enable-lz4 --enable-bzip2

# Documentation control
./configure --enable-docs
```

### Dependency Management

#### Library Requirements

| Component | Required Library | Version | Source | Detection Method |
|-----------|------------------|---------|--------|------------------|
| Core | NetCDF-C | v4.9+ | Unidata | pkg-config, FindNetCDF.cmake |
| Core | HDF5 | v1.12+ | HDF Group | pkg-config, FindHDF5.cmake |
| LZ4 Filter | LZ4 | Latest | https://github.com/lz4/lz4 | pkg-config, FindLZ4.cmake |
| BZIP2 Filter | BZIP2 | Latest | System library | FindBZip2.cmake |
| Docs | Doxygen | Latest | Optional | find_package(Doxygen) |

#### Dependency Detection
- **Automatic Discovery**: Build systems automatically locate required libraries
- **Graceful Degradation**: Missing dependencies result in clear error messages
- **Version Validation**: Compatibility checking for library versions
- **Custom Paths**: Support for non-standard library installation locations

### Installation System

#### Installation Architecture
- **Single Installation Path**: All components install to unified, configurable location
- **HDF5 Plugin Installation**: Filters installed to HDF5 plugin directory
- **Proper RPATH Configuration**: Shared libraries configured with correct runtime paths
- **Standard Paths**: Follows platform conventions for library installation
- **HDF5_PLUGIN_PATH**: Filters installed to location discoverable by HDF5

#### Installation Configuration
**CMake:**
```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

**Autotools:**
```bash
./autogen.sh
./configure --prefix=/usr/local
make
make install
```

#### Installation Structure
```
${PREFIX}/
├── lib/
│   └── plugin/
│       ├── libh5lz4.so
│       └── libh5bzip2.so
├── include/
│   └── nep/
│       └── [header files]
└── share/doc/nep/
    └── html/
        └── [Doxygen generated documentation]
```

#### Uninstall Support
Both build systems provide complete uninstall functionality:
```bash
make uninstall  # Removes all installed files
```

## Usage

### Setting Up HDF5 Plugin Path

To use NEP compression filters, set the HDF5_PLUGIN_PATH environment variable:

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

### Using Compression in NetCDF-4 Applications

Compression filters are used transparently through the standard NetCDF-4 API. The HDF5 library automatically loads the appropriate filter when reading or writing compressed data.

## Requirements and Performance Considerations

The implementation must maintain:
- **LZ4 Compression Performance**: >2x speed improvement over DEFLATE compression
- **BZIP2 Compression Ratio**: Better compression ratios than DEFLATE
- **Memory Efficiency**: Support for large datasets up to available system memory
- **NetCDF API Compatibility**: 100% compatibility with existing NetCDF-4 applications
- **Transparent Operation**: Compression filters work seamlessly with standard NetCDF-4 API
- **Data Integrity**: 100% lossless compression guarantee

## Performance Benchmarks

Based on a 150 MB NetCDF-4 dataset:

| Method | Write Time (s) | File Size (MB) | Read Time (s) | Compression Ratio |
|--------|----------------|----------------|---------------|-------------------|
| none   | 0.27          | 150.01         | 0.14          | 1.0×              |
| lz4    | 0.34          | 68.95          | 0.16          | 2.2×              |
| zstd   | 0.66          | 34.94          | 0.26          | 4.3×              |
| zlib   | 1.83          | 41.78          | 0.59          | 3.6×              |
| bzip2  | 22.14         | 22.39          | 5.90          | 6.7×              |

**Key Findings**:
- LZ4 offers minimal performance impact (79% write speed, 88% read speed) with 2.2× compression
- BZIP2 achieves highest compression ratio (6.7×) at the cost of performance (1% write speed, 2% read speed)

## Release Information

- **Version**: v1.0.0
- **Status**: Production Release
- **Release Date**: November 2025
- **Features**: LZ4 and BZIP2 compression support for HDF5/NetCDF-4 files

---

*Last Updated: November 2025*

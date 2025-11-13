# NEP – NetCDF Extension Pack
## Project Overview

The NetCDF Extension Pack (NEP) provides high-performance LZ4 compression and adds support for diverse Earth Science data formats through NetCDF User Defined Formats (UDF). NEP enables transparent access to GRIB2, CDF, and GeoTIFF files through the standard NetCDF API.

## Architecture

NEP uses the NetCDF User Defined Format (UDF) architecture to provide transparent access to different file formats, along with HDF5 filter plugins for compression. The architecture consists of:

1. **Core NetCDF API**: Standard API used by applications
2. **Format Detection Layer**: Automatically identifies file formats at runtime via magic numbers
3. **UDF Handler Framework**: Dynamically registers format handlers via nc_def_user_format()
4. **Format-Specific UDF Handlers**: Specialized modules implementing NC_Dispatch for each format
5. **HDF5 Filter Plugins**: Compression filters (LZ4) for HDF5/NetCDF-4 files
6. **Native Data Storage**: Original data files remain in their native formats

```
[Application Layer]
       │
[NetCDF API]
       │
[Format Detection via Magic Numbers]
       │
┌──────┴───────┬──────────────┐
│              │              │
[UDF Handlers] [HDF5 Filters] [Native NetCDF-4]
│              │              │
└──────┬───────┴──────────────┘
       │
[Data Files: GRIB2/CDF/GeoTIFF/Compressed NetCDF-4]
```

## Project Structure

The project is structured as follows:
- `/src` - UDF handler source code and compression filters
- `/test` - Unit tests for validation
- `/test/data` - Test data files (GRIB2, CDF, GeoTIFF samples)
- `/test_grib2` - GRIB2-specific tests (v2.0.0+)
- `/docs` - Project documentation
- `/hdf5_plugins` - HDF5 filter plugins (LZ4 compression)
- `/.github/workflows` - CI/CD pipeline configurations

## UDF Handler Design

UDF handlers serve as translation layers between the NetCDF API and native format libraries. Each handler implements the NC_Dispatch interface to:

1. **Register** with NetCDF via nc_def_user_format() with format-specific magic numbers
2. **Map** NetCDF data model concepts to the native format
3. **Translate** API calls to native format operations
4. **Optimize** performance for specific format characteristics

### Handler Implementation

Each UDF handler implements the NC_Dispatch structure defined by NetCDF, including:

- **File operations**: open, close, sync, etc.
- **Dataset operations**: read, write, get/put var, etc.
- **Attribute operations**: get, set, create, etc.
- **Dimension operations**: define, inquire, etc.
- **Variable operations**: define, inquire, read/write, etc.

### Current Implementation Status (v1.0.0)

1. **LZ4 Compression** (v1.0.0 - Released):
   - HDF5 filter plugin for LZ4 compression
   - High-performance lossless compression for NetCDF-4/HDF5 files
   - Several times faster than DEFLATE compression
   - Transparent integration with NetCDF-4 API

2. **GRIB2 UDF Handler** (v2.0.0 - In Development):
   - NC_Dispatch structure with file operations
   - Integration with NCEPLIBS-g2 library
   - Magic number registration for automatic format detection
   - File open/close operations (Sprint 1)

3. **CDF UDF Handler** (v3.0 - Planned):
   - Foundation for NASA CDF library integration
   - NC_Dispatch interface implementation
   - File operations and metadata access

4. **GeoTIFF UDF Handler** (v4.0 - Planned):
   - Integration with libgeotiff library
   - Geospatial metadata handling
   - NC_Dispatch implementation for GeoTIFF access

## Continuous Integration and Documentation

The project uses GitHub Actions for CI/CD with comprehensive documentation integration:

### Build and Test Pipeline
- Automatic builds on Ubuntu for both CMake and Autotools
- Installation of dependencies (HDF5, NetCDF-C, NCEPLIBS-g2, libgeotiff, NASA CDF, LZ4, Doxygen, etc.)
- Custom dependency builds with caching for performance
- Test execution via CMake (CTest) and Autotools
- Matrix testing across all UDF handler configurations

### Documentation Integration (v0.1.2)
- **Doxygen Build System Integration**: Complete documentation generation integrated into both build systems
  - CMake: `make docs` target using `find_package(Doxygen)` with conditional building
  - Autotools: `make docs` target with Doxygen detection in configure.ac
  - Template configuration: `Doxyfile.in` with automatic variable substitution
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
- **CI Documentation Builds**: Integrated documentation generation in CI pipeline
  - Documentation build matrix for both CMake and Autotools build systems
  - Zero-warning enforcement (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - GitHub Pages deployment prepared for future release


## Build System Architecture (v0.1.1)

### Dual Build System Support

NEP provides comprehensive support for both CMake and Autotools build systems to ensure maximum compatibility across diverse computing environments:

#### CMake Build System
- **Modern CMake 3.9+**: Leverages contemporary CMake features and best practices
- **Modular UDF Handler Compilation**: Each handler builds as an independent shared library
- **Advanced Dependency Detection**: Automatic discovery with comprehensive Find modules
- **Configurable Build Options**: Fine-grained control over UDF handler inclusion
- **Cross-Platform Support**: Unified build configuration across Linux and Unix
- **Installation Integration**: Complete install/uninstall target implementation

#### Autotools Build System
- **Traditional Configure/Make Workflow**: Standard GNU autotools compatibility
- **Legacy System Support**: Ensures compatibility with older computing environments
- **Parallel Configuration**: Maintains feature parity with CMake system
- **Portable Build Scripts**: Cross-platform shell script compatibility
- **Standard Installation Paths**: Follows GNU coding standards for installation

### UDF Handler Build Integration

Each UDF handler is compiled as a separate shared library with isolated dependencies:

```
src/
├── grib2dispatch.c          # GRIB2 UDF handler → libncgrib2.so
├── grib2*.c/h               # (requires NCEPLIBS-g2, v2.0.0+)
├── ncsqueeze.c              # Compression utilities
└── ncsqueeze.h

hdf5_plugins/
└── LZ4/                     # LZ4 compression filter (v1.0.0)
    └── H5Zlz4.c             # HDF5 filter plugin
```

### Shared Library Architecture

#### Dynamic Registration Framework
- **Runtime Registration**: UDF handlers register via nc_def_user_format() with magic numbers
- **Separate Compilation Units**: Each handler compiles to independent `.so` files
- **Dependency Isolation**: Format-specific libraries contained within respective handlers
- **NC_Dispatch Interface**: Standardized function pointers for NetCDF operations
- **Memory Management**: Proper cleanup and resource management for UDF handlers

#### Build Configuration Options

**CMake Configuration:**
```bash
# Enable/disable individual UDF handlers (default: ON)
cmake -DENABLE_GRIB2=ON -DENABLE_GEOTIFF=ON -DENABLE_CDF=ON
```

**Autotools Configuration:**
```bash
# Configure with specific UDF handlers (default: enabled)
./configure --enable-grib2 --enable-geotiff --enable-cdf
```

### Dependency Management

#### Library Requirements

| Feature | Required Library | Version | Source | Detection Method |
|---------|------------------|---------|--------|------------------|
| LZ4 Compression | LZ4 | v1.0.0 | https://github.com/lz4/lz4 | pkg-config, FindLZ4.cmake |
| GRIB2 UDF | NCEPLIBS-g2 | v2.0.0+ | NOAA/NCEP libraries | pkg-config, FindNCEPLIBS-g2.cmake |
| GeoTIFF UDF | libgeotiff | v4.0+ | OSGeo project | pkg-config, FindLibGeoTIFF.cmake |
| CDF UDF | NASA CDF | v3.0+ | https://cdf.gsfc.nasa.gov | FindNASACDF.cmake |

#### Dependency Detection
- **Automatic Discovery**: Build systems automatically locate required libraries
- **Graceful Degradation**: Missing dependencies result in clear error messages
- **Version Validation**: Compatibility checking for library versions
- **Custom Paths**: Support for non-standard library installation locations

### Installation System

#### Installation Architecture
- **Single Installation Path**: All components install to unified, configurable location
- **Shared Library Installation**: UDF handlers and HDF5 filters installed with proper RPATH configuration
- **CMake Integration**: Generated config files enable `find_package(NEP)` support
- **Standard Paths**: Follows platform conventions for library installation
- **HDF5 Plugin Path**: LZ4 filter installed to HDF5_PLUGIN_PATH for automatic discovery

#### Installation Configuration
**CMake:**
```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/nep
make install
```

**Autotools:**
```bash
./configure --prefix=/opt/nep
make install
```

#### Uninstall Support
Both build systems provide complete uninstall functionality:
```bash
make uninstall  # Removes all installed files
```

## Requirements and Performance Considerations

The implementation must maintain:
- **LZ4 Compression Performance**: >2x speed improvement over DEFLATE compression
- **Minimal UDF Overhead**: <5% performance overhead compared to native format access
- **Format-specific optimizations**: Preserve native format performance characteristics
- **Memory efficiency**: Support for large datasets up to available system memory
- **NetCDF API Compatibility**: 100% compatibility with existing NetCDF-C applications
- **Transparent Operation**: UDF handlers and compression filters work seamlessly with standard NetCDF API

## Release Roadmap

- **v1.0.0** (Released): LZ4 compression support
- **v2.0.0** (In Development): GRIB2 UDF handler with file operations
- **v3.0** (Planned): CDF UDF handler
- **v4.0** (Planned): GeoTIFF UDF handler

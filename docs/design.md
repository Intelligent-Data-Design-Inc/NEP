# NFEP – GRIB2 VOL Connector

## Project Overview

The NetCDF4/HDF5 Format Extension Pack (NFEP) is a unified framework that enables seamless access to diverse Earth Science data formats through standard NetCDF4 and HDF5 interfaces. This framework eliminates the need for data conversion or format-specific processing pipelines while preserving the optimizations of specialized formats.

## Architecture

NFEP uses the Virtual Object Layer (VOL) connector architecture of HDF5 to provide transparent access to different file formats. The architecture consists of:

1. **Core HDF5/NetCDF4 Interface**: Standard API used by applications
2. **Format Detection Layer**: Automatically identifies file formats at runtime
3. **VOL Connector Framework**: Dynamically loads appropriate connectors
4. **Format-Specific Connectors**: Specialized modules for each supported format
5. **Native Data Storage**: Original data files remain in their native formats

```
[Application Layer]
       │
[HDF5/NetCDF4 API]
       │
[Format Detection]
       │
┌──────┴───────┐
│              │
[VOL Connectors]    [Native HDF5]
│              │
└──────┬───────┘
       │
[Data Files in Native Formats]
```

## Project Structure

The project is structured as follows:
- `/src` - All VOL connector source code
- `/test` - Unit tests for validation
- `/docs` - Project documentation
- `/bin` - Compiled binaries and executable tools
- `/.github/workflows` - CI/CD pipeline configurations

## VOL Connector Design

VOL connectors serve as translation layers between the HDF5/NetCDF4 APIs and native format libraries. Each connector implements the necessary interfaces to:

1. **Detect** the appropriate file format
2. **Map** HDF5 data model concepts to the native format
3. **Translate** API calls to native format operations
4. **Optimize** performance for specific format characteristics

### Connector Implementation

Each VOL connector implements the H5VL_class_t interface defined by HDF5, including:

- **File operations**: create, open, close, etc.
- **Dataset operations**: read, write, create, etc.
- **Attribute operations**: get, set, create, etc.
- **Group operations**: create, open, iterate, etc.
- **Object operations**: open, copy, get info, etc.

### Current Implementation Status

As of 2025-08-27, the project has implemented:

1. **GRIB2 VOL Connector**: Initial implementation with:
   - Basic VOL connector structure
   - Registration with HDF5
   - Core file detection capabilities

2. **BUFR VOL Connector**: Skeleton implementation with:
   - Basic connector framework
   - Placeholder for NCEPLIBS-bufr integration

3. **CDF VOL Connector**: Initial structure with:
   - Foundation for NASA CDF library integration
   - Basic connector interface

## Continuous Integration and Documentation

The project uses GitHub Actions for CI/CD with comprehensive documentation integration:

### Build and Test Pipeline
- Automatic builds on Ubuntu for both CMake and Autotools
- Installation of dependencies (HDF5, NCEPLIBS, libgeotiff, NASA CDF, Doxygen, etc.)
- Custom dependency builds with caching for performance
- Test execution via CMake (CTest) and Autotools
- Matrix testing across all VOL connector configurations

### Documentation Integration (v0.1.2)
- **CI Documentation Builds**: Integrated documentation generation in CI pipeline
  - Documentation build matrix for both CMake and Autotools build systems
  - Zero-warning enforcement (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
  - GitHub Pages deployment planned for future release


## Build System Architecture (v0.1.1)

### Dual Build System Support

NFEP provides comprehensive support for both CMake and Autotools build systems to ensure maximum compatibility across diverse computing environments:

#### CMake Build System
- **Modern CMake 3.12+**: Leverages contemporary CMake features and best practices
- **Modular VOL Connector Compilation**: Each connector builds as an independent shared library
- **Advanced Dependency Detection**: Automatic discovery with comprehensive Find modules
- **Configurable Build Options**: Fine-grained control over VOL connector inclusion
- **Cross-Platform Support**: Unified build configuration across Linux, Unix, and Windows
- **Installation Integration**: Complete install/uninstall target implementation

#### Autotools Build System
- **Traditional Configure/Make Workflow**: Standard GNU autotools compatibility
- **Legacy System Support**: Ensures compatibility with older computing environments
- **Parallel Configuration**: Maintains feature parity with CMake system
- **Portable Build Scripts**: Cross-platform shell script compatibility
- **Standard Installation Paths**: Follows GNU coding standards for installation

### VOL Connector Build Integration

Each VOL connector is compiled as a separate shared library with isolated dependencies:

```
src/
├── bufr_vol_connector.c     # BUFR connector → libnfep_bufr_vol.so
├── bufr_vol_connector.h     # (requires NCEPLIBS-bufr)
├── cdf_vol_connector.c      # CDF connector → libnfep_cdf_vol.so
├── cdf_vol_connector.h      # (requires NASA CDF library)
├── grib2_vol_connector.c    # GRIB2 connector → libnfep_grib2_vol.so
├── grib2_vol_connector.h    # (requires NCEPLIBS-g2)
├── geotiff_vol_connector.c  # GeoTIFF connector → libnfep_geotiff_vol.so
└── geotiff_vol_connector.h  # (requires libgeotiff)
```

### Shared Library Architecture

#### Dynamic Loading Framework
- **Runtime Loading**: VOL connectors loaded on-demand using `dlopen()` or platform equivalent
- **Separate Compilation Units**: Each connector compiles to independent `.so`/`.dll` files
- **Dependency Isolation**: Format-specific libraries contained within respective connectors
- **Plugin Registration**: Standardized entry points for connector discovery and initialization
- **Memory Management**: Proper cleanup and resource management for loaded connectors

#### Build Configuration Options

**CMake Configuration:**
```bash
# Enable/disable individual VOL connectors (default: ON)
cmake -DENABLE_GRIB2=ON -DENABLE_BUFR=OFF -DENABLE_GEOTIFF=ON -DENABLE_CDF=ON
```

**Autotools Configuration:**
```bash
# Configure with specific VOL connectors (default: enabled)
./configure --enable-grib2 --disable-bufr --enable-geotiff --enable-cdf
```

### Dependency Management

#### Library Requirements

| VOL Connector | Required Library | Source | Detection Method |
|---------------|------------------|--------|------------------|
| GRIB2 | NCEPLIBS-g2 | NOAA/NCEP libraries | pkg-config, FindNCEPLIBS-g2.cmake |
| BUFR | NCEPLIBS-bufr | NOAA/NCEP libraries | pkg-config, FindNCEPLIBS-bufr.cmake |
| GeoTIFF | libgeotiff | OSGeo project | pkg-config, FindLibGeoTIFF.cmake |
| CDF | NASA CDF | https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/cdf39_1-dist-all.tar.gz | FindNASACDF.cmake |

#### Dependency Detection
- **Automatic Discovery**: Build systems automatically locate required libraries
- **Graceful Degradation**: Missing dependencies result in clear error messages
- **Version Validation**: Compatibility checking for library versions
- **Custom Paths**: Support for non-standard library installation locations

### Installation System

#### Installation Architecture
- **Single Installation Path**: All components install to unified, configurable location
- **Shared Library Installation**: VOL connectors installed with proper RPATH configuration
- **CMake Integration**: Generated config files enable `find_package(NFEP)` support
- **Standard Paths**: Follows platform conventions for library installation

#### Installation Configuration
**CMake:**
```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/nfep
make install
```

**Autotools:**
```bash
./configure --prefix=/opt/nfep
make install
```

#### Uninstall Support
Both build systems provide complete uninstall functionality:
```bash
make uninstall  # Removes all installed files
```

## Requirements and Performance Considerations

The implementation must maintain:
- Minimal overhead compared to native format access
- Format-specific optimizations
- Support for parallel I/O operations where possible
- Memory efficiency for large datasets
- Compatibility with existing NetCDF4/HDF5 applications
- Efficient dynamic loading of shared libraries

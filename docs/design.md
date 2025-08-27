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

## Continuous Integration

The project uses GitHub Actions for CI/CD with:
- Automatic builds on Ubuntu
- Installation of dependencies (MPICH, CMake, etc.)
- Custom HDF5 1.14.5 build with caching
- Test execution via CMake


## Build System Architecture (v0.1.1)

### Dual Build System Support

NFEP supports both CMake and Autotools build systems to ensure maximum compatibility:

#### CMake Build System
- Modern CMake 3.12+ configuration
- Modular VOL connector compilation
- Automatic dependency detection
- Configurable build options for each VOL type

#### Autotools Build System
- Traditional configure/make workflow
- Cross-platform compatibility
- Legacy system support
- Parallel configuration options with CMake

### VOL Connector Build Integration

Each VOL connector is compiled as a separate shared library:

```
src/
├── bufr_vol_connector.c     # BUFR connector (requires NCEPLIBS-bufr)
├── bufr_vol_connector.h
├── cdf_vol_connector.c      # CDF connector (requires NASA CDF library)
├── cdf_vol_connector.h
├── grib2_vol_connector.c    # GRIB2 connector (requires NCEPLIBS-g2)
├── grib2_vol_connector.h
├── geotiff_vol_connector.c  # GeoTIFF connector (requires libgeotiff)
└── geotiff_vol_connector.h
```

### Shared Library Architecture

- **Dynamic Loading**: VOL connectors are loaded at runtime using `dlopen()`
- **Separate Compilation**: Each connector builds to its own shared library
- **Dependency Isolation**: Format-specific dependencies are contained within each connector
- **Runtime Discovery**: Connectors register themselves with the NFEP framework

### Dependency Management

| VOL Connector | Required Library | Source |
|---------------|------------------|--------|
| GRIB2 | NCEPLIBS-g2 | NOAA/NCEP libraries |
| BUFR | NCEPLIBS-bufr | NOAA/NCEP libraries |
| GeoTIFF | libgeotiff | OSGeo project |
| CDF | NASA CDF | https://cdf.gsfc.nasa.gov/html/sw_and_docs.html |

## Requirements and Performance Considerations

The implementation must maintain:
- Minimal overhead compared to native format access
- Format-specific optimizations
- Support for parallel I/O operations where possible
- Memory efficiency for large datasets
- Compatibility with existing NetCDF4/HDF5 applications
- Efficient dynamic loading of shared libraries

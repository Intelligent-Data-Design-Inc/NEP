# NEP (NetCDF Extension Pack) v2.0.0 - Product Requirements Document

## 0. NEP v1.1.0 Fortran Wrappers

### 0.1 Overview
NEP v1.1.0 will add Fortran wrappers for existing NEP compression functions, allowing Fortran applications to use NEP compression capabilities through a Fortran-friendly API while preserving the behavior of the underlying C implementation.

### 0.2 Objectives
- Maintain existing NEP C API behavior with zero breaking changes when Fortran wrappers are enabled.
- Provide Fortran-callable wrappers for the existing compression functions used in v1.0.0.
- Integrate Fortran wrappers into both CMake and Autotools build systems.

### 0.3 Success Metrics
- No observable behavior changes to existing C applications (API, ABI, or compression behavior) when Fortran support is enabled.
- Successful compilation and installation of Fortran libraries and modules by default on supported platforms.
- All defined Fortran smoke tests pass in CI across supported configurations.

## 0.4 Key Features (Fortran Wrappers)

- Fortran-callable interfaces for NEP compression functions.
- Default-on Fortran build with an option to disable Fortran support.
- Consistent error handling and return codes matching the underlying C API.
- Basic test coverage via smoke tests for each exposed Fortran wrapper.

## 0.5 Technical Requirements (Fortran Wrappers)

### 0.5.1 API Requirements
- Provide Fortran wrappers for the compression functions exposed in the v1.0.0 C API.
- Define a consistent naming convention for Fortran entry points (for example, suffix-based or module-based) that maps directly to the underlying C symbols.
- Ensure Fortran wrappers map errors to the same NetCDF/NEP error codes as the corresponding C functions.
- Document any Fortran-specific argument handling (e.g., string length handling, array indexing expectations) in API comments and user documentation.

### 0.5.2 Build System Integration
- **CMake**:
  - Build Fortran wrappers by default.
  - Provide an option `-DENABLE_FORTRAN=ON/OFF`.
  - When `ENABLE_FORTRAN=OFF`, do not build or install any Fortran libraries or modules, and do not build or run Fortran tests.
- **Autotools**:
  - Build Fortran wrappers by default.
  - Provide options `--enable-fortran/--disable-fortran`.
  - When `--disable-fortran` is used, do not build or install any Fortran libraries or modules, and do not build or run Fortran tests.
- Fortran source code will live in `fsrc/`; Fortran tests will live in `ftest/`.

### 0.5.3 Testing and CI
- Implement at least one smoke test in `ftest/` for each exposed Fortran wrapper, covering:
  - Successful call with valid arguments.
  - Basic verification that the result is consistent with the corresponding C behavior where practical.
- Integrate Fortran tests into existing CI workflows:
  - For configurations where Fortran is enabled, build and run Fortran tests.
  - For configurations where Fortran is disabled, skip Fortran-related builds and tests cleanly.

### 0.5.4 Compatibility and Dependencies
- Enabling Fortran wrappers must not change the behavior of existing C APIs or compression functionality.
- Disabling Fortran must not affect any existing C-only workflows.
- Fortran wrappers will rely on the same core dependencies used by the C compression implementation (e.g., NetCDF-C, NetCDF-Fortran, HDF5, LZ4) without introducing new mandatory dependencies beyond a Fortran compiler and NetCDF-Fortran.

## 1. Executive Summary

### 1.1 Product Overview
NEP v2.0.0 will extend the NetCDF Extension Pack to support GRIB2 meteorological data format through a User Defined Format (UDF) handler, enabling transparent access to GRIB2 files through the standard NetCDF API.

### 1.2 Business Objectives
- Enable seamless access to GRIB2 meteorological data
- Eliminate need for data conversion from GRIB2 to NetCDF
- Maintain backward compatibility with v1.0.0 compression features
- Support meteorological and weather forecasting workflows

### 1.3 Success Metrics
- Zero breaking changes to existing applications
- <5% performance overhead compared to native GRIB2 access
- Successful integration with meteorological data workflows

## 2. Key Features (Planned)

- GRIB2 format support via UDF handler
- Automatic GRIB2 format detection
- Runtime-pluggable GRIB2 handler
- Standard NetCDF API access to GRIB2 data
- Integration with NCEPLIBS-g2 library

## 3. Technical Requirements (To Be Developed)

### 3.1 GRIB2 UDF Handler
- File open/close operations
- Metadata extraction
- Data reading functionality
- Variable information access

### 3.2 Dependencies
- NCEPLIBS-g2 library
- All v1.0.0 dependencies

## 4. Timeline (Planned)

- Sprint 1: GRIB2 file open/close
- Sprint 2: File metadata
- Sprint 3: Reading data
- Sprint 4: Variable metadata

---

*Version: 2.0.0 (Planning)*
*Status: Not yet implemented*

## 5. NEP v1.5.0 GeoTIFF Read Layer

### 5.1 Overview
NEP v1.5.0 will add support for GeoTIFF geospatial raster data format through a User Defined Format (UDF) handler, enabling transparent access to GeoTIFF files through the standard NetCDF API.

### 5.2 Business Objectives
- Enable seamless access to GeoTIFF geospatial raster data
- Eliminate need for data conversion from GeoTIFF to NetCDF
- Support geospatial analysis and remote sensing workflows
- Maintain backward compatibility with all previous NEP features
- Leverage libgeotiff for GeoTIFF file operations

### 5.3 Success Metrics
- Zero breaking changes to existing applications
- <5% performance overhead compared to native GeoTIFF access
- Successful integration with geospatial data workflows
- Full support for GeoTIFF metadata and georeferencing information

## 6. Key Features (GeoTIFF - Planned)

- GeoTIFF format support via UDF handler
- Automatic GeoTIFF format detection
- Runtime-pluggable GeoTIFF handler
- Standard NetCDF API access to GeoTIFF raster data
- Integration with libgeotiff library
- Support for georeferencing and coordinate system metadata
- Access to GeoTIFF tags and key-value pairs

## 7. Technical Requirements (GeoTIFF - To Be Developed)

### 7.1 GeoTIFF UDF Handler
- File open/close operations for GeoTIFF files
- Metadata extraction (dimensions, data types, georeferencing)
- Raster data reading functionality
- Variable information access (bands, coordinate systems)
- GeoTIFF tag and key reading
- Coordinate reference system (CRS) information extraction

### 7.2 Dependencies
- libgeotiff library (https://github.com/OSGeo/libgeotiff)
- All v1.0.0 dependencies (NetCDF-C, HDF5)
- Optional: PROJ library for coordinate transformations

### 7.3 Build System Integration
- Optional GeoTIFF support (enabled via build flags)
- CMake: `-DENABLE_GEOTIFF=ON/OFF`
- Autotools: `--enable-geotiff/--disable-geotiff`
- Automatic libgeotiff detection during configuration

### 7.4 API Design
- NC_Dispatch implementation for GeoTIFF format
- Standard NetCDF API functions for file operations
- Transparent access to GeoTIFF raster bands as NetCDF variables
- Metadata mapping from GeoTIFF tags to NetCDF attributes

## 8. Timeline (GeoTIFF - Planned)

- Sprint 1: GeoTIFF file open/close and format detection
- Sprint 2: Metadata extraction and dimension mapping
- Sprint 3: Raster data reading
- Sprint 4: Coordinate system and georeferencing support

---

*Version: 1.5.0 (Planning)*
*Status: Not yet implemented*
*Reference: ESDS-RFC-040v1.1.pdf in docs directory*

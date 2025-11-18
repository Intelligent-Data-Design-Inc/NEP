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

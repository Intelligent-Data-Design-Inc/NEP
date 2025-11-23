# NEP (NetCDF Extension Pack) v1.3.0 - Product Requirements Document

## 1. Executive Summary

### 1.1 Product Overview
NEP v1.3.0 will extend the NetCDF Extension Pack to support NASA Common Data Format (CDF) through a User Defined Format (UDF) handler, enabling transparent access to CDF files through the standard NetCDF API.

### 1.2 Business Objectives
- Enable seamless access to CDF space physics and satellite data
- Eliminate need for data conversion from CDF to NetCDF
- Maintain backward compatibility with v1.0.0, v1.1.0, and v1.2.0 features
- Support space physics research workflows

### 1.3 Success Metrics
- Zero breaking changes to existing applications
- <5% performance overhead compared to native CDF access
- Successful integration with space physics data workflows

## 2. Key Features

- CDF format support via UDF handler
- Automatic CDF format detection
- Runtime-pluggable CDF handler
- Standard NetCDF API access to CDF data
- Integration with NASA CDF library v3.9.x

## 3. Technical Requirements

### 3.1 CDF UDF Handler
- File open/close operations
- Metadata extraction
- Data reading functionality
- Variable information access

### 3.2 Dependencies
- NASA CDF library v3.9.x (built from source)
- All v1.0.0 and v2.0.0 dependencies

### 3.3 Build System Requirements
- Optional CDF support via build flags (default OFF)
- CMake: `-DENABLE_CDF=ON/OFF`
- Autotools: `--enable-cdf/--disable-cdf`
- Automatic CDF library detection with clear error messages
- Test data file integration in build systems

## 4. Timeline

### v1.3.0 Implementation
- **Sprint 1**: Add CDF library to CI and build systems
  - NASA CDF library v3.9.x integration
  - Build system configuration (CMake and Autotools)
  - CI workflow updates with CDF caching
  - Small test data file from NASA SPDF archives
  - Configuration file updates (HAVE_CDF macro)
  
- **Sprint 2**: Add CDF UDF handler
  - UDF handler structure with NC_Dispatch implementation
  - File open/close operations
  - Basic format detection
  
- **Sprint 3**: CDF file operations and metadata
  - Open test CDF file using NetCDF API
  - File metadata extraction
  - Variable metadata access
  - Data reading functionality

---

*Version: 1.3.0*
*Status: In planning*

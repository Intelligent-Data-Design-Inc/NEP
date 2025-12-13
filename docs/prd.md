# NEP (NetCDF Extension Pack) - Product Requirements Document

**Current Version**: v1.3.0

## 0. Version Overview

### v1.0.0 - Compression Filters (Released)
High-performance compression support for HDF5/NetCDF-4 files:
- LZ4 compression filter (optimized for speed)
- BZIP2 compression filter (optimized for compression ratio)
- HDF5 filter plugin architecture
- Dual build system support (CMake and Autotools)

### v1.1.0 - Fortran Wrappers (Released)
Fortran-callable interfaces for NEP compression functions:
- Fortran wrappers for v1.0.0 compression functions
- Default-on Fortran build with disable option
- Consistent error handling matching C API
- Smoke test coverage for Fortran wrappers

### v1.3.0 - CDF Format Support (Current - Released)
NASA Common Data Format (CDF) support via UDF handler:
- CDF UDF handler using NC_Dispatch interface
- Automatic CDF format detection
- Integration with NASA CDF library v3.9.x
- Transparent access to CDF files through NetCDF API

## 1. Executive Summary

### 1.1 Product Overview
The NetCDF Extension Pack (NEP) provides high-performance compression for HDF5/NetCDF-4 files through HDF5 filter plugins, and extends NetCDF to support additional scientific data formats through User Defined Format (UDF) handlers.

**Current Release (v1.3.0)** adds NASA Common Data Format (CDF) support, enabling transparent access to CDF space physics and satellite data through the standard NetCDF API.

### 1.2 Business Objectives
- Provide flexible compression options for NetCDF-4/HDF5 files
- Enable seamless access to CDF space physics and satellite data
- Eliminate need for data conversion from CDF to NetCDF
- Reduce storage requirements while maintaining data integrity
- Optimize I/O performance in HPC environments
- Support diverse scientific workflow requirements (compression, space physics)
- Maintain backward compatibility across all versions

### 1.3 Success Metrics
- Zero breaking changes to existing NetCDF applications
- LZ4 compression demonstrating >2x speed improvement over DEFLATE (v1.0.0)
- BZIP2 compression demonstrating better compression ratios than DEFLATE (v1.0.0)
- <5% performance overhead for CDF access compared to native CDF library (v1.3.0)
- Successful integration with space physics data workflows (v1.3.0)
- Production-ready stability and performance

## 2. Product Description

### 2.1 Target Users
- **Primary**: Scientific researchers working with NetCDF-4/HDF5 and CDF datasets
- **Secondary**: HPC system administrators optimizing I/O performance
- **Tertiary**: Space physics researchers working with NASA CDF data

### 2.2 Use Cases
1. **High-Performance Data Compression**: Fast I/O operations with LZ4 compression
2. **Archival Storage Optimization**: Maximum compression ratios with BZIP2
3. **Space Physics Data Access**: Direct access to NASA CDF files via NetCDF API
4. **HPC Workflow Optimization**: Reducing I/O bottlenecks in large-scale computing
5. **Real-Time Data Processing**: Fast compression for time-sensitive workflows
6. **Multi-Format Data Integration**: Unified API access to NetCDF and CDF formats

### 2.3 Key Features

#### v1.0.0 Features (Released)
- LZ4 lossless compression for HDF5/NetCDF-4 files with superior speed
- BZIP2 lossless compression for HDF5/NetCDF-4 files with superior compression ratios
- Transparent integration with standard NetCDF-4 API
- HDF5 filter plugin architecture
- Dual build system support (CMake and Autotools)

#### v1.1.0 Features (Released)
- Fortran-callable interfaces for NEP compression functions
- Default-on Fortran build with optional disable
- Consistent error handling matching C API

#### v1.3.0 Features (Current - Released)
- CDF format support via UDF handler
- Automatic CDF format detection via magic numbers
- Runtime-pluggable CDF handler
- Standard NetCDF API access to CDF data
- Integration with NASA CDF library v3.9.x
- File open/close operations for CDF files
- Metadata extraction from CDF files
- Data reading functionality from CDF files
- Variable information access from CDF files

## 3. Functional Requirements

### 3.1 Compression Support (v1.0.0 - Released)

#### FR-001: LZ4 Compression
- **Description**: High-performance lossless compression for HDF5/NetCDF-4 files
- **Technical Details**:
  - LZ4 lossless data compression algorithm from the LZ77 family
  - Focused on compression and decompression speed
  - Excellent trade-off between speed and compression ratio
- **Performance Characteristics**:
  - Compression speed 2-3x faster than DEFLATE
  - Decompression speed significantly faster than DEFLATE
  - Optimized for speed-critical workflows in HPC environments
- **Status**: ✅ Implemented and Released

#### FR-002: BZIP2 Compression
- **Description**: High-ratio lossless compression for HDF5/NetCDF-4 files
- **Technical Details**:
  - BZIP2 uses Burrows-Wheeler block sorting algorithm
  - Focused on achieving high compression ratios
  - Effective for repetitive scientific data patterns
- **Performance Characteristics**:
  - Compression ratios better than DEFLATE and LZ4
  - Ideal for archival storage
- **Status**: ✅ Implemented and Released

### 3.2 CDF Format Support (v1.3.0 - Released)

#### FR-003: CDF UDF Handler
- **Description**: User Defined Format handler for NASA CDF files
- **Technical Implementation**:
  - NC_Dispatch interface implementation
  - Translation layer between NetCDF API and NASA CDF library
  - Magic number-based format detection
  - Runtime registration via nc_def_user_format()
- **Operations**:
  - File open/close operations
  - Metadata extraction from CDF files
  - Data reading functionality
  - Variable information access
- **Dependencies**: NASA CDF library v3.9.x (built from source)
- **Status**: ✅ Implemented and Released

#### FR-004: CDF Build System Integration
- **Description**: Optional CDF support via build flags
- **Configuration**:
  - CMake: `-DENABLE_CDF=ON/OFF` (default: OFF)
  - Autotools: `--enable-cdf/--disable-cdf` (default: disabled)
- **Detection**: AC_CHECK_HEADERS + AC_SEARCH_LIBS pattern (Autotools), find_package/find_library (CMake)
- **Configuration Macros**: HAVE_CDF defined when enabled and found
- **Status**: ✅ Implemented and Released

### 3.3 Build System Integration (All Versions)

#### FR-005: Dual Build System Support
- **Description**: Complete build system support for both CMake and Autotools
- **Acceptance Criteria**:
  - Both build systems compile all components successfully
  - Enable/disable options work correctly for all features
  - Dependencies properly detected and linked
  - Clear error messages when required dependencies are missing
- **Status**: ✅ Implemented and Released

#### FR-006: Installation System
- **Description**: Complete installation target implementation
- **Installation Structure**:
  ```
  ${PREFIX}/
  ├── lib/
  │   └── plugin/
  │       ├── libh5lz4.so
  │       └── libh5bzip2.so
  ├── include/
  │   └── nep/
  └── share/doc/nep/
      └── html/
  ```
- **Status**: ✅ Implemented and Released

#### FR-007: Documentation System
- **Description**: Doxygen documentation generation integrated into build systems
- **Features**:
  - API reference documentation
  - Zero-warning enforcement
  - GitHub Pages deployment
- **Status**: ✅ Implemented and Released


## 4. Non-Functional Requirements

### 4.1 Performance Requirements

#### NFR-001: LZ4 Speed Performance
- **Requirement**: >2x faster compression/decompression than DEFLATE
- **Status**: ✅ Achieved (v1.0.0)

#### NFR-002: BZIP2 Compression Ratio
- **Requirement**: Better compression ratios than DEFLATE
- **Status**: ✅ Achieved (v1.0.0)

#### NFR-003: CDF Access Performance
- **Requirement**: <5% performance overhead compared to native CDF access
- **Status**: ✅ Achieved (v1.3.0)

#### NFR-004: Memory Efficiency
- **Requirement**: Support datasets up to available system memory
- **Status**: ✅ Achieved

### 4.2 Compatibility Requirements

#### NFR-005: Backward Compatibility
- **Requirement**: Zero breaking changes to existing NetCDF-4 applications
- **Status**: ✅ Maintained across all versions

#### NFR-006: Cross-Platform Support
- **Requirement**: Support Linux and Unix platforms
- **Status**: ✅ Achieved

### 4.3 Quality Requirements

#### NFR-007: Data Integrity
- **Requirement**: 100% lossless compression guarantee
- **Status**: ✅ Achieved (v1.0.0)

#### NFR-008: Stability
- **Requirement**: Production-ready stability
- **Status**: ✅ Achieved

## 5. Technical Architecture

### 5.1 System Components

#### System Components
1. **LZ4 HDF5 Filter Plugin**: High-speed compression filter (v1.0.0)
2. **BZIP2 HDF5 Filter Plugin**: High-ratio compression filter (v1.0.0)
3. **Fortran Wrappers**: Fortran-callable compression interfaces (v1.1.0)
4. **CDF UDF Handler**: NASA CDF format support (v1.3.0)
5. **Build System Integration**: CMake and Autotools support (all versions)
6. **Documentation System**: Doxygen-generated API documentation (all versions)

### 5.2 Technology Stack
- **Core Language**: C (for performance and compatibility)
- **Build Systems**: CMake and Autotools
- **Testing Framework**: Custom C testing framework
- **Documentation**: Doxygen for API docs

### 5.3 Dependencies

#### Core Dependencies
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+) for NetCDF-4 backend

#### Optional Dependencies
- LZ4 library for LZ4 compression (v1.0.0)
- BZIP2 library for BZIP2 compression (v1.0.0)
- NetCDF-Fortran for Fortran wrappers (v1.1.0)
- NASA CDF library v3.9.x for CDF support (v1.3.0)
- Doxygen for building documentation (optional)

### 5.4 Architecture Details

#### HDF5 Filter Plugin Architecture (v1.0.0)
- Dynamic loading via HDF5_PLUGIN_PATH
- Separate compilation units for each filter
- Automatic registration by HDF5
- Unique filter IDs for each algorithm

#### UDF Handler Architecture (v1.3.0)
- NC_Dispatch interface implementation
- Magic number-based format detection
- Runtime registration via nc_def_user_format()
- Translation layer to native format libraries

## 6. Quality Assurance

### 6.1 Testing Strategy
- **Unit Testing**: Individual component testing
- **Integration Testing**: NetCDF API compatibility testing
- **Performance Testing**: Benchmark against native implementations
- **Regression Testing**: Backward compatibility validation
- **Platform Testing**: Multi-platform verification
- **CI/CD**: GitHub Actions with comprehensive test coverage

### 6.2 Documentation Requirements
- API reference documentation
- User installation and usage guides
- Performance tuning guidelines
- Example code and use cases
- Format-specific documentation (CDF)

## 7. Success Criteria

### 7.1 v1.0.0 Launch Criteria (Released)
- [x] LZ4 compression filter implemented and tested
- [x] BZIP2 compression filter implemented and tested
- [x] >2x speed improvement over DEFLATE demonstrated (LZ4)
- [x] Better compression ratios than DEFLATE demonstrated (BZIP2)
- [x] 100% backward compatibility with existing applications
- [x] Complete API documentation
- [x] Successful testing on Linux/Unix platforms

### 7.2 v1.1.0 Launch Criteria (Released)
- [x] Fortran wrappers for compression functions
- [x] Default-on Fortran build with disable option
- [x] Smoke test coverage for Fortran wrappers
- [x] Zero breaking changes to C API

### 7.3 v1.3.0 Launch Criteria (Released)
- [x] CDF UDF handler implemented
- [x] File open/close operations for CDF files
- [x] Metadata extraction from CDF files
- [x] Data reading functionality from CDF files
- [x] Variable information access from CDF files
- [x] <5% performance overhead vs native CDF access
- [x] Integration with NASA CDF library v3.9.x
- [x] Build system integration (CMake and Autotools)
- [x] CI integration with CDF library caching
- [x] Test data integration


## 8. Timeline and Milestones

### v1.0.0 - Production Release (Released - November 2025)
- [x] LZ4 compression plugin
- [x] BZIP2 compression plugin
- [x] Comprehensive test suite
- [x] Complete documentation
- [x] CI/CD pipeline integration

### v1.1.0 - Fortran Wrappers (Released)
- [x] Fortran-callable interfaces
- [x] Build system integration
- [x] Smoke test coverage

### v1.3.0 - CDF Format Support (Released - December 2025)
- [x] NASA CDF library integration
- [x] CDF UDF handler implementation
- [x] Build system configuration
- [x] CI workflow updates
- [x] Test data integration
- [x] File operations and metadata access

---

*Current Version: 1.3.0*
*Last Updated: December 2025*

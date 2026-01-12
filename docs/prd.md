# NEP (NetCDF Extension Pack) - Product Requirements Document

## 1. Executive Summary

### 1.1 Product Overview
NEP (NetCDF Extension Pack) extends NetCDF-4 capabilities with high-performance compression filters and User Defined Format (UDF) handlers for accessing diverse scientific data formats through the standard NetCDF API.

### 1.2 Business Objectives
- Provide high-performance compression for NetCDF-4/HDF5 files
- Enable seamless access to multiple scientific data formats without conversion
- Support meteorological, geospatial, and space physics workflows
- Maintain full backward compatibility across all releases
- Simplify installation and deployment in HPC environments

### 1.3 Success Metrics
- Zero breaking changes across all releases
- High-performance compression with minimal overhead
- Successful integration with scientific data workflows
- Comprehensive test coverage and CI validation

---

## 2. Core Architecture

### 2.1 NetCDF User Defined Format (UDF) System
- **Dispatch Layer**: Format handlers implement `NC_Dispatch` structure with function pointers
- **Format Registration**: Handlers register via `nc_def_user_format(NC_UDFx, &dispatcher, magic_number)`
- **Magic Numbers**: Automatic format detection using file header signatures
- **File Operations**: Standard NetCDF API (`nc_open()`, `nc_close()`) with UDF mode flags
- **Error Handling**: Native NetCDF error codes for consistent error reporting

### 2.2 HDF5 Filter Plugin System
- **Compression Filters**: LZ4 and BZIP2 filters as HDF5 plugins
- **Transparent Integration**: Works with existing NetCDF-4 applications
- **Runtime Loading**: Filters loaded via `HDF5_PLUGIN_PATH`
- **Standard API**: NetCDF compression API for filter configuration

---

## 3. Compression Features (v1.0.0, v1.1.0)

### 3.1 LZ4 Compression
- **High-Performance**: 2-3x faster compression/decompression than DEFLATE
- **Speed Optimized**: Ideal for I/O-bound workflows
- **Lossless**: Guarantees data integrity
- **HPC Ready**: Designed for high-performance computing environments

**C API:**
- `nc_def_var_lz4(ncid, varid, level)` - Enable LZ4 compression
- `nc_inq_var_lz4(ncid, varid, lz4p, levelp)` - Query LZ4 settings

**Fortran API (v1.1.0):**
- `nf90_def_var_lz4(ncid, varid, level)` - Enable LZ4 compression
- `nf90_inq_var_lz4(ncid, varid, lz4p, levelp)` - Query LZ4 settings

### 3.2 BZIP2 Compression
- **High-Ratio**: Better compression ratios than DEFLATE
- **Archival Optimized**: Ideal for long-term storage
- **Block-Sorting Algorithm**: Effective for repetitive scientific data
- **Lossless**: Guarantees data integrity

**C API:**
- `nc_def_var_bzip2(ncid, varid, level)` - Enable BZIP2 compression
- `nc_inq_var_bzip2(ncid, varid, bzip2p, levelp)` - Query BZIP2 settings

**Fortran API (v1.1.0):**
- `nf90_def_var_bzip2(ncid, varid, level)` - Enable BZIP2 compression
- `nf90_inq_var_bzip2(ncid, varid, bzip2p, levelp)` - Query BZIP2 settings

### 3.3 Build Configuration
**CMake:**
- `BUILD_LZ4=ON/OFF` - Enable/disable LZ4 support (default: ON)
- `BUILD_BZIP2=ON/OFF` - Enable/disable BZIP2 support (default: ON)
- `ENABLE_FORTRAN=ON/OFF` - Enable/disable Fortran wrappers (default: ON)

**Autotools:**
- `--enable-lz4/--disable-lz4` - LZ4 support
- `--enable-bzip2/--disable-bzip2` - BZIP2 support
- `--enable-fortran/--disable-fortran` - Fortran wrappers

---

## 4. CDF Format Support (v1.3.0)

### 4.1 Overview
NASA Common Data Format (CDF) support via UDF handler enables transparent access to CDF files through the standard NetCDF API.

### 4.2 Features
- **File Operations**: `NC_CDF_open()` and `NC_CDF_close()` with proper resource management
- **Metadata Mapping**: CDF zVariables, attributes, and types mapped to NetCDF equivalents
- **Type System**: Complete mapping of CDF types to NetCDF types (CDF_INT4→NC_INT, CDF_REAL8→NC_DOUBLE, CDF_TIME_TT2000→NC_INT64)
- **Attribute Conventions**: FILLVAL attributes automatically renamed to _FillValue
- **Data Reading**: `NC_CDF_get_vara()` supporting scalars and multi-dimensional arrays

### 4.3 Build Configuration
**CMake:**
- `ENABLE_CDF=ON/OFF` - Enable/disable CDF support (default: OFF)

**Autotools:**
- `--enable-cdf/--disable-cdf` - CDF support

### 4.4 Dependencies
- NASA CDF Library v3.9.x (required when enabled)

### 4.5 Known Limitations
- Read-only access (NC_NOWRITE mode only)
- zVariables only (rVariables not supported)
- No write support

---

## 5. Spack Package Manager Support (v1.4.0)

### 5.1 Overview
Spack package recipes enable simplified installation and dependency management in HPC environments.

### 5.2 Features
- **NEP Spack Package**: CMake-based package with Fortran and compression variants
- **CDF Spack Package**: Standalone package for NASA CDF library v3.9.1
- **Automatic Dependencies**: Resolves NetCDF-C, HDF5, LZ4, BZIP2, NetCDF-Fortran
- **Variant Support**: `+fortran/-fortran`, `+lz4/-lz4`, `+bzip2/-bzip2`, `+docs/-docs`

### 5.3 Installation
```bash
# Install NEP with all features
spack install nep

# Install with minimal features
spack install nep~docs~fortran

# Install CDF library
spack install cdf
```

### 5.4 CI Integration
- Dedicated workflows for NEP and CDF packages
- Style validation with `spack style` and `spack audit`
- Installation testing with verbose logging

---

## 6. GeoTIFF Format Support (v1.5.0)

### 6.1 Overview
GeoTIFF geospatial raster data support via UDF handler enables transparent access to GeoTIFF files through the standard NetCDF API.

### 6.2 Features
- **File Operations**: `NC_GEOTIFF_open()` and `NC_GEOTIFF_close()` for lifecycle management
- **Format Detection**: Automatic TIFF magic number validation and GeoTIFF tag detection
- **Metadata Extraction**: Dimensions, variables, attributes from GeoTIFF structure
- **Data Access**: `NC_GEOTIFF_get_vara()` for reading raster data with type conversion
- **Endianness Support**: Handles little-endian and big-endian TIFF files
- **Security Hardening**: Validation against malformed files with bounds checking

### 6.3 Data Organization
- **Dimension Mapping**: GeoTIFF bands, rows, columns mapped to NetCDF dimensions
- **Variable Structure**: Raster data exposed as NetCDF variables
- **Attribute Preservation**: GeoTIFF tags and metadata as NetCDF attributes
- **Coordinate Systems**: Geospatial reference information preserved

### 6.4 Build Configuration
**CMake:**
- `ENABLE_GEOTIFF=ON/OFF` - Enable/disable GeoTIFF support

**Autotools:**
- `--enable-geotiff/--disable-geotiff` - GeoTIFF support

### 6.5 Dependencies
- libgeotiff (required when enabled)
- libtiff (transitive dependency)

### 6.6 Usage Example
```c
#include <netcdf.h>

int ncid, varid;
size_t width, height, bands;
float *data;

/* Open GeoTIFF file using NetCDF API */
nc_open("example.tif", NC_NOWRITE, &ncid);

/* Query dimensions */
nc_inq_dimlen(ncid, 0, &bands);
nc_inq_dimlen(ncid, 1, &height);
nc_inq_dimlen(ncid, 2, &width);

/* Read raster data */
data = malloc(width * height * sizeof(float));
nc_inq_varid(ncid, "raster", &varid);
nc_get_var_float(ncid, varid, data);

/* Close file */
nc_close(ncid);
```

### 6.7 Known Limitations
- Read-only access
- Single Image Directory (first IFD only for multi-page TIFF)
- Some exotic TIFF compression schemes may not be supported

---

## 7. Build Systems

### 7.1 CMake
- Minimum version: 3.9+
- Full feature support with configurable options
- Documentation generation with Doxygen
- Comprehensive test suite integration

### 7.2 Autotools
- Standard configure/make/make install workflow
- Feature parity with CMake
- Documentation generation support
- Test suite integration

### 7.3 Common Build Options
- `BUILD_DOCUMENTATION=ON/OFF` (CMake) / `--enable-docs/--disable-docs` (Autotools)
- Doxygen-generated API documentation
- Zero-warning builds enforced

---

## 8. Documentation (v1.2.0)

### 8.1 Features
- **Version-Driven**: All documentation derives version from `version.txt`
- **API Documentation**: Complete Doxygen coverage of C and Fortran APIs
- **Build Options**: Comprehensive documentation of configuration flags
- **GitHub Pages**: Automated deployment on releases

### 8.2 Documentation Structure
- Main page: Overview and installation
- API reference: Generated from source code
- Build options: Configuration guide
- Design documentation: Architecture details

---

## 9. Testing and Quality Assurance

### 9.1 Test Coverage
- **Compression Tests**: C and Fortran tests for LZ4 and BZIP2
- **CDF Tests**: Basic file operations and IMAP MAG L1B calibration data
- **GeoTIFF Tests**: 10 comprehensive test programs covering edge cases, errors, performance
- **CI Integration**: GitHub Actions with multiple build configurations

### 9.2 CI Matrix
- CMake and Autotools builds
- Compression combinations (all, no-bzip2, no-lz4)
- Fortran on/off configurations
- Documentation validation
- Format-specific tests (CDF, GeoTIFF)

### 9.3 Quality Gates
- Zero-warning builds
- All tests pass before merge
- Documentation builds without errors
- Style validation for Spack packages

---

## 10. Dependencies

### 10.1 Core Dependencies
- NetCDF-C v4.9+ (required)
- HDF5 v1.12+ (required)
- CMake v3.9+ or Autotools (build)

### 10.2 Optional Dependencies
- LZ4 library (for LZ4 compression)
- BZIP2 library (for BZIP2 compression)
- NetCDF-Fortran v4.5.4+ (for Fortran wrappers)
- NASA CDF Library v3.9.x (for CDF support)
- libgeotiff (for GeoTIFF support)
- libtiff (transitive, for GeoTIFF support)
- Doxygen and Graphviz (for documentation)

---

## 11. Release History

- **v0.1.3** (Nov 2025): Architecture shift from HDF5 VOL to NetCDF UDF, Doxygen documentation
- **v1.0.0**: LZ4 and BZIP2 compression filters
- **v1.1.0**: Fortran wrappers for compression functions
- **v1.2.0**: Documentation improvements and GitHub Pages deployment
- **v1.3.0**: NASA CDF format support via UDF handler
- **v1.4.0**: Spack package manager support for NEP and CDF
- **v1.5.0** (Jan 2026): GeoTIFF read support via UDF handler

---

*Document Version: 1.5.0*  
*Last Updated: January 2026*  
*Status: Reflects released features through v1.5.0*

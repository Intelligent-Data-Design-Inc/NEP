# NEP (NetCDF Extension Pack) - Product Requirements Document

## 1. Executive Summary

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
- `--enable-cdf` - Enable CDF support (default: disabled)

**Note on UDF slots:** Before v2.2.0, CDF and GRIB2 shared UDF slot 2 and were mutually exclusive. v2.2.0 moves CDF to UDF slot 4, removing the mutual-exclusivity restriction.

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
- `ENABLE_GEOTIFF=ON/OFF` - Enable/disable GeoTIFF support (default: OFF)

**Autotools:**
- `--enable-geotiff/--disable-geotiff` - GeoTIFF support (default: disabled)

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

## 7. GRIB2 Format Support (v1.7.0)

### 7.1 Overview
GRIB2 (General Regularly-distributed Information in Binary form, Edition 2) support via UDF handler enables transparent access to GRIB2 meteorological and oceanographic data files through the standard NetCDF API, using the NOAA NCEPLIBS-g2c library (`g2c_*` API).

### 7.2 Features
- **File Operations**: `NC_GRIB2_open()` and `NC_GRIB2_close()` with proper resource management
- **Format Detection**: Automatic GRIB2 magic number detection (`GRIB`) via UDF slot 2
- **Product Mapping**: Each GRIB2 product (one per message for single-product messages) exposed as a named `NC_FLOAT` NetCDF variable with shared `[y, x]` dimensions
- **Metadata Extraction**: Variable names from `g2c_param_abbrev()`; duplicate names uniquified with `_2`, `_3`, ... suffixes
- **Variable Attributes**: `long_name` (from parameter abbreviation), `_FillValue = 9.999e20f`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`
- **Global Attributes**: `Conventions = "GRIB2"`, `GRIB2_edition = 2`
- **Data Reading**: `NC_GRIB2_get_vara()` with `start`/`count` slicing; bitmap-masked (land) points filled with `_FillValue`
- **UDF Self-Loading**: `NC_GRIB2_initialize()` supports both manual registration and `.ncrc` autoload

### 7.3 Build Configuration
**CMake:**
- `ENABLE_GRIB2=ON/OFF` - Enable/disable GRIB2 support (default: OFF)

**Autotools:**
- `--enable-grib2/--disable-grib2` - GRIB2 support (default: disabled)

**Note on UDF slots:** GRIB2 uses UDF slot 2. Before v2.2.0, CDF shared this slot and the two formats were mutually exclusive. v2.2.0 moves CDF to UDF slot 4, allowing both formats to be enabled together.

### 7.4 Dependencies
- **NOAA NCEPLIBS-g2c** >= 2.1.0 (required when enabled); user supplies install path at configure time
- **libjasper** >= 3.0.0 (transitive dependency of g2c for JPEG2000 compression)
- g2c repo: https://github.com/NOAA-EMC/NCEPLIBS-g2c/
- g2c docs: https://noaa-emc.github.io/NCEPLIBS-g2c/

### 7.5 Known Limitations
- Read-only access (`NC_NOWRITE` mode only)
- Single shared grid assumed: all products in a file must share the same `nx`/`ny` (taken from the first product)
- 2D grids only (`[y, x]`); no time, level, or ensemble dimensions
- No coordinate variables: `x`/`y` dimensions have no associated lat/lon arrays
- `units` attribute not available: NCEPLIBS-g2c has no API to retrieve units strings
- One NetCDF variable per GRIB2 product; multi-product-per-message files use `prod_index` 0 only in Sprint 4

---

## 8. Example Programs (v3.5.1)

### 8.1 Overview
Comprehensive example programs in C and Fortran demonstrating NetCDF API usage for learning and reference purposes.

### 8.2 Features
- **Dual Language Support**: All examples provided in both C and Fortran
- **Read and Write Operations**: Each example creates a NetCDF file and reads it back to demonstrate both operations
- **Test Integration**: Examples run as automated tests to ensure correctness
- **Multiple Categories**: Classic NetCDF and NetCDF-4 examples covering basic to advanced features
- **Educational Value**: Demonstrates best practices and common usage patterns

### 8.3 Example Categories

#### Classic NetCDF Examples (C)
Located in `examples/classic/`:
- `simple_2D.c` - Basic 2D array creation and writing
- `coord_vars.c` - Working with coordinate variables
- `format_variants.c` - Different NetCDF format variants (classic, 64-bit offset, CDF-5)
- `size_limits.c` - Demonstrating size and dimension limits
- `unlimited_dim.c` - Using unlimited dimensions for time series data
- `var4d.c` - Creating and writing 4-dimensional variables

#### NetCDF-4 Examples (C)
Located in `examples/netcdf-4/`:
- `simple_nc4.c` - Basic NetCDF-4 file creation
- `compression.c` - Using compression filters (deflate, shuffle)
- `chunking_performance.c` - Chunking strategies and performance
- `multi_unlimited.c` - Multiple unlimited dimensions (NetCDF-4 feature)
- `user_types.c` - User-defined compound and enum types

#### Classic NetCDF Examples (Fortran)
Located in `examples/f_classic/`:
- `f_simple_2D.f90` - Basic 2D array in Fortran
- `f_coord_vars.f90` - Coordinate variables in Fortran
- `f_format_variants.f90` - Format variants in Fortran
- `f_size_limits.f90` - Size limits in Fortran
- `f_unlimited_dim.f90` - Unlimited dimensions in Fortran
- `f_var4d.f90` - 4D variables in Fortran

#### NetCDF-4 Examples (Fortran)
Located in `examples/f_netcdf-4/`:
- `f_simple_nc4.f90` - Basic NetCDF-4 in Fortran
- `f_compression.f90` - Compression in Fortran
- `f_chunking_performance.f90` - Chunking in Fortran
- `f_multi_unlimited.f90` - Multiple unlimited dimensions in Fortran
- `f_user_types.f90` - User-defined types in Fortran

#### Performance Examples (C)
Located in `examples/performance/`:
- `cache_tuning.c` - Chunk cache configuration: default vs enlarged cache, file-level vs variable-level settings, cache thrashing detection
- `chunking.c` - Chunk shape selection: time-optimized vs column-optimized chunks across time-slab and column-profile access patterns; outputs CSV (`chunk_shape,access_pattern,elapsed_s,MB_per_s`) suitable for plotting
- `deflate.c` - Deflate compression: zlib levels 0–9 with and without the shuffle filter across all 20 combinations; outputs CSV (`deflate_level,shuffle,compressed_bytes,ratio,write_s,read_s`) with compressed file size, compression ratio, write time, and read time
- `fill_values.c` - Fill value handling performance: demonstrates fill mode NC_FILL vs NC_NOFILL across classic and NetCDF-4 formats; outputs CSV (`format,fill_mode,write_s,read_s,file_bytes`) with write time, read time, and file size
- `endianness.c` - Endianness handling performance: demonstrates byte order (NC_ENDIAN_NATIVE, NC_ENDIAN_LITTLE, NC_ENDIAN_BIG) effects on write/read performance in NetCDF-4/HDF5 files; outputs CSV (`endian_mode,write_s,read_s,file_bytes`) with write time, read time, and file size

**Note**: Performance examples are excluded from regular CI. They are built and run only when `ENABLE_BENCHMARKS=ON` (CMake) or `--enable-benchmarks` (Autotools) is specified. All four examples operate on a 500×180×360 (time×lat×lon) NC_FLOAT temperature dataset matching a meteorological grid.

#### NcZarr Examples (v1.11.0)
Located in `examples/nczarr/`:

**C examples** (Sprint 1):
- `nczarr_simple.c` - Create, write, and read a local NcZarr dataset; demonstrates `file://simple_nczarr.zarr#mode=nczarr` URL, dimensions, variables, attributes, and data verification

**Fortran examples** (Sprint 2):
- `f_nczarr_simple.f90` - Fortran equivalent of `nczarr_simple.c`; produces the same 4×5 `temperature(y, x)` dataset at `file://f_simple_nczarr.zarr#mode=nczarr`; demonstrates Fortran column-major dimension ordering with NcZarr

**C examples** (Sprint 3):
- `nczarr_chunking.c` - Explicit chunk shape selection (2×5) for NcZarr; demonstrates `nc_def_var_chunking()` and `nc_inq_var_chunking()` with metadata verification
- `nczarr_compression.c` - Deflate level 4 + shuffle compression on a chunked NcZarr variable; demonstrates the recommended workflow of setting chunk shape before applying `nc_def_var_deflate()`

**Fortran examples** (Sprint 3):
- `f_nczarr_chunking.f90` - Fortran equivalent of `nczarr_chunking.c`; demonstrates `nf90_def_var_chunking()` with Fortran column-major dimension ordering
- `f_nczarr_compression.f90` - Fortran equivalent of `nczarr_compression.c`; demonstrates `nf90_def_var_deflate()` with explicit chunking

**C example** (Sprint 4):
- `nczarr_enhanced.c` — Enhanced data model in NcZarr: root group with `time` (unlimited) × `x=5` dimensions, `temperature(time,x)` and `pressure(time,x)` variables; child group `obs/` with an independent `station` (unlimited) dimension and `obs_value(station)`, `obs_time(station)` variables. Demonstrates `nc_def_grp()`, per-group unlimited dimensions, and `nc_inq_grp_ncid()`. Note: NcZarr supports groups and unlimited dims from the enhanced model; user-defined types (enum, compound) require HDF5 storage.

**Fortran example** (Sprint 4):
- `f_nczarr_enhanced.f90` — Fortran equivalent of `nczarr_enhanced.c`; demonstrates `nf90_def_grp()`, `nf90_def_dim()` with `NF90_UNLIMITED` in a child group, and `nf90_inq_grp_ncid()` with full C/Fortran parity.

**Note**: NcZarr examples are only built when NetCDF-C reports NcZarr support (`NC_HAS_NCZARR` in `netcdf_meta.h`) and examples are enabled. The Fortran examples additionally require `--enable-fortran` (Autotools) or `ENABLE_FORTRAN=ON` (CMake). Each example writes to a distinct Zarr directory store to allow safe parallel test execution.

### 8.4 Build Configuration
**CMake:**
- `BUILD_EXAMPLES=ON/OFF` - Enable/disable example programs (default: ON)
- `ENABLE_BENCHMARKS=ON/OFF` - Enable/disable performance benchmark programs (default: OFF)

**Autotools:**
- `--enable-examples/--disable-examples` - Example programs (default: enabled)
- `--enable-benchmarks` - Enable performance benchmark programs (default: disabled)

### 8.5 Dependencies
- NetCDF-C library (required for C examples)
- NetCDF-Fortran library (required for Fortran examples)
- C99 compiler
- Fortran 90+ compiler (for Fortran examples)

### 8.6 Usage
Examples are automatically built and run as tests:

```bash
# CMake
cmake -B build
cmake --build build
ctest --test-dir build

# Autotools
./configure
make
make check
```

### 8.7 Output Validation
Example output is validated to ensure correctness across code changes. Each example creates NetCDF files demonstrating the features being illustrated.

---

## 9. Parallel I/O Support (v1.9.0)

### 9.1 Overview
Parallel I/O support enables NEP to build and run test programs with MPI for high-performance computing environments. This provides the foundation for parallel NetCDF-4/HDF5 I/O operations.

### 9.2 Build Configuration

**CMake:**
- `ENABLE_PARALLEL_TESTS=ON/OFF` - Enable parallel I/O test programs (default: OFF)
- `MPIEXEC_EXECUTABLE=PATH` - Specify path to mpiexec/mpirun

**Autotools:**
- `--enable-parallel-tests` - Enable parallel I/O test programs
- `--with-mpiexec=PATH` - Specify path to mpiexec/mpirun

### 9.3 Dependencies
- MPI implementation (OpenMPI or MPICH)
- NetCDF-C built with parallel support (`NC_HAS_PARALLEL4`)
- NetCDF-Fortran (for Fortran parallel tests)

### 9.4 Configure Output
The configure step displays parallel I/O status:
```
Parallel I/O:
  Parallel tests enabled: yes
  Parallel NetCDF-C: yes (NC_HAS_PARALLEL4)
  mpiexec: /usr/bin/mpiexec
```

### 9.5 Example Programs

NEP includes parallel I/O example programs demonstrating collective NetCDF-4/HDF5 I/O in `examples/parallelIO/`:

**C: `square16_par.c`**
- Creates a 16×16 integer dataset using 4 MPI ranks
- Each rank writes an 8×8 quadrant filled with its rank number (0-3)
- Uses `nc_create_par()`, `nc_var_par_access(NC_COLLECTIVE)`, and `nc_put_vara_int()`
- Includes parallel read-back verification with `nc_open_par()` and `nc_get_vara_int()`
- Requires exactly 4 MPI processes

**Fortran: `f_square16_par.f90`**
- Identical functionality to C version
- Uses `nf90_create_par()`, `nf90_var_par_access(NF90_COLLECTIVE)`, and `nf90_put_var()`
- Demonstrates Fortran 90 NetCDF parallel I/O patterns

**Data Decomposition:**
```
Global 16x16 grid:
+--------+--------+
| Rank 0 | Rank 1 |
| (0,0)  | (8,0)  |
+--------+--------+
| Rank 2 | Rank 3 |
| (0,8)  | (8,8)  |
+--------+--------+
```
Each rank fills its quadrant with its rank number.

**Running:**
```bash
mpiexec -n 4 ./square16_par
ncdump square16_par.nc
```

### 9.6 CI Integration
Parallel I/O builds are tested in a separate CI workflow (`ci-parallel.yml`) with:
- Matrix: CMake/Autotools × OpenMPI/MPICH
- All dependencies (HDF5, NetCDF-C, NetCDF-Fortran) built with MPI compilers
- Parallel test directory structure for parallel I/O examples
- ncdump verification of parallel output files

---

## 10. Build Systems

### 10.1 CMake
- Minimum version: 3.9+
- Full feature support with configurable options
- Documentation generation with Doxygen
- Comprehensive test suite integration

### 10.2 Autotools
- Standard configure/make/make install workflow
- Feature parity with CMake
- Documentation generation support
- Test suite integration

### 10.3 Common Build Options
- `BUILD_DOCUMENTATION=ON/OFF` (CMake) / `--enable-docs/--disable-docs` (Autotools)
- Doxygen-generated API documentation
- Zero-warning builds enforced

---

## 11. Documentation (v1.2.0)

### 11.1 Features
- **Version-Driven**: All documentation derives version from `version.txt`
- **API Documentation**: Complete Doxygen coverage of C and Fortran APIs
- **Build Options**: Comprehensive documentation of configuration flags
- **GitHub Pages**: Automated deployment on releases

### 11.2 Documentation Structure
- Main page: Overview and installation
- API reference: Generated from source code
- Build options: Configuration guide
- Design documentation: Architecture details

---

## 12. Testing and Quality Assurance

### 12.1 Test Coverage
- **Compression Tests**: C and Fortran tests for LZ4 and BZIP2
- **CDF Tests**: Basic file operations and IMAP MAG L1B calibration data
- **GeoTIFF Tests**: 10 comprehensive test programs covering edge cases, errors, performance
- **Example Programs**: All examples run as tests to validate correctness
- **CI Integration**: GitHub Actions with multiple build configurations

### 12.2 CI Matrix
- CMake and Autotools builds
- Compression combinations (all, no-bzip2, no-lz4)
- Fortran on/off configurations
- Documentation validation
- Format-specific tests (CDF, GeoTIFF, GRIB2)

### 12.3 Quality Gates
- Zero-warning builds
- All tests pass before merge
- Documentation builds without errors
- Style validation for Spack packages

---

## 13. Dependencies

### 13.1 Core Dependencies
- NetCDF-C v4.9+ (required)
- HDF5 v1.12+ (required)
- CMake v3.9+ or Autotools (build)

### 13.2 Optional Dependencies
- LZ4 library (for LZ4 compression)
- BZIP2 library (for BZIP2 compression)
- NetCDF-Fortran v4.6.2+ (for Fortran wrappers; zstd API requires 4.6.0+)
- NASA CDF Library v3.9.x (for CDF support)
- libgeotiff (for GeoTIFF support)
- libtiff (transitive, for GeoTIFF support)
- NOAA NCEPLIBS-g2c (for GRIB2 support)
- CFITSIO >= 3.0 (for FITS support)
- libxml2 (for PDS4 support, v2.2.0)
- Doxygen and Graphviz (for documentation)

---

## 14. PDS4 Format Support (v2.2.0)

### 14.1 Overview

PDS4 (Planetary Data System version 4) support via UDF handler enables transparent
access to NASA/ESA Planetary Data System 4 label files through the standard NetCDF
API. PDS4 is used to archive and distribute planetary science data from missions
such as Mars Reconnaissance Orbiter, Curiosity, Perseverance, Cassini, and many
others.

v2.2.0 Sprint 3 delivers the dispatch skeleton: UDF5 slot registration, libxml2
label parsing and namespace validation, and an open/close round-trip. Metadata and
data reading will be added in subsequent releases.

### 14.2 Features

- **Format Detection**: Automatic `<?xml` magic-number detection; root namespace
  verified against `http://pds.nasa.gov/pds4/pds/v1`
- **UDF Registration**: PDS4 assigned to UDF slot 5 (`NC_UDF5`), permanently
  separate from all other NEP format slots
- **Open/Close Round-trip**: `nc_open()` / `nc_close()` succeed on any valid
  PDS4 XML label; returns `NC_ENOTNC` for XML files that are not PDS4 labels
- **libxml2 Integration**: XML parsing uses libxml2 with `XML_PARSE_NONET` to
  avoid network access during format detection

### 14.3 Build Configuration

CMake:
- `ENABLE_PDS4=ON/OFF` — Enable/disable PDS4 support (default: OFF)
- Requires `find_package(LibXml2 REQUIRED)` when enabled

Autotools:
- `--enable-pds4/--disable-pds4` — PDS4 support (default: disabled)
- Uses `AC_CHECK_HEADERS([libxml/parser.h])` + `AC_CHECK_LIB([xml2], [xmlReadFile])`

### 14.4 Dependencies

- libxml2 (`libxml2-dev` on Ubuntu / Debian; `libxml2-devel` on RHEL/Fedora)

### 14.5 Known Limitations (v2.2.0)

- No metadata (`nc_inq_*`) or data (`nc_get_vara`) reading yet; these return
  `NC_NOERR` / `NC_EINVAL` respectively via no-op dispatch functions
- Only `Product_Observational` root element type is validated; other PDS4 product
  classes are not checked in this sprint

---

## 15. Performance Examples (v1.10.0)

Performance benchmark programs gated by `ENABLE_BENCHMARKS` (CMake) or
`--enable-benchmarks` (Autotools). Not run in regular CI. Dataset for all
examples: 500×180×360 NC_FLOAT temperature (~129 MB uncompressed), chunk
shape 10×45×90.

- `cache_tuning.c` — HDF5 chunk cache configuration: demonstrates
  `nc_set_chunk_cache()` and `nc_set_var_chunk_cache()` with measurable
  performance differences; outputs CSV (`cache_config,access_pattern,elapsed_s,MB_per_s`)
- `chunking.c` — Chunk shape performance: demonstrates how chunk shape selection
  affects I/O performance across time-slab and column-profile access patterns;
  outputs CSV (`chunk_shape,access_pattern,elapsed_s,MB_per_s`)
- `deflate.c` — Deflate compression performance: zlib levels 0–9 with and without
  the shuffle filter (20 combinations); outputs CSV
  (`deflate_level,shuffle,compressed_bytes,ratio,write_s,read_s`)
- `fill_values.c` — Fill value performance: demonstrates fill mode ON vs OFF
  overhead across classic and NetCDF-4 formats (4 combinations); outputs CSV
  (`format,fill_mode,write_s,read_s,file_bytes`)
- `endianness.c` — Endianness handling performance: demonstrates byte order
  (`NC_ENDIAN_NATIVE`, `NC_ENDIAN_LITTLE`, `NC_ENDIAN_BIG`) effects on write/read
  performance in NetCDF-4/HDF5 files; outputs CSV
  (`endian_mode,write_s,read_s,file_bytes`)
- `zstandard.c` — Zstandard compression performance: demonstrates zstd levels
  (-7 to 22, representative subset of 12) with and without the shuffle filter
  (24 combinations); outputs CSV
  (`zstd_level,shuffle,compressed_bytes,ratio,write_s,read_s`)
- `szip.c` — SZIP compression performance: demonstrates NC_SZIP_NN and
  NC_SZIP_EC coding methods across pixels_per_block {2, 4, 8, 16, 32} (10
  combinations); outputs CSV
  (`coding,pixels_per_block,compressed_bytes,ratio,write_s,read_s`)
- `lz4.c` — LZ4 compression performance: demonstrates LZ4 levels 1–9 with
  and without the shuffle filter (18 combinations); outputs CSV
  (`lz4_level,shuffle,compressed_bytes,ratio,write_s,read_s`)
- `bzip2.c` — BZIP2 compression performance: demonstrates BZIP2 levels 1–9 with
  and without the shuffle filter (18 combinations); outputs CSV
  (`bzip2_level,shuffle,compressed_bytes,ratio,write_s,read_s`)
- `lossless.c` — Lossless compression comparison: compares the best-performing
  settings of all available filters (DEFLATE, Zstandard, SZIP, LZ4, BZIP2) with
  shuffle enabled; outputs CSV
  (`filter,level_or_pixels,compressed_bytes,ratio,write_s,read_s`)
- `quantize.c` — Quantization + compression performance: demonstrates all three
  quantization algorithms (BitGroom, GranularBitRound, BitRound) at each NSD/NSB
  level paired with all five lossless filters at their optimal settings; includes
  baseline lossless rows and max absolute error per combination; outputs CSV
  (`quantize_alg,nsd_or_nsb,filter,compressed_bytes,ratio,write_s,read_s,max_abs_err`)

---

## 16. Release History

- **v0.1.3** (Nov 2025): Architecture shift from HDF5 VOL to NetCDF UDF, Doxygen documentation
- **v1.0.0**: LZ4 and BZIP2 compression filters
- **v1.1.0**: Fortran wrappers for compression functions
- **v1.2.0**: Documentation improvements and GitHub Pages deployment
- **v1.3.0**: NASA CDF format support via UDF handler
- **v1.4.0**: Spack package manager support for NEP and CDF
- **v1.5.0** (Jan 2026): GeoTIFF read support via UDF handler
- **v1.7.0** (March 2026): GRIB2 read support via UDF handler
- **v1.9.0** (May 2026): Parallel I/O build system support and parallel test examples

---

*Document Version: 1.7.0*  
*Last Updated: March 2026*  
*Status: Reflects released features through v1.7.0*

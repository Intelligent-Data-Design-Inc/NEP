# NEP – NetCDF Extension Pack v2.7.1
## Project Overview

The NetCDF Extension Pack (NEP) v2.7.1 extends NetCDF-4 with high-performance compression filters and User Defined Format (UDF) handlers for accessing diverse scientific data formats through the standard NetCDF API. NEP provides flexible lossless compression with two complementary algorithms (LZ4 and BZIP2) and transparent access to GeoTIFF, GRIB2, FITS, NASA CDF, and NASA/ESA PDS4 files.

## Architecture

NEP consists of two primary architectural components:

### 1. HDF5 Filter Plugin Architecture

NEP uses the HDF5 filter plugin architecture to provide transparent compression for NetCDF-4 files:

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

### 2. NetCDF User Defined Format (UDF) System

NEP implements NetCDF's UDF system to provide transparent access to various scientific data formats:

1. **Core NetCDF API**: Standard API used by applications
2. **NC_Dispatch Layer**: Format-specific function pointer tables
3. **Format Handlers**: CDF, GeoTIFF, GRIB2, FITS, and PDS4 UDF handlers
4. **Format Libraries**: Integration with libgeotiff, NASA CDF library, NCEPLIBS-g2c, CFITSIO, and libxml2
5. **Multi-format Storage**: Access to NetCDF-4, CDF, GeoTIFF, GRIB2, FITS, and PDS4 files

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────┬─────────────┬───────────────┬────────────┬─────────────┐
│             │             │               │            │             │
[HDF5 Backend] [CDF Handler] [GeoTIFF Handler] [GRIB2 Handler] [FITS Handler] [PDS4 Handler]
│             │             │               │            │             │
│          [CDF Library] [libgeotiff]  [NCEPLIBS-g2c] [CFITSIO]   [libxml2]
│             │             │               │            │             │
└──────┬──────┴──────┬──────┴───────┬───────┴─────┬──────┴──────┬──────┘
       │             │              │             │             │
[NetCDF-4 Files] [CDF Files] [GeoTIFF Files] [GRIB2 Files] [FITS Files] [PDS4 Files]
```

#### UDF Slot Allocation

NetCDF-C exposes ten UDF slots (0–9). NEP assigns each format handler a permanent slot:

| Format | UDF Slot | NetCDF-C Constant | Notes |
|--------|----------|-------------------|-------|
| GeoTIFF BigTIFF | 0 | `NC_UDF0` | Large TIFF files (>4 GB) |
| GeoTIFF standard TIFF | 1 | `NC_UDF1` | Regular TIFF/GeoTIFF files |
| GRIB2 | 2 | `NC_UDF2` | NOAA/ECMWF GRIB2 messages |
| FITS | 3 | `NC_UDF3` | Astronomical images and tables (v2.0.0) |
| CDF | 4 | `NC_UDF4` | NASA CDF; moved from UDF2 in v2.2.0 Sprint 2 |
| PDS4 | 5 | `NC_UDF5` | NASA/ESA planetary data labels (v2.2.0 Sprint 3) |

Before v2.2.0, CDF and GRIB2 shared UDF slot 2 and were mutually exclusive. v2.2.0 Sprint 2 removes that restriction by moving CDF to its own slot.

## Project Structure

The project is structured as follows:
- `/src` - Core C source code including UDF handlers and compression filters
- `/fsrc` - Fortran wrappers for compression functions
- `/include` - Public header files
- `/test` - C unit tests for all features
- `/ftest` - Fortran unit tests
- `/test/data` - Test data files for CDF and GeoTIFF formats
- `/docs` - Project documentation
- `/docs/releases` - Release notes for each version
- `/.github/workflows` - CI/CD pipeline configurations
- `/spack` - Spack package manager recipes

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
| Core | NetCDF-C | v4.10.1+ | Unidata | pkg-config, FindNetCDF.cmake |
| Core | HDF5 | v2.1.1+ | HDF Group | pkg-config, FindHDF5.cmake |
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

## GeoTIFF Read Layer (v1.6.0)

### Overview

NEP v1.5.0 introduced support for GeoTIFF geospatial raster data format through a User Defined Format (UDF) handler. NEP v1.6.0 extends this with full CF-1.8 compliant CRS metadata including grid-mapping variables, coordinate variables, and coordinate bounds. This enables transparent access to GeoTIFF files through the standard NetCDF API, eliminating the need for format conversion in geospatial workflows.

For details on NetCDF's User Defined Format mechanism, see `docs/udf.md`.

### Architecture

The GeoTIFF read layer follows the NC_Dispatch pattern used for other UDF handlers in NEP:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [GeoTIFF UDF Handler]
│                 │
│            [libgeotiff]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [GeoTIFF Files]
```

### Key Components

#### GeoTIFF UDF Handler
- **Format Detection**: Automatic identification of GeoTIFF files via magic number and TIFF header inspection
- **NC_Dispatch Implementation**: Complete dispatch table for GeoTIFF file operations
- **File Operations**: Open, close, and query operations for GeoTIFF files
- **Metadata Extraction**: Dimensions, data types, georeferencing information, and GeoTIFF tags
- **Data Access**: Raster band reading with support for various data types

#### Raster Band Mapping
- **Single-band GeoTIFF**: Maps to 2D NetCDF variable (e.g., `data[y, x]`)
- **Multi-band GeoTIFF**: Maps to 3D NetCDF variable with band dimension (e.g., `data[band, y, x]`)
- **Variable Naming**: Configurable naming scheme for bands (default: `band1`, `band2`, etc.)

#### Georeferencing Metadata
Coordinate reference system (CRS) and georeferencing information stored following CF-1.8 conventions:

- **`crs` Grid-Mapping Variable**: A scalar variable holding all CRS parameters as attributes (`grid_mapping_name`, `semi_major_axis`, `inverse_flattening`, and projection-specific parameters). The data variable carries a `grid_mapping = "crs"` attribute pointing to it.
- **Coordinate Variables**: 1D coordinate variables are created from the GeoTIFF GeoTransform tags:
  - Geographic CRS: `lon[x]` (degrees_east) and `lat[y]` (degrees_north)
  - Projected CRS: `x[x]` (m) and `y[y]` (m)
  - The data variable carries a `coordinates = "lon lat"` (or `"x y"`) attribute.
- **Coordinate Bounds**: For pixel-as-area rasters (the default), bounds variables (`lon_bnds`, `lat_bnds` or `x_bnds`, `y_bnds`) are created and attached via `bounds` attributes on the coordinate variables.
- **Pixel Raster Type**: The GeoTIFF `GTRasterTypeGeoKey` is read to distinguish pixel-as-point (coordinate at pixel corner) from pixel-as-area (coordinate at pixel centre) rasters.
- **Graceful Degradation**: If GeoTIFF CRS tags are absent or malformed, `nc_open` succeeds and the raster `data` variable remains readable; no `crs` variable is created.
- **CF Compliance**: See `docs/cf-compliance.md` for the full attribute specification.

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| GeoTIFF Handler | libgeotiff | Latest stable | GeoTIFF file operations and metadata |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

**Note**: PROJ library support is out of scope for v1.6.0. Coordinate transformations will be addressed in future releases if needed.

### Build System Integration

#### GeoTIFF Support
GeoTIFF support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -DENABLE_GEOTIFF=ON   # Enable GeoTIFF support
cmake -DENABLE_GEOTIFF=OFF  # Disable GeoTIFF support (default)
```

**Autotools:**
```bash
./configure --enable-geotiff   # Enable GeoTIFF support
./configure --disable-geotiff  # Disable GeoTIFF support (default)
```

#### Dependency Detection
- Automatic libgeotiff detection during configuration
- Clear error messages if libgeotiff not found when enabled
- Graceful degradation when disabled

### Implementation

The GeoTIFF UDF handler is fully implemented in `src/geotifffile.c` and `src/geotiffdispatch.c` with the following features:

1. **File Operations**
   - `NC_GEOTIFF_open()` and `NC_GEOTIFF_close()` for lifecycle management
   - Automatic TIFF magic number validation and GeoTIFF tag detection
   - Resource cleanup and error handling

2. **Metadata Handling**
   - Dimension extraction (bands, rows, columns)
   - Variable creation with appropriate data types
   - CRS extraction via `extract_crs_parameters()` into `NC_GEOTIFF_CRS_INFO_T`
   - CRS validation via `validate_crs_completeness()`
   - CF-1.8 `crs` grid-mapping variable creation via `create_cf_crs_variable()`
   - Coordinate variable and bounds creation from GeoTransform tags

3. **Data Access**
   - `NC_GEOTIFF_get_vara()` for reading raster data
   - Type conversion between GeoTIFF and NetCDF data types
   - Support for multi-band imagery
   - Endianness handling for cross-platform compatibility

4. **Security Features**
   - Bounds checking and validation
   - Protection against malformed files
   - Memory management with proper cleanup

### Integration Points

- **Format Detection**: Integrated into NetCDF's file open logic
- **Dispatch Table**: Registered alongside existing format handlers
- **Error Handling**: Standard NetCDF error codes and messages
- **Testing**: Unit tests for each phase, integration tests with sample GeoTIFF files

### Test Data

The project includes sample GeoTIFF files:

**NASA MODIS NRT Global Flood Products (geographic CRS, Clarke 1866 ellipsoid)**
- `test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif`
- `test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif`

MODIS Daily L3 Global Flood Composite files at 250m resolution. `grid_mapping_name = "latitude_longitude"`, `semi_major_axis = 6378206.4`, `inverse_flattening = 294.978698`.

**ABBA Sinusoidal Projection (projected CRS, spherical Earth)**
- `test/data/ABBA_2022_C61_HNL.tif`

ABBA fire product in sinusoidal projection. `grid_mapping_name = "sinusoidal"`, `semi_major_axis = 6371007.181`, coordinate variables `x` and `y` in metres.

**Data Source**: NASA LANCE MODIS NRT Global Flood Product (MCDWD_L3_F1C_NRT)
- **Product**: MODIS Daily L3 Global Flood Composite 250m Linear Lat Lon Grid - NRT
- **Resolution**: ~250m
- **Format**: GeoTIFF with embedded georeferencing
- **Documentation**: https://www.earthdata.nasa.gov/data/instruments/viirs/near-real-time-data/nrt-global-flood-products

### Reference Documentation

- `docs/cf-compliance.md` - CF-1.8 grid mapping attribute specification for NEP GeoTIFF
- `docs/ESDS-RFC-040v1.1.pdf` - GeoTIFF format specification
- `docs/udf.md` - NetCDF User Defined Format documentation
- https://cfconventions.org/Data/cf-conventions/cf-conventions-1.8/cf-conventions.html#appendix-grid-mappings - CF-1.8 grid mapping appendix
- https://www.earthdata.nasa.gov/data/instruments/viirs/near-real-time-data/nrt-global-flood-products - VIIRS/MODIS NRT Global Flood Products (test data source)

## GRIB2 Format Support (v1.7.0)

### Overview

NEP v1.7.0 adds support for GRIB2 (General Regularly-distributed Information in Binary form, Edition 2) meteorological and oceanographic data files through a User Defined Format (UDF) handler, using the NOAA NCEPLIBS-g2c library. This enables transparent access to GRIB2 files through the standard NetCDF API.

### Architecture

The GRIB2 UDF handler follows the same NC_Dispatch pattern used for CDF and GeoTIFF handlers:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [GRIB2 UDF Handler]
│                 │
│             [NCEPLIBS-g2c]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [GRIB2 Files]
```

### Key Components

#### GRIB2 UDF Handler
- **Format Detection**: Automatic identification of GRIB2 files via magic number (`GRIB`) registered at UDF slot 2 (`NEP_UDF_GRIB2 = NC_UDF2`)
- **NC_Dispatch Implementation**: Complete dispatch table for GRIB2 file operations
- **File Operations**: `NC_GRIB2_open()` and `NC_GRIB2_close()` with proper resource management
- **Metadata Mapping**: GRIB2 products mapped to NetCDF variables with shared `y`/`x` dimensions, `NC_FLOAT` type, and per-variable + global attributes
- **Data Access**: `NC_GRIB2_get_vara()` expands full grid via `g2_getfld(expand=1)` with bitmap-to-`_FillValue` substitution and `start`/`count` slicing

#### Product-to-Variable Mapping
- Each GRIB2 product (one per message for single-product files) → one `NC_FLOAT` NetCDF variable with shared `[y, x]` dimensions
- Variable name from `g2c_param_abbrev()`; duplicate names uniquified with `_2`, `_3`, ... suffixes
- Shared `y` (ny) and `x` (nx) dimensions created once; all variables reference the same dim IDs
- Grid size taken from the first product via `g2c_inq_dim_info()`

#### Metadata Model
- **Variable attributes**: `long_name` (from abbreviation), `_FillValue = 9.999e20f`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`
- **Global attributes**: `Conventions = "GRIB2"`, `GRIB2_edition = 2`
- **Dispatch pointers**: `inq`, `inq_dimid`, `inq_dim`, `inq_var`, `inq_att`, `get_att`, `get_vara` — wired at Sprint 1; metadata populated at Sprint 3
- Note: `units` attribute is not provided — NCEPLIBS-g2c has no API to retrieve parameter units

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_GRIB2_FILE_INFO_T` | `include/grib2dispatch.h` | Per-file state: `g2cid`, `num_messages`, `num_products`, `num_x`/`num_y`, `*products` array |
| `NC_GRIB2_PROD_INFO_T` | `include/grib2dispatch.h` | Per-product inventory: `msg_index`, `prod_index`, `discipline`, `category`, `param_number`, `nx`, `ny`, `bytes_to_msg`, `bytes_in_msg`, `abbrev[64]` |
| `NC_VAR_GRIB2_INFO_T` | `include/grib2dispatch.h` | Per-variable: `msg_index`, `prod_index`, `bytes_to_msg`, `bytes_in_msg` (used by `get_vara` for direct file I/O) |

#### Memory Ownership
- `NC_GRIB2_FILE_INFO_T.products` array: allocated in `NC_GRIB2_open()`, freed in `NC_GRIB2_close()`
- `NC_VAR_GRIB2_INFO_T` per-variable structs: `var->format_var_info`, freed by netcdf-c group teardown
- Attribute `data` pointers (`NC_ATT_INFO_T.data`): `malloc`'d in `grib2_add_*_att()` helpers, freed by netcdf-c att teardown
- `g2_getfld()` output (`gribfield *`): freed with `g2_free()` within `NC_GRIB2_get_vara()`

#### `NC_GRIB2_open()` Pipeline
1. Magic detection: netcdf-c checks `GRIB` prefix before calling `NC_GRIB2_open()`
2. `g2c_open()` → `grib2_file->g2cid`; `strdup(path)` → `grib2_file->path`
3. Two-pass product inventory: `g2c_inq()` → `g2c_inq_msg()` → `g2c_inq_prod()` → populate `NC_GRIB2_PROD_INFO_T[]`
4. Per-message: `g2c_seekmsg()` captures `bytes_to_msg` / `bytes_in_msg` (correct 8-byte length via `hton64`); stored in `PROD_INFO_T` and `NC_VAR_GRIB2_INFO_T`
5. `nc4_dim_list_add()` for shared `y` and `x` dimensions
6. Per-product: `grib2_var_list_add()` → set dim IDs → add `GRIB2_*`, `long_name`, `_FillValue` attributes
7. Global: `Conventions`, `GRIB2_edition` attributes on root group

#### `NC_GRIB2_get_vara()` Data Path
1. `nc4_find_grp_h5_var()` → `NC_VAR_GRIB2_INFO_T` (`bytes_to_msg`, `bytes_in_msg`, `prod_index`)
2. `malloc(bytes_in_msg)` → `msgbuf`
3. `fopen(grib2_file->path, "rb")` + `fseeko(bytes_to_msg)` + `fread(bytes_in_msg bytes)` → raw GRIB2 message in `msgbuf`
4. `g2_getfld(msgbuf, prod_index+1, unpack=1, expand=1, &gfld)` → `gfld->fld[ngrdpts]` full grid, `gfld->bmap[i]` bitmap
5. Bitmap loop: `ibmap==0 && bmap[i]==0` (land/missing) → `full_buf[i] = 9.999e20f`; else `full_buf[i] = gfld->fld[i]`
6. Row-major `start`/`count` copy into caller buffer; `nc4_convert_type()` if `memtype != NC_FLOAT`
7. `g2_free(gfld)`, `free(full_buf)`, `free(msgbuf)`

#### UDF Slot and `.ncrc` Registration
- UDF slot 2 (`NC_UDF2`) used for GRIB2 in `include/nep.h`
- Starting with v2.2.0, CDF uses UDF slot 4 (`NC_UDF4`) and is no longer mutually exclusive with GRIB2
- `NC_GRIB2_initialize()` registers dispatch table via `.ncrc` autoload
- `nep.ncrc` UDF2 block: `NETCDF.UDF2.LIBRARY`, `NETCDF.UDF2.INIT=NC_GRIB2_initialize`, `NETCDF.UDF2.MAGIC=GRIB`

### Source Files

| File | Purpose |
|------|---------|
| `src/grib2dispatch.c` | NC_Dispatch table and `NC_GRIB2_initialize()` |
| `src/grib2file.c` | `NC_GRIB2_open()`, `NC_GRIB2_close()`, `NC_GRIB2_get_vara()` |
| `include/grib2dispatch.h` | Public header: `NC_GRIB2_FILE_INFO_T`, prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| GRIB2 Handler | NOAA NCEPLIBS-g2c | ≥ 2.1.0 | GRIB2 file operations and data unpacking |
| GRIB2 Handler | libjasper | ≥ 3.0.0 | JPEG2000 compression (transitive dep of g2c) |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

GRIB2 support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -DENABLE_GRIB2=ON   # Enable GRIB2 support
cmake -DENABLE_GRIB2=OFF  # Disable GRIB2 support (default)
```

**Autotools:**
```bash
./configure --enable-grib2   # Enable GRIB2 support
./configure --disable-grib2  # Disable GRIB2 support (default)
```

### Known Limitations
- Read-only access (NC_NOWRITE mode only)
- 2D grids only; ensemble/time/level dimensions deferred to future releases
- One NetCDF variable per GRIB2 message; multi-message aggregation not supported

---

## FITS Format Support (v2.0.0)

### Overview

NEP v2.0.0 adds support for FITS (Flexible Image Transport System) files through a User Defined Format (UDF) handler, using the CFITSIO library. FITS files from instruments such as HST, Chandra, and JWST can be opened with `nc_open()` and queried with standard NetCDF APIs — no conversion required.

### Architecture

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [FITS UDF Handler]
│                 │
│             [CFITSIO]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [FITS Files]
```

### Key Components

#### FITS UDF Handler
- **Format Detection**: Automatic identification via `"SIMPLE"` magic (first 6 bytes of every FITS file); registered at UDF slot 3 (`NEP_UDF_FITS = NC_UDF3`)
- **NC_Dispatch Implementation**: Complete dispatch table; `NC_FITS_initialize()` calls `nc_def_user_format(NC_UDF3, &FITS_dispatch_table, "SIMPLE")`
- **File Operations**: `NC_FITS_open()` calls `fits_open_file()` and stores the CFITSIO file handle; `NC_FITS_close()` calls `fits_close_file()`
- **Data I/O**: `NC_FITS_get_vara()` reads pixel hyperslabs via `fits_read_subset()` (image path) and table column data via `fits_read_col()` (table path)

#### HDU-to-Group Mapping
- **Primary HDU**: Image or null — goes in the root group; variable named `image`
- **Extension HDUs**: Each becomes a child group named from `EXTNAME` (sanitized) or `hdu_N` if absent
- **Image extension HDUs**: Same dim/variable/attribute mapping as primary HDU applied to the child group
- **Binary and ASCII table HDUs**: Row dimension from `NAXIS2`; one netCDF variable per column named from `TTYPEn`; `TUNITn` → `units` attributes; vector columns become 2D variables

#### Metadata Mapping
- **BITPIX → nc_type**: −64→`NC_DOUBLE`, −32→`NC_FLOAT`, 8→`NC_BYTE`, 16→`NC_SHORT`, 32→`NC_INT`, 64→`NC_INT64`
- **Dimension order**: Reversed from FITS column-major (`NAXIS1`=fastest) to netCDF row-major
- **Standard keyword mapping**: `BUNIT`→`units`, `BZERO`→`add_offset`, `BSCALE`→`scale_factor`, `BLANK`→`_FillValue`
- **All header keywords**: Stored as group-level string attributes on root group or child group
- **Column typecode → nc_type**: A→`NC_CHAR`, I→`NC_SHORT`, J→`NC_INT`, E→`NC_FLOAT`, D→`NC_DOUBLE`, etc.

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_FITS_FILE_INFO_T` | `include/fitsdispatch.h` | Per-file: CFITSIO `fitsfile *`, path |
| `NC_FITS_VAR_INFO_T` | `include/fitsdispatch.h` | Per-variable: `hdu_num`, `col_num` (0=image), `col_width` (string columns) |

#### `NC_FITS_get_vara()` Data Paths
- **Image**: Reverses `start[]/count[]` into `fpixel[]/lpixel[]`, calls `fits_read_subset()`; CFITSIO applies `BSCALE`/`BZERO` automatically when reading into floating-point types
- **Table column**: `fits_read_col()` with `firstrow = start[0]+1`, `firstelem = start[1]+1` for 2D; string columns use `char **` pointer array into a flat buffer

### Source Files

| File | Purpose |
|------|---------|
| `src/fitsdispatch.c` | NC_Dispatch table and `NC_FITS_initialize()` / `NC_FITS_finalize()` |
| `src/fitsfile.c` | `NC_FITS_open()`, `NC_FITS_close()`, `NC_FITS_get_vara()`, HDU readers |
| `include/fitsdispatch.h` | Public header: `NC_FITS_FILE_INFO_T`, `NC_FITS_VAR_INFO_T`, prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| FITS Handler | CFITSIO | ≥ 3.0 | FITS file I/O and HDU navigation |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

FITS support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -DENABLE_FITS=ON   # Enable FITS support
cmake -DENABLE_FITS=OFF  # Disable FITS support (default)
```

**Autotools:**
```bash
./configure --enable-fits   # Enable FITS support
./configure --disable-fits  # Disable FITS support (default)
```

### Test Data

**`test/data/WFPC2u5780205r_c0fx.fits`** — HST WFPC2 file with:
- Primary image HDU: `[4, 200, 200]` `NC_FLOAT` pixels
- ASCII table extension HDU: 49 columns × 4 rows

### Known Limitations
- Write support not implemented; `nc_create()` on a FITS file returns `NC_EPERM`
- `.ncrc` UDF autoload requires NetCDF-C main branch; local 4.10.0 requires explicit `NC_FITS_initialize()` call

---

## NASA/ESA PDS4 Format Support (v2.2.0)

### Overview

NEP v2.2.0 adds support for NASA/ESA Planetary Data System version 4 (PDS4) XML-labeled data products through a User Defined Format (UDF) handler using libxml2. PDS4 array images, binary tables, and character tables are accessible through the standard NetCDF API.

### Architecture

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [PDS4 UDF Handler]
│                 │
│             [libxml2]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [PDS4 XML Labels + Data Files]
```

### Key Components

#### PDS4 UDF Handler
- **Format Detection**: PDS4 XML label files (`.xml`) identified by `<?xml` / `Product_Observational` root element; registered at UDF slot 5 (`NEP_UDF_PDS4 = NC_UDF5`)
- **NC_Dispatch Implementation**: `NC_PDS4_initialize()` calls `nc_def_user_format(NC_UDF5, &PDS4_dispatch_table, NULL)`
- **XML Parsing**: libxml2 parses `Product_Observational` labels; PDS4 namespace validated on open
- **Data File Resolution**: Data filenames resolved relative to the XML label directory

#### Metadata Mapping

| PDS4 Element | NetCDF Mapping |
|---|---|
| `Identification_Area` fields | Global string attributes on root group |
| `Observation_Area` fields | Global string attributes on root group |
| `File_Area_Observational` | Child group named from `File/file_name` |
| `Array` / `Array_2D_Image` / `Array_2D_Spectrum` | netCDF dimensions (from `Axis_Array` sorted by `sequence_number`) + variable (from `Element_Array/data_type`) |
| `Table_Binary` / `Table_Character` / `Table_Delimited` | `record` dimension from `<records>`; one variable per `Field_*` with type and optional `units` attribute |

#### Data Reading (`NC_PDS4_get_vara`)
- **Array data**: Contiguous row-major reads with `start[]/count[]` hyperslab support; MSB/LSB byte-swap applied based on label byte-order
- **Binary table fields**: Per-record seek using `field_location` / `field_length` / `record_length`; byte-swap as required
- **ASCII table fields**: Fixed-width text parsed via `strtod` / `strtoll` into the mapped netCDF type
- **Delimited tables**: Row count derived from label `<records>`

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_PDS4_FILE_INFO_T` | `include/pds4dispatch.h` | Per-file: xmlDoc pointer, label path, data directory |
| `NC_PDS4_VAR_INFO_T` | `include/pds4dispatch.h` | Per-variable: file offset, record length, field offset/length, table/ASCII flags, byte-order |

### Source Files

| File | Purpose |
|------|---------|
| `src/pds4dispatch.c` | NC_Dispatch table and `NC_PDS4_initialize()` / `NC_PDS4_finalize()` |
| `src/pds4file.c` | `NC_PDS4_open()`, `NC_PDS4_close()`, `NC_PDS4_get_vara()`, XML metadata readers |
| `include/pds4dispatch.h` | Public header: `NC_PDS4_FILE_INFO_T`, `NC_PDS4_VAR_INFO_T`, prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| PDS4 Handler | libxml2 | ≥ 2.9 (`libxml2-dev`) | XML label parsing |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

PDS4 support defaults to **OFF** and must be enabled explicitly:

**CMake:**
```bash
cmake -DENABLE_PDS4=ON   # Enable PDS4 support
cmake -DENABLE_PDS4=OFF  # Disable PDS4 support (default)
```

**Autotools:**
```bash
./configure --enable-pds4   # Enable PDS4 support
./configure --disable-pds4  # Disable PDS4 support (default)
```

### Test Data

Located in `test/data/PDS4/`:
- `test_image.xml` / `test_image.img` — Array_2D_Image product
- `test_table_binary.xml` / `test_table_binary.dat` — Table_Binary product (3 fields × 5 records)
- `Table_Character_Example.xml` / `Table_Character_Example.tab` — Table_Character product (3 fields × 3 records)

Real mission test products also include MAVEN NGIMS/IUVS, Mars 2020 Perseverance Mastcam-Z, and New Horizons Alice `.lblx` / `.fit` pairs.

### Known Limitations
- Write support not implemented; `nc_create()` returns `NC_EPERM`
- `nc_get_vars()` / `nc_get_varm()` use default dispatch fallbacks (no strided native reads)
- Type conversion between `memtype` and file type not performed; caller must request the native type
- Grouped fields (`Group_Field_Binary`) and bit fields not yet supported
- Only `Product_Observational` root element type validated

---

## NASA CDF Format Support (v1.3.0)

### Overview

NEP v1.3.0 added support for NASA Common Data Format (CDF) files through a User Defined Format (UDF) handler. This enables transparent access to CDF space physics and satellite data through the standard NetCDF API.

### Architecture

The CDF UDF handler follows the same NC_Dispatch pattern used for other format handlers:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [CDF UDF Handler]
│                 │
│             [NASA CDF Library]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [CDF Files]
```

### Key Components

#### CDF UDF Handler
- **Format Detection**: Automatic identification of CDF files via magic number
- **NC_Dispatch Implementation**: Complete dispatch table for CDF file operations
- **File Operations**: `NC_CDF_open()` and `NC_CDF_close()` with proper resource management
- **Metadata Mapping**: CDF structures mapped to NetCDF equivalents

#### Type System
- **Type Mapping**: Complete mapping of CDF types to NetCDF types
  - CDF_INT4 → NC_INT
  - CDF_REAL8 → NC_DOUBLE
  - CDF_TIME_TT2000 → NC_INT64
- **Attribute Conventions**: FILLVAL attributes automatically renamed to _FillValue
- **Data Reading**: `NC_CDF_get_vara()` supporting scalars and arrays

#### Dependencies
- NASA CDF Library v3.9.x (required when enabled)

#### Build Integration
- CDF support defaults to **OFF**; enable via build flags:
  - CMake: `-DENABLE_CDF=ON` to enable (default: OFF)
  - Autotools: `--enable-cdf` to enable (default: disabled)
- CDF uses **UDF slot 4** (`NC_UDF4`) as of v2.2.0; moved from UDF2 to eliminate mutual-exclusivity with GRIB2

## Spack Package Manager Support (v1.4.0)

### Overview

NEP v1.4.0 added Spack package manager support for simplified installation and dependency management in HPC environments.

### Key Features

- **NEP Spack Package**: Complete Spack package recipe with CMake build system
- **CDF Spack Package**: Separate package for NASA CDF library v3.9.1
- **Variant Support**: `+fortran/-fortran`, `+lz4/-lz4`, `+bzip2/-bzip2`, `+docs/-docs`
- **Dependency Management**: Automatic resolution of NetCDF-C, HDF5, LZ4, BZIP2, and NetCDF-Fortran
- **CI Integration**: Dedicated GitHub Actions workflows for testing Spack packages

### Installation

```bash
# Install NEP with all features
spack install nep

# Install with minimal features
spack install nep~docs~fortran

# Install CDF library
spack install cdf
```

## Parallel I/O Support (v1.9.0)

### Overview

NEP v1.9.0 adds parallel I/O support for high-performance computing environments using MPI and parallel NetCDF-4/HDF5 features. This enables applications to perform collective I/O operations across multiple processes for improved performance on large datasets.

### Architecture

Parallel I/O in NEP follows the standard NetCDF-4 parallel pattern:

```
[Application Layer - MPI Ranks 0..N]
       │
[NetCDF-4 Parallel API]
       │
[Parallel NetCDF-4/HDF5]
       │
[MPI I/O Layer]
       │
[Parallel File System]
```

### Build Configuration

**CMake:**
- `ENABLE_PARALLEL_TESTS=ON/OFF` - Enable parallel I/O test programs (default: OFF)
- `MPIEXEC_EXECUTABLE=PATH` - Specify path to mpiexec/mpirun

**Autotools:**
- `--enable-parallel-tests` - Enable parallel I/O test programs
- `--with-mpiexec=PATH` - Specify path to mpiexec/mpirun

### Dependencies
- MPI implementation (OpenMPI or MPICH)
- NetCDF-C built with parallel support (`NC_HAS_PARALLEL4`)
- NetCDF-Fortran (for Fortran parallel tests)
- HDF5 with parallel support (for underlying parallel I/O)

### Example Programs

Parallel I/O examples in `examples/parallelIO/` demonstrate:
- **Collective I/O**: `nc_create_par()`, `nc_var_par_access(NC_COLLECTIVE)`
- **Data Decomposition**: 2D grid partitioning across MPI ranks
- **Verification**: Parallel read-back and data integrity checks
- **Error Handling**: Proper MPI and NetCDF error propagation

### Testing

Parallel tests run via `mpiexec -n 4` in CI:
- Matrix: CMake/Autotools × OpenMPI/MPICH
- ncdump verification of output files
- Automated data integrity validation

## Example Programs (v2.1.0+)

### Overview

NEP includes comprehensive example programs in C and Fortran serving as companion code for *The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition* (https://www.amazon.com/dp/B0H7Q1Z75L). Examples are organized into categories covering Classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and performance benchmarking.

### Architecture

Example programs are organized by category:

```
examples/
├── classic/          # Classic NetCDF examples in C
├── f_classic/        # Classic NetCDF examples in Fortran
├── netcdf-4/         # NetCDF-4 examples in C
├── f_netcdf-4/       # NetCDF-4 examples in Fortran
├── nczarr/           # NcZarr (local Zarr store) examples in C and Fortran
├── opendap/          # OPeNDAP remote access examples in C and Fortran
├── parallelIO/       # MPI parallel I/O examples in C and Fortran
└── performance/      # Compression benchmark examples (ENABLE_BENCHMARKS)
```

### Example Categories

#### Classic NetCDF Examples (C)
Located in `examples/classic/`:
- **simple_2D.c**: Basic 2D array creation and writing
- **coord_vars.c**: Working with coordinate variables
- **format_variants.c**: Different NetCDF format variants (classic, 64-bit offset, CDF-5)
- **size_limits.c**: Demonstrating size and dimension limits
- **unlimited_dim.c**: Using unlimited dimensions for time series data
- **var4d.c**: Creating and writing 4-dimensional variables

#### NetCDF-4 Examples (C)
Located in `examples/netcdf-4/`:
- **simple_nc4.c**: Basic NetCDF-4 file creation
- **compression.c**: Using compression filters (deflate, zstd, shuffle)
- **chunking_performance.c**: Chunking strategies and performance
- **multi_unlimited.c**: Multiple unlimited dimensions (NetCDF-4 feature)
- **user_types.c**: User-defined compound and enum types

#### Classic NetCDF Examples (Fortran)
Located in `examples/f_classic/`:
- **f_simple_2D.f90**: Basic 2D array in Fortran
- **f_coord_vars.f90**: Coordinate variables in Fortran
- **f_format_variants.f90**: Format variants in Fortran
- **f_size_limits.f90**: Size limits in Fortran
- **f_unlimited_dim.f90**: Unlimited dimensions in Fortran
- **f_var4d.f90**: 4D variables in Fortran

#### NetCDF-4 Examples (Fortran)
Located in `examples/f_netcdf-4/`:
- **f_simple_nc4.f90**: Basic NetCDF-4 in Fortran
- **f_compression.f90**: Compression in Fortran (deflate, zstd, shuffle)
- **f_chunking_performance.f90**: Chunking in Fortran
- **f_multi_unlimited.f90**: Multiple unlimited dimensions in Fortran
- **f_user_types.f90**: User-defined types in Fortran

#### NcZarr Examples (C and Fortran, v1.11.0)
Located in `examples/nczarr/`:
- **nczarr_simple.c / f_nczarr_simple.f90**: Create and read a local `file://...#mode=nczarr` store
- **nczarr_chunking.c / f_nczarr_chunking.f90**: Explicit chunk shape via `nc_def_var_chunking()`
- **nczarr_compression.c / f_nczarr_compression.f90**: Shuffle + deflate on a Zarr store
- **nczarr_enhanced.c / f_nczarr_enhanced.f90**: Hierarchical groups and multiple independent unlimited dimensions
- Built only when `HAVE_NCZARR` is detected in NetCDF-C

#### OPeNDAP Examples (C and Fortran)
Located in `examples/opendap/`:
- Remote data access using OPeNDAP constraint expressions
- Elapsed-time instrumentation via `gettimeofday()`

#### Parallel I/O Examples (C and Fortran, v1.9.0)
Located in `examples/parallelIO/`:
- **square16_par.c / f_square16_par.f90**: 2×2 MPI domain decomposition, collective I/O
- Uses `nc_create_par()` / `nc_var_par_access(NC_COLLECTIVE)` patterns
- Built when `ENABLE_PARALLEL_TESTS=ON` / `--enable-parallel-tests`

#### Performance Benchmark Examples (v1.10.0)
Located in `examples/performance/`:
- **cache_tuning.c**: File-level and variable-level chunk cache configuration
- **chunking.c**: Chunk shape impact on time-slab vs column-profile access
- **deflate.c**: Deflate levels 0–9 × shuffle (20 combinations)
- **zstandard.c**: Zstandard levels × shuffle (24 combinations)
- **szip.c**: SZIP `pixels_per_block` values with NC_SZIP_NN
- **lz4.c**: LZ4 levels × shuffle (18 combinations)
- **bzip2.c**: BZIP2 levels × shuffle (18 combinations)
- **lossless.c**: Head-to-head best-setting comparison of all lossless filters
- Built only when `ENABLE_BENCHMARKS=ON` / `--enable-benchmarks` (default OFF)

### Build System Integration

#### CMake Integration
Examples are integrated into the CMake build system with optional building:

```cmake
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
```

Each example subdirectory has its own `CMakeLists.txt` that:
- Compiles example programs
- Links against NetCDF-C (and NetCDF-Fortran for Fortran examples)
- Registers examples as CTest tests

#### Autotools Integration
Examples are integrated into the Autotools build system:

```bash
# Configure option
./configure --enable-examples  # default
./configure --disable-examples
```

Each example subdirectory has its own `Makefile.am` that:
- Compiles example programs
- Links against NetCDF libraries
- Registers examples in the test suite

### Test Integration

All examples run as automated tests to ensure:
- **Correctness**: Examples produce valid NetCDF files
- **API Usage**: Demonstrates proper API usage patterns
- **Regression Detection**: Catches breaking changes in NEP

#### Test Execution

**CMake:**
```bash
ctest --test-dir build
```

**Autotools:**
```bash
make check
```

Each example:
1. Creates a NetCDF file demonstrating specific features
2. Reads the file back to verify correctness
3. Validates data integrity
4. Exits with success (0) or failure (non-zero) status

### Dependencies

- **C Examples**: NetCDF-C library, C99 compiler
- **Fortran Examples**: NetCDF-Fortran library, Fortran 90+ compiler
- **Build Systems**: CMake 3.9+ or Autotools

Fortran examples are automatically skipped if:
- Fortran support is disabled (`ENABLE_FORTRAN=OFF` or `--disable-fortran`)
- NetCDF-Fortran library is not available
- No Fortran compiler is detected

### Educational Value

Examples demonstrate:
- **Best Practices**: Proper error handling, resource cleanup, API usage patterns
- **Feature Coverage**: Basic to advanced NetCDF features
- **Language Parity**: Equivalent functionality in C and Fortran
- **Real-world Patterns**: Common use cases in scientific computing

## Release Information

- **Version**: v2.7.1
- **Status**: Release Preparation
- **Release Date**: July 2026
- **Features**:
  - LZ4 and BZIP2 compression for HDF5/NetCDF-4 files (C and Fortran APIs)
  - GeoTIFF read support via UDF handler (UDF0/UDF1)
  - GRIB2 read support via UDF handler (UDF2)
  - FITS read support via UDF handler (UDF3)
  - NASA CDF read support via UDF handler (UDF4)
  - NASA/ESA PDS4 read support via UDF handler (UDF5), including `Array_3D_Image`, `Group_Field_Binary`, delimited tables, and real mission test data (MAVEN, Perseverance, New Horizons)
  - All five format readers can be enabled simultaneously
  - Spack package manager support with variants for every optional reader and utility
  - NcZarr, OPeNDAP, parallel I/O, and performance benchmark examples
  - Example programs as companion code for *The NetCDF Developer's Handbook, Second Edition*

---

*Last Updated: July 2026 (v2.7.1 release preparation)*

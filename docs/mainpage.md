# NEP (NetCDF Extension Pack)

## Overview

NEP (NetCDF Extension Pack) extends NetCDF-4 with high-performance compression and multi-format data access:

- **LZ4 Compression**: Speed-optimized lossless compression (2-3x faster than DEFLATE)
- **BZIP2 Compression**: High-ratio lossless compression for archival storage
- **CDF File Reader**: Read NASA Common Data Format files through the NetCDF API
- **GeoTIFF File Reader**: Read GeoTIFF geospatial raster files through the NetCDF API
- **GRIB2 File Reader**: Read GRIB2 meteorological and oceanographic data files through the NetCDF API

NEP provides flexible compression options and unified data access for diverse scientific data workflows.

## NetCDF Example Programs

NEP includes 26 comprehensive example programs demonstrating NetCDF API usage in both C and Fortran. These examples cover classic NetCDF and NetCDF-4 features, providing practical learning resources for users at all levels.

### Example Categories

**Quickstart Examples** (2 programs):
- quickstart.c - Minimal introduction to NetCDF (C) - the simplest starting point
- f_quickstart.f90 - Minimal introduction to NetCDF (Fortran) - the simplest starting point

**C Classic Examples** (6 programs):
- simple_2D.c - Basic 2D arrays
- coord_vars.c - Coordinate variables
- format_variants.c - Format variants
- size_limits.c - Size limits
- unlimited_dim.c - Unlimited dimensions
- var4d.c - 4D variables

**C NetCDF-4 Examples** (6 programs):
- simple_nc4.c - NetCDF-4 basics
- compression.c - Compression filters
- chunking_performance.c - Chunking strategies
- multi_unlimited.c - Multiple unlimited dimensions
- user_types.c - User-defined types
- groups.c - Hierarchical groups

**Fortran Classic Examples** (6 programs):
- f_simple_2D.f90 - Basic 2D arrays
- f_coord_vars.f90 - Coordinate variables
- f_format_variants.f90 - Format variants
- f_size_limits.f90 - Size limits
- f_unlimited_dim.f90 - Unlimited dimensions
- f_var4d.f90 - 4D variables

**Fortran NetCDF-4 Examples** (6 programs):
- f_simple_nc4.f90 - NetCDF-4 basics
- f_compression.f90 - Compression filters
- f_chunking_performance.f90 - Chunking strategies
- f_multi_unlimited.f90 - Multiple unlimited dimensions
- f_user_types.f90 - User-defined types
- f_groups.f90 - Hierarchical groups

### Key Features

- **Automated Testing**: All examples run as automated tests with CDL-based output validation
- **C/Fortran Equivalence**: Parallel implementations produce identical output, enabling cross-language learning
- **Comprehensive Documentation**: Doxygen-integrated with learning paths from beginner to advanced
- **Build Integration**: Examples build by default in both CMake and Autotools, can be disabled with `-DBUILD_EXAMPLES=OFF` or `--disable-examples`

### Learning Paths

**Absolute Beginners**: Start with quickstart.c / f_quickstart.f90 - the simplest possible introduction with just 6 data values

**Beginners**: Progress to simple_2D.c / f_simple_2D.f90 → coord_vars.c / f_coord_vars.f90 → unlimited_dim.c / f_unlimited_dim.f90

**Intermediate**: Continue with format_variants.c / f_format_variants.f90 → size_limits.c / f_size_limits.f90 → var4d.c / f_var4d.f90

**Advanced (NetCDF-4)**: Master simple_nc4.c / f_simple_nc4.f90 → compression.c / f_compression.f90 → chunking_performance.c / f_chunking_performance.f90 → groups.c / f_groups.f90 → multi_unlimited.c / f_multi_unlimited.f90 → user_types.c / f_user_types.f90

See examples/README.md for complete details, build instructions, and usage guides.

## Compression Algorithms

### LZ4 Compression

LZ4 is a lossless data compression algorithm from the LZ77 family of byte-oriented compression schemes. It prioritizes compression and decompression speed while maintaining good compression ratios.

**Key Features:**
- **High-Speed Compression**: 2-3x faster compression/decompression than DEFLATE
- **Lossless**: Guarantees data integrity with no data loss
- **Optimized for HPC**: Designed for speed-critical workflows in high-performance computing environments
- **Transparent Integration**: Works seamlessly with existing NetCDF-4 applications

**Performance Characteristics:**
- **Compression Speed**: Similar to LZO, several times faster than DEFLATE
- **Decompression Speed**: Significantly faster than LZO
- **Compression Ratio**: Good trade-off between speed and size reduction
- **Use Case**: Ideal for I/O-bound scientific workflows where speed matters more than maximum compression

### BZIP2 Compression

BZIP2 is a lossless data compression algorithm using the Burrows-Wheeler block sorting algorithm. It prioritizes achieving high compression ratios, making it ideal for archival storage and bandwidth-constrained scenarios.

**Key Features:**
- **Superior Compression Ratios**: Better compression than DEFLATE and significantly better than LZ4
- **Lossless**: Guarantees data integrity with no data loss
- **Block-Sorting Algorithm**: Particularly effective for repetitive patterns in scientific data
- **Transparent Integration**: Works seamlessly with existing NetCDF-4 applications

**Performance Characteristics:**
- **Compression Ratio**: Exceeds DEFLATE, ideal for storage optimization
- **Compression Speed**: Slower than LZ4 but faster than most high-ratio algorithms
- **Decompression Speed**: Moderate, suitable for archival workflows
- **Use Case**: Ideal for long-term archival storage, bandwidth-constrained transfers, and storage-limited environments

## Choosing the Right Algorithm

- **Use LZ4 when**: I/O speed is critical, working with real-time data, running on HPC systems, need fast decompression
- **Use BZIP2 when**: Storage space is limited, archiving data long-term, transferring over slow networks, compression ratio matters most

## CDF File Reader

NEP provides a User-Defined Format (UDF) handler that enables reading NASA Common Data Format (CDF) files using the familiar NetCDF API.

### What is CDF?

The Common Data Format (CDF) is a self-describing data format developed by NASA's Space Physics Data Facility (SPDF). CDF is widely used in space physics and heliophysics research for storing time-series and multi-dimensional scientific data from space missions.

**Key Characteristics:**
- Self-describing format with embedded metadata
- Support for multiple data types and dimensions
- Platform-independent binary format
- Optimized for space physics data
- Used by NASA missions: IMAP, MMS, Van Allen Probes, and many others

### CDF Support in NEP

**Transparent Access**: Read CDF files using standard NetCDF functions (`nc_open()`, `nc_get_var()`, `nc_get_att()`) without file conversion.

**Automatic Type Mapping**: CDF data types are automatically mapped to NetCDF equivalents:
- CDF_INT4 → NC_INT
- CDF_REAL8 → NC_DOUBLE
- CDF_TIME_TT2000 → NC_INT64 (for time variables)
- And more...

**Attribute Conventions**: CDF attributes are automatically converted to NetCDF conventions (e.g., FILLVAL → _FillValue).

**Use Cases:**
- Space physics and heliophysics research
- NASA mission data analysis
- Cross-format data integration (CDF + NetCDF)
- Legacy CDF data access in modern workflows

### Enabling CDF Support

CDF support is disabled by default (mutually exclusive with GRIB2). To enable:

```bash
# CMake
cmake -B build -DENABLE_CDF=ON -DENABLE_GRIB2=OFF

# Autotools
./configure --enable-cdf --disable-grib2
```

**Requirements**: NASA CDF library v3.9+ must be installed. Download from: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/

**Spack Users**: Install CDF separately with `spack install cdf`.

## GRIB2 File Reader

NEP provides a User-Defined Format (UDF) handler that enables reading GRIB2 (General Regularly-distributed Information in Binary form, Edition 2) files using the familiar NetCDF API.

### What is GRIB2?

GRIB2 is the standard binary format used by meteorological and oceanographic agencies worldwide (NOAA, ECMWF, and others) for distributing numerical weather prediction (NWP) model output, wave forecasts, and observational data.

**Key Characteristics:**
- Binary format optimized for gridded meteorological data
- Products organized by discipline, category, and parameter number
- Bitmap-based land/sea masking for ocean and atmosphere grids
- Used by NOAA GFS, NAM, HRRR, GDAS, and global wave forecast models
- Accessed via the NOAA NCEPLIBS-g2c library

### GRIB2 Support in NEP

**Transparent Access**: Read GRIB2 files using standard NetCDF functions (`nc_open()`, `nc_get_var()`, `ncdump`) without file conversion.

**Product-to-Variable Mapping**: Each GRIB2 product is exposed as a named `NC_FLOAT` NetCDF variable with shared `[y, x]` dimensions. Variable names come from `g2c_param_abbrev()`; duplicate names are uniquified with `_2`, `_3` suffixes.

**Full Grid Expansion**: `NC_GRIB2_get_vara()` uses `g2_getfld(expand=1)` to expand the full `[ny, nx]` grid, substituting `_FillValue = 9.999e20f` for bitmap-masked (e.g., land) points.

**Metadata**: Per-variable attributes (`long_name`, `_FillValue`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`) and global attributes (`Conventions = "GRIB2"`, `GRIB2_edition = 2`).

**`.ncrc` Autoload**: Place the `.ncrc` file in your home directory and `nc_open()` / `ncdump` work on `.grib2` files with no code changes.

**Use Cases:**
- Weather forecasting and NWP model output analysis
- Ocean wave and surge forecast data access
- NOAA GDAS, GFS, NAM, and HRRR data processing
- Cross-format meteorological workflows (GRIB2 + NetCDF)

### Enabling GRIB2 Support

GRIB2 support is enabled by default. To disable:

```bash
# CMake
cmake -B build -DENABLE_GRIB2=OFF

# Autotools
./configure --disable-grib2
```

**Requirements**: NOAA NCEPLIBS-g2c (>= 2.1.0) and libjasper (>= 3.0.0) must be installed. Supply the g2c install path at configure time.

**Note**: GRIB2 and CDF are mutually exclusive — both use UDF slot 2. Enable one or the other, not both.

## GeoTIFF File Reader

NEP provides a User-Defined Format (UDF) handler that enables reading GeoTIFF files using the familiar NetCDF API.

### What is GeoTIFF?

GeoTIFF is a public domain metadata standard that allows georeferencing information to be embedded within a TIFF (Tagged Image File Format) file. GeoTIFF is widely used in geographic information systems (GIS), remote sensing, and earth science applications for storing geospatial raster data.

**Key Characteristics:**
- Standard TIFF format with geospatial metadata tags
- Support for coordinate reference systems (CRS)
- Multi-band raster data support
- Platform-independent format
- Used by NASA, USGS, and earth observation missions

### GeoTIFF Support in NEP

**Transparent Access**: Read GeoTIFF files using standard NetCDF functions (`nc_open()`, `nc_get_var()`, `nc_inq_dim()`) without file conversion.

**Automatic Dimension Mapping**: GeoTIFF structure is automatically mapped to NetCDF dimensions:
- Bands → NetCDF dimension (if multi-band)
- Rows (height) → NetCDF dimension
- Columns (width) → NetCDF dimension

**CF-1.8 CRS Metadata**: Coordinate reference system information is emitted as a CF-1.8 compliant `crs` grid-mapping variable with coordinate variables (`lon`/`lat` or `x`/`y`) and optional bounds. See `docs/cf-compliance.md` for the full attribute specification.

**Metadata Preservation**: GeoTIFF tags and geospatial metadata are accessible as NetCDF attributes, including coordinate reference system information.

**Use Cases:**
- Remote sensing and satellite imagery analysis
- GIS data integration
- Earth observation data processing
- Cross-format geospatial workflows (GeoTIFF + NetCDF)

### Enabling GeoTIFF Support

GeoTIFF support is enabled by default. To disable:

```bash
# CMake
cmake -B build -DENABLE_GEOTIFF=OFF

# Autotools
./configure --disable-geotiff
```

**Requirements**: libgeotiff and libtiff libraries must be installed.

**Spack Users**: Install with `spack install nep +geotiff`.

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+) and its dependencies
- LZ4 library
- BZIP2 library
- NetCDF-Fortran library (optional, for Fortran support)
- NASA CDF library v3.9+ (optional, for CDF file reading; mutually exclusive with GRIB2)
- libgeotiff and libtiff (enabled by default, for GeoTIFF file reading)
- NOAA NCEPLIBS-g2c >= 2.1.0 and libjasper >= 3.0.0 (enabled by default, for GRIB2 file reading)

Build with CMake or Autotools:

```bash
# CMake
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build

# Autotools
./configure --prefix=/usr/local
make
make install
```

## Usage

### Compression

Once installed, both LZ4 and BZIP2 compression filters are available to NetCDF-4 applications through HDF5 filter plugins. Set the `HDF5_PLUGIN_PATH` environment variable and use standard NetCDF-4 compression APIs:

```bash
export HDF5_PLUGIN_PATH=/usr/local/lib/plugin
```

Choose the algorithm that best fits your workflow requirements—LZ4 for speed or BZIP2 for compression ratio.

### CDF File Reading

When CDF support is enabled, open and read CDF files using standard NetCDF functions:

```c
int ncid;
nc_open("data.cdf", NC_NOWRITE, &ncid);
// Use standard NetCDF API calls
nc_close(ncid);
```

No special code is needed—the CDF UDF handler automatically detects and processes CDF files.

### GRIB2 File Reading

When GRIB2 support is enabled (the default), open and read GRIB2 files using standard NetCDF functions:

```c
int ncid, varid;
float data[151 * 241];  /* ny=151, nx=241 for GDAS West Coast wave grid */

/* Open GRIB2 file using NetCDF API */
nc_open("gdaswave.t00z.wcoast.0p16.f000.grib2", NC_NOWRITE, &ncid);

/* Look up a variable by its GRIB2 parameter abbreviation */
nc_inq_varid(ncid, "WIND", &varid);

/* Read the full [ny][nx] grid; land points = 9.999e20 (_FillValue) */
nc_get_var_float(ncid, varid, data);

nc_close(ncid);
```

Or use `ncdump` directly after installing the `.ncrc` file:

```bash
ncdump -h gdaswave.t00z.wcoast.0p16.f000.grib2
ncdump -v WIND gdaswave.t00z.wcoast.0p16.f000.grib2
```

No special code is needed — the GRIB2 UDF handler automatically detects and processes GRIB2 files.

### GeoTIFF File Reading

When GeoTIFF support is enabled (the default), open and read GeoTIFF files using standard NetCDF functions:

```c
int ncid, varid, crs_varid;
size_t width, height;

// Open GeoTIFF file using NetCDF API
nc_open("satellite_image.tif", NC_NOWRITE, &ncid);

// Query dimensions
nc_inq_dimlen(ncid, 0, &height);  /* y */
nc_inq_dimlen(ncid, 1, &width);   /* x */

// Read a 100x100 hyperslab from the raster data variable
unsigned char *buf = malloc(100 * 100);
size_t start[2] = {0, 0}, count[2] = {100, 100};
nc_inq_varid(ncid, "data", &varid);
nc_get_vara_uchar(ncid, varid, start, count, buf);

// Access CF-1.8 CRS metadata (if present)
char grid_mapping_name[NC_MAX_NAME + 1];
double semi_major;
if (nc_inq_varid(ncid, "crs", &crs_varid) == NC_NOERR) {
    nc_get_att_text(ncid, crs_varid, "grid_mapping_name", grid_mapping_name);
    nc_get_att_double(ncid, crs_varid, "semi_major_axis", &semi_major);
}

// Close file
nc_close(ncid);
```

No special code is needed—the GeoTIFF UDF handler automatically detects and processes GeoTIFF files.

## Features and Options

- **Compression Algorithms**:
  - LZ4: speed-optimized lossless compression
  - BZIP2: high-ratio lossless compression
- **CDF File Reader**:
  - Read NASA CDF files via NetCDF API
  - Automatic type and attribute mapping
  - Support for TT2000 time variables
- **GeoTIFF File Reader**:
  - Read GeoTIFF geospatial raster files via NetCDF API
  - Automatic dimension mapping (bands, rows, columns)
  - CF-1.8 compliant CRS metadata (`crs` variable, coordinate variables, bounds)
  - See `docs/cf-compliance.md` for the CF grid mapping attribute specification
- **GRIB2 File Reader**:
  - Read GRIB2 meteorological/oceanographic files via NetCDF API
  - Each product exposed as a named `NC_FLOAT` variable with `[y, x]` dimensions
  - Full grid expansion with `_FillValue` for bitmap-masked (land) points
  - Per-variable GRIB2 metadata attributes and global `Conventions = "GRIB2"`
  - `.ncrc` autoload support for zero-code-change `nc_open()` / `ncdump` access
- **Configurable Build Options**:
  - LZ4 and BZIP2 support can be enabled or disabled at build time
  - CDF support is optional (disabled by default; mutually exclusive with GRIB2)
  - GeoTIFF and GRIB2 support are enabled by default; disable with `--disable-geotiff` / `--disable-grib2`
- **Fortran Support**:
  - Optional Fortran wrappers for NEP functions
  - Requires `netcdf-fortran` library when enabled

See the dedicated build and configuration page for full details on CMake and Autotools options:

- `docs/build-options.md`

## API Documentation

The API documentation is generated directly from the C and Fortran source code.

- **Versioned API docs**: Documentation is published per release (for example, `/NEP/v1.2.0/api/`).
- **Latest API docs**: A `latest` alias points to the most recent release documentation.

Key API reference pages:

- [C API (nep.c)](nep_8c.html)
- [Fortran API (nep.F90)](nep_8F90.html)

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

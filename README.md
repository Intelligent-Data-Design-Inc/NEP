# NEP (NetCDF Extension Pack)

## Overview

NEP is a unified framework that provides additional compression and enables seamless access to diverse Earth Science data formats through the standard NetCDF interface using User Defined Formats (UDF). It eliminates the need for data conversion or format-specific processing pipelines while preserving the optimizations of specialized formats.

## The Problem

NASA's Earth Science missions face significant challenges with data management:

- **Massive data volumes**: Unprecedented amounts of observational data in diverse formats
- **Complex integration**: Need to combine datasets with simulation outputs across heterogeneous computing environments
- **Inefficient workflows**: Current approaches require:
  - Data conversion between formats
  - Separate processing pipelines for each format
  - Specialized custom solutions for each data type

## The Solution

NEP provides a transparent, runtime-pluggable solution:

### Core Features
- **Automatic format detection**: NetCDF automatically identifies NASA format files at read time
- **Dynamic format loading**: Appropriate User Defined Format handlers are loaded on-demand
- **Transparent access**: Users can work with any supported format using the familiar NetCDF API
- **No conversion required**: Direct access to original data without preprocessing

### Supported Formats

| Format | Library |
|--------|---------|
| GRIB2 | [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2) |
| BUFR | [NCEPLIBS-bufr](https://github.com/NOAA-EMC/NCEPLIBS-bufr) |
| CDF | [NASA CDF](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html) |
| GeoTIFF | [libgeotiff](https://github.com/OSGeo/libgeotiff) |

## Key Benefits

- **Unified Interface**: Use existing NetCDF code with any supported format
- **Performance**: Preserves format-specific optimizations while providing standardized access
- **Efficiency**: Reduces storage requirements and computational overhead
- **Compatibility**: Works with existing scientific codebases without modification
- **Scalability**: Designed for exascale computing environments

---

### Visual Overview

![NEP with User Defined Formats for NetCDF Users](docs/images/NFEP%20with%20VOLs%20for%20NetCDF%20Users.png)
*Figure: NEP with User Defined Formats for NetCDF Users*

## Installation

### Prerequisites

NEP requires the following dependencies:

- **NetCDF-C library** (v4.9+)
- **HDF5 library** (v1.12+)
- **CMake** (v3.9+) or **Autotools** for building
- **Format-specific libraries** (optional, based on desired UDF handlers):
  - GRIB2: [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2)
  - BUFR: [NCEPLIBS-bufr](https://github.com/NOAA-EMC/NCEPLIBS-bufr)
  - GeoTIFF: [libgeotiff](https://github.com/OSGeo/libgeotiff)
  - CDF: [NASA CDF library](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html)

### Test Data

NEP includes test data files in the `test/data/` directory for validation:
- **GRIB2**: `gdaswave.t00z.wcoast.0p16.f000.grib2` - Sample meteorological data file
- Additional test files will be added for BUFR, CDF, and GeoTIFF formats in future releases

The build system automatically copies test data files to the build directory so tests can access them during both in-tree and out-of-tree builds.

### CMake Build and Installation

```bash
# Configure with desired UDF handlers
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DENABLE_GRIB2=ON \
    -DENABLE_BUFR=ON \
    -DENABLE_GEOTIFF=ON \
    -DENABLE_CDF=ON

# Build
cmake --build build

# Install
cmake --install build

# Uninstall (if needed)
cmake --build build --target uninstall
```

### Autotools Build and Installation

```bash
# Configure with desired UDF handlers
./configure --prefix=/usr/local \
    --enable-grib2 \
    --enable-bufr \
    --enable-geotiff \
    --enable-cdf

# Build
make

# Install
make install

# Uninstall (if needed)
make uninstall
```

### Configuration Options

Both build systems support enable/disable options for each UDF handler:

| CMake Option | Autotools Option | Default | Description |
|--------------|------------------|---------|-------------|
| `-DENABLE_GRIB2=ON/OFF` | `--enable-grib2/--disable-grib2` | ON/enabled | GRIB2 UDF handler |
| `-DENABLE_BUFR=ON/OFF` | `--enable-bufr/--disable-bufr` | ON/enabled | BUFR UDF handler |
| `-DENABLE_GEOTIFF=ON/OFF` | `--enable-geotiff/--disable-geotiff` | ON/enabled | GeoTIFF UDF handler |
| `-DENABLE_CDF=ON/OFF` | `--enable-cdf/--disable-cdf` | ON/enabled | CDF UDF handler |

### Using NEP in Your Project

After installation, you can use NEP in your CMake projects:

```cmake
find_package(NEP REQUIRED)
target_link_libraries(your_target PRIVATE NEP::grib2_udf)
```

Or link directly with the installed shared libraries:

```bash
gcc -o myapp myapp.c -L/usr/local/lib -lgrib2_udf
```

---

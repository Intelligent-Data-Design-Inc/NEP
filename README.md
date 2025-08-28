# NFEP (NetCDF4/HDF5 Format Extension Pack)

## Overview

NFEP is a unified framework that enables seamless access to diverse Earth Science data formats through standard NetCDF4 and HDF5 interfaces. It eliminates the need for data conversion or format-specific processing pipelines while preserving the optimizations of specialized formats.

## The Problem

NASA's Earth Science missions face significant challenges with data management:

- **Massive data volumes**: Unprecedented amounts of observational data in diverse formats
- **Complex integration**: Need to combine datasets with simulation outputs across heterogeneous computing environments
- **Inefficient workflows**: Current approaches require:
  - Data conversion between formats
  - Separate processing pipelines for each format
  - Specialized custom solutions for each data type

## The Solution

NFEP provides a transparent, runtime-pluggable solution:

### Core Features
- **Automatic format detection**: HDF5 automatically identifies NASA format files at read time
- **Dynamic connector loading**: Appropriate file format connectors are loaded on-demand
- **Transparent access**: Users can work with any supported format using familiar HDF5/NetCDF4 APIs
- **No conversion required**: Direct access to original data without preprocessing

### Supported Formats

| Format | Library |
|--------|---------|
| GRIB2 | [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2) |
| BUFR | [NCEPLIBS-bufr](https://github.com/NOAA-EMC/NCEPLIBS-bufr) |
| CDF | [NASA CDF](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html) |
| GeoTIFF | [libgeotiff](https://github.com/OSGeo/libgeotiff) |

## Key Benefits

- **Unified Interface**: Use existing NetCDF4/HDF5 code with any supported format
- **Performance**: Preserves format-specific optimizations while providing standardized access
- **Efficiency**: Reduces storage requirements and computational overhead
- **Compatibility**: Works with existing scientific codebases without modification
- **Scalability**: Designed for exascale computing environments

---

### Visual Overview

![NFEP with VOLs for HDF5 Users](docs/images/NFEP%20with%20VOLs%20for%20HDF5%20Users.png)
*Figure 1: NFEP with VOLs for HDF5 Users*

![NFEP with VOLs for NetCDF Users](docs/images/NFEP%20with%20VOLs%20for%20NetCDF%20Users.png)
*Figure 2: NFEP with VOLs for NetCDF Users*

## Installation

### Prerequisites

NFEP requires the following dependencies:

- **HDF5 library** (v1.12+)
- **CMake** (v3.9+) or **Autotools** for building
- **Format-specific libraries** (optional, based on desired VOL connectors):
  - GRIB2: [NCEPLIBS-g2](https://github.com/NOAA-EMC/NCEPLIBS-g2)
  - BUFR: [NCEPLIBS-bufr](https://github.com/NOAA-EMC/NCEPLIBS-bufr)
  - GeoTIFF: [libgeotiff](https://github.com/OSGeo/libgeotiff)
  - CDF: [NASA CDF library](https://cdf.gsfc.nasa.gov/html/sw_and_docs.html)

### CMake Build and Installation

```bash
# Configure with desired VOL connectors
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
# Configure with desired VOL connectors
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

Both build systems support enable/disable options for each VOL connector:

| CMake Option | Autotools Option | Default | Description |
|--------------|------------------|---------|-------------|
| `-DENABLE_GRIB2=ON/OFF` | `--enable-grib2/--disable-grib2` | ON/enabled | GRIB2 VOL connector |
| `-DENABLE_BUFR=ON/OFF` | `--enable-bufr/--disable-bufr` | ON/enabled | BUFR VOL connector |
| `-DENABLE_GEOTIFF=ON/OFF` | `--enable-geotiff/--disable-geotiff` | ON/enabled | GeoTIFF VOL connector |
| `-DENABLE_CDF=ON/OFF` | `--enable-cdf/--disable-cdf` | ON/enabled | CDF VOL connector |

### Using NFEP in Your Project

After installation, you can use NFEP in your CMake projects:

```cmake
find_package(NFEP REQUIRED)
target_link_libraries(your_target PRIVATE NFEP::grib2_vol_connector)
```

Or link directly with the installed shared libraries:

```bash
gcc -o myapp myapp.c -L/usr/local/lib -lgrib2_vol_connector
```

---

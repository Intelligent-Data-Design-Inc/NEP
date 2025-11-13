# NEP (NetCDF Extension Pack) v1.0.0

## Overview

NEP (NetCDF Extension Pack) provides high-performance LZ4 compression for HDF5/NetCDF-4 files. This release focuses on delivering blazing-fast lossless compression optimized for scientific data workflows.

## LZ4 Compression

LZ4 is a lossless data compression algorithm from the LZ77 family of byte-oriented compression schemes. It prioritizes compression and decompression speed while maintaining good compression ratios.

### Key Features

- **High-Speed Compression**: 2-3x faster compression/decompression than DEFLATE
- **Lossless**: Guarantees data integrity with no data loss
- **Optimized for HPC**: Designed for speed-critical workflows in high-performance computing environments
- **Transparent Integration**: Works seamlessly with existing NetCDF-4/HDF5 applications
- **HDF5 Filter Plugin**: Automatically available through HDF5_PLUGIN_PATH mechanism

### Performance Characteristics

- **Compression Speed**: Similar to LZO, several times faster than DEFLATE
- **Decompression Speed**: Significantly faster than LZO
- **Compression Ratio**: Good trade-off between speed and size reduction
- **Use Case**: Ideal for I/O-bound scientific workflows where speed matters more than maximum compression

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+)
- LZ4 library

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

Once installed, the LZ4 compression filter is automatically available to NetCDF-4 applications through the HDF5 plugin system. No code changes are required to use LZ4 compression with your existing NetCDF-4 files.

## Future Releases

Future versions of NEP will add User Defined Format (UDF) handlers for:
- **v2.0.0**: GRIB2 format support
- **v3.0**: CDF format support
- **v4.0**: GeoTIFF format support

## API Documentation

This documentation covers the internal API for NEP v1.0.0. For general usage information, see the project README.

## License

See the COPYRIGHT file for licensing information.

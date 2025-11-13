# NEP (NetCDF Extension Pack) v1.0.0

## Overview

NEP (NetCDF Extension Pack) provides high-performance compression for HDF5/NetCDF-4 files with two complementary algorithms: LZ4 (optimized for speed) and BZIP2 (optimized for compression ratio). This release delivers flexible lossless compression options for diverse scientific data workflows.

## Compression Algorithms

### LZ4 Compression

LZ4 is a lossless data compression algorithm from the LZ77 family of byte-oriented compression schemes. It prioritizes compression and decompression speed while maintaining good compression ratios.

**Key Features:**
- **High-Speed Compression**: 2-3x faster compression/decompression than DEFLATE
- **Lossless**: Guarantees data integrity with no data loss
- **Optimized for HPC**: Designed for speed-critical workflows in high-performance computing environments
- **Transparent Integration**: Works seamlessly with existing NetCDF-4/HDF5 applications
- **HDF5 Filter Plugin**: Automatically available through HDF5_PLUGIN_PATH mechanism

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
- **Transparent Integration**: Works seamlessly with existing NetCDF-4/HDF5 applications
- **HDF5 Filter Plugin**: Automatically available through HDF5_PLUGIN_PATH mechanism

**Performance Characteristics:**
- **Compression Ratio**: Exceeds DEFLATE, ideal for storage optimization
- **Compression Speed**: Slower than LZ4 but faster than most high-ratio algorithms
- **Decompression Speed**: Moderate, suitable for archival workflows
- **Use Case**: Ideal for long-term archival storage, bandwidth-constrained transfers, and storage-limited environments

## Choosing the Right Algorithm

- **Use LZ4 when**: I/O speed is critical, working with real-time data, running on HPC systems, need fast decompression
- **Use BZIP2 when**: Storage space is limited, archiving data long-term, transferring over slow networks, compression ratio matters most

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+)
- LZ4 library
- BZIP2 library

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

Once installed, both LZ4 and BZIP2 compression filters are automatically available to NetCDF-4 applications through the HDF5 plugin system. No code changes are required to use either compression algorithm with your existing NetCDF-4 files. Choose the algorithm that best fits your workflow requirements—LZ4 for speed or BZIP2 for compression ratio.

## Future Releases

Future versions of NEP will add User Defined Format (UDF) handlers for:
- **v2.0.0**: GRIB2 format support
- **v3.0**: CDF format support
- **v4.0**: GeoTIFF format support

## API Documentation

This documentation covers the internal API for NEP v1.0.0. For general usage information, see the project README.

## License

See the COPYRIGHT file for licensing information.

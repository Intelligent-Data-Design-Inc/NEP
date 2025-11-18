# NEP (NetCDF Extension Pack)

## Overview

NEP (NetCDF Extension Pack) provides high-performance compression for NetCDF-4 data with two complementary algorithms: LZ4 (optimized for speed) and BZIP2 (optimized for compression ratio). NEP extends the NetCDF ecosystem with flexible lossless compression options for diverse scientific data workflows.

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

## Installation

NEP requires:
- NetCDF-C library (v4.9+)
- HDF5 library (v1.12+) and its dependencies
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

Once installed, both LZ4 and BZIP2 compression filters are available to NetCDF-4 applications through the configured NEP libraries. Choose the algorithm that best fits your workflow requirements—LZ4 for speed or BZIP2 for compression ratio.

## Features and Options

- **Compression Algorithms**:
  - LZ4: speed-optimized lossless compression.
  - BZIP2: high-ratio lossless compression.
- **Configurable Compression Support**:
  - LZ4 and BZIP2 support can be enabled or disabled at build time.
- **Fortran Support**:
  - Optional Fortran wrappers for NEP compression functions.
  - When Fortran is enabled, the `netcdf-fortran` library is required.

See the dedicated build and configuration page for full details on CMake and Autotools options:

- `docs/build-options.md`

## API Documentation

The API documentation is generated directly from the C and Fortran source code.

- **Versioned API docs**: Documentation is published per release (for example, `/NEP/v1.2.0/api/`).
- **Latest API docs**: A `latest` alias points to the most recent release documentation.

Refer to the documentation header for the exact version of NEP corresponding to this build.

## License

See the COPYRIGHT file for licensing information.

# NEP Spack Installation Guide

## Overview

NEP can be installed using the Spack package manager for simplified dependency management in HPC environments.

**Status**: NEP Spack package submitted to spack/spack-packages repository (PR pending). CDF Spack package submitted to spack/spack-packages repository (PR pending).

## Basic Installation

```bash
spack install nep
```

## Installation Variants

### Disable Documentation
```bash
spack install nep~docs
```

### Compression Filter Options
```bash
# Only LZ4
spack install nep+lz4~bzip2

# Only BZIP2
spack install nep~lz4+bzip2
```

### Fortran Wrappers
```bash
# With Fortran wrappers (default)
spack install nep+fortran

# Without Fortran wrappers
spack install nep~fortran
```

### CDF Support
```bash
# Note: CDF variant temporarily removed from NEP package until CDF package is accepted into Spack
# CDF can be installed separately:
spack install cdf

# Once CDF package is in Spack, NEP will add back the +cdf variant
```

## Using NEP

Load NEP into your environment:
```bash
spack load nep
```

Set HDF5 plugin path:
```bash
export HDF5_PLUGIN_PATH=$(spack location -i nep)/lib/plugin
```

## Integration with Other Packages

```bash
# Install with specific NetCDF version
spack install nep ^netcdf-c@4.9.2

# Install with specific HDF5 version
spack install nep ^hdf5@1.14.0
```

## CDF Package

NASA CDF library is available as a separate Spack package:

```bash
# Install CDF
spack install cdf

# Load CDF
spack load cdf
```

The CDF package uses a custom Makefile build system and has no external dependencies beyond a system compiler.

## Development

### Package Locations

- **NEP Package**: `spack/NEP/package.py`
- **CDF Package**: `spack/cdf/package.py`

### CI Testing

Both packages have dedicated CI workflows:
- **NEP**: `.github/workflows/spack.yml`
- **CDF**: `.github/workflows/spack-cdf.yml`

The CI workflows test:
- Style and lint checks
- Package spec resolution
- Installation on Ubuntu with system compilers

## Troubleshooting

### Plugin Not Found
Ensure HDF5_PLUGIN_PATH is set correctly:
```bash
echo $HDF5_PLUGIN_PATH
ls $HDF5_PLUGIN_PATH
```

### Build Failures
Check Spack build log:
```bash
spack install --verbose nep
```

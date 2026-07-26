# NEP Spack Installation Guide

## Overview

NEP can be installed using the Spack package manager for simplified dependency management in HPC environments.

**Status**: The NEP package update is tracked in [spack/spack-packages#5557](https://github.com/spack/spack-packages/pull/5557). The in-repository recipe matches the upstream-compatible variant set.

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
spack install nep ^netcdf-c@4.10.1

# Install with specific HDF5 version
spack install nep ^hdf5@2.1.1
```

## Development

### Package Locations

- **NEP Package**: `spack/NEP/package.py`

### CI Testing

Both packages have dedicated CI workflows:
- **NEP**: `.github/workflows/spack.yml`

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

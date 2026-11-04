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

### Compression

NEP no longer ships LZ4/BZIP2 filters. See
[nep-compress](https://github.com/captainkirk99/nep-compress) for their
separate C/Fortran APIs and HDF5 plugin installation.

### Fortran Format Tests
```bash
# With Fortran format-reader tests (default)
spack install nep+fortran

# Without Fortran format-reader tests
spack install nep~fortran
```

## Using NEP

Load NEP into your environment:
```bash
spack load nep
```

Only set `HDF5_PLUGIN_PATH` if another project, such as nep-compress,
provides plugins for your application.

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
NEP does not install HDF5 compression plugins. Check the installation
and `HDF5_PLUGIN_PATH` instructions for the separate plugin provider.

### Build Failures
Check Spack build log:
```bash
spack install --verbose nep
```

# NEP Spack Installation Guide

## Overview

NEP can be installed using the Spack package manager for simplified dependency management in HPC environments.

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
# Enable NASA CDF format support
spack install nep+cdf

# Default (CDF disabled)
spack install nep~cdf
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

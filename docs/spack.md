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

### Build System
The Spack package uses the CMake build system only. For Autotools builds, use the standard build instructions in the main README.

### CDF Format Support
CDF format support is available in NEP v1.3.0+, but the CDF variant is not yet available in the Spack package because the CDF library is not currently in Spack. To use CDF support, build NEP manually with the `ENABLE_CDF` CMake option.

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

## Package Maintenance

The NEP Spack package is maintained in the official Spack repository at https://github.com/spack/spack.

### Updating for New Releases

When a new NEP version is released:

1. Calculate SHA256 checksum for the new release tarball:
   ```bash
   wget https://github.com/Intelligent-Data-Design-Inc/NEP/archive/vX.Y.Z.tar.gz
   sha256sum vX.Y.Z.tar.gz
   ```

2. Update `spack/package.py` in the NEP repository with the new version

3. Submit a PR to the spack/spack repository:
   - Fork https://github.com/spack/spack
   - Update `var/spack/repos/builtin/packages/nep/package.py`
   - Add the new version line with SHA256 checksum
   - Test the package builds correctly
   - Submit PR with clear description

4. Respond to reviewer feedback and iterate as needed

### Maintainers

- Ed Hartnett (@edhartnett)

### Testing Package Changes

Test package.py changes locally before submission:

```bash
# Install Spack if not already installed
git clone https://github.com/spack/spack.git ~/spack
. ~/spack/share/spack/setup-env.sh

# Test local package.py
spack install --test=root ./spack/package.py

# Test specific variants
spack install ./spack/package.py~docs
spack install ./spack/package.py+lz4~bzip2

# Verify installation
spack find nep
spack load nep
```

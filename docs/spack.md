# NEP Spack Installation Guide

## Overview

NEP can be installed using the Spack package manager for simplified dependency management in HPC environments.

**Status**: The NEP package update is tracked in [spack/spack-packages#5557](https://github.com/spack/spack-packages/pull/5557), which targets NEP 4.2.0 and waits on [spack/spack-packages#6569](https://github.com/spack/spack-packages/pull/6569) for `netcdf-c@4.10.1`. Until #6569 merges, CI concretizes against `^netcdf-c@main`.

## Basic Installation

```bash
spack install nep@4.2.0
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

### Format Readers

The `+geotiff` variant requires `nep@main` or a release after 4.2.0
(4.2.0 needs libgeotiff headers under `include/geotiff/`, which the
Spack `libgeotiff` package does not install).

```bash
# All Spack-supported format readers
spack install nep+geotiff+grib2+fits+pds4+pdb+mmcif

# NEXTCDF-4 UDF backend
spack install nep+nextcdf4
```

The CDF and DICOM readers are not available through Spack: there is no
`cdf` package yet ([spack/spack-packages#2797](https://github.com/spack/spack-packages/pull/2797))
and no `gdcm` package for the DICOM dependencies. The recipe keeps the
`cdf` variant commented out until #2797 merges.

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

Enabled format readers are registered through the `share/.ncrc` file the
package installs. Only set `HDF5_PLUGIN_PATH` if another project, such as
nep-compress, provides plugins for your application.

## Integration with Other Packages

```bash
# Install with specific NetCDF version
spack install nep ^netcdf-c@4.10.1

# Install with specific HDF5 version
spack install nep ^hdf5@2.1.1
```

## Development

### Package Locations

- **NEP recipe**: the overlay repository at
  `spack/repos/spack_repo/nep/packages/nep/package.py`. Register it with:

  ```bash
  spack repo add spack/repos/spack_repo/nep
  ```

- **CI environment**: `spack/ci/spack.yaml` pins the spec that
  `.github/workflows/spack.yml` builds with `install --test=root`.

### CI Testing

The `Spack CI` workflow (`.github/workflows/spack.yml`) runs on every
push and pull request:

- A `style` job running the upstream ruff/vermin checks
  (`ruff format --diff`, `ruff check --no-fix`, and `vermin`), using the
  `pyproject.toml` from the pinned spack-packages checkout.
- A `spack` job that audits the recipe, checks key specs, concretizes
  the pinned `spack/ci` environment (Spack `v1.2.2`, spack-packages
  `v2026.10.0`), and runs `spack install --test=root` on the checked-out
  source via a `develop:` spec.

## Updating the upstream PR

The upstream copy of the recipe lives in
[spack/spack-packages#5557](https://github.com/spack/spack-packages/pull/5557)
(branch `nep-update-v2.5.0` on the fork). To update it:

1. `git fetch upstream`, `git fetch origin`,
   `git switch nep-update-v2.5.0`, `git rebase upstream/develop`.
2. Copy `spack/repos/spack_repo/nep/packages/nep/package.py` over
   `repos/spack_repo/builtin/packages/nep/package.py` and confirm
   `diff` is empty — the two files are byte-identical.
3. Run the style checks (ruff format, ruff check, vermin) against the
   spack-packages `pyproject.toml`.
4. Commit and push to `origin nep-update-v2.5.0`, then update the PR
   title/body.

## Troubleshooting

### Plugin Not Found
NEP does not install HDF5 compression plugins. Check the installation
and `HDF5_PLUGIN_PATH` instructions for the separate plugin provider.

### Build Failures
Check Spack build log:
```bash
spack install --verbose nep
```

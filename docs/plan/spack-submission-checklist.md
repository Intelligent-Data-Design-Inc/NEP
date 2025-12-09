# Spack Repository Submission Checklist

This checklist guides the process of submitting the NEP package to the official Spack repository at https://github.com/spack/spack.

## Prerequisites

- [ ] v1.3.0 release published on GitHub
- [ ] Release tarballs available and tested
- [ ] SHA256 checksums verified
- [ ] Local Spack testing completed successfully
- [ ] CI Spack workflow passing

## Preparation Steps

### 1. Verify Package File
- [ ] `spack/package.py` contains v1.0.0 and v1.3.0 with correct checksums
- [ ] All variants documented and tested
- [ ] CDF variant included with conditional dependency
- [ ] Package description accurate and complete
- [ ] Maintainer information correct

### 2. Local Testing
- [ ] Install Spack locally: `git clone https://github.com/spack/spack.git ~/spack`
- [ ] Source Spack environment: `. ~/spack/share/spack/setup-env.sh`
- [ ] Test default build: `spack install --test=root ./spack/package.py`
- [ ] Test all variants:
  - [ ] `spack install ./spack/package.py~docs`
  - [ ] `spack install ./spack/package.py+lz4~bzip2`
  - [ ] `spack install ./spack/package.py~lz4+bzip2`
  - [ ] `spack install ./spack/package.py+cdf` (if CDF available in Spack)
- [ ] Verify installations: `spack find nep`
- [ ] Test loading: `spack load nep`
- [ ] Verify plugin files exist in expected locations

### 3. Package Validation
- [ ] Run style check: `spack style ./spack/package.py`
- [ ] Run audit: `spack audit packages nep` (after copying to Spack repo)
- [ ] Check package info: `spack info nep`
- [ ] Verify all variants listed correctly
- [ ] Verify all dependencies listed correctly
- [ ] No Python syntax errors

## Submission Process

### 1. Fork Spack Repository
- [ ] Fork https://github.com/spack/spack to your GitHub account
- [ ] Clone your fork: `git clone https://github.com/YOUR_USERNAME/spack.git`
- [ ] Add upstream remote: `git remote add upstream https://github.com/spack/spack.git`
- [ ] Create feature branch: `git checkout -b add-nep-package`

### 2. Add NEP Package
- [ ] Create package directory: `mkdir -p var/spack/repos/builtin/packages/nep`
- [ ] Copy package file: `cp /path/to/NEP/spack/package.py var/spack/repos/builtin/packages/nep/`
- [ ] Verify file location: `ls var/spack/repos/builtin/packages/nep/package.py`

### 3. Test in Spack Repository
- [ ] Source Spack from your fork: `. share/spack/setup-env.sh`
- [ ] Test package install: `spack install --test=root nep@1.3.0`
- [ ] Test package install: `spack install --test=root nep@1.0.0`
- [ ] Run audit: `spack audit packages nep`
- [ ] Run style check: `spack style var/spack/repos/builtin/packages/nep/package.py`
- [ ] Verify no errors or warnings

### 4. Commit and Push
- [ ] Stage changes: `git add var/spack/repos/builtin/packages/nep/package.py`
- [ ] Commit with clear message: `git commit -m "nep: new package for NetCDF Extension Pack"`
- [ ] Push to your fork: `git push origin add-nep-package`

### 5. Create Pull Request
- [ ] Go to https://github.com/spack/spack/pulls
- [ ] Click "New pull request"
- [ ] Select your fork and branch
- [ ] Fill out PR template with:
  - [ ] Package name and description
  - [ ] Link to NEP repository
  - [ ] Link to NEP documentation
  - [ ] Tested versions (1.0.0, 1.3.0)
  - [ ] Tested variants
  - [ ] Any special notes (e.g., CDF dependency)
- [ ] Submit pull request

## PR Description Template

```markdown
## Description

This PR adds the NEP (NetCDF Extension Pack) package to Spack.

NEP provides high-performance compression for HDF5/NetCDF-4 files with LZ4 and BZIP2 compression algorithms, and extends NetCDF to support additional scientific data formats including NASA CDF.

**Homepage**: https://github.com/Intelligent-Data-Design-Inc/NEP
**Documentation**: https://github.com/Intelligent-Data-Design-Inc/NEP/tree/main/docs

## Versions Tested

- v1.3.0 (latest)
- v1.0.0

## Variants Tested

- Default build (all features enabled)
- `~docs` (documentation disabled)
- `+lz4~bzip2` (LZ4 only)
- `~lz4+bzip2` (BZIP2 only)
- `+cdf` (with CDF support, if CDF package available)

## Dependencies

- netcdf-c@4.9:
- hdf5@1.12:+hl
- lz4 (optional, default on)
- bzip2 (optional, default on)
- cdf (optional, default off)
- doxygen (optional, for docs)

## Build System

CMake only

## Notes

- CDF support requires the `cdf` package in Spack. If not available, users can disable with `~cdf` (default).
- Package includes install verification via `check_install()` method.
- Tested on Ubuntu latest with Spack develop branch.

## Checklist

- [x] Package builds with `spack install nep`
- [x] All variants tested
- [x] `spack audit packages nep` passes
- [x] `spack style` passes
- [x] Package includes proper license information
- [x] Maintainer listed (@edhartnett)
```

## Post-Submission

### Respond to Reviews
- [ ] Monitor PR for reviewer comments
- [ ] Respond to feedback promptly
- [ ] Make requested changes if needed
- [ ] Push updates to same branch
- [ ] Re-test after changes

### After Merge
- [ ] Update NEP documentation to reference official Spack package
- [ ] Add note in README about Spack availability
- [ ] Close any related issues
- [ ] Announce availability to users

## CDF Dependency Note

**Important**: The `cdf` package may not exist in Spack. Options:

1. **Create CDF package first**: Submit a separate PR for CDF package before NEP
2. **Make CDF external**: Document that users need to install CDF externally
3. **Skip CDF initially**: Submit NEP without CDF variant, add it later

**Recommended**: Check if CDF exists in Spack first:
```bash
spack list cdf
spack info cdf
```

If CDF doesn't exist, consider submitting NEP without the CDF variant initially, or create a CDF package PR first.

## Troubleshooting

### Package Build Fails
- Check that all dependencies are available in Spack
- Verify SHA256 checksums are correct
- Test with `spack install --verbose nep` for detailed output
- Check Spack version compatibility

### Style Check Fails
- Run `spack style --fix var/spack/repos/builtin/packages/nep/package.py`
- Manually fix any remaining issues
- Re-run `spack style` to verify

### Audit Fails
- Review audit output carefully
- Fix any reported issues
- Common issues: missing dependencies, incorrect version format, license problems

## References

- Spack Documentation: https://spack.readthedocs.io/
- Spack Contribution Guide: https://spack.readthedocs.io/en/latest/contribution_guide.html
- Spack Package Guide: https://spack.readthedocs.io/en/latest/packaging_guide.html
- NEP Repository: https://github.com/Intelligent-Data-Design-Inc/NEP

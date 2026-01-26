# Issue #99: Example Output Validation with CDL Comparison - Summary

## Issue Details
- **Number**: #99
- **Title**: v1.5.1 Sprint 2: Example Output Validation with CDL Comparison
- **Status**: Open
- **Labels**: enhancement, v1.5.1, sprint-2
- **Dependencies**: Sprint 1 (examples must be integrated and running)

## Analysis Results

### Current Infrastructure
- **22 example programs** (11 C + 11 Fortran)
  - C: 6 classic + 5 NetCDF-4
  - Fortran: 6 classic + 5 NetCDF-4
- **Build systems**: CMake and Autotools with test integration
- **Current testing**: Only verifies successful execution (exit code 0)
- **No output validation** currently implemented

### Multi-File Examples Identified
Several examples create multiple output files requiring multiple CDL baselines:
- `format_variants`: 3 files (classic, 64-bit offset, CDF-5)
- `size_limits`: 3 files (classic, 64-bit offset, CDF-5)
- `compression`: 6 files (various compression settings)
- `chunking_performance`: 4 files (various chunking strategies)

**Total CDL files needed**: ~50 files

## Implementation Strategy

### 6-Phase Approach

1. **Phase 1: CDL Reference Files** - Generate baseline CDL files for all example outputs
2. **Phase 2: Validation Script** - Create reusable `validate_cdl.sh` script
3. **Phase 3: CMake Integration** - Add CDL validation to CMake test framework
4. **Phase 4: Autotools Integration** - Add wrapper scripts for Autotools tests
5. **Phase 5: Documentation** - Update README with validation details
6. **Phase 6: CI Verification** - Verify existing CI runs new validation tests

### Key Technical Decisions

**Validation Approach:**
- Run example → Generate CDL with `ncdump` → Compare with baseline using `diff`
- Store baselines in `examples/expected_output/` directory
- Naming: `<example_name>_expected.cdl` or `<example_name>_<variant>_expected.cdl`

**Build System Integration:**
- **CMake**: Combine example execution + validation in single test command
- **Autotools**: Create wrapper scripts that run example then validate output
- Both approaches ensure tests fail if output differs from expected

**Handling Challenges:**
- Timestamps: May need `--ignore-matching-lines` if timestamps appear in CDL
- Floating-point precision: Monitor for platform-specific differences
- Attribute ordering: NetCDF doesn't guarantee order (may need sorted comparison)

## Implementation Checklist (26 tasks)

### CDL Reference Files (6 tasks)
- [ ] Create `examples/expected_output/` directory
- [ ] Generate CDL for C classic examples (~9 files)
- [ ] Generate CDL for C NetCDF-4 examples (~16 files)
- [ ] Generate CDL for Fortran classic examples (~9 files)
- [ ] Generate CDL for Fortran NetCDF-4 examples (~16 files)
- [ ] Add all CDL files to repository

### Test Infrastructure (3 tasks)
- [ ] Create `examples/validate_cdl.sh` script
- [ ] Create Autotools wrapper scripts (22 total)
- [ ] Make all scripts executable

### Build System Updates (8 tasks)
- [ ] Update `examples/classic/CMakeLists.txt`
- [ ] Update `examples/netcdf-4/CMakeLists.txt`
- [ ] Update `examples/f_classic/CMakeLists.txt`
- [ ] Update `examples/f_netcdf-4/CMakeLists.txt`
- [ ] Update `examples/classic/Makefile.am`
- [ ] Update `examples/netcdf-4/Makefile.am`
- [ ] Update `examples/f_classic/Makefile.am`
- [ ] Update `examples/f_netcdf-4/Makefile.am`

### Documentation & Testing (5 tasks)
- [ ] Update `examples/README.md` with validation documentation
- [ ] Test CMake build with `ctest`
- [ ] Test Autotools build with `make check`
- [ ] Verify CI pipeline execution
- [ ] Test both in-tree and out-of-tree builds

## Success Criteria (8 items)

- [ ] All 22 example programs have corresponding expected CDL files
- [ ] CMake tests validate example output against CDL baselines
- [ ] Autotools tests validate example output against CDL baselines
- [ ] Tests fail appropriately when output differs from expected
- [ ] Documentation explains validation approach and baseline regeneration
- [ ] CI pipeline validates all examples automatically
- [ ] Both C and Fortran examples have CDL validation
- [ ] Both in-tree and out-of-tree builds work correctly

## Estimated Effort

- CDL Generation: 2-3 hours
- Script Creation: 2-3 hours
- CMake Integration: 3-4 hours
- Autotools Integration: 3-4 hours
- Documentation: 1-2 hours
- Testing & Validation: 2-3 hours

**Total: 13-19 hours**

## Files Created

1. `.windsurf/issue-99-implementation-plan.md` - Detailed implementation plan (382 lines)
2. `.windsurf/issue-99-summary.md` - This summary document

## Next Steps

1. Wait for Sprint 1 (Issue #98) completion
2. Begin Phase 1: Generate CDL reference files
3. Proceed through phases sequentially
4. Test thoroughly with both build systems
5. Verify CI integration

## Notes

- No additional dependencies required (`ncdump` is part of NetCDF-C)
- Performance impact minimal (~2-3 seconds total for all examples)
- Approach provides comprehensive regression testing for example outputs
- Compatible with both in-tree and out-of-tree builds

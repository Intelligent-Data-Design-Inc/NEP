# Issue #99 Implementation Plan: Example Output Validation with CDL Comparison

## Overview
Add automated validation of example program output by comparing generated NetCDF files against expected CDL (Common Data Language) baselines for regression testing.

## Current State Analysis

### Existing Infrastructure
- **Examples**: 11 C examples (6 classic, 5 NetCDF-4) and 11 Fortran examples
- **Build Systems**: Both CMake and Autotools with test integration
- **Current Tests**: Examples run as tests but only verify successful execution (exit code 0)
- **Test Framework**: 
  - CMake: `add_test()` with CTest
  - Autotools: `TESTS` variable in Makefile.am with `make check`

### Example Programs
**C Classic Examples** (`examples/classic/`):
- simple_2D, coord_vars, format_variants, size_limits, unlimited_dim, var4d

**C NetCDF-4 Examples** (`examples/netcdf-4/`):
- simple_nc4, compression, chunking_performance, multi_unlimited, user_types

**Fortran Classic Examples** (`examples/f_classic/`):
- f_simple_2D, f_coord_vars, f_format_variants, f_size_limits, f_unlimited_dim, f_var4d

**Fortran NetCDF-4 Examples** (`examples/f_netcdf-4/`):
- f_simple_nc4, f_compression, f_chunking_performance, f_multi_unlimited, f_user_types

## Implementation Plan

### Phase 1: CDL Reference Files Generation

**Tasks**:
1. Run all example programs to generate NetCDF output files
2. Use `ncdump` to create CDL representation of each output file
3. Store CDL files in `examples/expected_output/` directory
4. Use naming convention: `<example_name>_expected.cdl`

**Special Considerations**:
- **format_variants**: Creates 3 files (classic, 64-bit offset, CDF-5) → 3 CDL files
- **size_limits**: Creates 3 files → 3 CDL files
- **compression**: Creates 6 files (none, deflate1, deflate5, deflate9, shuffle, shuffle+deflate5) → 6 CDL files
- **chunking_performance**: Creates 4 files (contiguous, time-optimized, spatial-optimized, balanced) → 4 CDL files
- **Total**: ~35-40 CDL files for all examples

**CDL Generation Command**:
```bash
ncdump <output_file>.nc > expected_output/<example_name>_expected.cdl
```

**Directory Structure**:
```
examples/
├── expected_output/
│   ├── simple_2D_expected.cdl
│   ├── coord_vars_expected.cdl
│   ├── format_variants_classic_expected.cdl
│   ├── format_variants_64bit_offset_expected.cdl
│   ├── format_variants_64bit_data_expected.cdl
│   ├── size_limits_classic_expected.cdl
│   ├── size_limits_64bit_offset_expected.cdl
│   ├── size_limits_64bit_data_expected.cdl
│   ├── unlimited_dim_expected.cdl
│   ├── var4d_expected.cdl
│   ├── simple_nc4_expected.cdl
│   ├── compression_none_expected.cdl
│   ├── compression_deflate1_expected.cdl
│   ├── compression_deflate5_expected.cdl
│   ├── compression_deflate9_expected.cdl
│   ├── compression_shuffle_expected.cdl
│   ├── compression_shuffle_deflate5_expected.cdl
│   ├── chunking_performance_contiguous_expected.cdl
│   ├── chunking_performance_time_optimized_expected.cdl
│   ├── chunking_performance_spatial_optimized_expected.cdl
│   ├── chunking_performance_balanced_expected.cdl
│   ├── multi_unlimited_expected.cdl
│   ├── user_types_expected.cdl
│   ├── f_simple_2D_expected.cdl
│   ├── f_coord_vars_expected.cdl
│   ├── f_format_variants_classic_expected.cdl
│   ├── f_format_variants_64bit_offset_expected.cdl
│   ├── f_format_variants_64bit_data_expected.cdl
│   ├── f_size_limits_classic_expected.cdl
│   ├── f_size_limits_64bit_offset_expected.cdl
│   ├── f_size_limits_64bit_data_expected.cdl
│   ├── f_unlimited_dim_expected.cdl
│   ├── f_var4d_expected.cdl
│   ├── f_simple_nc4_expected.cdl
│   ├── f_compression_none_expected.cdl
│   ├── f_compression_deflate1_expected.cdl
│   ├── f_compression_deflate5_expected.cdl
│   ├── f_compression_deflate9_expected.cdl
│   ├── f_compression_shuffle_expected.cdl
│   ├── f_compression_shuffle_deflate5_expected.cdl
│   ├── f_chunking_performance_contiguous_expected.cdl
│   ├── f_chunking_performance_time_optimized_expected.cdl
│   ├── f_chunking_performance_spatial_optimized_expected.cdl
│   ├── f_chunking_performance_balanced_expected.cdl
│   ├── f_multi_unlimited_expected.cdl
│   └── f_user_types_expected.cdl
└── README.md (updated)
```

### Phase 2: Test Script Creation

**Approach**: Create reusable shell scripts for CDL validation

**Script**: `examples/validate_cdl.sh`
```bash
#!/bin/bash
# Validate NetCDF output against expected CDL

EXAMPLE_NAME=$1
OUTPUT_FILE=$2
EXPECTED_CDL=$3

# Run ncdump on generated file
ncdump "$OUTPUT_FILE" > "${OUTPUT_FILE}.cdl" 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: ncdump failed on $OUTPUT_FILE"
    exit 1
fi

# Compare with expected CDL
diff -u "$EXPECTED_CDL" "${OUTPUT_FILE}.cdl"
if [ $? -ne 0 ]; then
    echo "ERROR: Output differs from expected for $EXAMPLE_NAME"
    echo "Expected: $EXPECTED_CDL"
    echo "Generated: ${OUTPUT_FILE}.cdl"
    exit 1
fi

echo "PASS: $EXAMPLE_NAME output matches expected CDL"
rm -f "${OUTPUT_FILE}.cdl"
exit 0
```

**Per-Example Wrapper Scripts** (for Autotools):
- Create `examples/classic/test_<example>.sh` for each example
- Script runs example, then validates each output file
- Example for `simple_2D.sh`:
```bash
#!/bin/bash
./simple_2D || exit 1
../validate_cdl.sh simple_2D simple_2D.nc ../expected_output/simple_2D_expected.cdl
```

### Phase 3: CMake Integration

**File**: `examples/classic/CMakeLists.txt` (and similar for other directories)

**Changes**:
1. Install validation script to build directory
2. Modify test commands to include CDL validation
3. Handle multi-file examples

**Implementation**:
```cmake
# Copy validation script to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../validate_cdl.sh 
               ${CMAKE_CURRENT_BINARY_DIR}/validate_cdl.sh 
               COPYONLY)

# For single-file examples
add_test(NAME simple_2D_run COMMAND simple_2D)
add_test(NAME simple_2D_validate 
         COMMAND bash validate_cdl.sh simple_2D simple_2D.nc 
                 ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/simple_2D_expected.cdl)
set_tests_properties(simple_2D_validate PROPERTIES DEPENDS simple_2D_run)

# For multi-file examples (format_variants)
add_test(NAME format_variants_run COMMAND format_variants)
add_test(NAME format_variants_validate_classic
         COMMAND bash validate_cdl.sh format_variants format_classic.nc
                 ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_classic_expected.cdl)
add_test(NAME format_variants_validate_64bit_offset
         COMMAND bash validate_cdl.sh format_variants format_64bit_offset.nc
                 ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_64bit_offset_expected.cdl)
add_test(NAME format_variants_validate_64bit_data
         COMMAND bash validate_cdl.sh format_variants format_64bit_data.nc
                 ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/format_variants_64bit_data_expected.cdl)
set_tests_properties(format_variants_validate_classic PROPERTIES DEPENDS format_variants_run)
set_tests_properties(format_variants_validate_64bit_offset PROPERTIES DEPENDS format_variants_run)
set_tests_properties(format_variants_validate_64bit_data PROPERTIES DEPENDS format_variants_run)
```

**Alternative Approach** (simpler):
```cmake
# Create custom test that runs example + validation in one step
add_test(NAME simple_2D 
         COMMAND bash -c "./simple_2D && bash validate_cdl.sh simple_2D simple_2D.nc ${CMAKE_CURRENT_SOURCE_DIR}/../expected_output/simple_2D_expected.cdl")
```

### Phase 4: Autotools Integration

**File**: `examples/classic/Makefile.am` (and similar for other directories)

**Changes**:
1. Create wrapper test scripts for each example
2. Add scripts to TESTS variable
3. Ensure scripts are executable

**Implementation**:
```makefile
# Test wrapper scripts
TESTS = test_simple_2D.sh test_coord_vars.sh test_format_variants.sh \
        test_size_limits.sh test_unlimited_dim.sh test_var4d.sh

# Make scripts executable
EXTRA_DIST = test_simple_2D.sh test_coord_vars.sh test_format_variants.sh \
             test_size_limits.sh test_unlimited_dim.sh test_var4d.sh

# Clean up generated CDL files
CLEANFILES = *.nc *.cdl
```

**Example Wrapper Script** (`test_simple_2D.sh`):
```bash
#!/bin/bash
set -e
./simple_2D
bash ../validate_cdl.sh simple_2D simple_2D.nc ../expected_output/simple_2D_expected.cdl
```

### Phase 5: Documentation Updates

**File**: `examples/README.md`

**Additions**:
```markdown
## Output Validation

Starting in v1.5.1 Sprint 2, example output is validated against expected CDL (Common Data Language) files stored in `expected_output/`. This ensures examples continue producing correct output across code changes.

### CDL Validation Process

1. Each example program runs and generates NetCDF output files
2. `ncdump` converts the NetCDF files to CDL text format
3. The generated CDL is compared against reference CDL files
4. Tests fail if output differs from expected (indicating a regression)

### Regenerating Expected CDL Files

If you intentionally change example output (e.g., adding new variables or attributes), you must regenerate the expected CDL files:

```bash
# Run the example
cd examples/classic
./simple_2D

# Generate new expected CDL
ncdump simple_2D.nc > ../expected_output/simple_2D_expected.cdl

# Verify the change is intentional by reviewing the diff
git diff ../expected_output/simple_2D_expected.cdl
```

### Troubleshooting Validation Failures

**Common causes**:
- Code changes that unintentionally modify output format
- Changes to NetCDF library behavior
- Platform-specific differences (endianness, floating-point precision)

**Debugging steps**:
1. Run the example manually: `./simple_2D`
2. Generate CDL from output: `ncdump simple_2D.nc > actual.cdl`
3. Compare with expected: `diff -u ../expected_output/simple_2D_expected.cdl actual.cdl`
4. Review differences to determine if they are expected or indicate a bug

### Multi-File Examples

Some examples create multiple output files:
- **format_variants**: Creates 3 files (classic, 64-bit offset, CDF-5)
- **size_limits**: Creates 3 files (classic, 64-bit offset, CDF-5)
- **compression**: Creates 6 files (various compression settings)
- **chunking_performance**: Creates 4 files (various chunking strategies)

Each output file has a corresponding expected CDL file in `expected_output/`.
```

### Phase 6: CI Integration Verification

**No changes needed** - CI already runs `make check` (Autotools) and `ctest` (CMake)

**Verification**:
1. CI workflows in `.github/workflows/` already execute tests
2. CDL validation will automatically run as part of existing test suite
3. CI will fail if any example output changes unexpectedly

## Implementation Checklist

### CDL Reference Files
- [ ] Create `examples/expected_output/` directory
- [ ] Run all C classic examples and generate CDL files (6 examples → ~9 CDL files)
- [ ] Run all C NetCDF-4 examples and generate CDL files (5 examples → ~16 CDL files)
- [ ] Run all Fortran classic examples and generate CDL files (6 examples → ~9 CDL files)
- [ ] Run all Fortran NetCDF-4 examples and generate CDL files (5 examples → ~16 CDL files)
- [ ] Add CDL files to git repository

### Test Scripts
- [ ] Create `examples/validate_cdl.sh` validation script
- [ ] Make validation script executable
- [ ] Create wrapper scripts for Autotools (22 scripts total)
- [ ] Make wrapper scripts executable

### CMake Updates
- [ ] Update `examples/classic/CMakeLists.txt` with CDL validation
- [ ] Update `examples/netcdf-4/CMakeLists.txt` with CDL validation
- [ ] Update `examples/f_classic/CMakeLists.txt` with CDL validation
- [ ] Update `examples/f_netcdf-4/CMakeLists.txt` with CDL validation
- [ ] Test CMake build with `ctest`

### Autotools Updates
- [ ] Update `examples/classic/Makefile.am` with test scripts
- [ ] Update `examples/netcdf-4/Makefile.am` with test scripts
- [ ] Update `examples/f_classic/Makefile.am` with test scripts
- [ ] Update `examples/f_netcdf-4/Makefile.am` with test scripts
- [ ] Test Autotools build with `make check`

### Documentation
- [ ] Update `examples/README.md` with CDL validation documentation
- [ ] Add troubleshooting section
- [ ] Document how to regenerate expected CDL files
- [ ] Explain multi-file example handling

### Testing & Validation
- [ ] Verify all tests pass with CMake build
- [ ] Verify all tests pass with Autotools build
- [ ] Test intentional output change and CDL regeneration workflow
- [ ] Verify CI pipeline runs CDL validation
- [ ] Test both in-tree and out-of-tree builds

## Technical Considerations

### ncdump Availability
- `ncdump` is part of NetCDF-C package (required dependency)
- Available in all environments where examples can run
- No additional dependencies needed

### CDL Comparison Challenges
- **Timestamps**: Some NetCDF files include creation timestamps → may need filtering
- **Floating-point precision**: Platform differences may cause minor variations
- **Attribute order**: NetCDF doesn't guarantee attribute order → may need sorted comparison
- **Solution**: Use `diff -u` for human-readable output, consider `--ignore-matching-lines` for timestamps

### Build System Compatibility
- **In-tree builds**: Scripts reference `../expected_output/` relative path
- **Out-of-tree builds**: CMake uses `${CMAKE_CURRENT_SOURCE_DIR}` for absolute paths
- **VPATH builds**: Autotools handles source vs. build directory separation

### Performance Impact
- Running `ncdump` adds minimal overhead (~100ms per file)
- Total test time increase: ~2-3 seconds for all examples
- Acceptable for comprehensive regression testing

## Success Criteria

- [ ] All 22 example programs have corresponding expected CDL files
- [ ] CMake tests validate example output against CDL baselines
- [ ] Autotools tests validate example output against CDL baselines
- [ ] Tests fail appropriately when output differs from expected
- [ ] Documentation explains validation approach and baseline regeneration
- [ ] CI pipeline validates all examples automatically
- [ ] Both C and Fortran examples have CDL validation
- [ ] Both in-tree and out-of-tree builds work correctly

## Dependencies

**Sprint 1 (Issue #98)**: Examples must be integrated and running before CDL validation can be added.

## Estimated Effort

- **CDL Generation**: 2-3 hours (run examples, generate CDL, review output)
- **Script Creation**: 2-3 hours (validation script + wrapper scripts)
- **CMake Integration**: 3-4 hours (4 CMakeLists.txt files with multi-file handling)
- **Autotools Integration**: 3-4 hours (4 Makefile.am files + wrapper scripts)
- **Documentation**: 1-2 hours (README updates)
- **Testing & Validation**: 2-3 hours (verify both build systems, CI integration)

**Total**: 13-19 hours

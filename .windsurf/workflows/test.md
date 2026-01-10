---
description: run C/Fortran tests and validate implementation
auto_execution_mode: 3
---

# C/Fortran Testing and Validation Workflow

This workflow runs all C and Fortran tests to validate implementation against issue requirements.

## Prerequisites

Ensure the project is built with both CMake and Autotools build systems:
```bash
cd /home/ed/NEP
# CMake build
mkdir -p build && cd build
cmake ..
make
cd ..
# Autotools build
./autogen.sh
mkdir -p build_autotools && cd build_autotools
../configure
make
cd ..
```

## Steps

### 1. Parse Issue Implementation Plan
- Read the GitHub issue to identify testing requirements from the implementation plan
- Extract specific test scenarios mentioned in the issue
- Note any special testing requirements (edge cases, error conditions, etc.)

### 2. Run CMake Tests (C tests)
// turbo
```bash
cd /home/ed/NEP/build
ctest --output-on-failure
```
**Expected:** All C tests pass. Tests include LZ4 compression tests and UDF handler tests.

### 3. Run Autotools Tests (C tests)
// turbo
```bash
cd /home/ed/NEP/build_autotools/test
./run_tests.sh
```
**Expected:** All C tests pass with proper HDF5 plugin path configuration.

### 4. Run Fortran Tests (if built)
// turbo
```bash
cd /home/ed/NEP/build_autotools/ftest
./run_tests.sh
```
**Expected:** All Fortran tests pass. Tests include ncsqueeze compression tests.

### 5. Verify Test Coverage Requirements
// turbo
```bash
cd /home/ed/NEP/build
# Generate coverage report
gcov -r ../src/*.c ../src/*.h
# Calculate coverage percentage
coverage=$(gcov -r ../src/*.c | grep "Lines executed:" | awk '{sum += $3} END {print sum/NR}')
echo "Coverage: $coverage%"
```
**Expected:** Minimum 80% coverage for new/modified code as specified in issue requirements.

### 6. Check for Compiler Warnings
```bash
cd /home/ed/NEP/build
make clean
make VERBOSE=1 2>&1 | grep -i "warning"
```
**Expected:** No new warnings introduced. Review any warnings and fix if critical.

### 7. Report Results to GitHub Issue
Post a summary comment on the GitHub issue with test results:
- Test execution status (pass/fail)
- Coverage percentage (meets requirements?)
- Any warnings found
- Link to detailed test logs if needed

## Success Criteria

✅ **CMake Tests**: All CTest tests pass (LZ4, UDF handlers)  
✅ **Autotools C Tests**: All tests in test/run_tests.sh pass  
✅ **Fortran Tests**: All tests in ftest/run_tests.sh pass (if Fortran enabled)  
✅ **Test Coverage**: Minimum 80% coverage for new/modified code  
✅ **Compiler Warnings**: No critical warnings introduced  
✅ **GitHub Reporting**: Results posted to issue with clear status

## Troubleshooting

- **Test failures**: Check test output for specific failures, verify HDF5_PLUGIN_PATH is set correctly
- **Missing dependencies**: Ensure HDF5, NetCDF-C, NetCDF-Fortran, and LZ4 are installed
- **Plugin path issues**: Verify HDF5_PLUGIN_PATH includes the LZ4 plugin directory
- **Build failures**: Clean build directories and rebuild from scratch
- **GRIB2 test failures**: Ensure NCEPLIBS-g2 is installed if GRIB2 UDF handler is enabled

**ALL TESTS MUST PASS, NO TESTS MAY BE LEFT BROKEN**
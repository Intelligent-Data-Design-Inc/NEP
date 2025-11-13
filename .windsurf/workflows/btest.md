---
description: run C/Fortran tests and linters
auto_execution_mode: 3
---

# C/Fortran Testing and Linting Workflow

This workflow runs all C and Fortran tests to ensure code quality before committing.

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

### 1. Run CMake Tests (C tests)
// turbo
```bash
cd /home/ed/NEP/build
ctest --output-on-failure
```
**Expected:** All C tests pass. Tests include LZ4 compression tests and UDF handler tests.

### 2. Run Autotools Tests (C tests)
// turbo
```bash
cd /home/ed/NEP/build_autotools/test
./run_tests.sh
```
**Expected:** All C tests pass with proper HDF5 plugin path configuration.

### 3. Run Fortran Tests (if built)
// turbo
```bash
cd /home/ed/NEP/build_autotools/ftest
./run_tests.sh
```
**Expected:** All Fortran tests pass. Tests include ncsqueeze compression tests.

### 4. Check for Compiler Warnings
```bash
cd /home/ed/NEP/build
make clean
make VERBOSE=1 2>&1 | grep -i "warning"
```
**Expected:** No new warnings introduced. Review any warnings and fix if critical.

## Success Criteria

✅ **CMake Tests**: All CTest tests pass (LZ4, UDF handlers)  
✅ **Autotools C Tests**: All tests in test/run_tests.sh pass  
✅ **Fortran Tests**: All tests in ftest/run_tests.sh pass (if Fortran enabled)  
✅ **Compiler Warnings**: No critical warnings introduced

## Troubleshooting

- **Test failures**: Check test output for specific failures, verify HDF5_PLUGIN_PATH is set correctly
- **Missing dependencies**: Ensure HDF5, NetCDF-C, NetCDF-Fortran, and LZ4 are installed
- **Plugin path issues**: Verify HDF5_PLUGIN_PATH includes the LZ4 plugin directory
- **Build failures**: Clean build directories and rebuild from scratch
- **GRIB2 test failures**: Ensure NCEPLIBS-g2 is installed if GRIB2 UDF handler is enabled

**ALL TESTS MUST PASS, NO TESTS MAY BE LEFT BROKEN**
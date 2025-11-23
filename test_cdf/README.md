# CDF Library Integration Test

## Overview
This directory contains the test program for validating NASA CDF library integration in the NEP project (v1.3.0 Sprint 2).

## Test Program: tst_cdf_basic.c

### Purpose
Validates that the NASA CDF library is properly installed and functional by:
1. Creating a minimal CDF test file at runtime
2. Reading back the file and validating metadata
3. Cleaning up the test file

### Test Operations

#### File Creation
The test creates `tst_cdf_simple.cdf` with:
- **Global attributes**: title, institution
- **zVariable**: temperature (CDF_FLOAT, 10 elements)
- **Variable attributes**: units, long_name
- **Data**: Temperature values from 20.0 to 24.5°C

#### File Validation
The test reads back the file and verifies:
- File can be opened successfully
- Metadata queries work (encoding, majority, attributes, variables)
- Variable name matches expected value ("temperature")
- Global attributes are present and readable

#### Cleanup
The test removes the generated file after validation.

### Expected Output
```
=== NEP CDF Library Integration Test ===

Creating test CDF file: tst_cdf_simple.cdf
  ✓ Successfully created test file

Opening CDF file: tst_cdf_simple.cdf
  ✓ Successfully opened CDF file

Querying file metadata...
  File encoding: 6
  Majority: ROW_MAJOR
  Number of attributes: 4
  ✓ Successfully queried file metadata

Variables in file:
  zVariables: 1
  rVariables: 0
  ✓ Found expected number of zVariables

Querying zVariable information...
  Variable name: temperature
  Data type: 44
  Number of dimensions: 1
  ✓ Variable name matches expected value

Global attributes:
  title (global attribute)
  institution (global attribute)
  ✓ Successfully listed global attributes

Closing CDF file...
  ✓ Successfully closed CDF file

Cleaning up test file...
  ✓ Test file removed

=== Test Summary ===
✓ All tests PASSED
CDF library integration validated successfully.
```

## Build Systems

### CMake
- **Conditional build**: Only builds when `ENABLE_CDF=ON`
- **Test registration**: Registered with CTest
- **Run test**: `ctest --verbose` (shows full output)

### Autotools
- **Conditional build**: Only builds when `--enable-cdf` configured
- **Test registration**: Registered with `make check`
- **Run test**: `make check` (output in test-suite.log)

## CI Integration

The test runs automatically in CI when CDF is enabled (fortran=on, compression=all configuration).

### CI Output
- **CMake**: Full test output shown via `ctest --verbose`
- **Autotools**: Test log shown via `test_cdf/test-suite.log`

## Files
- `tst_cdf_basic.c`: Self-contained test program (creates, reads, validates, cleans up)
- `CMakeLists.txt`: CMake build configuration
- `Makefile.am`: Autotools build configuration
- `README.md`: This file

## Scope
This test validates CDF library integration only (v1.3.0 Sprint 2). UDF implementation comes in Sprint 3.

## Notes
- No binary test data files needed - test creates its own file at runtime
- Test is self-contained and portable
- Validates both CDF write and read operations
- Automatic cleanup ensures no artifacts left behind

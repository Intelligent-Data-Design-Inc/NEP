# NetCDF Example Programs

This directory contains example programs demonstrating NetCDF API usage in both C and Fortran.

## Example Categories

### Classic NetCDF Examples (C)
Located in `classic/`:
- **quickstart.c** - Minimal introduction to NetCDF - the simplest starting point for new users
- **simple_2D.c** - Basic 2D array creation and writing
- **coord_vars.c** - Working with coordinate variables
- **format_variants.c** - Different NetCDF format variants (classic, 64-bit offset, CDF-5)
- **size_limits.c** - Demonstrating size and dimension limits
- **unlimited_dim.c** - Using unlimited dimensions for time series data
- **var4d.c** - Creating and writing 4-dimensional variables

### NetCDF-4 Examples (C)
Located in `netcdf-4/`:
- **simple_nc4.c** - Basic NetCDF-4 file creation
- **compression.c** - Using compression filters (deflate, shuffle)
- **chunking_performance.c** - Chunking strategies and performance
- **multi_unlimited.c** - Multiple unlimited dimensions (NetCDF-4 feature)
- **user_types.c** - User-defined compound and enum types

### Classic NetCDF Examples (Fortran)
Located in `f_classic/`:
- **f_simple_2D.f90** - Basic 2D array in Fortran
- **f_coord_vars.f90** - Coordinate variables in Fortran
- **f_format_variants.f90** - Format variants in Fortran
- **f_size_limits.f90** - Size limits in Fortran
- **f_unlimited_dim.f90** - Unlimited dimensions in Fortran
- **f_var4d.f90** - 4D variables in Fortran

### NetCDF-4 Examples (Fortran)
Located in `f_netcdf-4/`:
- **f_simple_nc4.f90** - Basic NetCDF-4 in Fortran
- **f_compression.f90** - Compression in Fortran
- **f_chunking_performance.f90** - Chunking in Fortran
- **f_multi_unlimited.f90** - Multiple unlimited dimensions in Fortran
- **f_user_types.f90** - User-defined types in Fortran

### Future Examples
- `parallelIO/` - Reserved for parallel I/O examples
- `performance/` - Reserved for performance optimization examples

## Building Examples

### CMake
```bash
# Build with examples enabled (default)
cmake -B build
cmake --build build

# Disable examples
cmake -B build -DBUILD_EXAMPLES=OFF
cmake --build build
```

### Autotools
```bash
# Build with examples enabled (default)
./configure
make

# Disable examples
./configure --disable-examples
make
```

## Fortran Examples

Fortran examples require:
- Fortran compiler
- NetCDF-Fortran library
- `ENABLE_FORTRAN=ON` (CMake) or `--enable-fortran` (Autotools)

If Fortran support is disabled or NetCDF-Fortran is unavailable, Fortran examples are automatically skipped.

## Running Examples

Examples are automatically run as tests when built:

```bash
# CMake
ctest --test-dir build

# Autotools
make check
```

Each example creates NetCDF output files demonstrating the features being illustrated.

## Output Validation

Starting in v1.5.1 Sprint 2, example output is validated against expected CDL (Common Data Language) files stored in `expected_output/`. This ensures examples continue producing correct output across code changes.

## C and Fortran Equivalence

The C and Fortran examples are designed to be structurally similar and produce equivalent output:

### Structural Similarities (v1.5.1 Sprint 3)
- **Error Handling**: Both C and Fortran examples use inline error handling rather than separate error handling functions
- **Program Flow**: Examples follow the same structure:
  - Write phase: create file, define dimensions/variables, write data, close
  - Read phase: reopen file, verify metadata, verify data, close
- **File Format**: Both use NetCDF-4 format (`NC_NETCDF4` in C, `NF90_NETCDF4` in Fortran)

### Output Equivalence
C and Fortran versions of the same example produce identical NetCDF files (verified via `ncdump` CDL comparison):
- Same dimensions, variables, and attributes
- Same data values
- Only acceptable difference: filename in CDL header

### Verifying Equivalence
To verify that C and Fortran examples produce equivalent output:

```bash
# Run both examples
./simple_2D
./f_simple_2D

# Generate CDL from both outputs
ncdump simple_2D.nc > simple_2D.cdl
ncdump f_simple_2D.nc > f_simple_2D.cdl

# Compare (should show only filename difference)
diff -u simple_2D.cdl f_simple_2D.cdl
```

This equivalence ensures users can learn NetCDF concepts from either language and expect consistent behavior.

## Documentation

All examples are comprehensively documented with Doxygen comments explaining:
- What the example demonstrates
- Learning objectives and key concepts
- Prerequisites and related examples
- Compilation and usage instructions
- Expected output

### Viewing Documentation

The examples are integrated into the main NEP Doxygen documentation. After building the documentation:

```bash
# Build documentation
cmake -B build -DBUILD_DOCUMENTATION=ON
cmake --build build --target doc

# View in browser
open build/docs/html/examples.html
```

Or view the online documentation at: https://intelligent-data-design-inc.github.io/NEP/

### Learning Path

**For Absolute Beginners:**
1. Start with `quickstart.c` / `f_quickstart.f90` - Minimal introduction with just 6 data values
2. Progress to `simple_2D.c` / `f_simple_2D.f90` - Learn basic file operations with larger arrays
3. Add metadata with `coord_vars.c` / `f_coord_vars.f90` - Coordinate variables and attributes
4. Explore `unlimited_dim.c` / `f_unlimited_dim.f90` - Work with time-series data

**For Intermediate Users:**
4. Study `format_variants.c` / `f_format_variants.f90` - Understand format choices
5. Review `size_limits.c` / `f_size_limits.f90` - Learn size constraints
6. Master `var4d.c` / `f_var4d.f90` - Handle multi-dimensional data

**For Advanced Users (NetCDF-4):**
7. Begin with `simple_nc4.c` / `f_simple_nc4.f90` - NetCDF-4 format introduction
8. Apply `compression.c` / `f_compression.f90` - Reduce file sizes
9. Optimize with `chunking_performance.c` / `f_chunking_performance.f90` - Improve I/O performance
10. Utilize `multi_unlimited.c` / `f_multi_unlimited.f90` - Multiple unlimited dimensions
11. Explore `user_types.c` / `f_user_types.f90` - Complex data structures

### C vs Fortran

Each C example has a Fortran equivalent that produces identical output. Key differences:

**Array Ordering:**
- C: Row-major `data[NY][NX]` with dimensions `(y, x)`
- Fortran: Column-major `data(NX, NY)` with dimensions `(x, y)`

**Indexing:**
- C: 0-based (0 to N-1)
- Fortran: 1-based (1 to N)

**API Functions:**
- C: `nc_*` (e.g., `nc_create`, `nc_def_dim`)
- Fortran: `nf90_*` (e.g., `nf90_create`, `nf90_def_dim`)

**Dimension Order:**
- C: Slowest to fastest `(time, level, lat, lon)`
- Fortran: Fastest to slowest `(lon, lat, level, time)` - reversed!

## Dependencies

- NetCDF-C library (required)
- NetCDF-Fortran library (required for Fortran examples)
- C99 compiler
- Fortran 90+ compiler (for Fortran examples)

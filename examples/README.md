# NetCDF Example Programs

This directory contains example programs demonstrating NetCDF API usage in both C and Fortran.

## Example Categories

### Classic NetCDF Examples (C)
Located in `classic/`:
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

## Dependencies

- NetCDF-C library (required)
- NetCDF-Fortran library (required for Fortran examples)
- C99 compiler
- Fortran 90+ compiler (for Fortran examples)

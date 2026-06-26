## NetCDF Fortran Examples Roadmap

Guide to the Fortran examples in Chapter 7 of the NetCDF Developer's Handbook.

### Quick Reference

| Example | Path | What You Learn | Complexity |
|---------|------|----------------|------------|
| `f_quickstart.f90` | `examples/f_classic/` | Minimal example: create, define, write, close | Beginner |
| `f_simple_2D.f90` | `examples/f_classic/` | Basic file creation, dimensions, variables, attributes, fill values | Beginner |
| `f_coord_vars.f90` | `examples/f_classic/` | Coordinate variables, CF conventions, geospatial metadata | Beginner |
| `f_unlimited_dim.f90` | `examples/f_classic/` | Unlimited dimensions, record-based data growth, appending data | Intermediate |
| `f_size_limits.f90` | `examples/f_classic/` | Binary format selection, file size comparison | Intermediate |
| `f_simple_nc4.f90` | `examples/f_netcdf-4/` | NetCDF-4/HDF5 features, 64-bit integers, unsigned types | Intermediate |
| `f_groups.f90` | `examples/f_netcdf-4/` | Hierarchical groups, nested group navigation | Intermediate |
| `f_multi_unlimited.f90` | `examples/f_netcdf-4/` | Multiple unlimited dimensions (NetCDF-4 only) | Intermediate |
| `f_user_types.f90` | `examples/f_netcdf-4/` | Compound types, VLEN, enum user-defined types | Advanced |
| `f_compression.f90` | `examples/f_netcdf-4/` | Deflate compression, chunking, shuffle filter | Advanced |
| `f_chunking_performance.f90` | `examples/f_netcdf-4/` | Chunk size optimization, performance benchmarking | Advanced |

### Suggested Learning Paths

#### Path 1: NetCDF Fundamentals (Classic Model)
For users who need to read and write standard netCDF files:

1. **f_quickstart.f90** - Minimal working example to verify your setup
2. **f_simple_2D.f90** - Learn the basic workflow: create → define → write → close → read → verify
3. **f_coord_vars.f90** - Add coordinate variables and CF convention metadata
4. **f_unlimited_dim.f90** - Handle time series data with unlimited dimensions
5. **f_size_limits.f90** - Understand format options and size limitations

#### Path 2: NetCDF-4/HDF5 Advanced Features
For users who need compression, groups, or complex data structures:

1. **f_simple_nc4.f90** - Create NetCDF-4 files with new atomic types
2. **f_groups.f90** - Organize data hierarchically with groups and subgroups
3. **f_multi_unlimited.f90** - Use multiple unlimited dimensions (NetCDF-4 only)
4. **f_compression.f90** - Apply compression and optimize chunking for performance
5. **f_user_types.f90** - Define and use compound types and VLEN arrays

#### Path 3: High-Performance Computing
For HPC users working with large datasets:

1. **f_size_limits.f90** - Choose between CDF-5 and NetCDF-4 for large files
2. **f_chunking_performance.f90** - Optimize chunk sizes for your access patterns
3. **f_compression.f90** - Balance compression ratio with I/O performance

### Compiling the Examples

All examples use the same error-handling pattern and require linking against the netCDF Fortran library:

```bash
# Basic compilation (classic examples)
gfortran -o f_simple_2D f_simple_2D.f90 -lnetcdff -lnetcdf

# With explicit paths (if netCDF is not in standard location)
gfortran -o f_simple_2D f_simple_2D.f90 $(nf-config --fflags --flibs)

# NetCDF-4 examples (require HDF5 support in netCDF)
gfortran -o f_simple_nc4 f_simple_nc4.f90 $(nf-config --fflags --flibs)

# Parallel I/O examples (require MPI)
mpif90 -o f_parallel_io f_parallel_io.f90 $(nf-config --fflags --flibs)
```

The `nf-config` utility, installed with NetCDF-Fortran, provides the correct compiler and linker flags for your system. If `nf-config` is unavailable, use `nc-config` and add the Fortran library manually:

```bash
gfortran -o f_simple_2D f_simple_2D.f90 $(nc-config --cflags --libs) -lnetcdff
```

### Common Pattern: Error Handling

All examples use the same error handling pattern:

```fortran
if (retval /= nf90_noerr) then
    print *, 'Error: ', nf90_strerror(retval)
    stop 2
end if
```

Every netCDF function call is wrapped in error checking:

```fortran
retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
if (retval /= nf90_noerr) call handle_err(retval)
```

Many examples also provide a `handle_err()` subroutine:

```fortran
subroutine handle_err(errcode)
    use netcdf
    implicit none
    integer, intent(in) :: errcode
    print *, 'Error: ', nf90_strerror(errcode)
    stop 2
end subroutine handle_err
```

This pattern ensures that any API failure is immediately reported with a descriptive error message.

### File Organization

Examples are organized by feature set in the NetCDF Expansion Pack:

```
examples/
├── f_classic/          # Works with all netCDF formats
├── f_netcdf-4/        # Requires NetCDF-4/HDF5 support
└── f_parallel/        # Requires MPI and parallel netCDF (if available)
```

### Key Differences from C API

Fortran examples follow different conventions than C:

| Aspect | C | Fortran |
|--------|---|---------|
| Function prefix | `nc_` | `nf90_` |
| Constants | `NC_CLOBBER` | `NF90_CLOBBER` |
| Error handling | Return codes | Return codes (check with `/= nf90_noerr`) |
| Dimension order | Row-major (C-style) | Column-major (Fortran-style) |
| Array indexing | `data[y][x]` | `data(x, y)` |
| String handling | Null-terminated | Fixed-length, padded |

**Important:** In Fortran, dimensions are declared in the opposite order from C:
- C: `float data[NY][NX]` (y varies fastest)
- Fortran: `real :: data(NX, NY)` (x varies fastest)

### Prerequisites by Example

| Example | Requires |
|---------|----------|
| f_quickstart.f90 | NetCDF-Fortran 4.0+ |
| f_simple_2D.f90 | NetCDF-Fortran 4.0+ |
| f_coord_vars.f90 | NetCDF-Fortran 4.0+ |
| f_unlimited_dim.f90 | NetCDF-Fortran 4.0+ |
| f_size_limits.f90 | NetCDF-Fortran 4.4+ (for CDF-5) |
| f_simple_nc4.f90 | NetCDF-4/HDF5 support (NC_HAS_HDF5=1) |
| f_groups.f90 | NetCDF-4/HDF5 support |
| f_multi_unlimited.f90 | NetCDF-4/HDF5 support |
| f_user_types.f90 | NetCDF-4/HDF5 support |
| f_compression.f90 | NetCDF-4/HDF5 support |
| f_chunking_performance.f90 | NetCDF-4/HDF5 support |

Check your netCDF installation:
```bash
nc-config --has-hdf5      # Should print "yes" for NetCDF-4 examples
nc-config --has-parallel  # Should print "yes" for parallel examples
nf-config --version       # Verify Fortran library version
```

### Full Documentation

For complete explanations, code walkthroughs, and output examples, see Chapter 7: NetCDF Fortran Examples in the NetCDF Developer's Handbook.

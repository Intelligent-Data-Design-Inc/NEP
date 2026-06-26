## NetCDF C Examples Roadmap

Guide to the C examples in Chapter 6 of the NetCDF Developer's Handbook.

### Quick Reference

| Example | Path | What You Learn | Complexity |
|---------|------|----------------|------------|
| `simple_2D.c` | `examples/classic/` | Basic file creation, dimensions, variables, attributes, fill values | Beginner |
| `coord_vars.c` | `examples/classic/` | Coordinate variables, CF conventions, geospatial metadata | Beginner |
| `unlimited_dim.c` | `examples/classic/` | Unlimited dimensions, record-based data growth, appending data | Intermediate |
| `format_variants.c` | `examples/classic/` | Binary format selection, format detection, file size comparison | Intermediate |
| `simple_nc4.c` | `examples/netcdf-4/` | NetCDF-4/HDF5 features, groups, new atomic types | Intermediate |
| `groups.c` | `examples/netcdf-4/` | Hierarchical groups, nested group navigation | Intermediate |
| `user_types.c` | `examples/netcdf-4/` | Compound types, VLEN, enum, opaque user-defined types | Advanced |
| `compression.c` | `examples/netcdf-4/` | Deflate compression, chunking, shuffle filter | Advanced |
| `parallel_io.c` | `examples/parallel/` | MPI-based parallel I/O, collective operations | Advanced |

### Suggested Learning Paths

#### Path 1: NetCDF Fundamentals (Classic Model)
For users who need to read and write standard netCDF files:

1. **simple_2D.c** - Learn the basic workflow: create → define → write → close → read → verify
2. **coord_vars.c** - Add coordinate variables and CF convention metadata
3. **unlimited_dim.c** - Handle time series data with unlimited dimensions
4. **format_variants.c** - Understand binary format options and when to use each

#### Path 2: NetCDF-4/HDF5 Advanced Features
For users who need compression, groups, or complex data structures:

1. **simple_nc4.c** - Create NetCDF-4 files with new atomic types (64-bit integers, unsigned types)
2. **groups.c** - Organize data hierarchically with groups and subgroups
3. **compression.c** - Apply compression and optimize chunking for performance
4. **user_types.c** - Define and use compound types, VLEN arrays, and enums

#### Path 3: High-Performance Computing
For HPC users working with large datasets:

1. **format_variants.c** - Choose between CDF-5 and NetCDF-4 for large files
2. **compression.c** - Optimize chunk sizes for your access patterns
3. **parallel_io.c** - Implement MPI parallel I/O for distributed computing

### Compiling the Examples

All examples use the same error-handling pattern and require linking against the netCDF C library:

```bash
# Basic compilation (classic examples)
gcc -o simple_2D simple_2D.c -lnetcdf

# With explicit paths (if netCDF is not in standard location)
gcc -o simple_2D simple_2D.c $(nc-config --cflags --libs)

# NetCDF-4 examples (require HDF5 support in netCDF)
gcc -o simple_nc4 simple_nc4.c $(nc-config --cflags --libs)

# Parallel I/O examples (require MPI)
mpicc -o parallel_io parallel_io.c $(nc-config --cflags --libs)
```

The `nc-config` utility, installed with netCDF-C, provides the correct compiler and linker flags for your system.

### Common Pattern: Error Handling

All examples use the same error handling macro:

```c
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(2);}
```

Every netCDF function call is wrapped in error checking:

```c
if ((retval = nc_create(FILE_NAME, NC_CLOBBER, &ncid)))
    ERR(retval);
```

This pattern ensures that any API failure is immediately reported with a descriptive error message.

### File Organization

Examples are organized by feature set in the NetCDF Expansion Pack:

```
examples/
├── classic/          # Works with all netCDF formats
├── netcdf-4/        # Requires NetCDF-4/HDF5 support
└── parallel/        # Requires MPI and parallel netCDF
```

### Prerequisites by Example

| Example | Requires |
|---------|----------|
| simple_2D.c | netCDF-C 4.0+ |
| coord_vars.c | netCDF-C 4.0+ |
| unlimited_dim.c | netCDF-C 4.0+ |
| format_variants.c | netCDF-C 4.4+ (for CDF-5) |
| simple_nc4.c | NetCDF-4/HDF5 support (NC_HAS_HDF5=1) |
| groups.c | NetCDF-4/HDF5 support |
| user_types.c | NetCDF-4/HDF5 support |
| compression.c | NetCDF-4/HDF5 support |
| parallel_io.c | MPI and parallel I/O support (NC_HAS_PARALLEL=1) |

Check your netCDF installation:
```bash
nc-config --has-hdf5      # Should print "yes" for NetCDF-4 examples
nc-config --has-parallel  # Should print "yes" for parallel examples
```

### Full Documentation

For complete explanations, code walkthroughs, and output examples, see Chapter 6: NetCDF C Examples in the NetCDF Developer's Handbook.

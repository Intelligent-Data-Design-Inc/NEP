# PIO (Parallel I/O) and Parallel I/O with NetCDF

Understanding the ParallelIO (PIO) library for high-performance parallel I/O with netCDF on HPC systems. Use when writing parallel netCDF applications, optimizing I/O performance on many processors, or converting existing netCDF codes to use PIO.

## Core Concepts

### What is PIO?

PIO is a high-level parallel I/O C and Fortran library for structured grid applications that provides:
- A netCDF-like API for familiar programming patterns
- Ability to designate a subset of processors for I/O operations
- Support for both synchronous and asynchronous I/O modes
- Integration with netCDF, parallel-netcdf (PnetCDF), and HDF5 libraries
- Distributed array decomposition and reassembly

### Key Benefits

- **Scalability**: Reduces I/O contention on HPC systems with many processors
- **Flexibility**: Choose between different I/O modes and strategies
- **Performance**: Optimizes data movement between compute and I/O tasks
- **Compatibility**: Works with existing netCDF code with minimal changes (via netCDF integration)

## Architecture

### Library Structure

```
Fortran Application Code
         ↓
PIO Fortran Library (wrapper)
         ↓
PIO C Library (core functionality)
         ↓
    ┌────┴────┐
    ↓         ↓
netCDF-C   PnetCDF
    ↓
  HDF5 + compression (optional)
```

### Dependencies

**Required:**
- netCDF-C library (version 4.6.1+, built with MPI for parallel I/O)
- MPI-enabled C and Fortran compilers

**Optional:**
- PnetCDF (version 1.9.0+) for parallel classic format I/O
- HDF5 (MPI-enabled) for netCDF-4 support
- Compression libraries (zlib, szip)

## I/O Modes

### Intracomm Mode

All processors participate in computation, with a subset designated for I/O:
- I/O processors also do computational work
- Simpler setup for single computational units
- I/O tasks typically distributed across nodes to avoid bottlenecks

**Initialization:**
```c
// C
int iosysid;
PIOc_Init_Intracomm(MPI_COMM_WORLD, niotasks, ioproc_stride,
                    ioproc_start, PIO_REARR_SUBSET, &iosysid);

// With netCDF integration
nc_def_iosystem(MPI_COMM_WORLD, niotasks, ioproc_stride, 
                ioproc_start, PIO_REARR_SUBSET, &iosysid);
```

```fortran
! Fortran
call PIO_init(my_rank, MPI_COMM_WORLD, niotasks, num_aggregator, &
              stride, PIO_rearr_subset, pio_iosystem, base=1)
```

### Async Mode

Dedicated I/O processors service multiple computational units:
- I/O processors do NO computational work
- Multiple computation components can share I/O processors
- I/O processors wait in message loop for requests
- Better for complex multi-component applications

**Initialization:**
```c
// C
int iosysid;
PIOc_init_async(world_comm, num_io_procs, io_proc_list, 
                component_count, num_procs_per_comp, 
                proc_list, io_comm, comp_comm, &iosysid);

// With netCDF integration
nc_def_async(world_comm, num_io_procs, io_proc_list, 
             component_count, num_procs_per_comp, 
             proc_list, &iosysid);
```

## IOTYPEs (I/O Methods)

PIO supports four I/O types, specified when creating/opening files:

| IOTYPE | Library | Format | Parallel I/O | Compression | Notes |
|--------|---------|--------|--------------|-------------|-------|
| `PIO_IOTYPE_PNETCDF` | PnetCDF | Classic, 64-bit offset, CDF5 | Yes | No | Parallel classic formats only |
| `PIO_IOTYPE_NETCDF` | netCDF-C | Classic, 64-bit offset | No | No | Sequential, root I/O task only |
| `PIO_IOTYPE_NETCDF4C` | netCDF-C/HDF5 | NetCDF-4/HDF5 | No | Yes (level 1) | Sequential with compression |
| `PIO_IOTYPE_NETCDF4P` | netCDF-C/HDF5 | NetCDF-4/HDF5 | Yes | Yes | Parallel HDF5 I/O |

**Selection criteria:**
- Use `PIO_IOTYPE_PNETCDF` for best parallel performance with classic formats
- Use `PIO_IOTYPE_NETCDF4P` for parallel I/O with compression
- Use `PIO_IOTYPE_NETCDF4C` when compression is critical and parallel I/O not needed
- Use `PIO_IOTYPE_NETCDF` for compatibility/debugging

## Decompositions and Distributed Arrays

### Concept

A decomposition maps a global data array to local subarrays across processors:
- Each processor holds a portion of the global array
- Decomposition defines which global elements each processor owns
- One decomposition per netCDF data type
- Can be saved to/loaded from files

### Creating a Decomposition

**C API:**
```c
int ioid;
PIO_Offset *compdof;  // 1-based array mapping
int elements_per_pe = DIM_LEN / ntasks;

// Allocate and fill decomposition map
compdof = malloc(elements_per_pe * sizeof(PIO_Offset));
for (int i = 0; i < elements_per_pe; i++)
    compdof[i] = my_rank * elements_per_pe + i + 1;  // 1-based!

// Initialize decomposition
PIOc_InitDecomp(iosysid, PIO_INT, NDIM, dim_len, 
                elements_per_pe, compdof, &ioid, 
                NULL, NULL, NULL);
free(compdof);
```

**NetCDF Integration:**
```c
size_t *compdof;  // 0-based for netCDF integration
compdof = malloc(elements_per_pe * sizeof(size_t));
for (int i = 0; i < elements_per_pe; i++)
    compdof[i] = my_rank * elements_per_pe + i;  // 0-based!

nc_def_decomp(iosysid, PIO_INT, NDIM2, &dimlen[1], 
              elements_per_pe, compdof, &ioid, 1, NULL, NULL);
```

**Fortran API:**
```fortran
type(io_desc_t) :: iodesc
integer, dimension(:) :: compdof

! Define mapping (1-based in Fortran)
compdof = my_rank * elements_per_pe + (/ (i, i=1,elements_per_pe) /)

call PIO_initdecomp(pio_iosystem, PIO_int, dims, compdof, iodesc)
```

### Freeing Decompositions

Always free decomposition resources when done:
```c
// C
PIOc_freedecomp(iosysid, ioid);

// NetCDF integration
nc_free_decomp(iosysid, ioid);
```

```fortran
! Fortran
call PIO_freedecomp(pio_iosystem, iodesc)
```

### Saving/Loading Decompositions

```c
// Save decomposition to file
PIOc_write_nc_decomp(iosysid, filename, cmode, ioid, title, history, fortran_order);

// Load decomposition from file
PIOc_read_nc_decomp(iosysid, filename, &ioid, comm, PIO_INT, title, history, &fortran_order);
```

## Rearrangers

Rearrangers control how data moves between compute and I/O tasks:

### BOX Rearranger (`PIO_REARR_BOX`)

- Arranges data on I/O tasks to be contiguous on disk
- Requires all-to-all communication between compute and I/O tasks
- Better for underlying I/O library performance
- More communication overhead

**Example:** Global array `0-19` over 5 compute tasks, 2 I/O tasks:
```
Compute: {0,4,8,12} {16,1,5,9} {13,17,2,6} {10,14,18,3} {7,11,15,19}
I/O:     {0,1,2,3,4,5,6,7,8,9} {10,11,12,13,14,15,16,17,18,19}
```

### SUBSET Rearranger (`PIO_REARR_SUBSET`)

- Each I/O task associated with unique subset of compute tasks
- Each compute task sends to exactly one I/O task
- Less communication, but data may be fragmented on I/O tasks
- Usually scales better with task count

**Example:** Same distribution:
```
Compute: {0,4,8,12} {16,1,5,9} {13,17,2,6} {10,14,18,3} {7,11,15,19}
I/O:     {0,1,4,5,8,9,12,16} {2,3,6,7,10,11,13,14,15,17,18,19}
```

**Selection:** SUBSET typically performs better at scale; BOX may be better for small task counts.

## File Operations

### Creating Files

**PIO API:**
```c
// C
int ncid;
PIOc_createfile(iosysid, &ncid, &iotype, filename, PIO_CLOBBER);

// Define metadata
PIOc_def_dim(ncid, "time", NC_UNLIMITED, &dimid);
PIOc_def_var(ncid, "temperature", PIO_FLOAT, ndims, dimids, &varid);
PIOc_put_att_text(ncid, varid, "units", strlen("K"), "K");
PIOc_enddef(ncid);
```

```fortran
! Fortran
type(file_desc_t) :: pio_file
ret = PIO_createfile(pio_iosystem, pio_file, iotype, filename)
```

**NetCDF Integration:**
```c
// Use standard netCDF functions with NC_PIO flag
nc_create(filename, NC_PIO, &ncid);
nc_def_dim(ncid, "time", NC_UNLIMITED, &dimid);
nc_def_var(ncid, "temperature", NC_FLOAT, ndims, dimids, &varid);
```

### Opening Files

```c
// PIO API
PIOc_openfile(iosysid, &ncid, &iotype, filename, PIO_NOWRITE);

// NetCDF integration
nc_open(filename, NC_PIO, &ncid);
```

### Closing Files

```c
// PIO API
PIOc_closefile(ncid);

// NetCDF integration
nc_close(ncid);
```

## Reading and Writing Distributed Arrays

### Writing Data

**PIO API:**
```c
// Set record number (for unlimited dimension)
PIOc_setframe(ncid, varid, record_num);

// Write distributed array
PIOc_write_darray(ncid, varid, ioid, arraylen, local_data, NULL);
```

```fortran
! Fortran
integer, dimension(:) :: local_data
call PIO_write_darray(pio_file, pio_var, iodesc, local_data, ret_val)
```

**NetCDF Integration:**
```c
// Write distributed array (record number is parameter)
nc_put_vard_int(ncid, varid, ioid, record_num, local_data);
nc_put_vard_float(ncid, varid, ioid, record_num, local_data);
```

### Reading Data

**PIO API:**
```c
// Set record number
PIOc_setframe(ncid, varid, record_num);

// Read distributed array
PIOc_read_darray(ncid, varid, ioid, arraylen, local_data);
```

```fortran
! Fortran
call PIO_read_darray(pio_file, pio_var, iodesc, local_data, ret_val)
```

**NetCDF Integration:**
```c
// Read distributed array
nc_get_vard_int(ncid, varid, ioid, record_num, local_data);
nc_get_vard_float(ncid, varid, ioid, record_num, local_data);
```

## Error Handling

PIO provides three error handling modes:

| Mode | Behavior |
|------|----------|
| `PIO_INTERNAL_ERROR` | Errors cause abort (default) |
| `PIO_BCAST_ERROR` | Error codes broadcast to all tasks |
| `PIO_RETURN_ERROR` | Errors returned to caller |

**Setting error handler:**
```c
// Change default before initializing IOsystem
PIOc_set_iosystem_error_handling(PIO_DEFAULT, PIO_RETURN_ERROR, NULL);

// Change for specific IOsystem
PIOc_set_iosystem_error_handling(iosysid, PIO_RETURN_ERROR, NULL);
```

```fortran
! Fortran
call PIO_seterrorhandling(pio_iosystem, PIO_RETURN_ERROR)
call PIO_seterrorhandling(PIO_DEFAULT, PIO_RETURN_ERROR)
```

## Finalization

### Intracomm Mode

All processors call finalize:
```c
// C
PIOc_finalize(iosysid);

// NetCDF integration
nc_free_iosystem(iosysid);
```

```fortran
! Fortran
call PIO_finalize(pio_iosystem, ierr)
```

### Async Mode

- Compute processors call finalize
- I/O processors receive message and free resources
- When all IOsystems freed, I/O processors exit message loop

## NetCDF Integration

### Building with Integration

```bash
# Autotools
./configure --enable-netcdf-integration --enable-fortran

# CMake
cmake -DPIO_ENABLE_NETCDF_INTEGRATION=On \
      -DNetCDF_C_PATH=/path/to/netcdf \
      -DPnetCDF_PATH=/path/to/pnetcdf ..
```

### Converting Existing NetCDF Code

**Steps:**
1. Initialize IOsystem with `nc_def_iosystem()` or `nc_def_async()`
2. Use `NC_PIO` flag when creating/opening files
3. Define decomposition(s) with `nc_def_decomp()`
4. Replace `nc_put_vara_*()` with `nc_put_vard_*()`
5. Replace `nc_get_vara_*()` with `nc_get_vard_*()`
6. Free decompositions with `nc_free_decomp()`
7. Free IOsystem with `nc_free_iosystem()`

**Key functions:**

| NetCDF Integration | PIO API Equivalent | Purpose |
|-------------------|-------------------|---------|
| `nc_def_iosystem()` | `PIOc_Init_Intracomm()` | Initialize Intracomm mode |
| `nc_def_async()` | `PIOc_init_async()` | Initialize Async mode |
| `nc_def_decomp()` | `PIOc_init_decomp()` | Define decomposition |
| `nc_free_decomp()` | `PIOc_freedecomp()` | Free decomposition |
| `nc_put_vard_*()` | `PIOc_write_darray()` | Write distributed array |
| `nc_get_vard_*()` | `PIOc_read_darray()` | Read distributed array |
| `nc_free_iosystem()` | `PIOc_finalize()` | Free IOsystem |

### Default IOsystem

When using netCDF integration, the most recently created IOsystem becomes the default:
```c
// Get current default
int iosysid;
nc_get_iosystem(&iosysid);

// Set default (for multiple IOsystems)
nc_set_iosystem(iosysid);
```

## Common Patterns

### Basic PIO Workflow

```c
// 1. Initialize MPI
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

// 2. Initialize PIO IOsystem
PIOc_Init_Intracomm(MPI_COMM_WORLD, niotasks, stride, 
                    base, PIO_REARR_SUBSET, &iosysid);

// 3. Create decomposition
PIOc_InitDecomp(iosysid, PIO_FLOAT, ndims, dims, 
                maplen, compmap, &ioid, NULL, NULL, NULL);

// 4. Create file and define metadata
PIOc_createfile(iosysid, &ncid, &iotype, filename, PIO_CLOBBER);
PIOc_def_dim(ncid, "x", xdim, &dimids[0]);
PIOc_def_var(ncid, "data", PIO_FLOAT, ndims, dimids, &varid);
PIOc_enddef(ncid);

// 5. Write data
for (int rec = 0; rec < nrecs; rec++) {
    PIOc_setframe(ncid, varid, rec);
    PIOc_write_darray(ncid, varid, ioid, maplen, local_data, NULL);
}

// 6. Close file
PIOc_closefile(ncid);

// 7. Free decomposition
PIOc_freedecomp(iosysid, ioid);

// 8. Finalize PIO
PIOc_finalize(iosysid);

// 9. Finalize MPI
MPI_Finalize();
```

### NetCDF Integration Workflow

```c
// 1-2. Initialize MPI and IOsystem
MPI_Init(&argc, &argv);
nc_def_iosystem(MPI_COMM_WORLD, 1, 1, 0, 0, &iosysid);

// 3. Define decomposition
nc_def_decomp(iosysid, PIO_FLOAT, ndims, dims, 
              maplen, compmap, &ioid, 1, NULL, NULL);

// 4. Create file with NC_PIO flag
nc_create(filename, NC_PIO, &ncid);
nc_def_dim(ncid, "x", xdim, &dimids[0]);
nc_def_var(ncid, "data", NC_FLOAT, ndims, dimids, &varid);

// 5. Write distributed data
nc_put_vard_float(ncid, varid, ioid, 0, local_data);

// 6-8. Cleanup
nc_close(ncid);
nc_free_decomp(iosysid, ioid);
nc_free_iosystem(iosysid);
MPI_Finalize();
```

## Performance Considerations

### Choosing Number of I/O Tasks

- **Too few**: I/O becomes bottleneck
- **Too many**: Communication overhead increases
- **Typical**: 1-10% of total tasks for I/O
- **Distribute**: Spread I/O tasks across nodes

### Rearranger Selection

- Start with `PIO_REARR_SUBSET` (usually better scaling)
- Try `PIO_REARR_BOX` if I/O performance is poor
- Test both with your specific decomposition

### IOTYPE Selection

- `PIO_IOTYPE_PNETCDF`: Best parallel performance for classic formats
- `PIO_IOTYPE_NETCDF4P`: Good parallel performance with compression
- Avoid sequential IOTYPEs (`NETCDF`, `NETCDF4C`) at scale

### Decomposition Design

- Balance load across processors
- Minimize fragmentation on I/O tasks
- Consider data access patterns (contiguous vs. strided)
- Reuse decompositions when possible

## Debugging and Logging

### Enable Logging

```c
// Set log level (0=none, 1=errors, 2=warnings, 3=info, 4=debug)
PIOc_set_log_level(3);
```

```bash
# Build with logging support
./configure --enable-logging
cmake -DPIO_ENABLE_LOGGING=On
```

### Common Issues

**Decomposition errors:**
- Ensure 1-based indexing for PIO API, 0-based for netCDF integration
- Check that all global array elements are covered
- Verify no overlapping elements between tasks

**Performance issues:**
- Profile with different I/O task counts
- Test both rearrangers
- Check file system (Lustre striping, etc.)
- Monitor MPI communication patterns

**Build issues:**
- Ensure all libraries built with same MPI compiler
- Check netCDF built with `--enable-parallel-tests`
- Verify HDF5 built with parallel support

## Resources

- **Documentation**: https://ncar.github.io/ParallelIO/
- **GitHub**: https://github.com/NCAR/ParallelIO
- **Mailing List**: parallelio@googlegroups.com
- **Examples**: `examples/c/` and `examples/f03/` in source tree
- **NetCDF Integration Examples**: `tests/ncint/` and `tests/fncint/`

## Quick Reference

### Key Data Types

**C:**
- `int iosysid` - IOsystem ID
- `int ncid` - File ID
- `int ioid` - Decomposition ID
- `PIO_Offset *compmap` - Decomposition map (1-based)

**Fortran:**
- `type(iosystem_desc_t)` - IOsystem descriptor
- `type(file_desc_t)` - File descriptor
- `type(var_desc_t)` - Variable descriptor
- `type(io_desc_t)` - Decomposition descriptor

### Essential Functions

| Task | C Function | Fortran Subroutine |
|------|-----------|-------------------|
| Init Intracomm | `PIOc_Init_Intracomm()` | `PIO_init()` |
| Init Async | `PIOc_init_async()` | `PIO_init()` |
| Create decomp | `PIOc_InitDecomp()` | `PIO_initdecomp()` |
| Create file | `PIOc_createfile()` | `PIO_createfile()` |
| Write darray | `PIOc_write_darray()` | `PIO_write_darray()` |
| Read darray | `PIOc_read_darray()` | `PIO_read_darray()` |
| Free decomp | `PIOc_freedecomp()` | `PIO_freedecomp()` |
| Finalize | `PIOc_finalize()` | `PIO_finalize()` |

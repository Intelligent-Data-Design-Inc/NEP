# NetCDF Fortran 90 Interface Guide

## Overview

The NetCDF Fortran 90 interface provides a modern Fortran API to the NetCDF library. It wraps the underlying C library with type-safe Fortran 90 modules and interfaces.

**Primary Module**: `netcdf` (defined in `netcdf.f90`)

**Documentation**: https://docs.unidata.ucar.edu/netcdf-fortran/current/f90_The-NetCDF-Fortran-90-Interface-Guide.html

## Key Concepts

### Operating Modes

NetCDF datasets operate in one of two modes:

- **Define Mode**: Create dimensions, variables, and attributes. Cannot read/write variable data.
- **Data Mode**: Access data and modify existing attributes. Cannot create new dimensions, variables, or attributes.

### Identifiers

- **NetCDF ID**: Small non-negative integer returned when opening/creating a dataset
- **Variable ID**: Integer identifying a variable (1, 2, 3... in order of definition)
- **Dimension ID**: Integer identifying a dimension
- **Attribute**: Identified by name and associated variable/dataset

## Common Usage Patterns

### 1. Creating a NetCDF Dataset

```fortran
use netcdf
implicit none

integer :: ncid, status
integer :: x_dimid, y_dimid, time_dimid
integer :: temp_varid

! Create dataset
status = nf90_create("output.nc", NF90_CLOBBER, ncid)
if (status /= NF90_NOERR) call handle_error(status)

! Define dimensions (in define mode by default)
status = nf90_def_dim(ncid, "x", 100, x_dimid)
status = nf90_def_dim(ncid, "y", 50, y_dimid)
status = nf90_def_dim(ncid, "time", NF90_UNLIMITED, time_dimid)

! Define variables
status = nf90_def_var(ncid, "temperature", NF90_FLOAT, &
                      [x_dimid, y_dimid, time_dimid], temp_varid)

! Add attributes
status = nf90_put_att(ncid, temp_varid, "units", "celsius")

! End define mode, enter data mode
status = nf90_enddef(ncid)

! Write data
status = nf90_put_var(ncid, temp_varid, data_array)

! Close dataset
status = nf90_close(ncid)
```

### 2. Reading a Dataset with Known Names

```fortran
use netcdf
implicit none

integer :: ncid, varid, status
real, allocatable :: data(:,:,:)

! Open dataset for reading
status = nf90_open("input.nc", NF90_NOWRITE, ncid)

! Get variable ID from name
status = nf90_inq_varid(ncid, "temperature", varid)

! Read data
status = nf90_get_var(ncid, varid, data)

! Close dataset (optional for read-only)
status = nf90_close(ncid)
```

### 3. Reading a Dataset with Unknown Names

```fortran
use netcdf
implicit none

integer :: ncid, status
integer :: ndims, nvars, ngatts, unlimdimid
integer :: i, varid
character(len=NF90_MAX_NAME) :: varname

! Open dataset
status = nf90_open("input.nc", NF90_NOWRITE, ncid)

! Inquire about dataset contents
status = nf90_inquire(ncid, ndims, nvars, ngatts, unlimdimid)

! Loop through variables
do i = 1, nvars
    status = nf90_inquire_variable(ncid, i, name=varname)
    print *, "Variable: ", trim(varname)
end do

status = nf90_close(ncid)
```

## Core Function Categories

### Dataset Operations

- **`NF90_CREATE`**: Create a new NetCDF dataset
- **`NF90_OPEN`**: Open an existing dataset
- **`NF90_CLOSE`**: Close an open dataset
- **`NF90_REDEF`**: Enter define mode
- **`NF90_ENDDEF`**: Exit define mode, enter data mode
- **`NF90_SYNC`**: Synchronize dataset to disk
- **`NF90_ABORT`**: Close dataset without saving changes
- **`NF90_INQUIRE`**: Get information about dataset
- **`NF90_SET_FILL`**: Set fill mode for variables

### Dimension Operations

- **`NF90_DEF_DIM`**: Define a dimension
- **`NF90_INQ_DIMID`**: Get dimension ID from name
- **`NF90_INQUIRE_DIMENSION`**: Get dimension information
- **`NF90_RENAME_DIM`**: Rename a dimension

### Variable Operations

- **`NF90_DEF_VAR`**: Define a variable
- **`NF90_INQ_VARID`**: Get variable ID from name
- **`NF90_INQUIRE_VARIABLE`**: Get variable information
- **`NF90_PUT_VAR`**: Write data to a variable
- **`NF90_GET_VAR`**: Read data from a variable
- **`NF90_RENAME_VAR`**: Rename a variable
- **`NF90_DEF_VAR_FILL`**: Define fill parameters
- **`NF90_INQ_VAR_FILL`**: Get fill parameters
- **`NF90_DEF_VAR_FILTER`**: Define filter/compression
- **`NF90_INQ_VAR_FILTER`**: Get filter information

### Attribute Operations

- **`NF90_PUT_ATT`**: Write an attribute
- **`NF90_GET_ATT`**: Read an attribute
- **`NF90_INQ_ATTNAME`**: Get attribute name from number
- **`NF90_INQUIRE_ATTRIBUTE`**: Get attribute information
- **`NF90_RENAME_ATT`**: Rename an attribute
- **`NF90_DEL_ATT`**: Delete an attribute
- **`NF90_COPY_ATT`**: Copy attribute to another variable

### Group Operations (NetCDF-4)

- **`NF90_DEF_GRP`**: Create a group
- **`NF90_INQ_NCID`**: Get group ID from name
- **`NF90_INQ_GRPS`**: Get child group IDs
- **`NF90_INQ_GRPNAME`**: Get group name
- **`NF90_RENAME_GRP`**: Rename a group

### User-Defined Types (NetCDF-4)

- **`NF90_DEF_COMPOUND`**: Define compound type
- **`NF90_DEF_VLEN`**: Define variable-length type
- **`NF90_DEF_OPAQUE`**: Define opaque type
- **`NF90_DEF_ENUM`**: Define enumeration type

## Data Types

### NetCDF External Types and Fortran Constants

| NetCDF Type | Fortran 90 Constant | Bits |
|-------------|---------------------|------|
| byte        | NF90_BYTE          | 8    |
| char        | NF90_CHAR          | 8    |
| short       | NF90_SHORT         | 16   |
| int         | NF90_INT           | 32   |
| float       | NF90_FLOAT         | 32   |
| double      | NF90_DOUBLE        | 64   |
| ubyte       | NF90_UBYTE         | 8    |
| ushort      | NF90_USHORT        | 16   |
| uint        | NF90_UINT          | 32   |
| int64       | NF90_INT64         | 64   |
| uint64      | NF90_UINT64        | 64   |
| string      | NF90_STRING        | -    |

## Variable I/O Flexibility

The `NF90_PUT_VAR` and `NF90_GET_VAR` functions support flexible data access:

### Basic Usage
```fortran
! Write entire array
status = nf90_put_var(ncid, varid, data_array)

! Read entire array
status = nf90_get_var(ncid, varid, data_array)
```

### Subsetting with start/count
```fortran
! Write a subset starting at index (10,20) with size (5,10)
status = nf90_put_var(ncid, varid, data_array, &
                      start=[10,20], count=[5,10])
```

### Strided Access
```fortran
! Read every other element
status = nf90_get_var(ncid, varid, data_array, &
                      start=[1,1], count=[50,25], stride=[2,2])
```

### Mapped Access
```fortran
! Non-contiguous memory mapping
status = nf90_put_var(ncid, varid, data_array, &
                      start=[1,1], count=[10,10], map=[1,100])
```

## Error Handling

All NetCDF Fortran functions return an integer status code:

```fortran
integer :: status

status = nf90_open("file.nc", NF90_NOWRITE, ncid)
if (status /= NF90_NOERR) then
    print *, trim(nf90_strerror(status))
    stop "Error opening file"
end if
```

### Common Error Codes

- **`NF90_NOERR`**: No error
- **`NF90_EBADID`**: Invalid NetCDF ID
- **`NF90_ENOTVAR`**: Variable not found
- **`NF90_EINDEFINE`**: Operation not allowed in define mode
- **`NF90_ENOTINDEFINE`**: Operation requires define mode
- **`NF90_EINVAL`**: Invalid argument

### Error Message Function

**`NF90_STRERROR(status)`**: Returns descriptive error message string

## Mode Flags

### File Creation/Opening Modes

- **`NF90_NOWRITE`**: Open read-only
- **`NF90_WRITE`**: Open for writing
- **`NF90_CLOBBER`**: Overwrite existing file
- **`NF90_NOCLOBBER`**: Fail if file exists
- **`NF90_SHARE`**: Disable buffering for immediate writes
- **`NF90_NETCDF4`**: Create NetCDF-4/HDF5 file
- **`NF90_CLASSIC_MODEL`**: Use classic data model with NetCDF-4
- **`NF90_64BIT_OFFSET`**: Use CDF-2 format (large file support)
- **`NF90_64BIT_DATA`**: Use CDF-5 format (large variable support)

### Variable Storage Options (NetCDF-4)

- **`NF90_CHUNKED`**: Use chunked storage
- **`NF90_CONTIGUOUS`**: Use contiguous storage
- **`NF90_COMPACT`**: Use compact storage (small variables)

### Compression Options (NetCDF-4)

```fortran
! Define variable with compression
status = nf90_def_var(ncid, "data", NF90_FLOAT, dimids, varid, &
                      deflate_level=6, shuffle=.true.)
```

## Parallel I/O (NetCDF-4 with MPI)

```fortran
use netcdf
use mpi
implicit none

integer :: ncid, varid, status
integer :: comm, info

! Initialize MPI
call MPI_Init(ierr)
comm = MPI_COMM_WORLD
info = MPI_INFO_NULL

! Create parallel file
status = nf90_create_par("parallel.nc", &
                         IOR(NF90_NETCDF4, NF90_MPIIO), &
                         comm, info, ncid)

! Set collective access
status = nf90_var_par_access(ncid, varid, NF90_COLLECTIVE)

! Each process writes its portion
status = nf90_put_var(ncid, varid, local_data, &
                      start=[my_start], count=[my_count])

status = nf90_close(ncid)
call MPI_Finalize(ierr)
```

## Best Practices

### 1. Always Check Return Status
```fortran
if (status /= NF90_NOERR) call handle_error(status)
```

### 2. Close Files Explicitly
```fortran
status = nf90_close(ncid)
```

### 3. Use NF90_SYNC for Critical Data
```fortran
! Ensure data is written to disk
status = nf90_sync(ncid)
```

### 4. Minimize Define Mode Transitions
Define all dimensions, variables, and attributes before entering data mode to avoid performance overhead.

### 5. Use Chunking for Large Arrays (NetCDF-4)
```fortran
status = nf90_def_var(ncid, "data", NF90_FLOAT, dimids, varid, &
                      chunksizes=[100,100,1])
```

### 6. Enable Compression for Large Datasets
```fortran
status = nf90_def_var(ncid, "data", NF90_FLOAT, dimids, varid, &
                      deflate_level=4, shuffle=.true.)
```

## Relationship to C Library

The Fortran 90 interface is a wrapper around the NetCDF-C library:

1. **Module**: `netcdf.f90` provides Fortran 90 interfaces
2. **Binding**: Calls C functions via ISO_C_BINDING
3. **Naming**: Fortran functions use `NF90_` prefix (C uses `nc_`)
4. **Types**: Fortran constants map to C types
5. **Arrays**: Fortran column-major order vs C row-major (handled internally)

## Integration with NetCDF-C Architecture

The Fortran interface sits on top of the C library dispatch architecture:

```
Fortran 90 Application
        ↓
netcdf.f90 module (NF90_* functions)
        ↓
ISO_C_BINDING layer
        ↓
NetCDF-C API (nc_* functions)
        ↓
libdispatch (dispatch tables)
        ↓
Format-specific implementations
(NC3, HDF5, Zarr, DAP)
```

All format support, dispatch routing, and I/O operations are handled by the underlying C library.

## Additional Resources

- **Main Guide**: https://docs.unidata.ucar.edu/netcdf-fortran/current/
- **API Reference**: https://docs.unidata.ucar.edu/netcdf-fortran/current/f90_The-NetCDF-Fortran-90-Interface-Guide.html
- **Examples**: https://docs.unidata.ucar.edu/netcdf-fortran/current/examples.html

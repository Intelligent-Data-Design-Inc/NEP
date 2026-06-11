# NetCDF-C Programming Examples and Patterns

This reference provides practical examples and common programming patterns for working with NetCDF-C, based on official documentation and example programs.

## Overview

NetCDF-C provides example programs demonstrating:
- Basic file creation and reading (NetCDF-3)
- Enhanced features (NetCDF-4: groups, compression, user-defined types)
- Real-world patterns (meteorological data, time series, multidimensional arrays)

**Example Location**: `examples/C/` directory in NetCDF-C source

**Documentation**: https://docs.unidata.ucar.edu/netcdf-c/current/examples1.html

## Basic NetCDF-3 Examples

### Example 1: simple_xy - Minimal File Operations

**Files**: `simple_xy_wr.c`, `simple_xy_rd.c`

**Purpose**: Demonstrates absolute minimum operations to create and read a NetCDF file.

**What it creates**: 2D array (6x12) with dimensions "x" and "y", variable "data"

**Key Pattern - File Creation**:
```c
int ncid, x_dimid, y_dimid, varid;
int dimids[2];

// Create file (NC_CLOBBER overwrites existing)
nc_create("simple_xy.nc", NC_CLOBBER, &ncid);

// Define dimensions
nc_def_dim(ncid, "x", NX, &x_dimid);
nc_def_dim(ncid, "y", NY, &y_dimid);

// Define variable with dimensions
dimids[0] = x_dimid;
dimids[1] = y_dimid;
nc_def_var(ncid, "data", NC_INT, 2, dimids, &varid);

// End define mode (required before writing data)
nc_enddef(ncid);

// Write data
nc_put_var_int(ncid, varid, &data[0][0]);

// Close file
nc_close(ncid);
```

**Key Pattern - File Reading**:
```c
int ncid, varid;
int data_in[NX][NY];

// Open file for reading
nc_open("simple_xy.nc", NC_NOWRITE, &ncid);

// Get variable ID by name
nc_inq_varid(ncid, "data", &varid);

// Read entire variable
nc_get_var_int(ncid, varid, &data_in[0][0]);

// Close file
nc_close(ncid);
```

### Example 2: sfc_pres_temp - Adding Metadata

**Files**: `sfc_pres_temp_wr.c`, `sfc_pres_temp_rd.c`

**Purpose**: Demonstrates adding attributes and coordinate variables (CF conventions).

**What it creates**: Surface temperature and pressure on 6x12 lat/lon grid with metadata

**Key Pattern - Adding Attributes**:
```c
// Define variable
nc_def_var(ncid, "temperature", NC_FLOAT, 2, dimids, &temp_varid);

// Add units attribute
nc_put_att_text(ncid, temp_varid, "units", 11, "celsius");

// Add global attribute
nc_put_att_text(ncid, NC_GLOBAL, "title", 23, "Surface Temperature Data");
```

**Key Pattern - Coordinate Variables**:
```c
// Define latitude dimension
nc_def_dim(ncid, "latitude", NLAT, &lat_dimid);

// Define latitude coordinate variable (same name as dimension)
nc_def_var(ncid, "latitude", NC_FLOAT, 1, &lat_dimid, &lat_varid);
nc_put_att_text(ncid, lat_varid, "units", 13, "degrees_north");

// Write coordinate values
float lats[NLAT] = {25, 30, 35, 40, 45, 50};
nc_put_var_float(ncid, lat_varid, lats);
```

**Best Practice**: Coordinate variables should have the same name as their dimension and include units.

### Example 3: pres_temp_4D - Unlimited Dimensions and Time Series

**Files**: `pres_temp_4D_wr.c`, `pres_temp_4D_rd.c`

**Purpose**: Demonstrates 4D data with unlimited time dimension, writing one timestep at a time.

**What it creates**: Temperature and pressure with dimensions [time, level, lat, lon]

**Key Pattern - Unlimited Dimension**:
```c
// Define unlimited dimension (use NC_UNLIMITED for size)
nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dimid);

// Define variable with unlimited dimension first
int dimids[4] = {time_dimid, level_dimid, lat_dimid, lon_dimid};
nc_def_var(ncid, "temperature", NC_FLOAT, 4, dimids, &temp_varid);
```

**Key Pattern - Writing Time Steps**:
```c
// Write one time step at a time
for (int rec = 0; rec < NREC; rec++) {
    size_t start[4] = {rec, 0, 0, 0};  // Start at this time step
    size_t count[4] = {1, NLVL, NLAT, NLON};  // Write one time slice
    
    // Prepare data for this time step
    float temp_out[NLVL][NLAT][NLON];
    // ... fill temp_out ...
    
    // Write hyperslab
    nc_put_vara_float(ncid, temp_varid, start, count, &temp_out[0][0][0]);
}
```

**Key Pattern - Reading Time Steps**:
```c
// Read one time step
size_t start[4] = {rec, 0, 0, 0};
size_t count[4] = {1, NLVL, NLAT, NLON};
nc_get_vara_float(ncid, temp_varid, start, count, &temp_in[0][0][0]);
```

## NetCDF-4 Enhanced Examples

### Example 4: simple_nc4 - Groups and User-Defined Types

**Files**: `simple_nc4_wr.c`, `simple_nc4_rd.c`

**Purpose**: Demonstrates NetCDF-4 groups and compound types.

**What it creates**: Two groups with different data types (uint64 and compound)

**Key Pattern - Creating Groups**:
```c
int ncid, grp1_id, grp2_id;

// Create NetCDF-4 file
nc_create("simple_nc4.nc", NC_NETCDF4, &ncid);

// Create groups
nc_def_grp(ncid, "grp1", &grp1_id);
nc_def_grp(ncid, "grp2", &grp2_id);

// Define variable in group
nc_def_var(grp1_id, "data", NC_UINT64, 2, dimids, &varid);
```

**Key Pattern - Compound Types**:
```c
typedef struct {
    int i1;
    int i2;
} compound_data;

nc_type compound_typeid;

// Define compound type
nc_def_compound(grp2_id, sizeof(compound_data), "compound_t", &compound_typeid);
nc_insert_compound(grp2_id, compound_typeid, "i1", 
                   NC_COMPOUND_OFFSET(compound_data, i1), NC_INT);
nc_insert_compound(grp2_id, compound_typeid, "i2",
                   NC_COMPOUND_OFFSET(compound_data, i2), NC_INT);

// Use compound type for variable
nc_def_var(grp2_id, "data", compound_typeid, 2, dimids, &varid);
```

### Example 5: simple_xy_nc4 - Compression and Chunking

**Files**: `simple_xy_nc4_wr.c`, `simple_xy_nc4_rd.c`

**Purpose**: Demonstrates chunking, compression, and checksums (HDF5 features).

**Key Pattern - Chunking**:
```c
// Define variable
nc_def_var(ncid, "data", NC_INT, 2, dimids, &varid);

// Set chunking (required for compression)
size_t chunks[2] = {4, 4};  // Chunk size for each dimension
nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks);
```

**Key Pattern - Compression**:
```c
// Enable deflate compression (level 1-9, 9 = maximum compression)
int shuffle = NC_SHUFFLE;  // Shuffle filter improves compression
int deflate = 1;           // Enable deflate
int deflate_level = 5;     // Compression level
nc_def_var_deflate(ncid, varid, shuffle, deflate, deflate_level);
```

**Key Pattern - Checksums**:
```c
// Enable fletcher32 checksum for data integrity
nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32);
```

### Example 6: filter_example - Custom Filters

**Files**: `filter_example.c`

**Purpose**: Demonstrates using custom compression filters (e.g., bzip2).

**Key Pattern - Custom Filter**:
```c
// Define variable with chunking (required for filters)
nc_def_var(ncid, "data", NC_INT, 2, dimids, &varid);
size_t chunks[2] = {100, 100};
nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunks);

// Apply custom filter (bzip2 example)
unsigned int filter_id = 307;  // Bzip2 filter ID
size_t nparams = 1;
unsigned int params[1] = {9};  // Compression level
nc_def_var_filter(ncid, varid, filter_id, nparams, params);
```

## Common Programming Patterns

### Pattern 1: Error Handling

**Always check return codes**:
```c
int retval;

if ((retval = nc_create(FILE_NAME, NC_CLOBBER, &ncid)))
    ERR(retval);

// Or use macro
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return 2;}
```

### Pattern 2: Inquiry Functions

**Get file information without prior knowledge**:
```c
int ncid, ndims, nvars, ngatts, unlimdimid;

// Open file
nc_open("file.nc", NC_NOWRITE, &ncid);

// Get file metadata
nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);

// Inquire about specific dimension
char dim_name[NC_MAX_NAME+1];
size_t dim_len;
nc_inq_dim(ncid, dimid, dim_name, &dim_len);

// Inquire about variable
char var_name[NC_MAX_NAME+1];
nc_type var_type;
int var_ndims, var_dimids[NC_MAX_VAR_DIMS], var_natts;
nc_inq_var(ncid, varid, var_name, &var_type, &var_ndims, 
           var_dimids, &var_natts);
```

### Pattern 3: Subsetting Data (Hyperslabs)

**Read/write portions of arrays**:
```c
// Read a subset: time=5, all levels, lat 10-20, lon 30-40
size_t start[4] = {5, 0, 10, 30};
size_t count[4] = {1, NLVL, 10, 10};
float subset[NLVL][10][10];

nc_get_vara_float(ncid, varid, start, count, &subset[0][0][0]);
```

### Pattern 4: Strided Access

**Read every Nth element**:
```c
// Read every 2nd element in each dimension
size_t start[2] = {0, 0};
size_t count[2] = {NX/2, NY/2};
ptrdiff_t stride[2] = {2, 2};  // Skip every other element

nc_get_vars_float(ncid, varid, start, count, stride, data);
```

### Pattern 5: Fill Values

**Handle missing data**:
```c
// Set custom fill value
float fill_value = -999.0;
nc_def_var_fill(ncid, varid, NC_FILL, &fill_value);

// Disable fill values (for performance)
nc_def_var_fill(ncid, varid, NC_NOFILL, NULL);
```

### Pattern 6: Parallel I/O (NetCDF-4 with HDF5)

**MPI parallel access**:
```c
#include <netcdf_par.h>

// Initialize MPI
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

// Create file with parallel access
nc_create_par("parallel.nc", NC_NETCDF4|NC_MPIIO, 
              MPI_COMM_WORLD, MPI_INFO_NULL, &ncid);

// Set collective access for variable
nc_var_par_access(ncid, varid, NC_COLLECTIVE);

// Each process writes its portion
size_t start[1] = {rank * chunk_size};
size_t count[1] = {chunk_size};
nc_put_vara_float(ncid, varid, start, count, local_data);

// Close and finalize
nc_close(ncid);
MPI_Finalize();
```

## Best Practices from Examples

### File Creation
1. **Always use NC_CLOBBER or NC_NOCLOBBER** to control overwrite behavior
2. **Classic CDF-1 is the default**: `nc_create(path, NC_CLOBBER, &ncid)` creates a classic file — no format flag needed
3. **NC_CLASSIC_MODEL is only for NetCDF-4**: Use `NC_NETCDF4 | NC_CLASSIC_MODEL` to get HDF5 storage with classic data model restrictions (no groups, no user-defined types). Do NOT use `NC_CLASSIC_MODEL` alone for classic CDF-1 files.
4. **End define mode** with `nc_enddef()` before writing data
5. **Close files** with `nc_close()` to ensure data is flushed

### Dimensions
1. **Unlimited dimension first** in dimension order for best performance
2. **One unlimited dimension** in NetCDF-3, multiple allowed in NetCDF-4
3. **Coordinate variables** should match dimension names

### Variables
1. **Add units attribute** to all data variables (CF convention)
2. **Use appropriate data types** (NC_FLOAT for most scientific data)
3. **Enable chunking** before compression or filters

### Attributes
1. **Use standard names** from CF conventions when possible
2. **Add global attributes** for file-level metadata (title, history, etc.)
3. **Document missing values** with _FillValue or missing_value attributes

### Performance
1. **Use chunking** for large datasets accessed in subsets
2. **Enable compression** to reduce file size (deflate level 5 is good default)
3. **Write contiguously** when possible (avoid random access)
4. **Use collective I/O** in parallel applications

### NetCDF-4 Features
1. **Use groups** to organize related variables
2. **Compound types** for structured data (like C structs)
3. **Compression** is transparent to readers
4. **Checksums** ensure data integrity

## Tutorial Topics

The NetCDF-C tutorial covers these key areas:

### Data Model
- **Classic Model**: Dimensions, variables, attributes
- **Enhanced Model**: Groups, user-defined types, multiple unlimited dimensions
- **Unlimited Dimensions**: Growing datasets (time series)
- **Strings**: NC_STRING type in NetCDF-4

### File Operations
- **Creating Files**: Define mode vs data mode
- **Reading Known Structure**: When you know the schema
- **Reading Unknown Structure**: Generic file inspection
- **Subsets**: Hyperslabs, strides, mapped access

### Advanced Topics
- **Error Handling**: Return codes and nc_strerror()
- **HDF5 Interoperability**: Reading HDF5 files as NetCDF
- **Parallel I/O**: MPI-based parallel access
- **Fill Values**: Handling missing/unwritten data

## Command-Line Tools

### ncdump - Examine Files
```bash
# View file structure
ncdump -h file.nc

# View data
ncdump file.nc

# View as CDL
ncdump -c file.nc
```

### ncgen - Generate Files from CDL
```bash
# Create NetCDF file from CDL
ncgen -o output.nc input.cdl

# Create NetCDF-4 file
ncgen -k nc4 -o output.nc input.cdl
```

### nccopy - Copy and Convert
```bash
# Convert NetCDF-3 to NetCDF-4 with compression
nccopy -k nc4 -d 5 input.nc output.nc

# Rechunk file
nccopy -c "var:10,20,30" input.nc output.nc
```

## Example File Locations

In the NetCDF-C source tree:
- `examples/C/` - C examples
- `examples/CDL/` - CDL files for ncgen
- `nc_test/` - Test programs (also good examples)
- `nc_test4/` - NetCDF-4 test programs

## Additional Resources

**Official Documentation**:
- Tutorial: https://docs.unidata.ucar.edu/netcdf-c/current/tutorial_8dox.html
- Examples: https://docs.unidata.ucar.edu/netcdf-c/current/examples1.html
- API Reference: https://docs.unidata.ucar.edu/netcdf-c/current/modules.html

**CF Conventions**:
- http://cfconventions.org/ - Climate and Forecast metadata conventions

**Best Practices**:
- Use coordinate variables for dimensions
- Include units attributes
- Add descriptive global attributes
- Follow CF conventions when applicable
- Enable compression for large datasets
- Use chunking for subsetting access patterns

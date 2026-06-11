---
name: opendap
description: Understanding OPeNDAP (Open-source Project for a Network Data Access Protocol) for accessing remote scientific data via HTTP, including DAP2/DAP4 protocols, constraint expressions, data models, and client integration. Use when working with OPeNDAP URLs, writing data access code, or integrating with NetCDF.
metadata:
  author: opendap-documentation
  version: "1.0"
  date: "2026-01-19"
---

# OPeNDAP Skill

This skill provides comprehensive knowledge of OPeNDAP to help you access, serve, and work with remote scientific data effectively.

## Overview

OPeNDAP (Open-source Project for a Network Data Access Protocol) provides a way for researchers to access scientific data anywhere on the Internet from a wide variety of programs. It uses a client/server architecture built on HTTP and provides flexible data subsetting through constraint expressions.

**Key Features**:
- Network-transparent data access via URLs
- Data subsetting at the server (reduces bandwidth)
- Format-independent data model
- Compatible with NetCDF, HDF5, and other formats
- Supports gridded data, sequences, and complex structures

## Core Concepts

### 1. Client/Server Architecture

OPeNDAP uses a web-based client/server model similar to the World Wide Web:

- **Server (Hyrax)**: Translates data from storage format to DAP format for transmission
- **Client**: Requests data via URLs and translates DAP format to local API format
- **Protocol**: HTTP-based Data Access Protocol (DAP)

**Data Flow**:
```
User Program → OPeNDAP Client Library → HTTP Request → OPeNDAP Server
                                                              ↓
User Program ← Translated Data ← DAP Response ← Read Local Files
```

### 2. OPeNDAP URLs

An OPeNDAP URL identifies a dataset and optionally includes a constraint expression:

**Basic URL Structure**:
```
http://server.domain/path/to/dataset.nc
```

**URL with Constraint Expression**:
```
http://server.domain/path/to/dataset.nc?variable[start:stop]&selection_clause
```

**URL Suffixes** (Service Endpoints):
- `.dds` - Dataset Descriptor Structure (DAP2 - data shape)
- `.das` - Data Attribute Structure (DAP2 - metadata)
- `.dmr.xml` - Dataset Metadata Response (DAP4 - combined structure)
- `.dods` - Binary data (DAP2)
- `.dap` - Binary data (DAP4)
- `.ascii` - ASCII representation of data
- `.html` - Web form interface
- `.info` - Combined DDS + DAS in HTML

### 3. Data Access Protocol (DAP)

**DAP2** (Older, widely supported):
- Separate DDS and DAS responses
- Binary data in .dods format
- Simpler data model

**DAP4** (Newer, enhanced):
- Unified DMR (Dataset Metadata Response) in XML
- Enhanced data model with groups
- Better support for complex types
- Improved performance

## OPeNDAP Data Model

### Base Types

- **Byte, Int16, Int32, Int64** - Integer types
- **UInt16, UInt32, UInt64** - Unsigned integers  
- **Float32, Float64** - Floating-point numbers
- **String** - Character strings
- **URL** - Uniform Resource Locators

### Constructor Types

- **Array** - Multi-dimensional arrays with indexing
- **Structure** - Collection of related variables
- **Sequence** - Ordered collection of instances (like database rows)
- **Grid** - Array with coordinate map vectors

**Example Grid**:
```
Grid {
  Array:
    Int16 sst[time=1857][lat=89][lon=180];
  Maps:
    Float64 time[time=1857];
    Float64 lat[lat=89];
    Float64 lon[lon=180];
} sst;
```

## Constraint Expressions

Constraint expressions allow you to subset data on the server before transmission.

### Syntax

```
URL?projection&selection
```

- **Projection**: Comma-separated list of variables to return
- **Selection**: Boolean expressions to filter data (prefixed with &)

### Array Subsetting

**Single element**:
```
?variable[index]
?sst[0][10][20]
```

**Range (start:stop)**:
```
?variable[start:stop]
?sst[0:10][20:30][40:50]
```

**Stride (start:stride:stop)**:
```
?variable[start:stride:stop]
?sst[0:2:100]  # Every 2nd element from 0 to 100
```

### Selection Clauses

**Comparison operators**: `<`, `>`, `<=`, `>=`, `=`, `!=`

**Examples**:
```
?station&station.temp>20.0
?station&station.lat>0.0&station.lon<-60.0
?station&station.month={4,5,6,7}  # List for OR
```

### Server Functions

Hyrax servers support functions for advanced operations:

**geogrid()** - Subset by geographic coordinates:
```
?geogrid(sst, 62, 206, 56, 210, "19722<time<19755")
```

**linear_scale()** - Scale data values:
```
?linear_scale(sst, 0.01, 0)  # y = 0.01*x + 0
```

**version()** - List available functions:
```
?version()
```

Call any function with no arguments to see its documentation.

## Using OPeNDAP with NetCDF

### NetCDF-C Library

The NetCDF-C library has built-in OPeNDAP support. Simply use an OPeNDAP URL instead of a file path:

```c
#include <netcdf.h>

int ncid, varid;
char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";

// Open remote dataset
nc_open(url, NC_NOWRITE, &ncid);

// Access variables normally
nc_inq_varid(ncid, "sst", &varid);

// Read data with subsetting
size_t start[] = {0, 0, 0};
size_t count[] = {1, 10, 10};
float data[10][10];
nc_get_vara_float(ncid, varid, start, count, &data[0][0]);

nc_close(ncid);
```

### URL with Constraint Expression

You can include constraint expressions in the URL:

```c
char *url = "http://server.org/data.nc?sst[0:10][20:30][40:50]";
nc_open(url, NC_NOWRITE, &ncid);
```

### Fortran Programs

```fortran
program read_opendap
  use netcdf
  implicit none
  
  integer :: ncid, varid, status
  character(len=256) :: url
  real :: data(10, 10)
  
  url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
  
  status = nf90_open(url, NF90_NOWRITE, ncid)
  status = nf90_inq_varid(ncid, "sst", varid)
  status = nf90_get_var(ncid, varid, data, &
                        start=[1,1,1], count=[10,10,1])
  status = nf90_close(ncid)
  
end program read_opendap
```

## Common Workflows

### 1. Exploring a Dataset

**Step 1**: Get the DMR/DDS to see structure:
```
http://server.org/data.nc.dmr.xml
http://server.org/data.nc.dds  (DAP2)
```

**Step 2**: Get attributes:
```
http://server.org/data.nc.das  (DAP2)
```

**Step 3**: Use .info for combined view:
```
http://server.org/data.nc.info
```

### 2. Subsetting Data

**Step 1**: Identify variable and dimensions from DMR/DDS

**Step 2**: Construct constraint expression:
```
?variable[time_start:time_end][lat_start:lat_end][lon_start:lon_end]
```

**Step 3**: Test with .ascii to verify:
```
http://server.org/data.nc.ascii?sst[0:1][10:20][30:40]
```

**Step 4**: Use in your program with full URL

### 3. Working with Sequences

Sequences are like database tables with rows of data:

**Get specific fields**:
```
?sequence.field1,sequence.field2
```

**Filter rows**:
```
?sequence.field1,sequence.field2&sequence.field1>100
```

**Multiple conditions**:
```
?sequence&sequence.temp>20&sequence.depth<100
```

## Client Tools

### Matlab

Matlab 2012a+ has built-in OPeNDAP support via NetCDF interface:

```matlab
url = 'http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz';
ncid = netcdf.open(url);
data = netcdf.getVar(ncid, varid);
netcdf.close(ncid);
```

### Python

Using netCDF4-python or xarray:

```python
import netCDF4
url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
ds = netCDF4.Dataset(url)
sst = ds.variables['sst'][0:10, 20:30, 40:50]
```

### Command-line Tools

**ncdump** (with OPeNDAP-enabled NetCDF):
```bash
ncdump -h "http://server.org/data.nc"
ncdump -v sst "http://server.org/data.nc?sst[0:10][20:30]"
```

## Troubleshooting

### Common Issues

**1. URL not recognized**:
- Ensure NetCDF library is compiled with DAP support
- Check URL syntax (http:// or https://)

**2. Constraint expression errors**:
- Verify variable names match DDS/DMR exactly
- Check array bounds (0-indexed)
- Ensure proper quoting in shell commands

**3. Performance issues**:
- Use constraint expressions to reduce data transfer
- Request only needed variables
- Consider server-side functions for processing

**4. Authentication**:
- Some servers require credentials
- Use .netrc file or URL-embedded credentials
- Check server documentation for auth methods

## Quick Reference

### Essential URL Patterns

```
# Get metadata
http://server/dataset.nc.dmr.xml  (DAP4)
http://server/dataset.nc.dds      (DAP2)
http://server/dataset.nc.das      (DAP2)

# Get data
http://server/dataset.nc?var[0:10]
http://server/dataset.nc.ascii?var[0:10]

# Multiple variables
http://server/dataset.nc?var1,var2,var3

# With selection
http://server/dataset.nc?var&var>100
```

### Constraint Expression Operators

- Array: `[start:stop]`, `[start:stride:stop]`
- Comparison: `<`, `>`, `<=`, `>=`, `=`, `!=`
- String match: `~=` (regex)
- Lists: `{val1,val2,val3}` (OR operation)
- Structure fields: `structure.field`

## Additional Resources

See [references/PROTOCOL.md](references/PROTOCOL.md) for DAP2 and DAP4 protocol details.

See [references/CONSTRAINTS.md](references/CONSTRAINTS.md) for comprehensive constraint expression examples.

See [references/DATA-MODEL.md](references/DATA-MODEL.md) for complete data type documentation.

See [references/CLIENT-USAGE.md](references/CLIENT-USAGE.md) for client integration examples.

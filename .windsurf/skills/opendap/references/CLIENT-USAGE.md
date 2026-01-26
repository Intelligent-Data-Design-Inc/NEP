# OPeNDAP Client Usage

This document provides examples of using OPeNDAP with various client libraries and tools.

## C Programs with NetCDF

### Basic Data Access

```c
#include <netcdf.h>
#include <stdio.h>

int main() {
    int ncid, varid, status;
    size_t start[3] = {0, 0, 0};
    size_t count[3] = {1, 10, 10};
    float data[10][10];
    
    char *url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz";
    
    status = nc_open(url, NC_NOWRITE, &ncid);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error opening URL: %s\n", nc_strerror(status));
        return 1;
    }
    
    status = nc_inq_varid(ncid, "sst", &varid);
    status = nc_get_vara_float(ncid, varid, start, count, &data[0][0]);
    
    printf("Data at [0][5][5]: %f\n", data[5][5]);
    
    nc_close(ncid);
    return 0;
}
```

### Using Constraint Expressions

```c
char *url = "http://server.org/data.nc?sst[0:10][20:30][40:50]";
nc_open(url, NC_NOWRITE, &ncid);
```

### Reading Metadata

```c
int ndims, nvars, ngatts, unlimdimid;
char varname[NC_MAX_NAME+1];

nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);

for (int i = 0; i < nvars; i++) {
    nc_inq_varname(ncid, i, varname);
    printf("Variable %d: %s\n", i, varname);
}
```

## Fortran Programs

### Basic Example

```fortran
program read_opendap
  use netcdf
  implicit none
  
  integer :: ncid, varid, status
  integer :: start(3), count(3)
  real :: data(10, 10, 1)
  character(len=256) :: url
  
  url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
  
  status = nf90_open(url, NF90_NOWRITE, ncid)
  if (status /= NF90_NOERR) then
    print *, "Error: ", trim(nf90_strerror(status))
    stop
  end if
  
  status = nf90_inq_varid(ncid, "sst", varid)
  
  start = [1, 1, 1]
  count = [10, 10, 1]
  status = nf90_get_var(ncid, varid, data, start=start, count=count)
  
  print *, "Sample value: ", data(5, 5, 1)
  
  status = nf90_close(ncid)
  
end program read_opendap
```

### Time Series Extraction

```fortran
program time_series
  use netcdf
  implicit none
  
  integer :: ncid, varid, status, nt
  real, allocatable :: temp(:)
  character(len=256) :: url
  
  url = "http://server.org/data.nc?temp[0:1000][45][90]"
  
  status = nf90_open(url, NF90_NOWRITE, ncid)
  status = nf90_inq_varid(ncid, "temp", varid)
  status = nf90_inq_dimlen(ncid, 1, nt)
  
  allocate(temp(nt))
  status = nf90_get_var(ncid, varid, temp)
  
  print *, "Time series length: ", nt
  print *, "Mean: ", sum(temp)/nt
  
  deallocate(temp)
  status = nf90_close(ncid)
  
end program time_series
```

## Python Examples

### Using netCDF4-python

```python
import netCDF4 as nc

url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
dataset = nc.Dataset(url)

print("Variables:", dataset.variables.keys())

sst = dataset.variables['sst']
print("Shape:", sst.shape)
print("Units:", sst.units)

data = sst[0:10, 20:30, 40:50]
print("Subset shape:", data.shape)

dataset.close()
```

### Using xarray

```python
import xarray as xr

url = "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"
ds = xr.open_dataset(url)

print(ds)

sst_subset = ds.sst.isel(time=slice(0, 10), lat=slice(20, 30), lon=slice(40, 50))
mean_sst = sst_subset.mean()

print(f"Mean SST: {mean_sst.values}")
```

### With Constraint Expressions

```python
url_with_constraint = "http://server.org/data.nc?sst[0:100][0:50][0:80]"
ds = nc.Dataset(url_with_constraint)
```

## Matlab Examples

### Basic Access

```matlab
url = 'http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz';
ncid = netcdf.open(url);

[numdims, numvars, numglobalatts, unlimdimid] = netcdf.inq(ncid);
fprintf('Number of variables: %d\n', numvars);

varid = netcdf.inqVarID(ncid, 'sst');
data = netcdf.getVar(ncid, varid, [0,0,0], [10,10,1]);

fprintf('Sample value: %f\n', data(5,5));

netcdf.close(ncid);
```

### Subsetting

```matlab
url = 'http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz';
ncid = netcdf.open(url);
varid = netcdf.inqVarID(ncid, 'sst');

start = [2, 7, 0];
count = [10, 8, 1];
stride = [1, 1, 1];
data = netcdf.getVar(ncid, varid, start, count, stride);

imagesc(data');
colorbar;
title('SST Subset');
```

## Command-Line Tools

### ncdump

```bash
# View header
ncdump -h "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz"

# Dump specific variable
ncdump -v sst "http://server.org/data.nc?sst[0:10][20:30]"

# ASCII output
ncdump "http://server.org/data.nc" > output.txt
```

### curl

```bash
# Get DDS
curl "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz.dds"

# Get DAS
curl "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz.das"

# Get ASCII data
curl "http://test.opendap.org/dap/data/nc/sst.mnmean.nc.gz.ascii?sst[0][0:5][0:5]"
```

## Error Handling

### C Error Handling

```c
int status;
char *url = "http://server.org/data.nc";

status = nc_open(url, NC_NOWRITE, &ncid);
if (status != NC_NOERR) {
    fprintf(stderr, "NetCDF error: %s\n", nc_strerror(status));
    if (status == NC_ENOTNC) {
        fprintf(stderr, "Not a valid NetCDF/OPeNDAP URL\n");
    }
    return 1;
}
```

### Python Error Handling

```python
try:
    ds = nc.Dataset(url)
    data = ds.variables['sst'][:]
except RuntimeError as e:
    print(f"OPeNDAP error: {e}")
except KeyError as e:
    print(f"Variable not found: {e}")
finally:
    if 'ds' in locals():
        ds.close()
```

## Performance Tips

### Minimize Requests

```c
// Bad - multiple small requests
for (int i = 0; i < 100; i++) {
    nc_get_vara_float(ncid, varid, &i, &one, &value);
}

// Good - single larger request
nc_get_vara_float(ncid, varid, start, count, data);
```

### Use Constraint Expressions

```python
# Bad - download everything then subset
ds = nc.Dataset("http://server.org/data.nc")
subset = ds.variables['sst'][0:10, 20:30, 40:50]

# Good - subset at server
url = "http://server.org/data.nc?sst[0:10][20:30][40:50]"
ds = nc.Dataset(url)
subset = ds.variables['sst'][:]
```

### Cache Metadata

```python
# Cache dataset structure
ds = nc.Dataset(url)
var_names = list(ds.variables.keys())
dims = {name: len(ds.dimensions[name]) for name in ds.dimensions}

# Reuse cached info for multiple accesses
for var_name in var_names:
    data = ds.variables[var_name][:]
```

## Authentication

### Using .netrc

Create `~/.netrc`:
```
machine server.org
login username
password mypassword
```

Set permissions:
```bash
chmod 600 ~/.netrc
```

### URL-Embedded Credentials

```c
char *url = "http://username:password@server.org/data.nc";
```

Note: Not recommended for security reasons.

## Best Practices

1. **Check metadata first** before requesting data
2. **Use constraint expressions** to minimize data transfer
3. **Handle errors gracefully** with proper error checking
4. **Cache when possible** to reduce repeated requests
5. **Close connections** when done
6. **Test URLs** with .ascii or .dds before using in code
7. **Use appropriate data types** matching server types
8. **Consider time zones** for temporal data
9. **Validate data** after retrieval (check for fill values)
10. **Document URLs** and constraint expressions in code

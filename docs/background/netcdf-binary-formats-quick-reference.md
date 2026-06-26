## NetCDF Binary Formats Quick Reference

Quick guide to choosing the right netCDF binary format for your use case.

| Format | Use When | Don't Use When |
|--------|----------|----------------|
| **Classic** | <2GB files, maximum compatibility, legacy tools, simple deployment | Large files, need compression, 64-bit integers, or enhanced model features |
| **CDF-5** | >4GB variables, 64-bit integers, parallel I/O without HDF5 dependency, HPC with PnetCDF | Need compression, groups, user-defined types, or enhanced model features |
| **NetCDF-4/HDF5** | Compression, groups, user-defined types, chunking for partial reads, parallel I/O | HDF5 library dependency is problematic (embedded systems, minimal containers), simple archival needs |
| **ncZarr** | Cloud object storage (S3/GCS/Azure), HTTP access patterns, serverless workflows | Local filesystem (creates many small files), sequential read workloads, long-term archival stability |

### Format Detection

Format is automatically detected on read. The same `nc_open()` call works for all formats.
Format only matters at file creation time.

```c
// Works for all formats - library auto-detects
nc_open("data.nc", NC_NOWRITE, &ncid);
```

### Creating Files

| Format | C Flag | Fortran Flag |
|--------|--------|--------------|
| Classic | (default) `NC_CLOBBER` | (default) `NF90_CLOBBER` |
| 64-bit Offset | `NC_64BIT_OFFSET` | `NF90_64BIT_OFFSET` |
| CDF-5 | `NC_64BIT_DATA` or `NC_CDF5` | `NF90_64BIT_DATA` |
| NetCDF-4/HDF5 | `NC_NETCDF4` | `NF90_NETCDF4` |
| NetCDF-4 Classic Model | `NC_NETCDF4 \| NC_CLASSIC_MODEL` | `IOR(NF90_NETCDF4, NF90_CLASSIC_MODEL)` |
| ncZarr | `NC_ZARR` | `NF90_ZARR` |

### Converting Between Formats

Use `nccopy` to convert between compatible formats:

```bash
# Convert to NetCDF-4 with compression
nccopy -k nc4 -d 4 input.nc output.nc

# Convert to CDF-5
nccopy -k cdf5 input.nc output.nc

# Convert to classic (only if no enhanced features used)
nccopy -k classic input.nc output.nc
```

Not all conversions are possible. Files using enhanced model features (groups, user-defined types, multiple unlimited dimensions) cannot be converted to classic formats.

### Detailed Sections

For complete technical details and code examples, see Chapter 4: Binary Formats in the NetCDF Developer's Handbook.

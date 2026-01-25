# NetCDF-C Dispatch Tables Reference

This reference documents all dispatch table implementations in NetCDF-C.

## Dispatch Table Overview

The `NC_Dispatch` structure contains function pointers for all NetCDF operations. Each format implements this interface.

**Definition**: `include/netcdf_dispatch.h:34-256`

**Current Version**: `NC_DISPATCH_VERSION = 5`

## NC3 Dispatch Table (NetCDF-3)

**File**: `libsrc/nc3dispatch.c:81-174`

**Table Name**: `NC3_dispatcher`

**Model**: `NC_FORMATX_NC3`

### Implementation Summary

**Fully Implemented**:
- File operations: create, open, close, sync, abort, redef, enddef
- Dimensions: def_dim, inq_dim, inq_dimid, inq_unlimdim, rename_dim
- Variables: def_var, inq_var, inq_varid, rename_var
- Attributes: put_att, get_att, inq_att, del_att, rename_att
- Variable I/O: get_vara, put_vara
- Inquiry: inq, inq_format, inq_type
- Fill values: set_fill, def_var_fill

**Delegated to NCDEFAULT**:
- get_vars, put_vars (strided access)
- get_varm, put_varm (mapped access)

**Returns NC_ENOTNC4** (not supported):
- Groups: def_grp, rename_grp, inq_grps, inq_grp_parent
- User-defined types: def_compound, def_vlen, def_enum, def_opaque
- Compression: def_var_deflate, def_var_fletcher32
- Chunking: def_var_chunking, set_var_chunk_cache
- Filters: def_var_filter, inq_var_filter_ids
- Endianness: def_var_endian
- Quantization: def_var_quantize

**Special Implementations**:
- `inq_unlimdims`: Returns single unlimited dimension (NC3 has max 1)
- `inq_ncid`: Returns same ncid (no groups)
- `inq_grpname`: Returns "/" (root only)
- `inq_varids`, `inq_dimids`: Returns sequential IDs 0..n-1

### Key Functions

```c
static const NC_Dispatch NC3_dispatcher = {
    .model = NC_FORMATX_NC3,
    .dispatch_version = NC_DISPATCH_VERSION,
    
    .create = NC3_create,
    .open = NC3_open,
    .redef = NC3_redef,
    ._enddef = NC3__enddef,
    .sync = NC3_sync,
    .abort = NC3_abort,
    .close = NC3_close,
    
    .get_vara = NC3_get_vara,
    .put_vara = NC3_put_vara,
    .get_vars = NCDEFAULT_get_vars,
    .put_vars = NCDEFAULT_put_vars,
    
    // ... more function pointers
};
```

## HDF5 Dispatch Table (NetCDF-4/HDF5)

**File**: `libhdf5/hdf5dispatch.c:19-114`

**Table Name**: `HDF5_dispatcher`

**Model**: `NC_FORMATX_NC4`

### Implementation Summary

**Fully Implemented**:
- All file operations
- All dimension operations (with HDF5 dimension scales)
- All variable operations (with chunking, compression, filters)
- All attribute operations (including reserved attributes)
- All group operations (hierarchical groups)
- All user-defined types (compound, vlen, enum, opaque)
- Compression and filters
- Chunking and endianness
- Parallel I/O (if HDF5 built with parallel support)
- Quantization (NetCDF-4.8+)

**Delegated to NCDEFAULT**:
- get_varm, put_varm (mapped access)

**HDF5-Specific Features**:
- Dimension scales for dimensions
- Reserved attributes (_NCProperties, _Netcdf4Coordinates, etc.)
- Filter plugins
- Chunk cache tuning
- Parallel I/O via MPI

### Key Functions

```c
static const NC_Dispatch HDF5_dispatcher = {
    .model = NC_FORMATX_NC4,
    .dispatch_version = NC_DISPATCH_VERSION,
    
    .create = NC4_create,
    .open = NC4_open,
    
    .def_dim = HDF5_def_dim,
    .inq_dim = HDF5_inq_dim,
    .rename_dim = HDF5_rename_dim,
    
    .def_var = NC4_def_var,
    .get_vara = NC4_get_vara,
    .put_vara = NC4_put_vara,
    .get_vars = NC4_get_vars,
    .put_vars = NC4_put_vars,
    
    .def_var_deflate = NC4_def_var_deflate,
    .def_var_chunking = NC4_def_var_chunking,
    .def_var_filter = NC4_hdf5_def_var_filter,
    
    .def_grp = NC4_def_grp,
    .def_compound = NC4_def_compound,
    
    // ... more function pointers
};
```

## Zarr Dispatch Table (NCZarr)

**File**: `libnczarr/zdispatch.c:19-111`

**Table Name**: `NCZ_dispatcher`

**Model**: `NC_FORMATX_NCZARR`

### Implementation Summary

**Fully Implemented**:
- File operations (create, open, close, sync)
- Variable I/O (get_vara, put_vara, get_vars, put_vars)
- Zarr-specific metadata operations
- Codec/filter pipeline
- Chunk caching

**Delegated to NC4 (libsrc4)**:
- Most inquiry operations (inq_type, inq_dimid, inq_varid, etc.)
- Group operations (inq_grps, inq_grpname, etc.)
- Many metadata operations

**Returns NC_NOTNC4** (not supported):
- User-defined types (compound, vlen, enum, opaque)
- Some type operations

**Zarr-Specific Features**:
- JSON metadata (.zarray, .zgroup, .zattrs)
- Multiple storage backends (file, S3, ZIP)
- Codec pipeline (blosc, zlib, etc.)
- Dimension separator (. or /)

### Key Functions

```c
static const NC_Dispatch NCZ_dispatcher = {
    .model = NC_FORMATX_NCZARR,
    .dispatch_version = NC_DISPATCH_VERSION,
    
    .create = NCZ_create,
    .open = NCZ_open,
    .close = NCZ_close,
    .sync = NCZ_sync,
    
    .get_vara = NCZ_get_vara,
    .put_vara = NCZ_put_vara,
    .get_vars = NCZ_get_vars,
    .put_vars = NCZ_put_vars,
    
    // Many operations delegate to NC4_*
    .inq_type = NCZ_inq_type,      // Calls NC4_inq_type
    .inq_dimid = NCZ_inq_dimid,    // Calls NC4_inq_dimid
    
    .def_var_filter = NCZ_def_var_filter,
    .def_var_chunking = NCZ_def_var_chunking,
    
    // User-defined types not supported
    .def_compound = NC_NOTNC4_def_compound,
    .def_vlen = NC_NOTNC4_def_vlen,
    
    // ... more function pointers
};
```

## DAP2 Dispatch Table (OPeNDAP)

**File**: `libdap2/ncd2dispatch.c`

**Table Name**: `NCD2_dispatcher`

**Model**: `NC_FORMATX_DAP2`

### Implementation Summary

**Fully Implemented**:
- File operations (open, close)
- Variable I/O with constraint expressions
- Metadata inquiry
- Attribute access
- Remote data access via HTTP

**Not Supported** (read-only protocol):
- create, redef, enddef
- def_dim, def_var, put_att
- put_vara, put_vars
- All NetCDF-4 features

**DAP2-Specific Features**:
- Constraint expressions for subsetting
- DDS/DAS parsing
- HTTP caching
- URL-based access

### Key Functions

```c
static const NC_Dispatch NCD2_dispatcher = {
    .model = NC_FORMATX_DAP2,
    .dispatch_version = NC_DISPATCH_VERSION,
    
    .create = NULL,  // Not supported
    .open = NCD2_open,
    .close = NCD2_close,
    
    .get_vara = NCD2_get_vara,
    .put_vara = NULL,  // Read-only
    
    .inq = NCD2_inq,
    .inq_var = NCD2_inq_var,
    .get_att = NCD2_get_att,
    
    // ... more function pointers
};
```

## DAP4 Dispatch Table (OPeNDAP)

**File**: `libdap4/ncd4dispatch.c`

**Table Name**: `NCD4_dispatcher`

**Model**: `NC_FORMATX_DAP4`

### Implementation Summary

**Fully Implemented**:
- File operations (open, close)
- Variable I/O
- Metadata inquiry (DMR parsing)
- Group support
- Enhanced type system

**Not Supported** (read-only protocol):
- create, redef, enddef
- def_dim, def_var, put_att
- put_vara, put_vars
- User-defined types (read-only)

**DAP4-Specific Features**:
- DMR (XML metadata)
- Groups and hierarchies
- Checksums
- Chunked transfer encoding

### Key Functions

```c
static const NC_Dispatch NCD4_dispatcher = {
    .model = NC_FORMATX_DAP4,
    .dispatch_version = NC_DISPATCH_VERSION,
    
    .create = NULL,  // Not supported
    .open = NCD4_open,
    .close = NCD4_close,
    
    .get_vara = NCD4_get_vara,
    .put_vara = NULL,  // Read-only
    
    .inq_grps = NCD4_inq_grps,  // Groups supported
    
    // ... more function pointers
};
```

## User-Defined Format Tables

**Files**: `libdispatch/dfile.c:49-52`

**Table Names**: `UDF0_dispatch_table`, `UDF1_dispatch_table`

**Models**: `NC_FORMATX_UDF0`, `NC_FORMATX_UDF1`

### Registration

Users can register custom formats via `nc_def_user_format()`:

```c
int nc_def_user_format(int mode_flag, 
                       NC_Dispatch* dispatch_table,
                       char* magic_number);
```

**Requirements**:
- Dispatch table version must match `NC_DISPATCH_VERSION`
- Magic number max 8 bytes (optional)
- Must implement all required operations

## Dispatch Table Selection

**File**: `libdispatch/dinfermodel.c`

### Selection Logic

1. **Magic Number Detection**:
   - CDF1: `0x43 0x44 0x46 0x01` ("CDF\001")
   - CDF2: `0x43 0x44 0x46 0x02` ("CDF\002")
   - CDF5: `0x43 0x44 0x46 0x05` ("CDF\005")
   - HDF5: `0x89 0x48 0x44 0x46 0x0d 0x0a 0x1a 0x0a`
   - User-defined: Custom magic numbers

2. **URL Scheme Parsing**:
   - `http://`, `https://` → DAP2 or DAP4
   - `s3://` → Zarr with S3 backend
   - `file://` → Local file (check magic)

3. **Mode Flags**:
   - `NC_NETCDF4` → HDF5 or Zarr
   - `NC_CLASSIC_MODEL` → NetCDF-3 API with NetCDF-4 file
   - `NC_64BIT_OFFSET` → CDF2
   - `NC_64BIT_DATA` → CDF5
   - `NC_ZARR` → Zarr format

4. **File Extension** (hints):
   - `.nc` → NetCDF-3 or NetCDF-4
   - `.nc4` → NetCDF-4/HDF5
   - `.h5`, `.hdf5` → HDF5
   - `.zarr` → Zarr

### Dispatch Table Registration

**Initialization** (called at library startup):
```c
NCDISPATCH_initialize()
    → NC3_initialize()      // Sets NC3_dispatch_table
    → NC_HDF5_initialize()  // Sets HDF5_dispatch_table
    → NCZ_initialize()      // Sets NCZ_dispatch_table
    → NCD2_initialize()     // Sets NCD2_dispatch_table
    → NCD4_initialize()     // Sets NCD4_dispatch_table
```

## Function Pointer Conventions

### Return Values
- `NC_NOERR` (0) on success
- Negative error codes on failure
- `NC_ENOTNC4` for unsupported NetCDF-4 features
- `NC_EINVAL` for invalid parameters

### Common Stubs

**NC_NOOP_*** - No-operation stubs (return NC_NOERR)
**NC_NOTNC4_*** - Not-NetCDF-4 stubs (return NC_ENOTNC4)
**NCDEFAULT_*** - Default implementations (in libdispatch)

### NCDEFAULT Implementations

**File**: `libdispatch/dvar.c`

- `NCDEFAULT_get_vars()` - Implements strided access using get_vara
- `NCDEFAULT_put_vars()` - Implements strided writes using put_vara
- `NCDEFAULT_get_varm()` - Implements mapped access using get_vars
- `NCDEFAULT_put_varm()` - Implements mapped writes using put_vars

## Dispatch Version History

- **Version 1**: Original dispatch table
- **Version 2**: Added filter operations
- **Version 3**: Replaced filteractions with specific filter functions
- **Version 4**: Added quantization support
- **Version 5**: Current version (additional enhancements)

**Compatibility**: Dispatch tables must match the library's dispatch version exactly.

## Testing Dispatch Tables

Each format has its own test suite:
- `nc_test/` - NetCDF-3 tests
- `nc_test4/` - NetCDF-4/HDF5 tests
- `nczarr_test/` - Zarr tests
- `ncdap_test/` - DAP2 tests

**Dispatch testing**: Tests verify that operations route correctly and return appropriate errors for unsupported features.

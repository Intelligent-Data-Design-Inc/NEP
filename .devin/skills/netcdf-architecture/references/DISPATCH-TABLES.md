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

**Files**: `libdispatch/dfile.c`, `libdispatch/ddispatch.c`

**Table Names**: `UDF0_dispatch_table` through `UDF9_dispatch_table`

**Models**: `NC_FORMATX_UDF0` through `NC_FORMATX_UDF9`

**Mode Flags**: `NC_UDF0` through `NC_UDF9`

### Overview

NetCDF-C provides 10 user-defined format slots that allow developers to extend the library with custom file formats and storage backends. Each slot can be independently configured with its own dispatch table, initialization function, and optional magic number.

### UDF Slot Organization

- **UDF0, UDF1**: Original slots, mode flags in lower 16 bits
- **UDF2-UDF9**: Extended slots, mode flags in upper 16 bits

### Registration Methods

#### Programmatic Registration

Users can register custom formats via `nc_def_user_format()`:

```c
int nc_def_user_format(int mode_flag, 
                       NC_Dispatch* dispatch_table,
                       char* magic_number);
```

**Parameters**:
- `mode_flag`: One of `NC_UDF0` through `NC_UDF9`, optionally combined with other mode flags (e.g., `NC_NETCDF4`)
- `dispatch_table`: Pointer to your `NC_Dispatch` structure
- `magic_number`: Optional magic number string (max `NC_MAX_MAGIC_NUMBER_LEN` bytes) for automatic format detection, or NULL

**Example**:
```c
extern NC_Dispatch my_format_dispatcher;

// Register UDF in slot 0 with magic number
nc_def_user_format(NC_UDF0 | NC_NETCDF4, &my_format_dispatcher, "MYFORMAT");

// Now files with "MYFORMAT" magic number will use your dispatcher
int ncid;
nc_open("myfile.dat", 0, &ncid);  // Auto-detects format
```

#### Query Registered UDFs

Use `nc_inq_user_format()` to query registered formats:

```c
int nc_inq_user_format(int mode_flag, 
                       NC_Dispatch** dispatch_table,
                       char* magic_number);
```

**Example**:
```c
NC_Dispatch *disp;
char magic[NC_MAX_MAGIC_NUMBER_LEN + 1];
nc_inq_user_format(NC_UDF0, &disp, magic);
```

#### RC File Configuration

UDFs can be automatically loaded from RC file configuration:

**RC File Format** (`.ncrc`, `.daprc`, or `.dodsrc`):
```ini
NETCDF.UDF<N>.LIBRARY=/full/path/to/library.so
NETCDF.UDF<N>.INIT=initialization_function_name
NETCDF.UDF<N>.MAGIC=OPTIONAL_MAGIC_NUMBER
```

**Example**:
```ini
# Load custom format in UDF0
NETCDF.UDF0.LIBRARY=/usr/local/lib/libmyformat.so
NETCDF.UDF0.INIT=myformat_init
NETCDF.UDF0.MAGIC=MYFORMAT

# Load scientific data format in UDF3
NETCDF.UDF3.LIBRARY=/opt/scidata/lib/libscidata.so
NETCDF.UDF3.INIT=scidata_initialize
NETCDF.UDF3.MAGIC=SCIDATA
```

**RC File Requirements**:
- `LIBRARY`: Must be a full absolute path to the shared library
- `INIT`: Name of the initialization function in the library
- `MAGIC`: Optional magic number for automatic format detection
- Both `LIBRARY` and `INIT` must be present; partial configuration is ignored with a warning

**RC File Search Order**:
1. `$HOME/.ncrc`
2. `$HOME/.daprc`
3. `$HOME/.dodsrc`
4. `$CWD/.ncrc`
5. `$CWD/.daprc`
6. `$CWD/.dodsrc`

### Plugin Loading Process

Plugins are loaded during library initialization (`nc_initialize()`):

1. RC files are parsed
2. For each configured UDF slot:
   - Library is loaded using `dlopen()` (Unix) or `LoadLibrary()` (Windows)
   - Init function is located using `dlsym()` or `GetProcAddress()`
   - Init function is called
   - Init function must call `nc_def_user_format()` to register the dispatch table
3. Dispatch table ABI version is verified
4. Magic number (if provided) is registered for automatic format detection

**Note**: Library handles are intentionally not closed; they remain loaded for the lifetime of the process.

### Plugin Implementation Requirements

**Dispatch Table Requirements**:
- Dispatch table version must match `NC_DISPATCH_VERSION`
- Must implement all required operations or use pre-defined stubs
- Magic number max `NC_MAX_MAGIC_NUMBER_LEN` bytes (optional)

**Initialization Function Requirements**:
1. Must be exported (not static)
2. Must call `nc_def_user_format()` to register dispatch table
3. Should return `NC_NOERR` on success, error code on failure
4. Name must match RC file `INIT` key

**Example Initialization Function**:
```c
#include <netcdf.h>

extern NC_Dispatch my_dispatcher;

// Initialization function - must be exported
int my_plugin_init(void) {
    int ret;
    
    // Register dispatch table with magic number
    ret = nc_def_user_format(NC_UDF0 | NC_NETCDF4, 
                             &my_dispatcher,
                             "MYFMT");
    if (ret != NC_NOERR)
        return ret;
    
    // Additional initialization if needed
    // ...
    
    return NC_NOERR;
}
```

### Pre-defined Dispatch Functions

For operations your format doesn't support, use these pre-defined functions:

**Read-only stubs** (`libdispatch/dreadonly.c`):
- `NC_RO_create`, `NC_RO_redef`, `NC_RO__enddef`, `NC_RO_sync`
- `NC_RO_set_fill`, `NC_RO_def_dim`, `NC_RO_def_var`, `NC_RO_put_att`
- `NC_RO_put_vara`, `NC_RO_put_vars`, `NC_RO_put_varm`
- Returns `NC_EPERM` (operation not permitted)

**Not NetCDF-4 stubs** (`libdispatch/dnotnc4.c`):
- `NC_NOTNC4_def_grp`, `NC_NOTNC4_def_compound`, `NC_NOTNC4_def_vlen`
- `NC_NOTNC4_def_var_deflate`, `NC_NOTNC4_def_var_chunking`
- Returns `NC_ENOTNC4` (not a NetCDF-4 file)

**Not NetCDF-3 stubs** (`libdispatch/dnotnc3.c`):
- Returns `NC_ENOTNC3` (not a NetCDF-3 file)

**No-op stubs**:
- `NC_NOOP_*` - Returns `NC_NOERR` without doing anything

**Default implementations** (`libdispatch/dvar.c`):
- `NCDEFAULT_get_vars`, `NCDEFAULT_put_vars` - Strided access using get_vara/put_vara
- `NCDEFAULT_get_varm`, `NCDEFAULT_put_varm` - Mapped access using get_vars/put_vars

**NetCDF-4 inquiry functions** (`libsrc4/`):
- `NC4_inq`, `NC4_inq_type`, `NC4_inq_dimid`, `NC4_inq_varid`
- Use internal metadata model for inquiry operations

### Example Minimal Dispatch Table

```c
#include "netcdf_dispatch.h"

static NC_Dispatch my_dispatcher = {
    NC_FORMATX_UDF0,        /* Use UDF slot 0 */
    NC_DISPATCH_VERSION,    /* Current ABI version */
    
    NC_RO_create,           /* Read-only: use predefined function */
    my_open,                /* Custom open function */
    NC_RO_redef,
    NC_RO__enddef,
    NC_RO_sync,
    my_abort,
    my_close,
    NC_RO_set_fill,
    my_inq_format,
    my_inq_format_extended,
    
    /* Inquiry functions - can use NC4_* defaults */
    NC4_inq,
    NC4_inq_type,
    NC4_inq_dimid,
    NC4_inq_varid,
    
    /* Variable I/O */
    my_get_vara,
    NC_RO_put_vara,         /* Read-only */
    NCDEFAULT_get_vars,     /* Use default strided implementation */
    NC_RO_put_vars,
    NCDEFAULT_get_varm,     /* Use default mapped implementation */
    NC_RO_put_varm,
    
    /* NetCDF-4 features not supported */
    NC_NOTNC4_def_grp,
    NC_NOTNC4_def_compound,
    NC_NOTNC4_def_vlen,
    NC_NOTNC4_def_var_deflate,
    NC_NOTNC4_def_var_chunking,
    
    /* ... continue for all ~70 function pointers ... */
};
```

### Magic Numbers and Format Detection

Magic numbers enable automatic format detection when opening files.

**How Magic Numbers Work**:
1. When `nc_open()` is called without a specific format flag
2. The file's first bytes are read
3. They are compared against all registered magic numbers (built-in and user-defined)
4. If a match is found, the corresponding dispatcher is used

**Magic Number Best Practices**:
- Use unique, distinctive strings (4-8 bytes recommended)
- Place at the beginning of your file format
- Avoid conflicts with existing formats:
  - NetCDF-3: "CDF\001", "CDF\002", "CDF\005"
  - HDF5/NetCDF-4: "\211HDF\r\n\032\n"
- Maximum length: `NC_MAX_MAGIC_NUMBER_LEN` bytes

### Platform Considerations

**Unix/Linux/macOS**:
- Shared libraries: `.so` extension
- Dynamic loading: `dlopen()` and `dlsym()`
- Library paths: Use absolute paths or ensure libraries are in `LD_LIBRARY_PATH`

**Windows**:
- Shared libraries: `.dll` extension
- Dynamic loading: `LoadLibrary()` and `GetProcAddress()`
- Library paths: Use absolute paths or ensure DLLs are in system `PATH`

**Building Plugins**:

Unix:
```bash
gcc -shared -fPIC -o libmyplugin.so myplugin.c -lnetcdf
```

Windows:
```batch
cl /LD myplugin.c netcdf.lib
```

### Security Considerations

- **Full paths required**: RC files must specify absolute library paths to prevent path injection attacks
- **Code execution**: Plugins execute arbitrary code in your process; only load trusted libraries
- **Validation**: The library verifies dispatch table ABI version but cannot validate plugin behavior
- **Permissions**: Ensure plugin libraries have appropriate file permissions

### Common Errors

**NC_EINVAL: Invalid dispatch table version**
- Cause: Plugin was compiled against a different version of NetCDF-C
- Solution: Recompile plugin against current NetCDF-C version

**Plugin not loaded (no error)**
- Cause: Partial RC configuration (LIBRARY without INIT, or vice versa)
- Solution: Check that both LIBRARY and INIT keys are present in RC file

**Library not found**
- Cause: Incorrect path in NETCDF.UDF*.LIBRARY
- Solution: Use absolute path; verify file exists and has correct permissions

**Init function not found**
- Cause: Function name mismatch or missing export
- Solution: Verify function name matches INIT key; ensure function is exported (not static)

### Testing UDFs

**Enable Logging**:
```bash
export NC_LOG_LEVEL=3
./myprogram
```

**Verify RC File is Read**:
```bash
echo "NETCDF.UDF0.LIBRARY=/tmp/test.so" > ~/.ncrc
echo "NETCDF.UDF0.INIT=test_init" >> ~/.ncrc
# Run program and check for warnings about missing library
```

**Check Plugin Exports** (Unix):
```bash
nm -D libmyplugin.so | grep init
```

**Check Plugin Exports** (Windows):
```batch
dumpbin /EXPORTS myplugin.dll
```

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

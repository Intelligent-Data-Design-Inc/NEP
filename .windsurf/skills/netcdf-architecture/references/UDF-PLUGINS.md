# NetCDF-C User-Defined Format (UDF) Plugin Development

This reference provides comprehensive guidance for developing UDF plugins for NetCDF-C.

## Overview

User-Defined Formats (UDFs) allow developers to extend NetCDF-C with custom file formats and storage backends through a plugin system. The library provides 10 independent UDF slots (UDF0-UDF9) that can be registered programmatically or via RC file configuration.

## Plugin Architecture

A UDF plugin consists of three main components:

1. **Dispatch Table**: `NC_Dispatch` structure with function pointers implementing the netCDF API
2. **Initialization Function**: Called during plugin loading to register the dispatch table
3. **Format-Specific Code**: Implementation of file I/O and data operations

## Plugin Lifecycle

1. Library initialization (`nc_initialize()`)
2. RC file parsing (if configured)
3. Plugin library loading (`dlopen`/`LoadLibrary`)
4. Init function location (`dlsym`/`GetProcAddress`)
5. Init function execution
6. Dispatch table registration via `nc_def_user_format()`
7. Plugin remains loaded for process lifetime

## Dispatch Table Implementation

### Required Fields

```c
typedef struct NC_Dispatch {
    int model;              /* NC_FORMATX_UDF0 through NC_FORMATX_UDF9 */
    int dispatch_version;   /* Must be NC_DISPATCH_VERSION */
    
    /* Function pointers for all netCDF operations (~70 total) */
    int (*create)(...);
    int (*open)(...);
    int (*close)(...);
    int (*get_vara)(...);
    int (*put_vara)(...);
    /* ... many more functions ... */
} NC_Dispatch;
```

**Location**: `include/netcdf_dispatch.h`

### Minimal Example

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
    NC4_inq_unlimdim,
    NC4_inq_grpname,
    NC4_inq_grpname_full,
    NC4_inq_grp_parent,
    NC4_inq_grp_full_ncid,
    NC4_inq_varids,
    NC4_inq_dimids,
    NC4_inq_typeids,
    NC4_inq_type_equal,
    NC4_inq_user_type,
    NC4_inq_typeid,
    
    /* Variable I/O */
    my_get_vara,
    NC_RO_put_vara,         /* Read-only */
    NCDEFAULT_get_vars,     /* Use default strided implementation */
    NC_RO_put_vars,
    NCDEFAULT_get_varm,     /* Use default mapped implementation */
    NC_RO_put_varm,
    
    /* Attributes */
    NC4_get_att,
    NC_RO_put_att,
    
    /* Dimensions */
    NC4_inq_dim,
    NC_RO_def_dim,
    NC4_inq_unlimdims,
    NC_RO_rename_dim,
    
    /* Variables */
    NC4_inq_var_all,
    NC_RO_def_var,
    NC_RO_rename_var,
    NC4_var_par_access,
    NC_RO_def_var_fill,
    
    /* NetCDF-4 features not supported */
    NC_NOTNC4_show_metadata,
    NC_NOTNC4_inq_grps,
    NC_NOTNC4_inq_ncid,
    NC_NOTNC4_inq_format_extended,
    NC_NOTNC4_inq_var_all,
    NC_NOTNC4_def_grp,
    NC_NOTNC4_rename_grp,
    NC_NOTNC4_inq_user_type,
    NC_NOTNC4_def_compound,
    NC_NOTNC4_insert_compound,
    NC_NOTNC4_insert_array_compound,
    NC_NOTNC4_inq_compound_field,
    NC_NOTNC4_inq_compound_fieldindex,
    NC_NOTNC4_def_vlen,
    NC_NOTNC4_def_enum,
    NC_NOTNC4_def_opaque,
    NC_NOTNC4_def_var_deflate,
    NC_NOTNC4_def_var_fletcher32,
    NC_NOTNC4_def_var_chunking,
    NC_NOTNC4_def_var_endian,
    NC_NOTNC4_def_var_filter,
    NC_NOTNC4_set_var_chunk_cache,
    NC_NOTNC4_get_var_chunk_cache,
    NC_NOTNC4_inq_var_filter_ids,
    NC_NOTNC4_inq_var_filter_info,
    NC_NOTNC4_def_var_quantize,
    NC_NOTNC4_inq_var_quantize,
};
```

## Pre-defined Functions

Use these for operations your format doesn't support:

### Read-Only Stubs

**File**: `libdispatch/dreadonly.c`

Returns `NC_EPERM` (operation not permitted):
- `NC_RO_create` - File creation
- `NC_RO_redef` - Enter define mode
- `NC_RO__enddef` - Leave define mode
- `NC_RO_sync` - Synchronize to disk
- `NC_RO_set_fill` - Set fill mode
- `NC_RO_def_dim` - Define dimension
- `NC_RO_def_var` - Define variable
- `NC_RO_rename_dim` - Rename dimension
- `NC_RO_rename_var` - Rename variable
- `NC_RO_put_att` - Write attribute
- `NC_RO_del_att` - Delete attribute
- `NC_RO_put_vara` - Write variable data
- `NC_RO_put_vars` - Write strided data
- `NC_RO_put_varm` - Write mapped data
- `NC_RO_def_var_fill` - Define fill value

### Not NetCDF-4 Stubs

**File**: `libdispatch/dnotnc4.c`

Returns `NC_ENOTNC4` (not a NetCDF-4 file):
- `NC_NOTNC4_def_grp` - Define group
- `NC_NOTNC4_rename_grp` - Rename group
- `NC_NOTNC4_def_compound` - Define compound type
- `NC_NOTNC4_def_vlen` - Define variable-length type
- `NC_NOTNC4_def_enum` - Define enumeration type
- `NC_NOTNC4_def_opaque` - Define opaque type
- `NC_NOTNC4_def_var_deflate` - Define compression
- `NC_NOTNC4_def_var_fletcher32` - Define checksums
- `NC_NOTNC4_def_var_chunking` - Define chunking
- `NC_NOTNC4_def_var_endian` - Define endianness
- `NC_NOTNC4_def_var_filter` - Define filter
- `NC_NOTNC4_def_var_quantize` - Define quantization

### Default Implementations

**File**: `libdispatch/dvar.c`

Generic implementations built on simpler operations:
- `NCDEFAULT_get_vars` - Strided read using `get_vara`
- `NCDEFAULT_put_vars` - Strided write using `put_vara`
- `NCDEFAULT_get_varm` - Mapped read using `get_vars`
- `NCDEFAULT_put_varm` - Mapped write using `put_vars`

### NetCDF-4 Inquiry Functions

**Files**: `libsrc4/*.c`

Use internal metadata model for inquiry operations:
- `NC4_inq` - Inquire about file
- `NC4_inq_type` - Inquire about type
- `NC4_inq_dimid` - Get dimension ID
- `NC4_inq_varid` - Get variable ID
- `NC4_inq_unlimdim` - Get unlimited dimension
- `NC4_inq_grpname` - Get group name
- `NC4_inq_varids` - Get all variable IDs
- `NC4_inq_dimids` - Get all dimension IDs
- `NC4_get_att` - Get attribute value
- `NC4_inq_var_all` - Get all variable info

## Initialization Function

### Function Signature

```c
int plugin_init(void);
```

### Requirements

1. Must be exported (not static)
2. Must call `nc_def_user_format()` to register dispatch table
3. Should return `NC_NOERR` on success, error code on failure
4. Name must match RC file INIT key

### Example Implementation

```c
#include <netcdf.h>

/* Your dispatch table */
extern NC_Dispatch my_dispatcher;

/* Initialization function - must be exported */
int my_plugin_init(void)
{
    int ret;
    
    /* Register dispatch table with magic number */
    ret = nc_def_user_format(NC_UDF0 | NC_NETCDF4, 
                             &my_dispatcher,
                             "MYFMT");
    if (ret != NC_NOERR)
        return ret;
    
    /* Additional initialization if needed */
    /* ... */
    
    return NC_NOERR;
}
```

## Implementing Key Dispatch Functions

### Open Function

```c
int my_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
            void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    /* 1. Open your file format */
    /* 2. Populate internal metadata structures */
    /* 3. Store format-specific data in NC->dispatchdata */
    /* 4. Return NC_NOERR on success */
    
    return NC_NOERR;
}
```

### Close Function

```c
int my_close(int ncid, void *v)
{
    /* 1. Clean up resources */
    /* 2. Close file handles */
    /* 3. Free format-specific data */
    
    return NC_NOERR;
}
```

### Abort Function

```c
int my_abort(int ncid, void *v)
{
    /* 1. Discard any pending changes */
    /* 2. Clean up resources */
    /* 3. Close file handles */
    
    return NC_NOERR;
}
```

### Format Inquiry Functions

```c
int my_inq_format(int ncid, int *formatp)
{
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;  /* Or appropriate format */
    return NC_NOERR;
}

int my_inq_format_extended(int ncid, int *formatp, int *modep)
{
    if (formatp)
        *formatp = NC_FORMATX_UDF0;
    if (modep)
        *modep = NC_UDF0 | NC_NETCDF4;
    return NC_NOERR;
}
```

### Variable I/O Functions

```c
int my_get_vara(int ncid, int varid, const size_t *start,
                const size_t *count, void *value, nc_type memtype)
{
    /* 1. Validate parameters */
    /* 2. Read data from your format */
    /* 3. Convert to requested memory type if needed */
    /* 4. Copy to value buffer */
    
    return NC_NOERR;
}
```

## Building Plugins

### Unix/Linux/macOS

**Makefile**:
```makefile
CC = gcc
CFLAGS = -fPIC -I/usr/local/include
LDFLAGS = -shared -L/usr/local/lib -lnetcdf

myplugin.so: myplugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

install:
	cp myplugin.so /usr/local/lib/
```

**Command line**:
```bash
gcc -shared -fPIC -I/usr/local/include -o myplugin.so myplugin.c -lnetcdf
```

### Windows

**Command line**:
```batch
cl /LD /I"C:\netcdf\include" myplugin.c /link /LIBPATH:"C:\netcdf\lib" netcdf.lib
```

### CMake

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyPlugin)

find_package(netCDF REQUIRED)

add_library(myplugin SHARED myplugin.c)
target_link_libraries(myplugin netCDF::netcdf)
target_include_directories(myplugin PRIVATE ${netCDF_INCLUDE_DIRS})

install(TARGETS myplugin LIBRARY DESTINATION lib)
```

## Testing Plugins

### Unit Testing

```c
/* test_plugin.c */
#include <netcdf.h>
#include <assert.h>

extern NC_Dispatch my_dispatcher;
extern int my_plugin_init(void);

int main() {
    int ret;
    NC_Dispatch *disp;
    
    /* Test initialization */
    ret = my_plugin_init();
    assert(ret == NC_NOERR);
    
    /* Verify registration */
    ret = nc_inq_user_format(NC_UDF0, &disp, NULL);
    assert(ret == NC_NOERR);
    assert(disp == &my_dispatcher);
    
    printf("Plugin tests passed\n");
    return 0;
}
```

### Integration Testing

```c
/* test_integration.c */
#include <netcdf.h>

int main() {
    int ncid, ret;
    
    /* Initialize and register plugin */
    my_plugin_init();
    
    /* Test file operations */
    ret = nc_open("testfile.dat", NC_UDF0, &ncid);
    if (ret != NC_NOERR) {
        fprintf(stderr, "Open failed: %s\n", nc_strerror(ret));
        return 1;
    }
    
    /* Test operations */
    int format;
    nc_inq_format(ncid, &format);
    
    nc_close(ncid);
    printf("Integration test passed\n");
    return 0;
}
```

### RC File Testing

Create `.ncrc`:
```ini
NETCDF.UDF0.LIBRARY=/path/to/myplugin.so
NETCDF.UDF0.INIT=my_plugin_init
NETCDF.UDF0.MAGIC=MYFMT
```

Test automatic loading:
```c
int main() {
    /* Plugin loads automatically during nc_initialize() */
    int ncid;
    nc_open("file_with_magic.dat", 0, &ncid);  /* Auto-detects format */
    nc_close(ncid);
    return 0;
}
```

## Debugging

### Enable NetCDF Logging

```bash
export NC_LOG_LEVEL=3
./test_program
```

### Check Symbol Exports

**Unix**:
```bash
nm -D libmyplugin.so | grep init
```

**Windows**:
```batch
dumpbin /EXPORTS myplugin.dll
```

### GDB Debugging

```bash
gdb ./test_program
(gdb) break my_plugin_init
(gdb) run
(gdb) backtrace
```

### Common Issues

**Plugin not loaded**:
- Check RC file syntax
- Verify both LIBRARY and INIT are present
- Use absolute path for LIBRARY

**Init function not found**:
- Ensure function is not static
- Check function name matches INIT key
- Verify symbol is exported

**ABI version mismatch**:
- Recompile against current netCDF-C headers
- Check `NC_DISPATCH_VERSION` value

## Best Practices

1. **Error Handling**: Return appropriate `NC_E*` error codes
2. **Memory Management**: Clean up in close/abort functions
3. **Thread Safety**: Use thread-safe operations if needed
4. **Logging**: Use `nclog` functions for diagnostic output
5. **Documentation**: Document your format and API
6. **Testing**: Test all code paths thoroughly
7. **Versioning**: Version your plugin and document compatibility

## Magic Numbers

### How They Work

When `nc_open()` is called without a specific format flag:
1. File's first bytes are read
2. Compared against all registered magic numbers
3. If match found, corresponding UDF dispatcher is used

### Best Practices

- Use unique, distinctive strings (4-8 bytes recommended)
- Place at beginning of file format
- Avoid conflicts with existing formats:
  - NetCDF-3: "CDF\001", "CDF\002", "CDF\005"
  - HDF5/NetCDF-4: "\211HDF\r\n\032\n"
- Maximum length: `NC_MAX_MAGIC_NUMBER_LEN` bytes

### Example

```c
/* File format with magic number */
FILE *fp = fopen("mydata.dat", "wb");
fwrite("MYDATA", 1, 6, fp);  /* Magic number */
/* ... write your data ... */
fclose(fp);

/* Register UDF with magic number */
nc_def_user_format(NC_UDF0 | NC_NETCDF4, &my_dispatcher, "MYDATA");

/* Open automatically detects format */
int ncid;
nc_open("mydata.dat", 0, &ncid);  /* No mode flag needed! */
```

## Reference Files

- **Dispatch table definition**: `include/netcdf_dispatch.h`
- **Pre-defined functions**: `libdispatch/dreadonly.c`, `libdispatch/dnotnc*.c`
- **Example implementations**: `libhdf5/hdf5dispatch.c`, `libsrc/nc3dispatch.c`
- **Test plugins**: `nc_test4/test_plugin_lib.c`
- **Plugin loading**: `libdispatch/dutil.c`, `libdispatch/drc.c`
- **Registration API**: `libdispatch/ddispatch.c`

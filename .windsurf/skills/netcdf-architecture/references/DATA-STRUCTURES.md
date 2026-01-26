# NetCDF-C Data Structures Reference

This reference documents the key data structures used throughout NetCDF-C.

## Common Structures

### NC - File Handle (nc.h)

```c
typedef struct NC {
    int ext_ncid;                    // External ID (user-visible)
    int int_ncid;                    // Internal ID (format-specific)
    const struct NC_Dispatch* dispatch; // Function pointer table
    void* dispatchdata;              // Format-specific metadata
    char* path;                      // File path
    int mode;                        // Open mode flags
} NC;
```

**Location**: `include/nc.h:23-30`

**Purpose**: Common handle for all open files, regardless of format.

**Key Fields**:
- `ext_ncid`: The ID returned to users, managed globally
- `int_ncid`: Format-specific ID (e.g., NetCDF-3 has its own ID space)
- `dispatch`: Points to format-specific function table
- `dispatchdata`: Points to `NC3_INFO`, `NC_FILE_INFO_T`, etc.

### NC_Dispatch - Function Pointer Table (netcdf_dispatch.h)

```c
struct NC_Dispatch {
    int model;                       // NC_FORMATX_NC3, NC_FORMATX_NC4, etc.
    int dispatch_version;            // Must match NC_DISPATCH_VERSION
    
    // File operations
    int (*create)(const char *path, int cmode, ...);
    int (*open)(const char *path, int mode, ...);
    int (*redef)(int);
    int (*_enddef)(int, size_t, size_t, size_t, size_t);
    int (*sync)(int);
    int (*abort)(int);
    int (*close)(int, void *);
    
    // Metadata operations
    int (*def_dim)(int, const char *, size_t, int *);
    int (*def_var)(int, const char *, nc_type, int, const int *, int *);
    int (*put_att)(int, int, const char *, nc_type, size_t, const void *, nc_type);
    
    // Variable I/O
    int (*get_vara)(int, int, const size_t *, const size_t *, void *, nc_type);
    int (*put_vara)(int, int, const size_t *, const size_t *, const void *, nc_type);
    
    // ... ~60 more function pointers
};
```

**Location**: `include/netcdf_dispatch.h:34-256`

**Implementations**:
- `NC3_dispatcher` - NetCDF-3 (libsrc/nc3dispatch.c)
- `HDF5_dispatcher` - HDF5 (libhdf5/hdf5dispatch.c)
- `NCZ_dispatcher` - Zarr (libnczarr/zdispatch.c)
- `NCD2_dispatcher` - DAP2 (libdap2/ncd2dispatch.c)
- `NCD4_dispatcher` - DAP4 (libdap4/ncd4dispatch.c)

## NetCDF-3 Structures (libsrc)

### NC3_INFO - NetCDF-3 File Metadata

```c
typedef struct NC3_INFO {
    size_t chunk;           // Chunk size hint for I/O
    size_t xsz;            // External file size
    size_t begin_var;      // Offset to non-record variables
    size_t begin_rec;      // Offset to record variables
    size_t recsize;        // Size of one record
    size_t numrecs;        // Number of records written
    
    NC_dimarray dims;      // Dimensions
    NC_attrarray attrs;    // Global attributes
    NC_vararray vars;      // Variables
    
    ncio* nciop;          // I/O provider
    int flags;            // File flags
    int old_format;       // CDF-1, CDF-2, or CDF-5
} NC3_INFO;
```

**Location**: Defined across `include/nc3internal.h`

### NC_dim - Dimension

```c
typedef struct {
    NC_string* name;       // Dimension name
    size_t size;          // Dimension length (NC_UNLIMITED for unlimited)
} NC_dim;
```

### NC_var - Variable

```c
typedef struct NC_var {
    NC_string* name;       // Variable name
    size_t ndims;         // Number of dimensions
    int* dimids;          // Dimension IDs
    NC_attrarray attrs;   // Variable attributes
    nc_type type;         // Data type
    size_t len;           // Product of dimension sizes
    size_t begin;         // Offset in file
    
    // For record variables
    size_t* shape;        // Cached dimension sizes
    size_t* dsizes;       // Cached dimension products
} NC_var;
```

### NC_attr - Attribute

```c
typedef struct NC_attr {
    NC_string* name;       // Attribute name
    nc_type type;         // Data type
    size_t nelems;        // Number of elements
    void* xvalue;         // Attribute value (external representation)
} NC_attr;
```

### NC_dimarray, NC_vararray, NC_attrarray

```c
typedef struct NC_dimarray {
    size_t nalloc;         // Allocated size
    size_t nelems;         // Number of elements
    NC_hashmap* hashmap;   // For fast name lookup
    NC_dim** value;        // Array of dimension pointers
} NC_dimarray;

// Similar for NC_vararray and NC_attrarray
```

**Key Feature**: Hash maps for O(1) name lookup

## NetCDF-4 Structures (libsrc4, libhdf5, libnczarr)

### NC_FILE_INFO_T - File Metadata

```c
typedef struct NC_FILE_INFO_T {
    NC_GRP_INFO_T* root_grp;        // Root group
    int no_write;                    // Read-only flag
    int ignore_att_convention;       // Ignore _NCProperties
    void* format_file_info;          // Format-specific data (HDF5/Zarr)
    
    // Provenance tracking
    char* provenance;
    int provenance_size;
} NC_FILE_INFO_T;
```

**Location**: `include/nc4internal.h`

### NC_GRP_INFO_T - Group Metadata

```c
typedef struct NC_GRP_INFO_T {
    NC_OBJ hdr;                      // Name and ID
    struct NC_FILE_INFO_T* nc4_info; // Parent file
    struct NC_GRP_INFO_T* parent;    // Parent group (NULL for root)
    
    NCindex* children;               // Child groups
    NCindex* dim;                    // Dimensions
    NCindex* att;                    // Attributes
    NCindex* type;                   // User-defined types
    NCindex* vars;                   // Variables
    
    void* format_grp_info;           // Format-specific data
} NC_GRP_INFO_T;
```

**Key Feature**: Hierarchical group structure with NCindex for fast lookup

### NC_VAR_INFO_T - Variable Metadata

```c
typedef struct NC_VAR_INFO_T {
    NC_OBJ hdr;                      // Name and ID
    char* alt_name;                  // Alternate name (for format differences)
    struct NC_GRP_INFO_T* container; // Parent group
    
    size_t ndims;                    // Number of dimensions
    int* dimids;                     // Dimension IDs
    NC_DIM_INFO_T** dim;            // Dimension pointers
    
    nc_bool_t is_new_var;           // Newly created
    nc_bool_t was_coord_var;        // Was a coordinate variable
    nc_bool_t became_coord_var;     // Became a coordinate variable
    nc_bool_t fill_val_changed;     // Fill value changed
    nc_bool_t attr_dirty;           // Attributes need rewriting
    nc_bool_t created;              // Already created in file
    nc_bool_t written_to;           // Has data been written
    
    struct NC_TYPE_INFO* type_info; // Type information
    int atts_read;                  // Attributes read flag
    nc_bool_t meta_read;            // Metadata read flag
    nc_bool_t coords_read;          // Coordinates read flag
    
    NCindex* att;                   // Attributes
    
    nc_bool_t no_fill;              // No fill value
    void* fill_value;               // Fill value
    
    size_t* chunksizes;             // Chunk sizes (if chunked)
    int storage;                    // NC_CHUNKED, NC_CONTIGUOUS, NC_COMPACT
    int endianness;                 // NC_ENDIAN_NATIVE, NC_ENDIAN_LITTLE, NC_ENDIAN_BIG
    int parallel_access;            // NC_COLLECTIVE or NC_INDEPENDENT
    
    struct ChunkCache {
        size_t size;                // Cache size in bytes
        size_t nelems;              // Number of cache slots
        float preemption;           // Preemption policy
    } chunkcache;
    
    int quantize_mode;              // Quantization mode
    int nsd;                        // Number of significant digits
    
    void* format_var_info;          // Format-specific data
    void* filters;                  // Filter list
} NC_VAR_INFO_T;
```

**Location**: `include/nc4internal.h:166-201`

### NC_DIM_INFO_T - Dimension Metadata

```c
typedef struct NC_DIM_INFO_T {
    NC_OBJ hdr;                     // Name and ID
    struct NC_GRP_INFO_T* container; // Parent group
    size_t len;                     // Dimension length
    nc_bool_t unlimited;            // Is unlimited
    nc_bool_t extended;             // Needs extension
    nc_bool_t too_long;             // Length too large for size_t
    void* format_dim_info;          // Format-specific data
    struct NC_VAR_INFO* coord_var;  // Coordinate variable
} NC_DIM_INFO_T;
```

### NC_ATT_INFO_T - Attribute Metadata

```c
typedef struct NC_ATT_INFO_T {
    NC_OBJ hdr;                     // Name and ID
    struct NC_OBJ* container;       // Parent group or variable
    size_t len;                     // Number of elements
    nc_bool_t dirty;                // Modified flag
    nc_bool_t created;              // Already created
    nc_type nc_typeid;              // Data type
    void* format_att_info;          // Format-specific data
    void* data;                     // Attribute value
} NC_ATT_INFO_T;
```

### NC_TYPE_INFO_T - User-Defined Type

```c
typedef struct NC_TYPE_INFO_T {
    NC_OBJ hdr;                     // Name and ID
    struct NC_GRP_INFO_T* container; // Parent group
    unsigned rc;                     // Reference count
    int endianness;                  // Byte order
    size_t size;                     // Size in bytes
    nc_bool_t committed;             // Committed to file
    nc_type nc_type_class;           // NC_VLEN, NC_COMPOUND, NC_OPAQUE, NC_ENUM
    void* format_type_info;          // Format-specific data
    int varsized;                    // Variable-sized flag
    
    union {
        struct {
            NClist* enum_member;     // Enum members
            nc_type base_nc_typeid;  // Base type
        } e;
        
        struct {
            NClist* field;           // Compound fields
        } c;
        
        struct {
            nc_type base_nc_typeid;  // Base type
        } v;
    } u;
} NC_TYPE_INFO_T;
```

### NC_FIELD_INFO_T - Compound Field

```c
typedef struct NC_FIELD_INFO_T {
    NC_OBJ hdr;                     // Name and ID
    nc_type nc_typeid;              // Field type
    size_t offset;                  // Offset in compound
    int ndims;                      // Number of dimensions
    int* dim_size;                  // Dimension sizes
    void* format_field_info;        // Format-specific data
} NC_FIELD_INFO_T;
```

### NC_OBJ - Common Object Header

```c
typedef struct NC_OBJ {
    NC_SORT sort;                   // NCVAR, NCDIM, NCATT, NCTYP, NCGRP, NCFIL
    char* name;                     // Object name
    int id;                         // Object ID
} NC_OBJ;
```

**Purpose**: Common header for all indexed objects. All structures that go into NCindex must start with NC_OBJ.

## Index Structures

### NCindex - Fast Lookup Index

```c
typedef struct NCindex {
    size_t count;                   // Number of entries
    size_t alloc;                   // Allocated size
    void** content;                 // Array of NC_OBJ* pointers
} NCindex;
```

**Location**: `include/ncindex.h`

**Operations**: O(1) by ID, O(n) by name (uses linear search)

### NC_hashmap - Hash Map

```c
typedef struct NC_hashmap {
    size_t size;                    // Hash table size
    size_t count;                   // Number of entries
    struct NC_hentry** table;       // Hash table
} NC_hashmap;
```

**Location**: `include/nchashmap.h`

**Operations**: O(1) average case for name lookup

## I/O Structures

### ncio - I/O Provider (NetCDF-3)

```c
typedef struct ncio {
    const char* path;               // File path
    int ioflags;                    // I/O flags
    off_t offset;                   // Current offset
    size_t extent;                  // File extent
    size_t nciop_size;              // Provider-specific size
    
    // Function pointers
    int (*rel)(ncio*, off_t, int);
    int (*get)(ncio*, off_t, size_t, int, void**);
    int (*move)(ncio*, off_t, off_t, size_t);
    int (*sync)(ncio*);
    int (*filesize)(ncio*, off_t*);
    int (*pad_length)(ncio*, off_t);
    int (*close)(ncio*, int);
    
    void* pvt;                      // Private data
} ncio;
```

**Location**: `libsrc/ncio.h`

**Implementations**: posixio, memio, httpio, s3io

## Global State

### NCglobalstate - Global Configuration

```c
typedef struct NCglobalstate {
    char* tempdir;                  // Temporary directory
    char* home;                     // Home directory
    char* cwd;                      // Current working directory
    
    struct NCRCinfo* rcinfo;        // RC file info
    
    struct {
        size_t size;                // Chunk cache size
        size_t nelems;              // Number of elements
        float preemption;           // Preemption policy
    } chunkcache;
    
    struct {
        int threshold;              // Alignment threshold
        int alignment;              // Alignment value
        int defined;                // Set flag
    } alignment;
    
    struct {
        char* default_region;       // AWS region
        char* config_file;          // Config file path
        char* profile;              // Profile name
        char* access_key_id;        // Access key
        char* secret_access_key;    // Secret key
    } aws;
    
    NClist* pluginpaths;            // Filter plugin paths
} NCglobalstate;
```

**Location**: `libdispatch/ddispatch.c`

**Access**: `NC_getglobalstate()`

## Utility Structures

### NC_string - Counted String

```c
typedef struct {
    size_t nchars;                  // String length
    char* cp;                       // String data
} NC_string;
```

### NClist - Dynamic List

```c
typedef struct NClist {
    size_t alloc;                   // Allocated size
    size_t length;                  // Current length
    void** content;                 // Array of pointers
} NClist;
```

### NCbytes - Dynamic Byte Buffer

```c
typedef struct NCbytes {
    size_t alloc;                   // Allocated size
    size_t length;                  // Current length
    char* content;                  // Buffer
} NCbytes;
```

## Format-Specific Structures

### HDF5-Specific (NC_HDF5_FILE_INFO_T, etc.)

Stored in `format_file_info`, `format_var_info`, etc. fields.

Contains HDF5 handles (hid_t), property lists, and other HDF5-specific data.

### Zarr-Specific (NCZ_FILE_INFO_T, etc.)

Stored in `format_file_info`, `format_var_info`, etc. fields.

Contains Zarr metadata (JSON), chunk cache, zmap handles, codec information.

## Memory Management

**Allocation**: Most structures use `calloc()` for zero-initialization

**Deallocation**: Each structure type has a corresponding `free_*()` function

**Reference Counting**: User-defined types use reference counting (`NC_TYPE_INFO_T.rc`)

**String Handling**: NC_string structures manage their own memory

## Thread Safety

**Global State**: Protected by internal locks (implementation-dependent)

**File Handles**: Not thread-safe - one thread per file handle

**Parallel I/O**: Uses MPI for coordination, not threading

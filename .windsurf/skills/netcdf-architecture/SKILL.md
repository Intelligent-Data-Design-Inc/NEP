---
name: netcdf-architecture
description: Understanding the NetCDF-C library architecture including dispatch tables, format implementations (NetCDF-3, HDF5, Zarr, DAP), I/O layers, and metadata structures. Use when working on NetCDF-C codebase, debugging format issues, adding new features, or understanding how different storage backends interact.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-01-14"
---

# NetCDF-C Architecture Skill

This skill provides comprehensive knowledge of the NetCDF-C library architecture to help you navigate, understand, and modify the codebase effectively.

## Overview

NetCDF-C is a multi-format I/O library built on a **dispatch table architecture** that provides a unified API across 7+ storage formats. The core design pattern uses function pointer tables to route operations to format-specific implementations.

## Core Architecture Pattern

### Dispatch Table Design

Every file format implements the same `NC_Dispatch` interface containing ~70 function pointers:

```c
struct NC_Dispatch {
    int model;                    // Format identifier
    int dispatch_version;         // Compatibility version
    
    // File operations
    int (*create)(...);
    int (*open)(...);
    int (*close)(...);
    
    // Variable I/O
    int (*get_vara)(...);
    int (*put_vara)(...);
    
    // Metadata operations
    int (*def_dim)(...);
    int (*def_var)(...);
    int (*put_att)(...);
    
    // ... ~60 more function pointers
};
```

**Location**: `include/netcdf_dispatch.h`

### Common File Handle (NC Structure)

Every open file is represented by an `NC` struct:

```c
typedef struct NC {
    int ext_ncid;              // External ID (user-visible)
    int int_ncid;              // Internal ID (format-specific)
    const NC_Dispatch* dispatch;  // Function pointer table
    void* dispatchdata;        // Format-specific metadata
    char* path;                // File path
    int mode;                  // Open mode flags
} NC;
```

**Location**: `include/nc.h`

## Directory Structure

### Primary Libraries

- **`libdispatch/`** - Central routing layer, API entry points, utilities
- **`libsrc/`** - Classic NetCDF-3 implementation (CDF-1, CDF-2, CDF-5)
- **`libsrc4/`** - NetCDF-4 enhanced model coordination
- **`libhdf5/`** - HDF5 storage backend
- **`libnczarr/`** - Zarr cloud-native storage
- **`libdap2/`** + **`oc2/`** - OPeNDAP DAP2 client
- **`libdap4/`** - OPeNDAP DAP4 client
- **`libhdf4/`** - HDF4 file access (optional)

### Support Libraries

- **`include/`** - Public API headers and internal interfaces
- **`libncpoco/`** - Portable components
- **`libncxml/`** - XML parsing for DAP4
- **`liblib/`** - Additional utilities

## Key Components by Library

### libdispatch/ - The Routing Layer

**Purpose**: Provides unified API facade and routes calls to appropriate format implementations.

**Critical Files**:
- `ddispatch.c` - Dispatch initialization, global state management
- `dfile.c` - File open/create orchestration, format detection
- `dvarget.c`, `dvarput.c` - Variable I/O entry points
- `dvar.c`, `datt.c`, `ddim.c` - Metadata operation entry points
- `dinfermodel.c` - Format detection (magic numbers, URLs)

**Format Detection Logic**:
1. Check magic number (first 8 bytes)
2. Parse URL scheme (http://, s3://, file://)
3. Analyze mode flags (NC_NETCDF4, NC_CLASSIC_MODEL, etc.)
4. Select appropriate dispatch table

**Utilities**:
- `ncjson.c` - JSON parsing
- `ncuri.c` - URI parsing
- `dauth.c` - Authentication
- `dhttp.c` - HTTP operations
- `ds3util.c` - S3/cloud utilities

### libsrc/ - Classic NetCDF-3

**Purpose**: Implements traditional binary NetCDF formats.

**Dispatch Table**: `NC3_dispatcher` in `nc3dispatch.c`

**Metadata Structure**: `NC3_INFO` - Simple arrays with hashmaps

**Critical Files**:
- `nc3dispatch.c` (517 lines) - Dispatch table implementation
- `nc3internal.c` - Metadata management
- `ncx.c` (743KB) - XDR-like encoding/decoding for all data types
- `putget.c` (353KB) - Variable I/O operations
- `attr.c` (47KB) - Attribute operations
- `var.c`, `dim.c` - Variable and dimension management

**I/O Abstraction (ncio layer)**:
- `posixio.c` - Standard POSIX file I/O
- `memio.c` - In-memory files
- `httpio.c` - HTTP byte-range access
- `s3io.c` - S3 object storage

**Data Structures**:
```c
typedef struct NC3_INFO {
    NC_dimarray dims;      // Dimensions
    NC_attrarray attrs;    // Global attributes
    NC_vararray vars;      // Variables
    size_t xsz;           // External size
    size_t begin_var;     // Offset to variables
    size_t begin_rec;     // Offset to record data
    size_t recsize;       // Record size
    // ... more fields
} NC3_INFO;
```

### libsrc4/ - NetCDF-4 Coordination

**Purpose**: Thin coordination layer for NetCDF-4 enhanced features (groups, user-defined types).

**Note**: This is NOT a complete implementation - it delegates to HDF5 or Zarr backends.

**Files**:
- `nc4dispatch.c` - Minimal initialization
- `nc4attr.c`, `nc4dim.c`, `nc4var.c` - Enhanced metadata operations
- `nc4grp.c` - Group operations
- `nc4type.c` - User-defined type operations
- `nc4internal.c` - Common infrastructure

### libhdf5/ - HDF5 Storage Backend

**Purpose**: Implements NetCDF-4 using HDF5 as the storage format.

**Dispatch Table**: `HDF5_dispatcher` in `hdf5dispatch.c`

**Metadata Structure**: `NC_FILE_INFO_T` with hierarchical groups

**Critical Files**:
- `hdf5dispatch.c` (152 lines) - Dispatch table
- `nc4hdf.c` (87KB) - Core HDF5 integration
- `hdf5open.c` (99KB) - File opening, metadata reading from HDF5
- `hdf5var.c` (85KB) - Variable I/O with chunking, compression, filters
- `hdf5attr.c` (28KB) - Attribute operations
- `hdf5filter.c` - Filter/compression plugin management
- `H5FDhttp.c` - HTTP virtual file driver for byte-range access

**Key Data Structures**:
```c
typedef struct NC_FILE_INFO_T {
    NC_GRP_INFO_T* root_grp;    // Root group
    int no_write;                // Read-only flag
    void* format_file_info;      // HDF5-specific data
    // ... more fields
} NC_FILE_INFO_T;

typedef struct NC_VAR_INFO_T {
    NC_OBJ hdr;                  // Name and ID
    NC_GRP_INFO_T* container;    // Parent group
    size_t ndims;                // Number of dimensions
    int* dimids;                 // Dimension IDs
    size_t* chunksizes;          // Chunk sizes
    int storage;                 // Chunked/contiguous/compact
    int endianness;              // Byte order
    void* filters;               // Compression filters
    // ... more fields
} NC_VAR_INFO_T;
```

**Delegates to**: HDF5 library → HDF5 VFD layer → actual storage

### libnczarr/ - Zarr Storage

**Purpose**: Cloud-native storage using Zarr format specification.

**Dispatch Table**: `NCZ_dispatcher` in `zdispatch.c`

**Metadata Structure**: `NC_FILE_INFO_T` (same as HDF5)

**Critical Files**:
- `zdispatch.c` (323 lines) - Dispatch table
- `zarr.c` - Main Zarr implementation
- `zsync.c` (84KB) - Data synchronization, chunk management
- `zvar.c` (76KB) - Variable operations
- `zfilter.c` - Codec pipeline (compression, filters)
- `zxcache.c` - Chunk caching

**Storage Abstraction (zmap)**:
- `zmap.c` - Abstract storage interface
- `zmap_file.c` - Filesystem backend
- `zmap_s3sdk.c` - AWS S3 backend
- `zmap_zip.c` - ZIP archive backend

**Key Feature**: JSON metadata (.zarray, .zgroup, .zattrs files)

### libdap2/ + oc2/ - OPeNDAP DAP2 Client

**Purpose**: Access remote OPeNDAP servers using DAP2 protocol.

**Dispatch Table**: `NCD2_dispatcher` in `ncd2dispatch.c`

**Components**:
- `ncd2dispatch.c` (85KB) - Dispatch implementation
- `getvara.c` (44KB) - Maps NetCDF API to DAP requests
- `constraints.c` - DAP constraint expression handling
- `cache.c` - Response caching

**OC2 Library** (OPeNDAP Client in `oc2/`):
- `oc.c` (62KB) - Main client implementation
- `dapparse.c`, `daplex.c` - DDS/DAS parsing
- `ocdata.c` - Data retrieval and decoding
- `occurlfunctions.c` - HTTP/libcurl integration

### libdap4/ - OPeNDAP DAP4 Client

**Purpose**: Access remote DAP4 servers (newer protocol).

**Dispatch Table**: `NCD4_dispatcher` in `ncd4dispatch.c`

**Critical Files**:
- `ncd4dispatch.c` (24KB) - Dispatch table
- `d4parser.c` (49KB) - DMR (Dataset Metadata Response) parsing
- `d4data.c` - Binary data handling
- `d4chunk.c` - Chunked response processing
- `d4meta.c` (34KB) - Metadata translation to NetCDF model
- `d4curlfunctions.c` - HTTP operations

## Common Patterns

### 1. API Call Flow

```
User calls nc_get_vara(ncid, varid, start, count, data)
    ↓
libdispatch/dvarget.c
    ↓
Lookup NC* from ncid → get dispatch table
    ↓
dispatch->get_vara(...)
    ↓
Format-specific implementation:
    • NC3_get_vara() → ncx.c XDR decode → ncio read
    • NC4_get_vara() → HDF5 API → chunk cache → decompress
    • NCZ_get_vara() → zmap retrieve → codec pipeline
    • NCD2_get_vara() → HTTP request → parse DDS/DAS
```

### 2. File Opening

```
nc_open(path, mode, &ncid)
    ↓
libdispatch/dfile.c: NC_open()
    ↓
dinfermodel.c: Detect format
    • Check magic number
    • Parse URL scheme
    • Analyze mode flags
    ↓
Select dispatch table
    ↓
dispatch->open(path, mode, ...)
    ↓
Format-specific open implementation
    ↓
Return ncid to user
```

### 3. Metadata Access

All formats use indexed structures for fast lookup:
- **NC3**: Arrays with `NC_hashmap`
- **NC4/HDF5/Zarr**: `NCindex` (hash-based index)

## Important Headers

### Public API
- `netcdf.h` - Main public API
- `netcdf_par.h` - Parallel I/O extensions
- `netcdf_filter.h` - Filter API
- `netcdf_mem.h` - In-memory file API

### Internal Interfaces
- `ncdispatch.h` - Dispatch layer interfaces
- `netcdf_dispatch.h` - NC_Dispatch structure definition
- `nc.h` - NC structure and common functions
- `nc3internal.h` - NetCDF-3 internal structures
- `nc4internal.h` - NetCDF-4 internal structures
- `nc3dispatch.h`, `nc4dispatch.h`, `hdf5dispatch.h` - Format-specific dispatch headers

## When to Use This Skill

Use this skill when:
- **Adding new features** to NetCDF-C
- **Debugging format-specific issues** (e.g., HDF5 vs Zarr differences)
- **Understanding data flow** through the library
- **Implementing new dispatch tables** or storage backends
- **Modifying I/O operations** (chunking, compression, filters)
- **Working with metadata structures** (groups, types, dimensions)
- **Investigating performance issues** (caching, I/O patterns)
- **Integrating new protocols** (new remote access methods)

## Quick Reference

### Find the Right File

**For API entry points**: Look in `libdispatch/d*.c`
**For NetCDF-3 operations**: Look in `libsrc/`
**For HDF5 operations**: Look in `libhdf5/`
**For Zarr operations**: Look in `libnczarr/`
**For remote access**: Look in `libdap2/` or `libdap4/`
**For data encoding**: Look in `libsrc/ncx.c`
**For I/O backends**: Look in `libsrc/*io.c` or `libnczarr/zmap*.c`

### Common Tasks

**Adding a new API function**:
1. Add to `include/netcdf.h`
2. Add entry point in `libdispatch/`
3. Add to `NC_Dispatch` structure
4. Implement in each format's dispatch table

**Adding a new format**:
1. Create new library directory
2. Implement `NC_Dispatch` table
3. Register in `libdispatch/ddispatch.c`
4. Add format detection logic

**Debugging I/O issues**:
1. Enable logging: `export NETCDF_LOG_LEVEL=5`
2. Check dispatch table selection
3. Trace through format-specific implementation
4. Check I/O layer (ncio, HDF5 VFD, zmap)

## Additional Resources

See [references/COMPONENTS.md](references/COMPONENTS.md) for detailed component descriptions.

See [references/DATA-STRUCTURES.md](references/DATA-STRUCTURES.md) for complete data structure documentation.

See [references/DISPATCH-TABLES.md](references/DISPATCH-TABLES.md) for all dispatch table implementations.

See [references/EXAMPLES.md](references/EXAMPLES.md) for programming examples and common patterns.

See [references/FORTRAN-INTERFACE.md](references/FORTRAN-INTERFACE.md) for NetCDF Fortran 90 API documentation and usage patterns.

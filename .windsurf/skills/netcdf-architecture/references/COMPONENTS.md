# NetCDF-C Component Details

This reference provides detailed information about each major component in the NetCDF-C library.

## libdispatch/ - Central Routing Layer

### Core Files

**ddispatch.c** (477 lines)
- Global state management (`NCglobalstate`)
- Dispatch initialization (`NCDISPATCH_initialize()`, `NCDISPATCH_finalize()`)
- Atomic type utilities
- Alignment configuration
- Manages: temp directories, home directory, RC files, chunk cache defaults

**dfile.c** (2225 lines)
- File open/create orchestration
- User-defined format registration (`nc_def_user_format()`)
- Format detection coordination
- NC structure lifecycle management

**dinfermodel.c** (46KB)
- Format detection from magic numbers
- URL scheme parsing (http://, https://, s3://, file://)
- Mode flag analysis
- Dispatch table selection logic

**dvarget.c / dvarput.c**
- Variable I/O entry points
- Type conversion coordination
- Stride/index calculation
- Delegates to format-specific `get_vara()` / `put_vara()`

**dvar.c, datt.c, ddim.c, dgroup.c, dtype.c**
- Metadata operation entry points
- Validation and error checking
- Delegation to format-specific implementations

### Utility Files

**ncjson.c** (35KB) - JSON parsing for Zarr metadata
**ncuri.c** (35KB) - URI parsing and manipulation
**ncbytes.c** - Dynamic byte buffer management
**dauth.c** (12KB) - Authentication (AWS, bearer tokens)
**dhttp.c** (25KB) - HTTP operations via libcurl
**ds3util.c** (32KB) - S3/cloud storage utilities
**nclist.c** - Dynamic list data structure
**nchashmap.c** (149KB) - Hash map implementation
**ncindex.c** - Index structure for fast metadata lookup

## libsrc/ - Classic NetCDF-3

### Format Support
- CDF-1: Classic format (32-bit offsets)
- CDF-2: 64-bit offset format
- CDF-5: 64-bit data format (large variables)

### Core Implementation

**nc3dispatch.c** (517 lines)
- `NC3_dispatcher` table with ~70 function pointers
- Implements all required dispatch operations
- Returns `NC_ENOTNC4` for NetCDF-4-only features
- Stubs for groups, user-defined types, compression

**nc3internal.c** (1784 lines)
- `NC3_INFO` structure management
- Metadata lifecycle (create, duplicate, free)
- Type checking (`nc3_cktype()`)
- Format version handling

**ncx.c** (743KB - generated from ncx.m4)
- External Data Representation (XDR-like)
- Encoding/decoding for all atomic types
- Byte swapping for endianness
- Padding and alignment
- Platform-specific optimizations

**putget.c** (353KB - generated from putget.m4)
- Variable data I/O operations
- Type conversion matrix (all type combinations)
- Strided access implementation
- Record variable handling
- Fill value management

**attr.c** (47KB - generated from attr.m4)
- Attribute CRUD operations
- Attribute type conversion
- Global vs variable attributes
- Attribute renaming and deletion

**var.c** (18KB)
- Variable definition and inquiry
- Variable renaming
- Coordinate variable detection
- Record variable management

**dim.c** (10KB)
- Dimension definition and inquiry
- Unlimited dimension handling
- Dimension renaming

### I/O Layer (ncio abstraction)

**ncio.h / ncio.c**
- Abstract I/O interface
- Strategy pattern for different backends
- Buffer management

**posixio.c** (45KB)
- POSIX file I/O (open, read, write, seek)
- Memory-mapped I/O option
- File locking
- Platform-specific optimizations

**memio.c** (20KB)
- In-memory file implementation
- Dynamic buffer growth
- Extract to external memory

**httpio.c** (8KB)
- HTTP byte-range requests
- Read-only access
- Caching strategy

**s3io.c** (8KB)
- S3 object storage access
- Byte-range requests
- AWS SDK integration

### Data Structures

```c
typedef struct NC3_INFO {
    size_t chunk;           // Chunk size hint
    size_t xsz;            // External file size
    size_t begin_var;      // Offset to non-record variables
    size_t begin_rec;      // Offset to record variables
    size_t recsize;        // Size of one record
    size_t numrecs;        // Number of records
    NC_dimarray dims;      // Dimensions
    NC_attrarray attrs;    // Global attributes
    NC_vararray vars;      // Variables
    ncio* nciop;          // I/O provider
    int flags;            // File flags
    // ... more fields
} NC3_INFO;

typedef struct NC_var {
    NC_string* name;       // Variable name
    size_t ndims;         // Number of dimensions
    int* dimids;          // Dimension IDs
    NC_attrarray attrs;   // Variable attributes
    nc_type type;         // Data type
    size_t len;           // Product of dimension sizes
    size_t begin;         // Offset in file
    // ... more fields
} NC_var;
```

## libhdf5/ - HDF5 Storage Backend

### Core Files

**hdf5dispatch.c** (152 lines)
- `HDF5_dispatcher` table
- Initialization/finalization
- HTTP VFD registration

**nc4hdf.c** (87KB)
- Core HDF5 integration
- File creation with HDF5 API
- Metadata synchronization
- Group/dataset creation
- Attribute handling

**hdf5open.c** (99KB)
- File opening logic
- HDF5 metadata reading
- Dimension scale detection
- Coordinate variable identification
- User-defined type reconstruction

**hdf5var.c** (85KB)
- Variable I/O operations
- Chunking coordination with HDF5
- Filter pipeline management
- Type conversion
- Parallel I/O support

**hdf5attr.c** (28KB)
- Attribute operations via HDF5
- Reserved attribute handling (_NCProperties, etc.)
- Type conversion for attributes

**hdf5filter.c** (16KB)
- Filter plugin management
- HDF5 filter pipeline integration
- Compression (deflate, szip, etc.)
- Custom filter registration

**hdf5dim.c, hdf5grp.c, hdf5type.c**
- Dimension, group, and type operations
- HDF5 dimension scales
- Group hierarchy management
- User-defined type translation

**H5FDhttp.c** (28KB)
- HTTP Virtual File Driver
- Byte-range request support
- Read-only remote access
- Caching strategy

### Key Features
- Uses HDF5 dimension scales for dimensions
- Stores NetCDF metadata in HDF5 attributes
- Supports chunking, compression, filters
- Parallel I/O via HDF5 parallel features
- Backward compatible with pure HDF5 files (with limitations)

## libnczarr/ - Zarr Storage

### Core Files

**zdispatch.c** (323 lines)
- `NCZ_dispatcher` table
- Many operations delegate to libsrc4
- Zarr-specific implementations for I/O and metadata

**zarr.c** (8KB)
- Main Zarr format implementation
- Format version handling
- Metadata JSON generation

**zsync.c** (84KB)
- Data synchronization between memory and storage
- Chunk reading/writing
- Metadata persistence (.zarray, .zgroup, .zattrs)
- Cache management

**zvar.c** (76KB)
- Variable operations
- Chunk coordinate calculation
- Data assembly from chunks
- Fill value handling

**zfilter.c** (37KB)
- Codec pipeline implementation
- Compression (blosc, zlib, etc.)
- Filter chaining
- Plugin support

**zxcache.c** (27KB)
- Chunk cache implementation
- LRU eviction
- Dirty chunk tracking
- Write-back strategy

### Storage Abstraction (zmap)

**zmap.c** (11KB)
- Abstract storage interface
- Key-value semantics
- Backend selection

**zmap_file.c** (31KB)
- Filesystem backend
- Directory structure (.zarray, .zgroup files)
- Atomic writes

**zmap_s3sdk.c** (15KB)
- AWS S3 backend
- Object storage operations
- Credential management

**zmap_zip.c** (22KB)
- ZIP archive backend
- Read-only access
- Efficient random access

### Zarr Format Details
- JSON metadata (.zarray, .zgroup, .zattrs)
- Chunked storage (one file per chunk)
- Codec pipeline (compression, filters)
- Dimension separator (. or /)
- V2 and V3 format support (partial)

## libdap2/ + oc2/ - DAP2 Client

### libdap2/ Files

**ncd2dispatch.c** (85KB)
- `NCD2_dispatcher` table
- Complete dispatch implementation
- Constraint handling integration

**getvara.c** (44KB)
- Maps NetCDF API to DAP requests
- Constraint expression generation
- Subsetting and striding
- Type conversion

**constraints.c** (25KB)
- DAP constraint expression parsing
- Projection and selection
- Optimization

**cache.c** (13KB)
- HTTP response caching
- Cache invalidation
- Disk-based cache

### oc2/ - OPeNDAP Client Library

**oc.c** (62KB)
- Main client implementation
- Connection management
- Request/response handling
- Error handling

**dapparse.c / daplex.c**
- DDS (Dataset Descriptor Structure) parsing
- DAS (Dataset Attribute Structure) parsing
- Lexical analysis

**ocdata.c** (10KB)
- Binary data decoding
- XDR stream processing
- Data assembly

**occurlfunctions.c** (9KB)
- libcurl integration
- HTTP request building
- Authentication
- SSL/TLS support

### DAP2 Protocol
- DDS: Describes data structure
- DAS: Describes attributes
- DODS: Binary data response
- Constraint expressions for subsetting

## libdap4/ - DAP4 Client

### Core Files

**ncd4dispatch.c** (24KB)
- `NCD4_dispatcher` table
- DAP4-specific operations

**d4parser.c** (49KB)
- DMR (Dataset Metadata Response) parsing
- XML-based metadata
- Namespace handling

**d4data.c** (14KB)
- Binary data handling
- Checksum verification
- Data assembly

**d4chunk.c** (6KB)
- Chunked response processing
- Streaming data support

**d4meta.c** (34KB)
- Metadata translation to NetCDF model
- Group hierarchy construction
- Type mapping

**d4curlfunctions.c** (15KB)
- HTTP operations
- Authentication
- Error handling

### DAP4 Protocol
- DMR: XML metadata response
- Binary data with checksums
- Chunked transfer encoding
- Enhanced type system

## Support Libraries

### libncpoco/
Portable components for cross-platform compatibility

### libncxml/
XML parsing for DAP4 DMR responses

### liblib/
Additional utility code and compatibility layers

## Global State (NCglobalstate)

Located in `libdispatch/ddispatch.c`:

```c
typedef struct NCglobalstate {
    char* tempdir;              // Temporary directory
    char* home;                 // Home directory
    char* cwd;                  // Current working directory
    
    struct NCRCinfo* rcinfo;    // RC file configuration
    
    struct {
        size_t size;            // Chunk cache size
        size_t nelems;          // Number of elements
        float preemption;       // Preemption policy
    } chunkcache;
    
    struct {
        int threshold;          // Alignment threshold
        int alignment;          // Alignment value
        int defined;            // Whether set
    } alignment;
    
    struct {
        char* default_region;   // AWS region
        char* config_file;      // AWS config file
        char* profile;          // AWS profile
        char* access_key_id;    // Access key
        char* secret_access_key; // Secret key
    } aws;
    
    NClist* pluginpaths;        // Filter plugin paths
} NCglobalstate;
```

## Filter/Codec System

### HDF5 Filters
- Standard: deflate, shuffle, fletcher32, szip
- Plugin system for custom filters
- Filter IDs registered with HDF Group
- Parameters passed as unsigned int arrays

### Zarr Codecs
- JSON-based codec configuration
- Codec chain (multiple codecs)
- Standard: blosc, zlib, gzip, lz4, zstd
- Extensible through plugins

## Parallel I/O Support

### HDF5 Parallel
- Uses MPI-IO via HDF5
- Collective and independent I/O
- Requires parallel HDF5 build

### PnetCDF (libsrcp/)
- Experimental parallel I/O for NetCDF-3
- MPI-IO based
- Separate dispatch table

## Testing Structure

- `nc_test/` - NetCDF-3 tests
- `nc_test4/` - NetCDF-4/HDF5 tests
- `nczarr_test/` - Zarr tests
- `ncdap_test/` - DAP2 tests
- `unit_test/` - Unit tests
- `h5_test/` - HDF5 interoperability tests

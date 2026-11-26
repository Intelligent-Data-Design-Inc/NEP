# NEP – NetCDF Expansion Pack v4.3.0
## Project Overview

NEP provides NetCDF User Defined Format (UDF) readers — NASA CDF, FITS, PDS4, GeoTIFF, GRIB2, DICOM, legacy PDB and PDBx/mmCIF — and the experimental NEXTCDF-4 backend, a drop-in replacement for netCDF-4/HDF5 with 16-bit floats, reference types and the latest HDF5 superblock support. LZ4 and BZIP2 filter plugins and wrappers moved to [nep-compress](https://github.com/captainkirk99/nep-compress). Older releases also bundled those filters.

## NEXTCDF-4 Fortran binding (v4.3.0 Sprint 1R)

`fsrc/nep_nextcdf4.F90` and its private C byte-buffer bridge implement the installed
`nep_nextcdf4` module / `libnepnextcdf4f`, built only with NEXTCDF-4 and Fortran ON.
NetCDF-Fortran >=4.6.2 and compiler-compatible modules are required for that
combination. Rank-specific wrappers (scalar–7) are generated from one template;
selection validation, dimension reversal and owned packing preserve one-based
Fortran semantics and noncontiguous buffers. C bridges forward raw 16-bit patterns
to existing half APIs; no new conversion engine or on-disk layout is introduced.
`NEP::nepnextcdf4f` exports the installed module directory and NetCDF-Fortran
linkage. Reference and general enhanced-type value wrappers remain deferred beyond
v4.3.0. See [Fortran binding](nextcdf4_fortran.md) for the exact public contract.

## NEXTCDF-4 conversion engine

NEP-owned `nx4_convert_type()` is the permanent engine for supported numeric variable/attribute conversion and quantization, selected after Sprint 1H's OFF/ON comparison. The temporary `NEP_INTERNAL_CONVERT` option and `NEP_HAS_INTERNAL_CONVERT` marker were removed; setting the option fails at configure time. HDF5 receives the matching native numeric type; IEEE float16 conversion remains an explicit HDF5 exception until Sprint 1I. Bfloat16 narrowing uses round-to-nearest-even. The independent `libnep` bfloat16 API, on-disk types and compatibility flags are unchanged. The engine reports `NC_ERANGE` after completed conversion/I/O, without rolling back attributes or dimension growth. HDF5 transfer callbacks observe exceptions on the retained float16 path; a source check catches silent bfloat16 saturation, and explicit float16 attribute conversion uses an observing transfer list. Invalid real-to-integer attribute casts use guarded conversion, and quantization preserves nonfinite values. No Spack contract changes are required.

## NEXTCDF-4 string ownership (v4.3.0 Sprint 1P)

Standalone `NC_STRING` values and string leaves in compound/array/vlen graphs use pointer-sized memory slots, recursive dynamic-type tracking, and owned read/attribute/fill payloads. Write inputs stay caller-owned. `nx4_copy_data` initializes recursive copies and unwinds partial failures; stored string character sets are matched in native memory datatypes without transcoding. Fill caches always hold element buffers, not bare string bytes. Missing unlimited elements are filled at logical selection positions, including multidimensional striding. Dynamic filled datasets allocate early so HDF5 2.1.1 does not attempt a heap write during read-only missing-chunk reads; sparse extensions may use more storage. New strings use UTF-8 tags, legacy ASCII tags remain supported, and wider fixed-length HDF5 strings stay unsupported. See [NEXTCDF-4](nextcdf4.md) for reclaim APIs, fill rules and interchange limits. No public API or packaging contract changes.

## Architecture

The format handlers and NEXTCDF-4 backend remain in NEP; compression is independently distributed.

### 1. HDF5 Filter Plugin Architecture

Historical NEP releases bundled HDF5 filter plugins. In v4.2.0, nep-compress owns them; NEP's NEXTCDF-4 backend still supports generic HDF5 filters supplied by the application environment:

1. **Core NetCDF API**: Standard API used by applications
2. **HDF5 Filter Pipeline**: Compression filters integrated into HDF5 I/O operations
3. **External Filter Plugins**: LZ4/BZIP2/zstd and others supplied by nep-compress or the application environment, discovered through `HDF5_PLUGIN_PATH`
4. **NetCDF-4/HDF5 Storage**: Compressed data files

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[HDF5 Library]
       │
[HDF5 Filter Pipeline]
       │
┌──────┴──────────────────┐
│                         │
[External Filter Plugins — nep-compress LZ4/BZIP2, system zstd/szip, ...]
│                         │
└──────┬──────────────────┘
       │
[Compressed NetCDF-4/HDF5 Files]
```

### 2. NetCDF User Defined Format (UDF) System

NEP implements NetCDF's UDF system to provide transparent access to various scientific data formats:

1. **Core NetCDF API**: Standard API used by applications
2. **NC_Dispatch Layer**: Format-specific function pointer tables
3. **Format Handlers**: CDF, GeoTIFF, GRIB2, FITS, PDS4, and DICOM UDF handlers
4. **Format Libraries**: Integration with libgeotiff, NASA CDF library, NCEPLIBS-g2c, CFITSIO, libxml2, and libdicom/libjpeg
5. **Multi-format Storage**: Access to NetCDF-4, CDF, GeoTIFF, GRIB2, FITS, PDS4, and DICOM files

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────┬─────────────┬───────────────┬────────────┬─────────────┐
│             │             │               │            │             │
[HDF5 Backend] [CDF Handler] [GeoTIFF Handler] [GRIB2 Handler] [FITS Handler] [PDS4 Handler]
│             │             │               │            │             │
│          [CDF Library] [libgeotiff]  [NCEPLIBS-g2c] [CFITSIO]   [libxml2]
│             │             │               │            │             │
└──────┬──────┴──────┬──────┴───────┬───────┴─────┬──────┴──────┬──────┘
       │             │              │             │             │
[NetCDF-4 Files] [CDF Files] [GeoTIFF Files] [GRIB2 Files] [FITS Files] [PDS4 Files]
```

#### UDF Slot Allocation

Current netcdf-c `main` exposes 64 UDF slots (UDF0–UDF63). NEP assigns each format handler a permanent slot in UDF0–UDF9:

| Format | UDF Slot | NetCDF-C Constant | Notes |
|--------|----------|-------------------|-------|
| GeoTIFF BigTIFF | 0 | `NC_UDF0` | Large TIFF files (>4 GB) |
| GeoTIFF standard TIFF | 1 | `NC_UDF1` | Regular TIFF/GeoTIFF files |
| GRIB2 | 2 | `NC_UDF2` | NOAA/ECMWF GRIB2 messages |
| FITS | 3 | `NC_UDF3` | Astronomical images and tables (v2.0.0) |
| CDF | 4 | `NC_UDF4` | NASA CDF; moved from UDF2 in v2.2.0 Sprint 2 |
| PDS4 | 5 | `NC_UDF5` | NASA/ESA planetary data labels (v2.2.0 Sprint 3) |
| DICOM | 6 | `NC_UDF6` | DICOM medical imaging (v3.0.0 Sprint 3) |
| Legacy PDB | 7 | `NC_UDF7` | Legacy protein structure data |
| PDBx/mmCIF | 8 | `NC_UDF8` | Modern protein structure data |
| NEXTCDF-4 | 9 | `NC_UDF9` | Optional HDF5 backend selected explicitly with `NC_NEXTCDF4` |

Before v2.2.0, CDF and GRIB2 shared UDF slot 2 and were mutually exclusive. v2.2.0 Sprint 2 removes that restriction by moving CDF to its own slot. NEXTCDF-4 occupies UDF9 and has no magic-number registration because HDF5 files already belong to the built-in NetCDF-4 backend; applications select it explicitly with `NC_NEXTCDF4`.

NEXTCDF-4 implements the full v4.0.0 feature set. `nc_create()` writes `_Nextcdf4Backend` and, in `NC_NETCDF4_MODEL` mode, `_Nextcdf4Model`; `nc_open()` validates those markers and reconstructs groups, dimensions, variables, attributes, and user-defined types from the HDF5 file. Native and `NC_CLASSIC_MODEL` files use HDF5 Superblock v3 (latest format bounds), while `NC_NETCDF4_MODEL` files use Superblock v1 for compatibility with upstream netcdf-c and HDF5 1.10.x. The implementation includes dimension-scale creation and attachment with `_Netcdf4Dimid` and `_Netcdf4Coordinates`, fixed-size and unlimited-dimension variable I/O, `nc_redef`/`nc_enddef` transitions, attribute put/get/rename/delete, arbitrarily nested groups, compound/enum/opaque/vlen user-defined types including vlens of user types, nested vlens, and compounds containing vlens, `NC_STRING` standalone and nested string values, chunking, compression, filters, fill values, endianness, quantization, correct dimension and variable renaming, small floating-point types (`NC_FLOAT16`, `NC_BFLOAT16`, FP8/FP6/FP4), complex numbers (`NC_COMPLEX`, `NC_DOUBLECOMPLEX`), bitfields (`NC_BITFIELD8/16/32/64`), and HDF5 references. NEXTCDF-4 supports the unified `H5T_STD_REF` reference datatype, exposed as a single opaque `nc_type` `NC_STD_REF` whose elements are 64-byte `H5R_ref_t` tokens (`nc_std_ref_t`). The `nc_ref_*`/`nc_deref_*` API in `src/nextcdf4/nxt4ref.c` creates and resolves object, region, and attribute references, including external-file targets; `nc_deref_*` return `NC_ENOTNC4` for external references.

## NEXTCDF-4 session controls (v4.3.0 Sprint 1O)

`NEXTCDF4_set_fill` stores the per-session default in shared `NC_FILE_INFO_T.fill_mode`; new eligible variables inherit it before eager dataset creation. Existing variables, user-type/string/reference fill rules and per-variable overrides are unchanged. Reopen resets the session default to `NC_FILL`.

`NEXTCDF4_set/get_var_chunk_cache` use `var->chunkcache` and real dataset access property lists. Creation, deferred rebuild and loading apply the request; live changes close all owned aliases identified by HDF5 object tokens, then reopen without changing object identity or DCPL storage. Coordinate variables and scale handles must reopen together. Overrides are not persisted. `NEXTCDF4_rename_grp` moves and flushes links, updating metadata indices without recreating descendants. Native and NetCDF-4-model modes support groups; classic mode does not.

Create/open validate unsupported memory/sharing/parallel bits before side effects (`NC_DISKLESS`, `NC_INMEMORY`, `NC_MPIIO`/`NC_MPIPOSIX`, `NC_SHARE`, `NC_PERSIST`, `NC_MMAP` → `NC_EINVAL`). Image files remain deferred. Native serial SWMR writers use `NC_SWMR_WRITE` (deferred to enddef on create, immediate after metadata load on writable open) or `nc_start_swmr_write`; `nc_swmr_flush` and active `nc_sync` publish all datasets/scales without record atomicity or power-loss durability. Only fixed-size atomic variables are supported, with no declared user types or string/reference variables; schema/attributes freeze at activation. MPI-IO and compatibility modes are rejected. Readers open with `NC_SWMR_READ` on a superblock-v3 native or `NC_NETCDF4_MODEL` file and call `nc_swmr_refresh` to refresh every variable and dimension-scale dataset and update cached dimension lengths; multiple readers and either close order are supported. See [NEXTCDF-4](nextcdf4.md). Sprint 1V2 implements `nc_var_par_access` for fixed-size parallel variable I/O; dynamic/string/reference parallel variable transfers return `NC_ENOTBUILT`.

## NEXTCDF-4 parallel I/O (v4.3.0 Sprint 1V1)

With `NEP_ENABLE_NEXTCDF4_PARALLEL=ON` (requires `NEP_ENABLE_NEXTCDF4`, an MPI C compiler, `HDF5_IS_PARALLEL` and `NC_HAS_PARALLEL4`; emits `NEP_HAS_NEXTCDF4_PARALLEL` in `nep_meta.h`), `nc_create_par`/`nc_open_par` reach the backend. Upstream `nc_create_par` rejects any create mode without `NC_NETCDF4` when NetCDF-C lacks PnetCDF, so creates pass `NC_NEXTCDF4|NC_NETCDF4`; opens pass bare `NC_NEXTCDF4`. The dispatch `parameters` pointer carries an `NC_MPI_INFO` layout (`{comm, info}`; declared privately as `NX4_MPI_INFO` since upstream's struct is not installed). `NEXTCDF4_create`/`NEXTCDF4_open` call `H5Pset_fapl_mpio` and enable collective metadata ops (`H5Pset_all_coll_metadata_ops`, `H5Pset_coll_metadata_write`) on the FAPL, `MPI_Comm_dup`/`MPI_Info_dup` the handles into `NEXTCDF4_FILE_INFO_T`, and free them in `NEXTCDF4_free_file` after `H5Fclose`. Without the feature, a non-NULL `parameters` returns `NC_ENOPAR` (memory images still hit the earlier `NC_EINVAL`). All ranks must make identical define-mode calls. Sprint 1V2's `NEXTCDF4_var_par_access` stores a session-only preference in `NEXTCDF4_VAR_INFO_T.parallel_access`; `var_io` creates/closes an HDF5 transfer property list and calls `H5Pset_dxpl_mpio`. Empty/fill-only collective selections still enter HDF5. Collective unlimited writes reduce proposed extents with `MPI_MAX` over the owned communicator before growing variable/scale datasets; independent growth returns `NC_ECANTEXTEND`. Filters defined in parallel force collective mode; independent filtered writes are rejected while independent filtered reads are permitted (HDF5 parallel filter rule). Reopen resets preferences, so loaded filtered variables require explicit collective selection before writes. Fixed-size types, including NEP small floats, are supported; dynamic/string/reference variable I/O returns `NC_ENOTBUILT` before mutation. Conversion range warnings remain local after completed I/O; no general rank-local validation consensus is added. `ncnextcdf4` links `MPI::MPI_C` only when enabled. `test/nextcdf4/tst_nextcdf4_parallel` runs via `mpiexec -n 4` under label `nextcdf4_parallel` when `NEP_ENABLE_PARALLEL_TESTS` is on; `ci-parallel.yml` builds netcdf-c `main` (64-slot) with parallel support. Sprint 1V3 extends the label with near-verbatim ports of the eight upstream `nc_test4` parallel tests under `test/nextcdf4/nc_test4/` (each registered twice — native `NC_NEXTCDF4` and `NC_NEXTCDF4|NC_NETCDF4_MODEL` — via the `nep_nc_test4_par_create_mode()`/`nep_nc_test4_par_open_mode()` helpers in the `nc_tests.h` shim), `tst_nextcdf4_interchange` (NEXTCDF-4 reads unmarked upstream files; upstream reads `NC_NETCDF4_MODEL` files), `tst_nextcdf4_parallel_plugin` (zstd required, bzip2 gated, via `HDF5_PLUGIN_PATH`), and the disk-bound `examples/nextcdf4/parallel_perf.c` benchmark under `NEP_ENABLE_BENCHMARKS`. `ci-parallel-mpich.yml` runs the same label on a separate cached MPICH + parallel HDF5 2.1.1 + netcdf-c `main` stack. Sprint 1V3 also persists quantize mode via the upstream `_Quantize*` dataset attributes (`nxt4meta.c` `nx4_write_quantize_att`/`nx4_quantize_from_att`) so `nc_inq_var_quantize` works after reopen. Spack: `+nextcdf4+parallel @4.3.0:` maps to the option — no new variant.

## Project Structure

The project is structured as follows:
- `/src` - Core C source code including UDF handlers and logging
- `/src/nextcdf4` - NEXTCDF-4 rewrite of the NetCDF-4/HDF5 backend
- `/fsrc` - Fortran source for the `nep_nextcdf4` module
- `/ftest` - Fortran unit tests (format readers and NEXTCDF-4 binding)
- `/include` - Public header files
- `/test` - C unit tests for all features
- `/test/data` - Test data files for CDF and GeoTIFF formats
- `/examples` - C/Fortran example and benchmark programs
- `/docs` - Project documentation
- `/docs/releases` - Release notes for each version
- `/.github/workflows` - CI/CD pipeline configurations
- `/spack` - Spack overlay recipe and CI environment
- `/conda` - Conda recipe and staged-recipes generator

## Compression Filter Design

Compression filter implementation, C/Fortran wrappers, build options and
HDF5 plugin deployment are maintained by
[nep-compress](https://github.com/captainkirk99/nep-compress).
NEP does not install an LZ4 or BZIP2 plugin.

## Visualization Example Architecture

Optional Python visualization examples in `examples/viz/` demonstrate direct use of `netCDF4.Dataset` with NEP UDF files. The build-tree `.ncrc` file autoloads enabled UDF dispatch libraries through `NCRCENV_RC`; `NETCDF_RC` identifies its containing directory and `LD_LIBRARY_PATH` provides NEP and format-library dependencies. The Python `netCDF4` extension must be built from source against the same NetCDF-C installation as NEP, because a wheel with another `libnetcdf` cannot load the in-process UDF handlers.

Each script writes a 150-DPI grayscale PNG and companion metadata file to the build tree through `plot_common.py`. The shared verifier runs after the conditional format scripts in CMake, requiring a PNG/metadata pair per enabled plot, ordered non-empty `title`, `caption`, and `alt_text` fields, captions of at most 75 words, dimensions no larger than 8.0 by 6.1 inches, and equal RGB channels. Artifacts are test outputs only and are not installed.

## Continuous Integration and Documentation

The project uses GitHub Actions for CI/CD with comprehensive documentation integration:

### Build and Test Pipeline
- Automatic CMake builds on Ubuntu
- Installation of dependencies (HDF5, NetCDF-C, optional format libraries, Doxygen)
- Custom dependency builds with caching for performance
- Test execution via CTest
- Format-reader testing and optional general NetCDF benchmarks
- NEXTCDF-4 dedicated jobs: normal (full suite + docs), ASan/UBSan sanitizers, and an lcov code-coverage job that publishes an HTML coverage report artifact

### Documentation Integration
- **Doxygen Build System Integration**: Documentation generation integrated with CMake
  - CMake: `cmake --build build --target docs` using `find_package(Doxygen)` with conditional building
  - Template configuration: `Doxyfile.in` with automatic variable substitution
  - Build option: `-DNEP_BUILD_DOCUMENTATION=ON/OFF`
- **CI Documentation Builds**: Integrated CMake documentation generation in CI
  - Documentation builds use the CMake configuration
  - Zero-warning enforcement (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - GitHub Pages deployment for API documentation

## Build System Architecture

### CMake Build System

NEP uses CMake as its sole supported build system.
- **Modern CMake 3.19+**: Leverages contemporary CMake features and best practices
- **Modular Compilation**: Each enabled format handler builds as an independent shared library
- **Advanced Dependency Detection**: Automatic discovery with comprehensive Find modules
- **Configurable Build Options**: Fine-grained control over optional format readers
- **Cross-Platform Support**: Unified build configuration across Linux and Unix
- **Installation Integration**: Complete install/uninstall target implementation
- **Build Metadata Header**: `include/nep_meta.h` is generated from `include/nep_meta.h.in` at configure time and reports the NEP version, build date, compiler, and enabled features (`NEP_HAS_*` macros) to consuming projects.

### HDF5 Filter Plugin Integration

NEP does not compile or install LZ4/BZIP2 plugins. HDF5 may load
application-provided plugins for NetCDF-4/NEXTCDF-4 I/O, including
plugins installed by nep-compress.

### HDF5 Filter Plugin Architecture

#### Dynamic Loading Framework
- **Runtime Discovery**: Filters discovered via HDF5_PLUGIN_PATH environment variable
- **Separate Compilation Units**: External filter plugins are not NEP targets
- **Automatic Registration**: HDF5 automatically registers filters when needed
- **Filter ID Management**: Unique filter IDs for each compression algorithm
- **Memory Management**: Proper cleanup and resource management

#### Build Configuration Options

**CMake Configuration:**
```bash
# Installation path configuration
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local

# Documentation control
cmake -S . -B build -DNEP_BUILD_DOCUMENTATION=ON
```

### Dependency Management

#### Library Requirements

| Component | Required Library | Version | Source | Detection Method |
|-----------|------------------|---------|--------|------------------|
| Core | NetCDF-C | v4.10.1+ (v4.10.2+ / current main for NEXTCDF-4's 64-slot UDF) | Unidata | pkg-config, FindNetCDF.cmake |
| Core | HDF5 | v1.14+ (v2.1.1 for NEXTCDF-4) | HDF Group | `cmake/NEPFindHDF5.cmake` imported targets |
| DICOM Reader | libdicom + libjpeg | Latest | https://github.com/ImagingLib/libdicom + system | CMake `find_library`, `find_path`, and `find_package(JPEG)` |
| Docs | Doxygen | Latest | Optional | find_package(Doxygen) |

#### Dependency Detection
- **Automatic Discovery**: Build systems automatically locate required libraries
- **Graceful Degradation**: Missing dependencies result in clear error messages
- **Version Validation**: Compatibility checking for library versions
- **Custom Paths**: Support for non-standard library installation locations

### Installation System

#### Installation Architecture
- **Single Installation Path**: All components install to unified, configurable location
- **Format Library Installation**: Enabled UDF dispatch libraries installed to `${CMAKE_INSTALL_LIBDIR}`
- **Proper RPATH Configuration**: Shared libraries configured with correct runtime paths
- **Standard Paths**: Follows platform conventions for library installation
- **Plugin Independence**: HDF5 filters are supplied separately by nep-compress or other providers

#### Installation Configuration
**CMake:**
```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

#### Installation Structure
```
${PREFIX}/
├── lib/
│   ├── libnep.so             # NEP logging and bfloat16 I/O functions
│   └── libnc*.so             # Enabled UDF format handlers
├── include/
│   └── nep.h
└── share/doc/nep/
    └── html/
        └── [Doxygen generated documentation]
```

#### Uninstall Support
The CMake build provides uninstall functionality:
```bash
cmake --build build --target uninstall  # Removes all installed files
```

## Usage

### Compression supplied separately

Set `HDF5_PLUGIN_PATH` to the plugin directories installed by
[nep-compress](https://github.com/captainkirk99/nep-compress) when using
LZ4/BZIP2. NEP's format readers and NEXTCDF-4 do not build or install
those plugins. Historical benchmark data remains in prior releases.

## GeoTIFF Read Layer (v1.6.0)

### Overview

NEP v1.5.0 introduced support for GeoTIFF geospatial raster data format through a User Defined Format (UDF) handler. NEP v1.6.0 extends this with full CF-1.8 compliant CRS metadata including grid-mapping variables, coordinate variables, and coordinate bounds. This enables transparent access to GeoTIFF files through the standard NetCDF API, eliminating the need for format conversion in geospatial workflows.

For details on NetCDF's User Defined Format mechanism, see `docs/udf.md`.

### Architecture

The GeoTIFF read layer follows the NC_Dispatch pattern used for other UDF handlers in NEP:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [GeoTIFF UDF Handler]
│                 │
│            [libgeotiff]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [GeoTIFF Files]
```

### Key Components

#### GeoTIFF UDF Handler
- **Format Detection**: Automatic identification of GeoTIFF files via magic number and TIFF header inspection
- **NC_Dispatch Implementation**: Complete dispatch table for GeoTIFF file operations
- **File Operations**: Open, close, and query operations for GeoTIFF files
- **Metadata Extraction**: Dimensions, data types, georeferencing information, and GeoTIFF tags
- **Data Access**: Raster band reading with support for various data types

#### Raster Band Mapping
- **Single-band GeoTIFF**: Maps to 2D NetCDF variable (e.g., `data[y, x]`)
- **Multi-band GeoTIFF**: Maps to 3D NetCDF variable with band dimension (e.g., `data[band, y, x]`)
- **Variable Naming**: Configurable naming scheme for bands (default: `band1`, `band2`, etc.)

#### Georeferencing Metadata
Coordinate reference system (CRS) and georeferencing information stored following CF-1.8 conventions:

- **`crs` Grid-Mapping Variable**: A scalar variable holding all CRS parameters as attributes (`grid_mapping_name`, `semi_major_axis`, `inverse_flattening`, and projection-specific parameters). The data variable carries a `grid_mapping = "crs"` attribute pointing to it.
- **Coordinate Variables**: 1D coordinate variables are created from the GeoTIFF GeoTransform tags:
  - Geographic CRS: `lon[x]` (degrees_east) and `lat[y]` (degrees_north)
  - Projected CRS: `x[x]` (m) and `y[y]` (m)
  - The data variable carries a `coordinates = "lon lat"` (or `"x y"`) attribute.
- **Coordinate Bounds**: For pixel-as-area rasters (the default), bounds variables (`lon_bnds`, `lat_bnds` or `x_bnds`, `y_bnds`) are created and attached via `bounds` attributes on the coordinate variables.
- **Pixel Raster Type**: The GeoTIFF `GTRasterTypeGeoKey` is read to distinguish pixel-as-point (coordinate at pixel corner) from pixel-as-area (coordinate at pixel centre) rasters.
- **Graceful Degradation**: If GeoTIFF CRS tags are absent or malformed, `nc_open` succeeds and the raster `data` variable remains readable; no `crs` variable is created.
- **CF Compliance**: See `docs/cf-compliance.md` for the full attribute specification.

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| GeoTIFF Handler | libgeotiff | Latest stable | GeoTIFF file operations and metadata |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

**Note**: PROJ library support is out of scope for v1.6.0. Coordinate transformations will be addressed in future releases if needed.

### Build System Integration

#### GeoTIFF Support
GeoTIFF support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -S . -B build -DNEP_ENABLE_GEOTIFF=ON   # Enable GeoTIFF support
cmake -S . -B build -DNEP_ENABLE_GEOTIFF=OFF  # Disable GeoTIFF support (default)
```

#### Dependency Detection
- Automatic libgeotiff detection during configuration
- Clear error messages if libgeotiff not found when enabled
- Graceful degradation when disabled

### Implementation

The GeoTIFF UDF handler is fully implemented in `src/geotifffile.c` and `src/geotiffdispatch.c` with the following features:

1. **File Operations**
   - `NC_GEOTIFF_open()` and `NC_GEOTIFF_close()` for lifecycle management
   - Automatic TIFF magic number validation and GeoTIFF tag detection
   - Resource cleanup and error handling

2. **Metadata Handling**
   - Dimension extraction (bands, rows, columns)
   - Variable creation with appropriate data types
   - CRS extraction via `extract_crs_parameters()` into `NC_GEOTIFF_CRS_INFO_T`
   - CRS validation via `validate_crs_completeness()`
   - CF-1.8 `crs` grid-mapping variable creation via `create_cf_crs_variable()`
   - Coordinate variable and bounds creation from GeoTransform tags

3. **Data Access**
   - `NC_GEOTIFF_get_vara()` for reading raster data
   - Type conversion between GeoTIFF and NetCDF data types
   - Support for multi-band imagery
   - Endianness handling for cross-platform compatibility

4. **Security Features**
   - Bounds checking and validation
   - Protection against malformed files
   - Memory management with proper cleanup

### Integration Points

- **Format Detection**: Integrated into NetCDF's file open logic
- **Dispatch Table**: Registered alongside existing format handlers
- **Error Handling**: Standard NetCDF error codes and messages
- **Testing**: Unit tests for each phase, integration tests with sample GeoTIFF files

### Test Data

The project includes sample GeoTIFF files:

**NASA MODIS NRT Global Flood Products (geographic CRS, Clarke 1866 ellipsoid)**
- `test/data/MCDWD_L3_F1C_NRT.A2025353.h00v02.061.tif`
- `test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif`

MODIS Daily L3 Global Flood Composite files at 250m resolution. `grid_mapping_name = "latitude_longitude"`, `semi_major_axis = 6378206.4`, `inverse_flattening = 294.978698`.

**ABBA Sinusoidal Projection (projected CRS, spherical Earth)**
- `test/data/ABBA_2022_C61_HNL.tif`

ABBA fire product in sinusoidal projection. `grid_mapping_name = "sinusoidal"`, `semi_major_axis = 6371007.181`, coordinate variables `x` and `y` in metres.

**Data Source**: NASA LANCE MODIS NRT Global Flood Product (MCDWD_L3_F1C_NRT)
- **Product**: MODIS Daily L3 Global Flood Composite 250m Linear Lat Lon Grid - NRT
- **Resolution**: ~250m
- **Format**: GeoTIFF with embedded georeferencing
- **Documentation**: https://www.earthdata.nasa.gov/data/instruments/viirs/near-real-time-data/nrt-global-flood-products

### Reference Documentation

- `docs/cf-compliance.md` - CF-1.8 grid mapping attribute specification for NEP GeoTIFF
- `docs/ESDS-RFC-040v1.1.pdf` - GeoTIFF format specification
- `docs/udf.md` - NetCDF User Defined Format documentation
- https://cfconventions.org/Data/cf-conventions/cf-conventions-1.8/cf-conventions.html#appendix-grid-mappings - CF-1.8 grid mapping appendix
- https://www.earthdata.nasa.gov/data/instruments/viirs/near-real-time-data/nrt-global-flood-products - VIIRS/MODIS NRT Global Flood Products (test data source)

## GRIB2 Format Support (v1.7.0)

### Overview

NEP v1.7.0 adds support for GRIB2 (General Regularly-distributed Information in Binary form, Edition 2) meteorological and oceanographic data files through a User Defined Format (UDF) handler, using the NOAA NCEPLIBS-g2c library. This enables transparent access to GRIB2 files through the standard NetCDF API.

### Architecture

The GRIB2 UDF handler follows the same NC_Dispatch pattern used for CDF and GeoTIFF handlers:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [GRIB2 UDF Handler]
│                 │
│             [NCEPLIBS-g2c]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [GRIB2 Files]
```

### Key Components

#### GRIB2 UDF Handler
- **Format Detection**: Automatic identification of GRIB2 files via magic number (`GRIB`) registered at UDF slot 2 (`NEP_UDF_GRIB2 = NC_UDF2`)
- **NC_Dispatch Implementation**: Complete dispatch table for GRIB2 file operations
- **File Operations**: `NC_GRIB2_open()` and `NC_GRIB2_close()` with proper resource management
- **Metadata Mapping**: GRIB2 products mapped to NetCDF variables with shared `y`/`x` dimensions, `NC_FLOAT` type, and per-variable + global attributes
- **Data Access**: `NC_GRIB2_get_vara()` expands full grid via `g2_getfld(expand=1)` with bitmap-to-`_FillValue` substitution and `start`/`count` slicing

#### Product-to-Variable Mapping
- Each GRIB2 product (one per message for single-product files) → one `NC_FLOAT` NetCDF variable with shared `[y, x]` dimensions
- Variable name from `g2c_param_abbrev()`; duplicate names uniquified with `_2`, `_3`, ... suffixes
- Shared `y` (ny) and `x` (nx) dimensions created once; all variables reference the same dim IDs
- Grid size taken from the first product via `g2c_inq_dim_info()`

#### Metadata Model
- **Variable attributes**: `long_name` (from abbreviation), `_FillValue = 9.999e20f`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`
- **Global attributes**: `Conventions = "GRIB2"`, `GRIB2_edition = 2`
- **Dispatch pointers**: `inq`, `inq_dimid`, `inq_dim`, `inq_var`, `inq_att`, `get_att`, `get_vara` — wired at Sprint 1; metadata populated at Sprint 3
- Note: `units` attribute is not provided — NCEPLIBS-g2c has no API to retrieve parameter units

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_GRIB2_FILE_INFO_T` | `src/grib2internal.h` | Per-file state: `g2cid`, `num_messages`, `num_products`, `num_x`/`num_y`, `*products` array |
| `NC_GRIB2_PROD_INFO_T` | `src/grib2internal.h` | Per-product inventory: `msg_index`, `prod_index`, `discipline`, `category`, `param_number`, `nx`, `ny`, `bytes_to_msg`, `bytes_in_msg`, `abbrev[64]` |
| `NC_VAR_GRIB2_INFO_T` | `src/grib2internal.h` | Per-variable: `msg_index`, `prod_index`, `bytes_to_msg`, `bytes_in_msg` (used by `get_vara` for direct file I/O) |

#### Memory Ownership
- `NC_GRIB2_FILE_INFO_T.products` array: allocated in `NC_GRIB2_open()`, freed in `NC_GRIB2_close()`
- `NC_VAR_GRIB2_INFO_T` per-variable structs: `var->format_var_info`, freed by netcdf-c group teardown
- Attribute `data` pointers (`NC_ATT_INFO_T.data`): `malloc`'d in `grib2_add_*_att()` helpers, freed by netcdf-c att teardown
- `g2_getfld()` output (`gribfield *`): freed with `g2_free()` within `NC_GRIB2_get_vara()`

#### `NC_GRIB2_open()` Pipeline
1. Magic detection: netcdf-c checks `GRIB` prefix before calling `NC_GRIB2_open()`
2. `g2c_open()` → `grib2_file->g2cid`; `strdup(path)` → `grib2_file->path`
3. Two-pass product inventory: `g2c_inq()` → `g2c_inq_msg()` → `g2c_inq_prod()` → populate `NC_GRIB2_PROD_INFO_T[]`
4. Per-message: `g2c_seekmsg()` captures `bytes_to_msg` / `bytes_in_msg` (correct 8-byte length via `hton64`); stored in `PROD_INFO_T` and `NC_VAR_GRIB2_INFO_T`
5. `nc4_dim_list_add()` for shared `y` and `x` dimensions
6. Per-product: `grib2_var_list_add()` → set dim IDs → add `GRIB2_*`, `long_name`, `_FillValue` attributes
7. Global: `Conventions`, `GRIB2_edition` attributes on root group

#### `NC_GRIB2_get_vara()` Data Path
1. `nc4_find_grp_h5_var()` → `NC_VAR_GRIB2_INFO_T` (`bytes_to_msg`, `bytes_in_msg`, `prod_index`)
2. `malloc(bytes_in_msg)` → `msgbuf`
3. `fopen(grib2_file->path, "rb")` + `fseeko(bytes_to_msg)` + `fread(bytes_in_msg bytes)` → raw GRIB2 message in `msgbuf`
4. `g2_getfld(msgbuf, prod_index+1, unpack=1, expand=1, &gfld)` → `gfld->fld[ngrdpts]` full grid, `gfld->bmap[i]` bitmap
5. Bitmap loop: `ibmap==0 && bmap[i]==0` (land/missing) → `full_buf[i] = 9.999e20f`; else `full_buf[i] = gfld->fld[i]`
6. Row-major `start`/`count` copy into caller buffer; `nc4_convert_type()` if `memtype != NC_FLOAT`
7. `g2_free(gfld)`, `free(full_buf)`, `free(msgbuf)`

#### UDF Slot and `.ncrc` Registration
- UDF slot 2 (`NC_UDF2`) used for GRIB2 in `include/nep.h`
- Starting with v2.2.0, CDF uses UDF slot 4 (`NC_UDF4`) and is no longer mutually exclusive with GRIB2
- `NC_GRIB2_initialize()` registers dispatch table via `.ncrc` autoload
- `nep.ncrc` UDF2 block: `NETCDF.UDF2.LIBRARY`, `NETCDF.UDF2.INIT=NC_GRIB2_initialize`, `NETCDF.UDF2.MAGIC=GRIB`

### Source Files

| File | Purpose |
|------|---------|
| `src/grib2dispatch.c` | NC_Dispatch table and `NC_GRIB2_initialize()` |
| `src/grib2file.c` | `NC_GRIB2_open()`, `NC_GRIB2_close()`, `NC_GRIB2_get_vara()` |
| `include/grib2dispatch.h` | Installed public header: `NC_FORMATX_NC_GRIB2`, `NC_GRIB2_initialize()`/`NC_GRIB2_finalize()` |
| `src/grib2internal.h` | Private header: `NC_GRIB2_FILE_INFO_T`/`NC_GRIB2_PROD_INFO_T`/`NC_VAR_GRIB2_INFO_T`, dispatch prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| GRIB2 Handler | NOAA NCEPLIBS-g2c | ≥ 2.1.0 | GRIB2 file operations and data unpacking |
| GRIB2 Handler | libjasper | ≥ 3.0.0 | JPEG2000 compression (transitive dep of g2c) |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

GRIB2 support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -S . -B build -DNEP_ENABLE_GRIB2=ON   # Enable GRIB2 support
cmake -S . -B build -DNEP_ENABLE_GRIB2=OFF  # Disable GRIB2 support (default)
```

### Known Limitations
- Read-only access (NC_NOWRITE mode only)
- 2D grids only; ensemble/time/level dimensions deferred to future releases
- One NetCDF variable per GRIB2 message; multi-message aggregation not supported

---

## FITS Format Support (v2.0.0)

### Overview

NEP v2.0.0 adds support for FITS (Flexible Image Transport System) files through a User Defined Format (UDF) handler, using the CFITSIO library. FITS files from instruments such as HST, Chandra, and JWST can be opened with `nc_open()` and queried with standard NetCDF APIs — no conversion required.

### Architecture

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [FITS UDF Handler]
│                 │
│             [CFITSIO]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [FITS Files]
```

### Key Components

#### FITS UDF Handler
- **Format Detection**: Automatic identification via `"SIMPLE"` magic (first 6 bytes of every FITS file); registered at UDF slot 3 (`NEP_UDF_FITS = NC_UDF3`)
- **NC_Dispatch Implementation**: Complete dispatch table; `NC_FITS_initialize()` calls `nc_def_user_format(NC_UDF3, &FITS_dispatch_table, "SIMPLE")`
- **File Operations**: `NC_FITS_open()` calls `fits_open_file()` and stores the CFITSIO file handle; `NC_FITS_close()` calls `fits_close_file()`
- **Data I/O**: `NC_FITS_get_vara()` reads pixel hyperslabs via `fits_read_subset()` (image path) and table column data via `fits_read_col()` (table path)

#### HDU-to-Group Mapping
- **Primary HDU**: Image or null — goes in the root group; variable named `image`
- **Extension HDUs**: Each becomes a child group named from `EXTNAME` (sanitized) or `hdu_N` if absent
- **Image extension HDUs**: Same dim/variable/attribute mapping as primary HDU applied to the child group
- **Binary and ASCII table HDUs**: Row dimension from `NAXIS2`; one netCDF variable per column named from `TTYPEn`; `TUNITn` → `units` attributes; vector columns become 2D variables

#### Metadata Mapping
- **BITPIX → nc_type**: −64→`NC_DOUBLE`, −32→`NC_FLOAT`, 8→`NC_BYTE`, 16→`NC_SHORT`, 32→`NC_INT`, 64→`NC_INT64`
- **Dimension order**: Reversed from FITS column-major (`NAXIS1`=fastest) to netCDF row-major
- **Standard keyword mapping**: `BUNIT`→`units`, `BZERO`→`add_offset`, `BSCALE`→`scale_factor`, `BLANK`→`_FillValue`
- **All header keywords**: Stored as group-level string attributes on root group or child group
- **Column typecode → nc_type**: A→`NC_CHAR`, I→`NC_SHORT`, J→`NC_INT`, E→`NC_FLOAT`, D→`NC_DOUBLE`, etc.

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_FITS_FILE_INFO_T` | `src/fitsinternal.h` | Per-file: CFITSIO `fitsfile *`, path |
| `NC_FITS_VAR_INFO_T` | `src/fitsinternal.h` | Per-variable: `hdu_num`, `col_num` (0=image), `col_width` (string columns) |

#### `NC_FITS_get_vara()` Data Paths
- **Image**: Reverses `start[]/count[]` into `fpixel[]/lpixel[]`, calls `fits_read_subset()`; CFITSIO applies `BSCALE`/`BZERO` automatically when reading into floating-point types
- **Table column**: `fits_read_col()` with `firstrow = start[0]+1`, `firstelem = start[1]+1` for 2D; string columns use `char **` pointer array into a flat buffer

### Source Files

| File | Purpose |
|------|---------|
| `src/fitsdispatch.c` | NC_Dispatch table and `NC_FITS_initialize()` / `NC_FITS_finalize()` |
| `src/fitsfile.c` | `NC_FITS_open()`, `NC_FITS_close()`, `NC_FITS_get_vara()`, HDU readers |
| `include/fitsdispatch.h` | Installed public header: `NC_FORMATX_NC_FITS`, `NC_FITS_initialize()`/`NC_FITS_finalize()` |
| `src/fitsinternal.h` | Private header: `NC_FITS_FILE_INFO_T`, `NC_FITS_VAR_INFO_T`, dispatch prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| FITS Handler | CFITSIO | ≥ 3.0 | FITS file I/O and HDU navigation |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

FITS support defaults to **OFF** as of v2.2.0 and must be enabled explicitly:

**CMake:**
```bash
cmake -S . -B build -DNEP_ENABLE_FITS=ON   # Enable FITS support
cmake -S . -B build -DNEP_ENABLE_FITS=OFF  # Disable FITS support (default)
```

### Test Data

**`test/data/WFPC2u5780205r_c0fx.fits`** — HST WFPC2 file with:
- Primary image HDU: `[4, 200, 200]` `NC_FLOAT` pixels
- ASCII table extension HDU: 49 columns × 4 rows

### Known Limitations
- Write support not implemented; `nc_create()` on a FITS file returns `NC_EPERM`
- `.ncrc` UDF autoload requires NetCDF-C main branch; local 4.10.0 requires explicit `NC_FITS_initialize()` call

---

## NASA/ESA PDS4 Format Support (v2.2.0)

### Overview

NEP v2.2.0 adds support for NASA/ESA Planetary Data System version 4 (PDS4) XML-labeled data products through a User Defined Format (UDF) handler using libxml2. PDS4 array images, binary tables, and character tables are accessible through the standard NetCDF API.

### Architecture

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [PDS4 UDF Handler]
│                 │
│             [libxml2]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [PDS4 XML Labels + Data Files]
```

### Key Components

#### PDS4 UDF Handler
- **Format Detection**: PDS4 XML label files (`.xml`) identified by `<?xml` / `Product_Observational` root element; registered at UDF slot 5 (`NEP_UDF_PDS4 = NC_UDF5`)
- **NC_Dispatch Implementation**: `NC_PDS4_initialize()` calls `nc_def_user_format(NC_UDF5, &PDS4_dispatch_table, NULL)`
- **XML Parsing**: libxml2 parses `Product_Observational` labels; PDS4 namespace validated on open
- **Data File Resolution**: Data filenames resolved relative to the XML label directory

#### Metadata Mapping

| PDS4 Element | NetCDF Mapping |
|---|---|
| `Identification_Area` fields | Global string attributes on root group |
| `Observation_Area` fields | Global string attributes on root group |
| `File_Area_Observational` | Child group named from `File/file_name` |
| `Array` / `Array_2D_Image` / `Array_2D_Spectrum` | netCDF dimensions (from `Axis_Array` sorted by `sequence_number`) + variable (from `Element_Array/data_type`) |
| `Table_Binary` / `Table_Character` / `Table_Delimited` | `record` dimension from `<records>`; one variable per `Field_*` with type and optional `units` attribute |

#### Data Reading (`NC_PDS4_get_vara`)
- **Array data**: Contiguous row-major reads with `start[]/count[]` hyperslab support; MSB/LSB byte-swap applied based on label byte-order
- **Binary table fields**: Per-record seek using `field_location` / `field_length` / `record_length`; byte-swap as required
- **ASCII table fields**: Fixed-width text parsed via `strtod` / `strtoll` into the mapped netCDF type
- **Delimited tables**: Row count derived from label `<records>`

#### Key Data Structures

| Struct | Location | Purpose |
|--------|----------|---------|
| `NC_PDS4_FILE_INFO_T` | `src/pds4internal.h` | Per-file: xmlDoc pointer, label path, data directory |
| `NC_PDS4_VAR_INFO_T` | `src/pds4internal.h` | Per-variable: file offset, record length, field offset/length, table/ASCII flags, byte-order |

### Source Files

| File | Purpose |
|------|---------|
| `src/pds4dispatch.c` | NC_Dispatch table and `NC_PDS4_initialize()` / `NC_PDS4_finalize()` |
| `src/pds4file.c` | `NC_PDS4_open()`, `NC_PDS4_close()`, `NC_PDS4_get_vara()`, XML metadata readers |
| `include/pds4dispatch.h` | Installed public header: `NC_FORMATX_NC_PDS4`, `NC_PDS4_initialize()`/`NC_PDS4_finalize()` |
| `src/pds4internal.h` | Private header: `NC_PDS4_FILE_INFO_T`, `NC_PDS4_VAR_INFO_T`, dispatch prototypes |

### Dependencies

| Component | Library | Version | Purpose |
|-----------|---------|---------|---------|
| PDS4 Handler | libxml2 | ≥ 2.9 (`libxml2-dev`) | XML label parsing |
| Core | NetCDF-C | v4.10.1+ | NetCDF API |
| Core | HDF5 | v2.1.1+ | HDF5 backend |

### Build System Integration

PDS4 support defaults to **OFF** and must be enabled explicitly:

**CMake:**
```bash
cmake -S . -B build -DNEP_ENABLE_PDS4=ON   # Enable PDS4 support
cmake -S . -B build -DNEP_ENABLE_PDS4=OFF  # Disable PDS4 support (default)
```

### Test Data

Located in `test/data/PDS4/`:
- `test_image.xml` / `test_image.img` — Array_2D_Image product
- `test_table_binary.xml` / `test_table_binary.dat` — Table_Binary product (3 fields × 5 records)
- `Table_Character_Example.xml` / `Table_Character_Example.tab` — Table_Character product (3 fields × 3 records)

Real mission test products also include MAVEN NGIMS/IUVS, Mars 2020 Perseverance Mastcam-Z, and New Horizons Alice `.lblx` / `.fit` pairs.

### Known Limitations
- Write support not implemented; `nc_create()` returns `NC_EPERM`
- `nc_get_vars()` / `nc_get_varm()` use default dispatch fallbacks (no strided native reads)
- Type conversion between `memtype` and file type not performed; caller must request the native type
- Grouped fields (`Group_Field_Binary`) and bit fields not yet supported
- Only `Product_Observational` root element type validated

---

## NASA CDF Format Support (v1.3.0)

### Overview

NEP v1.3.0 added support for NASA Common Data Format (CDF) files through a User Defined Format (UDF) handler. This enables transparent access to CDF space physics and satellite data through the standard NetCDF API.

### Architecture

The CDF UDF handler follows the same NC_Dispatch pattern used for other format handlers:

```
[Application Layer]
       │
[NetCDF-4 API]
       │
[NC_Dispatch Layer]
       │
┌──────┴──────────┐
│                 │
[HDF5 Backend]   [CDF UDF Handler]
│                 │
│             [NASA CDF Library]
│                 │
└──────┬──────────┘
       │
[NetCDF-4/HDF5 Files]  [CDF Files]
```

### Key Components

#### CDF UDF Handler
- **Format Detection**: Automatic identification of CDF files via magic number
- **NC_Dispatch Implementation**: Complete dispatch table for CDF file operations
- **File Operations**: `NC_CDF_open()` and `NC_CDF_close()` with proper resource management
- **Metadata Mapping**: CDF structures mapped to NetCDF equivalents

#### Type System
- **Type Mapping**: Complete mapping of CDF types to NetCDF types
  - CDF_INT4 → NC_INT
  - CDF_REAL8 → NC_DOUBLE
  - CDF_TIME_TT2000 → NC_INT64
- **Attribute Conventions**: FILLVAL attributes automatically renamed to _FillValue
- **Data Reading**: `NC_CDF_get_vara()` supporting scalars and arrays

#### Dependencies
- NASA CDF Library v3.9.x (required when enabled)

#### Build Integration
- CDF support defaults to **OFF**; enable it with `-DNEP_ENABLE_CDF=ON`.
- CDF uses **UDF slot 4** (`NC_UDF4`) as of v2.2.0; moved from UDF2 to eliminate mutual-exclusivity with GRIB2

## Spack Package Manager Support (v1.4.0)

### Overview

NEP v1.4.0 added Spack package manager support for simplified installation and dependency management in HPC environments.

### Key Features

- **NEP Spack Package**: Complete Spack package recipe with CMake build system
- **CDF Spack Package**: Separate package for NASA CDF library v3.9.1
- **Variant Support**: `+fortran/-fortran`, format-reader variants, `+docs/-docs`; filter variants belonged to pre-v4.2.0 releases. The recipe starts at NEP 4.2.0; the CDF and DICOM readers are not available through Spack (no `cdf` or `gdcm` package)
- **Dependency Management**: Automatic resolution of NetCDF-C, HDF5, optional reader libraries, and NetCDF-Fortran
- **CI Integration**: Dedicated GitHub Actions workflows for testing Spack packages; the NEP workflow runs upstream ruff/vermin checks plus `spack audit`, and a pinned `spack/ci` environment (Spack v1.2.2, spack-packages v2026.10.0) that builds the checked-out source with `spack install --test=root` on every push and pull request

### Installation

```bash
# Install NEP with all features
spack install nep

# Install with minimal features
spack install nep~docs~fortran

# Install CDF library
spack install cdf
```

## Parallel I/O Support (v1.9.0)

### Overview

NEP v1.9.0 adds parallel I/O support for high-performance computing environments using MPI and parallel NetCDF-4/HDF5 features. This enables applications to perform collective I/O operations across multiple processes for improved performance on large datasets.

### Architecture

Parallel I/O in NEP follows the standard NetCDF-4 parallel pattern:

```
[Application Layer - MPI Ranks 0..N]
       │
[NetCDF-4 Parallel API]
       │
[Parallel NetCDF-4/HDF5]
       │
[MPI I/O Layer]
       │
[Parallel File System]
```

### Build Configuration

**CMake:**
- `NEP_ENABLE_PARALLEL_TESTS=ON/OFF` - Enable parallel I/O test programs (default: OFF)
- `MPIEXEC_EXECUTABLE=PATH` - Specify path to mpiexec/mpirun

### Dependencies
- MPI implementation (OpenMPI or MPICH)
- NetCDF-C built with parallel support (`NC_HAS_PARALLEL4`)
- NetCDF-Fortran (for Fortran parallel tests)
- HDF5 with parallel support (for underlying parallel I/O)

### Example Programs

The parallel I/O example programs moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0. NEP's own MPI test programs (under `test/` and `ftest/`, built with `NEP_ENABLE_PARALLEL_TESTS`) demonstrate:
- **Collective I/O**: `nc_create_par()`, `nc_var_par_access(NC_COLLECTIVE)`
- **Data Decomposition**: 2D grid partitioning across MPI ranks
- **Verification**: Parallel read-back and data integrity checks
- **Error Handling**: Proper MPI and NetCDF error propagation

### Testing

Parallel tests run via `mpiexec -n 4` in CI:
- CMake build with OpenMPI
- ncdump verification of output files
- Automated data integrity validation

## Example Programs (v2.1.0+)

### Overview

NEP includes example programs exercising NEP features: the NEXTCDF-4 backend, UDF format readers (DICOM, PDB, Python visualizations), and disk-bound performance benchmarks. The generic NetCDF teaching examples (Classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and all Fortran versions) moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0, where they remain companion code for *The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition* (https://www.amazon.com/dp/B0H7Q1Z75L).

### Architecture

Example programs are organized by feature area:

```
examples/
├── nextcdf4/         # NEXTCDF-4 float16/bfloat16 examples and benchmarks in C
├── performance/      # Disk-bound NetCDF benchmarks (NEP_ENABLE_BENCHMARKS)
├── dicom/            # DICOM UDF reader example
├── pdb/              # PDB UDF reader example
└── viz/              # Python UDF visualization examples (NEP_ENABLE_VIZ_EXAMPLES)
```

### Example Categories

#### NEXTCDF-4 Examples (C)
Located in `examples/nextcdf4/` (built when `-DNEP_ENABLE_NEXTCDF4=ON`):
- **bfloat16.c / float16.c**: Store real GDAS temperature data as `NC_BFLOAT16` / `NC_FLOAT16`
- **make_bfloat16_sample.c / make_float16_sample.c**: Convert the GDAS sample to small-float files
- **bfloat16_perf.c, bfloat16_io_perf.c, float16_perf.c, float16_io_perf.c, conversion_perf.c**: Size and I/O benchmarks with CSV output

#### Performance Benchmark Examples (v1.10.0)
Located in `examples/performance/`:
- **cache_tuning.c**: File-level and variable-level chunk cache configuration
- **chunking.c**: Chunk shape impact on time-slab vs column-profile access
- **chunking_performance.c**: Chunking strategies across four layouts (moved from `examples/netcdf-4/` in v4.3.0)
- **deflate.c**: Deflate levels 0–9 × shuffle (20 combinations)
- **zstandard.c**: Zstandard levels × shuffle (24 combinations)
- **szip.c**: SZIP `pixels_per_block` values with NC_SZIP_NN
- **endianness.c**, **fill_values.c**: Byte-order and fill-mode cost
- All benchmarks use `examples/perf_disk.h` disk-bound timing (`fsync` on writes, page-cache drop before reads)
- LZ4/BZIP2 benchmarks are maintained by [nep-compress](https://github.com/captainkirk99/nep-compress).
- General benchmarks are run only with `NEP_ENABLE_BENCHMARKS=ON` (default OFF).

#### UDF Reader Examples (C)
- `examples/dicom/dicom_read.c`: Read a DICOM file through the DICOM UDF handler
- `examples/pdb/pdb_read.c`: Read a PDB file through the PDB UDF handler
- `examples/viz/`: Optional Python visualizations for enabled UDF formats

### Build System Integration

#### CMake Integration
Examples are integrated into the CMake build system with optional building:

```cmake
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
```

Each example subdirectory has its own `CMakeLists.txt` that:
- Compiles example programs
- Links against NetCDF-C
- Registers examples as CTest tests

### Test Integration

All examples run as automated tests to ensure:
- **Correctness**: Examples produce valid NetCDF files
- **API Usage**: Demonstrates proper API usage patterns
- **Regression Detection**: Catches breaking changes in NEP

#### Test Execution

**CMake:**
```bash
ctest --test-dir build
```

Each example:
1. Creates a NetCDF file demonstrating specific features
2. Reads the file back to verify correctness
3. Validates data integrity
4. Exits with success (0) or failure (non-zero) status

### Dependencies

- **C Examples**: NetCDF-C library, C99 compiler
- **Build System**: CMake 3.19+

HDF5 is found by `cmake/NEPFindHDF5.cmake` (any version 1.14.0 or newer, linked through FindHDF5's imported targets, with a SONAME check against `libnetcdf`). NEXTCDF-4 requires HDF5 2.1.1 or newer: configuring with `-DNEP_ENABLE_NEXTCDF4=ON` against an older HDF5 is a fatal configure error. NEXTCDF-4 also requires netcdf-c headers with the 64-slot UDF scheme (`NC_UDF` and `NC_MAX_UDF_FORMATS >= 64` — netcdf-c 4.10.2 or `main` until it ships); 10-slot headers are a fatal configure error because their UDF flag values differ and their `NC_reclaim_data` only covers UDF slots 0-1.

### Educational Value

Examples demonstrate:
- **Best Practices**: Proper error handling, resource cleanup, API usage patterns
- **Feature Coverage**: NEXTCDF-4 small-float types, UDF readers, and performance tuning
- **Real-world Patterns**: Common use cases in scientific computing

## Release Information

- **Version**: v2.7.1
- **Status**: Release Preparation
- **Release Date**: July 2026
- **Features**:
  - LZ4 and BZIP2 compression for HDF5/NetCDF-4 files (C and Fortran APIs)
  - GeoTIFF read support via UDF handler (UDF0/UDF1)
  - GRIB2 read support via UDF handler (UDF2)
  - FITS read support via UDF handler (UDF3)
  - NASA CDF read support via UDF handler (UDF4)
  - NASA/ESA PDS4 read support via UDF handler (UDF5), including `Array_3D_Image`, `Group_Field_Binary`, delimited tables, and real mission test data (MAVEN, Perseverance, New Horizons)
  - All five format readers can be enabled simultaneously
  - Spack package manager support with variants for every optional reader and utility
  - Disk-bound performance benchmark examples and NEXTCDF-4 example programs
  - Generic netCDF example programs maintained in the *NetCDF Developer's Handbook* repository (companion code for the Second Edition)

---

*Last Updated: July 2026 (v2.7.1 release preparation)*

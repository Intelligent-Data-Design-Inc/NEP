# NEXTCDF-4 Backend

NEXTCDF-4 is a clean-room rewrite of the NetCDF-4/HDF5 backend delivered as a NEP User Defined Format (UDF) expansion pack. A UDF is a plug-in file format registered with netcdf-c's dispatch mechanism; current netcdf-c `main` provides 64 UDF slots (UDF0–UDF63), and NEXTCDF-4 occupies slot 9 (`NC_UDF9`). The backend provides the full NetCDF-4 enhanced data model, variable I/O, storage features, and a set of new atomic types while coexisting with netcdf-c's built-in HDF5 backend. See [NEXTCDF-4 Architecture](nextcdf4_architecture.md) for how the dispatch table, per-object state, and hidden attributes fit together.

**Explicit selection required**: because HDF5 files already belong to netcdf-c's built-in NetCDF-4 backend, NEXTCDF-4 does not register the HDF5 magic number. Open and create NEXTCDF-4 files by passing `NC_NEXTCDF4`.

> **Experimental**: NEXTCDF-4 is experimental and under active development. Not all NetCDF-4 features are implemented; see the Known Limitations section below.

## Fortran binding

With NEXTCDF-4 and Fortran enabled, `use nep_nextcdf4` provides initialization,
mode/type constants and `nf90_{get,put}_var_{float16,bfloat16}` bit-buffer generics
(scalar–rank 7, optional start/count/stride). NetCDF-Fortran >=4.6.2 and compatible
compiler modules are required. See [NEXTCDF-4 Fortran](nextcdf4_fortran.md) for
one-based selection conventions, installed `NEP::nepnextcdf4f` linkage and a
complete example. Reference wrappers and broader enhanced-type value interfaces
are deferred beyond v4.3.0; exported constants alone do not provide those interfaces.

## Internal conversion engine

NEXTCDF-4 converts all supported numeric variable and attribute data with the private, NEP-owned `nx4_convert_type()` engine, selected as the permanent engine after Sprint 1H's OFF/ON comparison. It replaced the temporary `NEP_INTERNAL_CONVERT` CMake option; passing that option now fails at configure time. There is no runtime or file-format switch.

Standard float/double calls on bfloat16 use round-to-nearest-even, with single rounding from double and signed infinity on finite overflow. For example, 449 narrows to 448 instead of HDF5's 450. IEEE float16 conversion joined the engine in Sprint 1J (v4.3.0), with the same round-to-nearest-even and `NC_ERANGE` semantics; the former HDF5 `H5Tconvert` path was removed. HDF5 also handles file byte order and storage representation.

Matching-type raw-bit I/O, FP8/FP6/FP4 typed-call restrictions and the independent `nc_*_bfloat16`/`nc_*_float16` wrappers are unchanged. Float/double conversion for bfloat16 attributes is supported; fill-value limitations still apply. Variable/attribute conversions and quantized writes complete all values, then return `NC_ERANGE` on finite overflow, integer range violations or nonfinite-to-integer conversion (#381). A range warning does not roll back writes: every value is processed, attributes may be replaced and unlimited dimensions may grow. Ordinary precision loss, fractional truncation and floating underflow do not cause range errors; floating NaN/infinity inputs remain valid for floating destinations. Structural, permission and HDF5 failures take precedence over range warnings. Historical HDF5-engine benchmark artifacts are kept for comparison in `examples/nextcdf4/conversion_*.csv`; new measurements use the internal engine.

## Creating and Opening Files

```c
#include <netcdf.h>

int ncid;

/* Create a native NEXTCDF-4 file */
nc_create("example.nc", NC_NEXTCDF4 | NC_CLOBBER, &ncid);

/* Open an existing NEXTCDF-4 file */
nc_open("example.nc", NC_NEXTCDF4 | NC_NOWRITE, &ncid);
```

NEXTCDF-4 also supports `.ncrc` autoload via UDF slot 9 (`NETCDF.UDF9.LIBRARY` and `NETCDF.UDF9.INIT`), but files must still be selected with `NC_NEXTCDF4`; autoload only avoids calling `NC_NEXTCDF4_initialize()` explicitly.

## Enabling

**CMake:**
```bash
cmake -B build -DNEP_ENABLE_NEXTCDF4=ON
```

**Dependencies:**
- netcdf-c with the 64-slot UDF scheme (`NC_MAX_UDF_FORMATS >= 64`) and `NC_Dispatch` support (`NC_UDF9`) — netcdf-c 4.10.2 or newer once released; netcdf-c `main` until then. The older 10-slot headers are rejected at configure time.
- HDF5 2.1.1 or newer (required).

NEXTCDF-4 availability is advertised at compile time through `nep_meta.h`:

```c
#include <nep_meta.h>

#if NEP_HAS_NEXTCDF4
/* NEXTCDF-4 is compiled in. */
#endif
```

## Compatibility Modes

| Mode | Base flag | Compatibility flag | Superblock | Purpose |
|------|-----------|--------------------|------------|---------|
| Native | `NC_NEXTCDF4` | — | v3 | Full enhanced model and all new types |
| Classic model | `NC_NEXTCDF4` | `NC_CLASSIC_MODEL` | v3 | NetCDF-3 data model on HDF5 |
| NetCDF-4 model | `NC_NEXTCDF4` | `NC_NETCDF4_MODEL` | v1 | Files readable by upstream netcdf-c |

**Native mode** allows all NEXTCDF-4-specific types and HDF5 2.1.1 features.

**`NC_CLASSIC_MODEL`** restricts the file to the classic NetCDF-3 data model: root group only, one unlimited dimension, classic atomic types, no chunking, compression, filters, or user-defined types.

**`NC_NETCDF4_MODEL`** produces files that upstream netcdf-c linked against HDF5 1.10+ can open. It allows the standard enhanced NetCDF-4 data model (groups, user-defined types, chunking, compression) but forbids NEXTCDF-4-specific atomic types (`NC_FLOAT16`, small floats, complex, bitfield, reference) so the file remains fully compatible with upstream.

`NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` are mutually exclusive.

## Fill defaults, cache control and group renaming

`nc_set_fill(ncid, NC_FILL/NC_NOFILL, &old_mode)` changes a session default shared by all groups. Each session begins with `NC_FILL`; eligible variables capture the default when defined, not at `nc_enddef`. Existing variables are unchanged. Standard numeric/character and NEP fixed-size numeric/complex/bitfield variables inherit it; strings, user-defined types and references retain their existing fill rules. `nc_def_var_fill` remains the per-variable override before commitment. Read-only calls return `NC_EPERM`, and invalid modes return `NC_EINVAL`. Unwritten no-fill values are unspecified. Changing the file default neither rebuilds datasets nor persists a new file-wide attribute.

`nc_set_var_chunk_cache(ncid, varid, bytes, slots, preemption)` applies real HDF5 raw chunk-cache settings. Preemption must be finite and in [0,1]; zero bytes/slots are allowed. Calls work in define/data mode, read-only sessions and all compatibility modes. Pending variables retain requests until creation; live datasets reopen with the new access properties, closing/reopening coordinate-scale aliases together. Values, filters, attributes and dimension attachments are preserved. `nc_get_var_chunk_cache` reports effective live chunked-dataset properties (stored requests for pending/nonchunked variables) and accepts NULL outputs. Scalar/contiguous variables accept settings but have no raw chunk cache to tune. Overrides last only for the open session; definitions and opens start from `nc_set_chunk_cache` global defaults.

`nc_rename_grp(grpid, name)` works in native and `NC_NETCDF4_MODEL` modes, as does group creation. Rename moves the parent link and flushes it, preserving group/descendant IDs, data, types and dimension relationships; inquiries immediately use the new path. Enhanced-mode rename may automatically enter define mode; call `nc_enddef` before subsequent reads. Root rename returns `NC_EBADGRPID`, read-only rename `NC_EPERM`, and duplicate names (including the existing name) `NC_ENAMEINUSE`. Classic-model groups remain unsupported (`NC_ENOTNC4`). Object-reference identity is preserved; previously saved path strings are not rewritten.

## Unsupported file modes and parallel I/O

NEXTCDF-4 is a serial disk-file backend in v4.3.0. Its create/open entries reject `NC_DISKLESS`, `NC_INMEMORY`, `NC_MPIIO`/`NC_MPIPOSIX` (aliases), `NC_SHARE`, `NC_PERSIST` and `NC_MMAP` with `NC_EINVAL`, before creating, clobbering or opening a file. NetCDF-C may reject incompatible flag combinations before backend dispatch. Use NetCDF-C's memory APIs when testing memory requests: passing `NC_INMEMORY` directly to ordinary `nc_open` without an image parameter can assert inside upstream model inference before NEXTCDF-4 is reached.

All NEXTCDF-4 parallel I/O is deferred beyond v4.3.0; `nc_var_par_access` still returns `NC_ENOTNC4`. NEP's general MPI tests exercise upstream NetCDF, not NEXTCDF-4 parallel support. In-memory image files and SWMR are also unsupported.

## Diagnostics and Logging

NEXTCDF-4 uses the distinct `ELOG(())` macro, patterned after netcdf-c's `LOG(())` convention in `libsrc4` and `libhdf5`. NEP logging is independent of the underlying NetCDF-C logging build. It is compiled in by default and can be disabled with `-DNEP_ENABLE_LOGGING=OFF`. Build-time availability is reported in `nep_meta.h`:

```c
#include <nep_meta.h>

#if NEP_HAS_LOGGING
/* NEXTCDF-4 is instrumented for diagnostics. */
#endif
```

To control diagnostics:

- Set the `NEP_LOG_LEVEL` environment variable before opening a file. The NEXTCDF-4 initialization reads this variable and applies it with `nep_set_log_level()`.
- Call `nep_set_log_level(level)` directly, where `level` is `0` (errors only), `1` (major messages), `2` (lifecycle events), `3` (metadata operations), or higher for more detail. `nep_set_log_level(-1)` disables all output.

Messages are written to `stderr`. Instrumented NEXTCDF-4 paths include file creation and open, close/abort, define/end-define mode transitions, dimension and variable definition, and the metadata loader. HDF5 failures additionally dump the HDF5 error stack.

## Supported Atomic Types

| NetCDF Type | HDF5 Type | Compatibility Mode |
|-------------|-----------|--------------------|
| `NC_BYTE` | `H5T_STD_I8LE/BE` | All |
| `NC_UBYTE` | `H5T_STD_U8LE/BE` | All |
| `NC_CHAR` | `H5T_C_S1` | All |
| `NC_SHORT` | `H5T_STD_I16LE/BE` | All |
| `NC_USHORT` | `H5T_STD_U16LE/BE` | All |
| `NC_INT` | `H5T_STD_I32LE/BE` | All |
| `NC_UINT` | `H5T_STD_U32LE/BE` | All |
| `NC_INT64` | `H5T_STD_I64LE/BE` | All |
| `NC_UINT64` | `H5T_STD_U64LE/BE` | All |
| `NC_FLOAT` | `H5T_IEEE_F32LE/BE` | All |
| `NC_DOUBLE` | `H5T_IEEE_F64LE/BE` | All |
| `NC_FLOAT16` | `H5T_IEEE_F16LE/BE` | Native only |
| `NC_BFLOAT16` | `H5T_FLOAT_BFLOAT16LE/BE` | Native only |
| `NC_FLOAT8_E4M3` | `H5T_FLOAT_F8E4M3` | Native only |
| `NC_FLOAT8_E5M2` | `H5T_FLOAT_F8E5M2` | Native only |
| `NC_FLOAT6_E2M3` | `H5T_FLOAT_F6E2M3` | Native only |
| `NC_FLOAT6_E3M2` | `H5T_FLOAT_F6E3M2` | Native only |
| `NC_FLOAT4_E2M1` | `H5T_FLOAT_F4E2M1` | Native only |
| `NC_COMPLEX` | `H5T_COMPLEX` over `H5T_NATIVE_FLOAT` | Native only |
| `NC_DOUBLECOMPLEX` | `H5T_COMPLEX` over `H5T_NATIVE_DOUBLE` | Native only |
| `NC_BITFIELD8/16/32/64` | `H5T_STD_B*LE/BE` | Native only |
| `NC_STD_REF` | `H5T_STD_REF` | Native only |

Small floating-point and bitfield types are exchanged as raw bit patterns by the untyped calls; `NC_FLOAT16` and `NC_BFLOAT16` also accept typed `float`/`double` calls (see [NEXTCDF4_float16.md](NEXTCDF4_float16.md)). Complex numbers use the memory layout `{ float r; float i; }` or `{ double r; double i; }` but are stored as native HDF5 2.x `H5T_COMPLEX` datatypes, not as ordinary compounds — a user compound with `{r,i}` fields remains an ordinary user compound. `NC_STD_REF` variables store 64-byte `H5R_ref_t` reference tokens; the `nc_ref_*`/`nc_deref_*` functions create and resolve them.

## User-Defined Types

NEXTCDF-4 supports the enhanced-model user types — compounds (with atomic, enum, opaque, array, nested-compound and vlen fields), integer enums, fixed-size opaque types and variable-length types — as variables and attributes in both native and `NC_NETCDF4_MODEL` files, fully interchangeable with upstream NetCDF-4:

```c
struct sample { int id; float temp; unsigned char flag; };
nc_type cmpid, enumid, opid;

nc_def_compound(ncid, sizeof(struct sample), "sample", &cmpid);
nc_insert_compound(ncid, cmpid, "id",   offsetof(struct sample, id),   NC_INT);
nc_insert_compound(ncid, cmpid, "temp", offsetof(struct sample, temp), NC_FLOAT);
nc_insert_compound(ncid, cmpid, "flag", offsetof(struct sample, flag), NC_UBYTE);

nc_def_enum(ncid, NC_INT, "cloud_class", &enumid);
nc_insert_enum(ncid, enumid, "clear",  &((int){0}));
nc_insert_enum(ncid, enumid, "cumulus",&((int){1}));

nc_def_opaque(ncid, 8, "blob", &opid);

nc_def_var(ncid, "samples", cmpid, 1, &dimid, &varid);
nc_enddef(ncid);              /* types commit here */
nc_put_var(ncid, varid, data);          /* untyped access copies raw elements */
```

Semantics:

- Types are group-scoped and visible to descendant groups; committed datatypes are written at `_enddef` and member inserts are rejected afterwards (`NC_ENOTINDEFINE`).
- I/O is matching-type or untyped (`nc_put/get_var`, `var1`, `vara`, `vars`); typed calls that request numeric conversion of a user type return `NC_EBADTYPE`. Memory and file layouts are the type's exact sizes and field offsets — no conversion engine is involved.
- Attributes of user types work the same way (`nc_put_att`/`nc_get_att` with the user typeid or untyped), including zero-length attributes, and preserve their type across close/reopen.
- Classic-model files reject all user-type calls (`NC_ENOTNC4`).
- Compounds nest: fields may be other fixed-size compounds, enums, opaques, or fixed-size multidimensional arrays (`nc_insert_array_compound`) of any of those. Nested types are committed before their containers and each group tracks link creation order, so upstream netCDF reads the types back in dependency order.
- For compounds containing nested compound members, NEXTCDF-4 writes a lossless declared-layout record per element (upstream's own convention under HDF5 2.1.1 overflows dataset storage and loses the last record's tail). Foreign files written with upstream's serialized-compound layout are read through the packed native datatype, matching upstream's own read capability.
- Variable-length (`nc_def_vlen`) types work end to end: vlens of atomic and user-defined bases, nested vlens, and compounds (and array fields) containing vlens, for variables and attributes in native and `NC_NETCDF4_MODEL` modes. Vlen elements are `nc_vlen_t {size_t len; void *p}` descriptors. `nc_put_vlen_element` borrows the caller's payload pointer (the data must stay valid until `nc_put_var*` completes); `nc_get_vlen_element` copies `len * base_size` bytes into caller storage. Reads return deep copies owned by the caller — free them with `nc_reclaim_data` (recursive, frees nested payloads only) or `nc_reclaim_data_all` (also frees the top-level buffer), and always reclaim before `nc_close` while file/type context exists. The legacy `nc_free_vlen`/`nc_free_vlens`/`nc_free_string` helpers are flat-only: use `nc_reclaim_data*` for anything nested. `nc_copy_data`/`nc_copy_data_all` deep-copy dynamic values into caller-provided or fresh top-level storage.
- Strings (Sprint 1P): `NC_STRING` variables and attributes survive close/reopen in native and `NC_NETCDF4_MODEL` files. Strings may also be scalar or array compound members, vlen bases, and leaves of recursively nested compound/vlen graphs. `NC_STD_REF` remains excluded from user-defined type graphs; classic-model files reject strings and user types.

### Strings, fills and ownership

Each standalone string element is a `char *`; an array is a `char **`. Vlen-of-string payloads are arrays of pointers, not character arrays. Compound string fields use pointer-sized slots at the declared offsets. Write calls borrow application buffers for the duration of the call; attribute and fill caches deep-copy their contents. NULL standalone input elements are normalized to empty strings. String values are NUL-terminated: bytes after the first NUL are not part of the value. New files tag variable-length strings as UTF-8, matching upstream, without validating, normalizing or transcoding payload bytes. Legacy ASCII-tagged variable-length strings remain readable and writable; memory datatypes preserve stored character-set tags recursively.

Successful reads return independently owned payloads. Free standalone arrays with `nc_free_string(count, values)`; this releases the strings, not the pointer array. Use `nc_reclaim_data(ncid, xtype, values, count)` for nested values, then free an allocated outer buffer yourself, or use `nc_reclaim_data_all` for both. Reclaim nested values before closing the file because type metadata is needed. Vlen element helpers still borrow/copy descriptors and pointer values; those aliases are not additional owned deep copies.

Standalone string variables default to an allocated empty string on read. Custom `nc_def_var_fill`/`_FillValue` strings are owned internally, and fill inquiries return independent copies. `nc_def_var_fill(..., NC_NOFILL, ...)` returns `NC_EINVAL` for standalone strings. File-wide fill defaults do not change string/user-type fill rules. Dynamic user-type defaults remain zeroed descriptors (empty vlens/null embedded pointers). Dynamic filled datasets use early HDF5 allocation at creation/extension: this materializes fills while writable, avoiding HDF5 2.1.1's attempt to write a vlen heap when reading an unallocated chunk read-only. Large sparse dynamic datasets can therefore consume more storage when extended.

`NC_NETCDF4_MODEL` string output is readable by upstream netcdf-c; NEXTCDF-4 can read and update upstream-created string variables/attributes and nested types. Foreign anonymous compound reads retain the packed-native-layout limitation described above. One-byte fixed-length HDF5 strings remain `NC_CHAR`; wider fixed-length HDF5 strings are unsupported. This is selected string interoperability, not support for every HDF5 schema or the entire upstream test suite. The three upstream ports `tst_strings`, `tst_strings2` and `tst_atts_string_rewrite` run in both modes alongside focused regressions under the `nextcdf4_strings` CTest label and the existing normal/sanitizer CI jobs.

## Storage and Metadata

NEXTCDF-4 stores NetCDF dimensions as HDF5 dimension scales and attaches them to variable datasets. It writes the following hidden attributes for fast, reliable round-tripping:

| Attribute | Location | Purpose |
|-----------|----------|---------|
| `_Nextcdf4Backend` | root group | Identifies the file as created by NEXTCDF-4 |
| `_Nextcdf4Model` | root group | Present only in `NC_NETCDF4_MODEL` mode |
| `_Netcdf4Dimid` | dimension scale | Stores the stable dimension ID |
| `_Nextcdf4Varid` | variable dataset | Stores the stable variable ID |
| `_Netcdf4Coordinates` | variable | Stores the variable's dimension IDs |

These attributes are filtered from user-visible attribute lists.

NEXTCDF-4 also writes the upstream provenance attributes so `h5dump`, `ncdump -s` and third-party tools treat its files exactly like upstream NetCDF-4 files:

| Attribute | Storage | Content |
|-----------|---------|---------|
| `_NCProperties` | stored on root group | `version=2,netcdf=<netcdf-c version>,hdf5=<HDF5 version>,nextcdf4=<NEP version>` — upstream's shape plus a `nextcdf4` key |
| `_nc3_strict` | stored on root group | `1` (int), `NC_CLASSIC_MODEL` files only |
| `_IsNetcdf4` | virtual | `1` when the file carries netCDF-4 markers; synthesized on name lookup |
| `_SuperblockVersion` | virtual | the HDF5 FCPL superblock version; synthesized on name lookup |

Like upstream, `_NCProperties`/`_nc3_strict` are written at `_enddef`/`nc_sync`/`nc_close`, only for files NEXTCDF-4 created, and never overwrite an existing stamp — an updated file keeps its original creator's provenance, and a file that never had a stamp stays unstamped. `_IsNetcdf4` and `_SuperblockVersion` are never stored: `nc_inq_att`/`nc_get_att` answer them by name on the global attribute list (`NC_INT`, length 1), `nc_inq_attid` returns `NC_EATTMETA`, and none of the names appear in `nc_inq_natts`/`nc_inq_attname`. `_Format` is synthesized by tools from `nc_inq_format` (`NC_FORMAT_NETCDF4`), not stored. All of these names remain reserved — `nc_put_att`/`nc_rename_att`/`nc_del_att` reject them.

## Filters

NEXTCDF-4 supports the full NetCDF-4 filter model on non-scalar variables. Filters are stored in the shared `NC_VAR_INFO_T::filters` list — the same structure upstream netcdf-c uses — and are applied to the HDF5 dataset creation property list in pipeline order.

```c
/* Convenience functions for the standard filters */
nc_def_var_deflate(ncid, varid, 1 /* shuffle */, 1 /* deflate */, 4 /* level */);
nc_def_var_fletcher32(ncid, varid, NC_FLETCHER32);
nc_def_var_szip(ncid, varid, NC_SZIP_NN, 16);

/* Generic filter API for plugin filters such as bzip2 (307) or zstd (32015) */
unsigned int level = 3;
nc_def_var_filter(ncid, varid, H5Z_FILTER_ZSTD, 1, &level);

/* Inquiry */
size_t nfilters;
unsigned int ids[8], params[8];
nc_inq_var_filter_ids(ncid, varid, &nfilters, ids);
nc_inq_var_filter_info(ncid, varid, ids[0], &nparams, params);
nc_inq_filter_avail(ncid, H5Z_FILTER_BZIP2); /* NC_ENOFILTER if the plugin is absent */
```

Semantics match upstream netcdf-c:

- Defining a filter forces chunked storage; a filtered variable cannot be set contiguous.
- Scalar variables cannot have filters (`NC_EINVAL`).
- Filters cannot be removed once defined; re-defining a filter id replaces its parameters.
- Fletcher32 is always first in the pipeline, then shuffle, then remaining filters in definition order.
- Deflate and szip are mutually exclusive; a deflate level of 0 adds no filter.
- All dataset-property setters — filters, chunking, deflate, fletcher32, fill, endianness, quantization — return `NC_ELATEDEF` once the variable's dataset is committed (after `_enddef` commits define mode); calls outside define mode return `NC_ENOTINDEFINE`.
- `nc_inq_var_filter_ids` on a filterless variable returns `NC_NOERR` with a count of zero.

Plugin filters work through HDF5's plugin mechanism: set `HDF5_PLUGIN_PATH` (or install plugins in HDF5's default plugin directory) and the filter becomes available to `nc_def_var_filter` and to data I/O. When a file is opened whose variable uses a filter that is unavailable in the current process, the variable's metadata — including the filter pipeline — remains readable, but reads and writes return `NC_ENOFILTER`.

`NC_CLASSIC_MODEL` files permit no filters: every filter and storage setter (`nc_def_var_filter`, `nc_def_var_deflate`, `nc_def_var_fletcher32`, `nc_def_var_szip`, `nc_def_var_chunking`) returns `NC_EINVAL`.

Filters may be applied to coordinate variables. Setter-triggered dataset rebuilds are deferred to `_enddef`, which detaches and re-attaches dependent variables' dimension-scale references around the recreation, so dependents keep valid attachments in any definition order.

## Example: Create a File with a Variable

```c
#include <netcdf.h>

int ncid, dimid, varid;
size_t start[2] = {0, 0}, count[2] = {2, 3};
float data[2][3] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};

nc_create("example.nc", NC_NEXTCDF4 | NC_CLOBBER, &ncid);
nc_def_dim(ncid, "y", 2, &dimid);
nc_def_dim(ncid, "x", 3, &dimid);
nc_def_var(ncid, "data", NC_FLOAT, 2, (int[]){dimid - 1, dimid}, &varid);
nc_enddef(ncid);
nc_put_vara_float(ncid, varid, start, count, &data[0][0]);
nc_close(ncid);
```

## Example: bfloat16 Storage

bfloat16 (brain floating point) is a 16-bit floating-point format with 1 sign bit, 8 exponent bits and 7 mantissa bits. It has the same range as float32 — the exponent width is identical — but only about 2-3 significant decimal digits of precision, and takes half the storage. `NC_BFLOAT16` halves the file size for data where a relative error of up to 2^-8 (about 0.4%) is acceptable.

NEXTCDF-4 stores bfloat16 as raw bit patterns; conversion between float32 and bfloat16 can be done in user code or by the internal conversion engine through the typed calls. The rules for `NC_BFLOAT16` variables:

- Use `nc_put_vara_bfloat16`/`nc_get_vara_bfloat16` (or the untyped `nc_put_vara`/`nc_get_vara`) with a `uint16_t` buffer, or the typed float/double calls, which the internal engine converts with round-to-nearest-even (finite overflow becomes signed infinity, reported as `NC_ERANGE` after all values are processed). The `nc_*_bfloat16` functions also convert to and from `NC_FLOAT`/`NC_DOUBLE` variables with round-to-nearest-even (see [NEXTCDF4_float16.md](NEXTCDF4_float16.md)). The same applies to `NC_FLOAT16` and the `nc_*_float16` functions, with binary16's smaller range (maximum 65504).
- Fill values work: `nc_def_var_fill` and a `_FillValue` attribute both store the raw bfloat16 bits in the dataset's fill message, which survives close/reopen and fills unwritten elements; the `_FillValue` attribute is visible through `nc_inq_att` after reopen.

See **[NEXTCDF4_float16.md](NEXTCDF4_float16.md)** for the full float16/bfloat16 user guide, a walkthrough of the `examples/nextcdf4/bfloat16.c` example, and the benchmark results.

## Reference API

NEXTCDF-4 exposes HDF5 2.x unified references (`H5T_STD_REF` /
`H5R_ref_t`) as `NC_STD_REF`, with functions for creating and
resolving object, region, and attribute references — including
references into external files. These are only available in native
mode and are declared in `nep.h`. See **[refs.md](refs.md)** for the
full reference.

```c
#include <nep.h>

nc_std_ref_t ref;

/* Create an object reference to a variable, group, or user-defined type */
nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref);

/* Resolve a reference back to NetCDF ids */
int obj_type, grpid, id;
nc_deref_object(ncid, &ref, &obj_type, &grpid, &id);
nc_ref_free(&ref);
```

Region references identify a regular strided hyperslab selection
inside a variable; attribute references name an attribute on any
object kind:

```c
nc_std_ref_t region, att;
size_t start[2] = {0, 0}, count[2] = {1, 10};

nc_ref_region(ncid, varid, start, count, NULL, &region);
nc_ref_attr(ncid, NC_REF_OBJ_VAR, varid, "units", &att);
```

Tokens hold an open reference to their file — release them with
`nc_ref_free()` before closing the file.

## Known Limitations

- `NC_NEXTCDF4` must be passed explicitly; there is no magic-number dispatch for HDF5 files.
- `nc_put_varm` and `nc_get_varm` are not implemented in NEXTCDF-4 because the mapped I/O API is deprecated.
- `nc_deref_*` functions do not follow references into external files (`NC_ENOTNC4`); use `nc_ref_get_file_name`/`nc_ref_get_obj_name` to resolve them, or raw HDF5.
- Reference-typed variables cannot be nested inside compound types.
- Small floating-point types (`NC_FLOAT16`, `NC_BFLOAT16`, FP8/FP6/FP4) and `NC_STD_REF` may be coordinate variables in native mode; the dimension-scale dataset carries the variable's dtype, and ordering/comparison semantics of non-IEEE encodings are the caller's concern.
- Small floating-point types (`NC_FLOAT6_E2M3`, `NC_FLOAT4_E2M1`) have no default fill value because their value spaces are too small for a useful sentinel.
- Typed calls on FP8/FP6/FP4 variables, and integer/text/string typed calls on any small float, return `NC_EBADTYPE` (`NC_ECHAR` for text).
- Out-of-range conversion results can differ between engines even though both return `NC_ERANGE` after completing the operation; see the conversion experiment above ([issue #381](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/381)).
- Def-var setters (`nc_def_var_fill`, `nc_def_var_chunking`, `nc_def_var_deflate`, `nc_def_var_endian`, `nc_def_var_fletcher32`, `nc_def_var_filter`) defer their dataset rebuild to `_enddef`; each variable is rebuilt at most once and a rebuilt variable leaves one orphaned HDF5 object header (~200-400 bytes) of dead space in the file.

## Resources

- [NEXTCDF-4 Architecture](nextcdf4_architecture.md) — dispatch table, per-object state, hidden attributes, and the metadata lifecycle.
- `docs/plan/NEXTCDF4_plan.md` — full architecture, compatibility contract, and type mapping.
- `docs/roadmap.md` — detailed v4.0.0 implementation sprints and acceptance criteria.
- NetCDF-C UDF documentation: https://docs.unidata.ucar.edu/netcdf/NUG/user_defined_formats.html

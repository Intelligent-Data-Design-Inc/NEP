# NEXTCDF-4 Architecture {#nextcdf4_architecture}

This page explains how the NEXTCDF-4 backend works internally. It is
written for readers who know netCDF at the `nc_open()`/`nc_put_var()`
level but have not looked inside a format backend. The user-facing
guide is @ref md_docs_2nextcdf4.

## The UDF mechanism

netcdf-c lets third-party libraries register additional on-disk
formats through *User Defined Format* (UDF) dispatch tables. The
current netcdf-c `main` provides 64 UDF slots (UDF0–UDF63); each slot
holds one format identifier and one dispatch table. NEP registers
NEXTCDF-4 in slot 9: the public constant `NC_NEXTCDF4` is
`NEP_UDF_NEXTCDF4`, which is `NC_UDF9` (see `include/nep.h` and
`include/nextcdf4dispatch.h`).

Registration happens once per process:

- `NC_NEXTCDF4_initialize()` calls netcdf-c's `nc_def_user_format()`
  to install the `NEXTCDF4_dispatcher` table in slot 9, and reads the
  `NEP_LOG_LEVEL` environment variable.
- `NC_NEXTCDF4_finalize()` releases process-wide state.

NEXTCDF-4 does **not** claim the HDF5 magic bytes — those belong to
netcdf-c's built-in NetCDF-4 backend. Files must be created and
opened explicitly with `NC_NEXTCDF4` in the mode flags. A `.ncrc`
autoload entry (`NETCDF.UDF9.LIBRARY`/`NETCDF.UDF9.INIT`) can spare
the application the `NC_NEXTCDF4_initialize()` call, but the mode
flag is still required.

## The dispatch table

Every netCDF API call (`nc_open`, `nc_def_dim`, `nc_put_vara`, ...)
is routed through a table of function pointers, `NC_Dispatch`, one
per open file. `NEXTCDF4_dispatcher` in `src/nextcdf4/nxt4dispatch.c`
fills that table three ways:

- **NEXTCDF-4 implementations** — `NEXTCDF4_create`, `NEXTCDF4_open`,
  `NEXTCDF4_def_dim`, `NEXTCDF4_put_vara`, and friends. These are the
  functions documented in `src/nextcdf4/*.c`.
- **Shared netcdf-c helpers** — calls not specific to NEXTCDF-4
  delegate to the library's own `NC4_*` functions (`NC4_inq`,
  `NC4_inq_dimid`, `NC4_inq_att*`, `NC4_inq_varids`, ...), which work
  on the shared `NC_*_INFO_T` metadata structures.
- **Stubs** — unsupported operations such as mapped `varm` I/O return
  `NC_ENOTBUILT`; parallel variable access, fill, cache, group rename and
  filter availability use NEXTCDF-4 implementations.

The struct has no designated initializers, so table entries appear in
exact positional order.

## Per-object state

netcdf-c keeps one in-memory metadata tree per open file, built from
`NC_FILE_INFO_T`, `NC_GRP_INFO_T`, `NC_DIM_INFO_T`, `NC_VAR_INFO_T`,
`NC_ATT_INFO_T` and `NC_TYPE_INFO_T`. Each of these structs has a
`format_*_info` field where a backend hangs its private state.
NEXTCDF-4's private structs are declared in
`src/nextcdf4/nxt4internal.h`:

| Struct | Hangs off | Holds |
| ------ | --------- | ----- |
| `NEXTCDF4_FILE_INFO_T` | `NC_FILE_INFO_T` | `hdfid` (the HDF5 file), `rootid`, `path`, `mode`, `no_write`, `define_mode`, `netcdf4_model`, `backend_marked`, `created_by_us`, `ncproperties` |
| `NEXTCDF4_GRP_INFO_T` | `NC_GRP_INFO_T` | open `hid_t` for the HDF5 group |
| `NEXTCDF4_DIM_INFO_T` | `NC_DIM_INFO_T` | `hid_t` of the dimension-scale dataset |
| `NEXTCDF4_VAR_INFO_T` | `NC_VAR_INFO_T` | `hid_t` of the variable dataset plus `flags` (e.g. `NC_HDF5_VAR_FILTER_MISSING`); the filter pipeline itself lives on the shared `NC_VAR_INFO_T::filters` list |
| `NEXTCDF4_TYPE_INFO_T` | `NC_TYPE_INFO_T` | committed HDF5 datatype `hid_t` |

`NEXTCDF4_add_file`/`NEXTCDF4_get_file`/`NEXTCDF4_free_file` in
`nxt4file.c` manage the file-level record. The rule throughout:
whoever opens an HDF5 object closes it; `H5Tcopy` results are owned
by the caller.

## Hidden attributes and dimension scales

NEXTCDF-4 stores bookkeeping as ordinary HDF5 attributes that must be
hidden from `nc_inq_att*` results. The names are macros in
`nxt4internal.h`, and `is_reserved_att()` in `nxt4meta.c` filters
them out:

| Macro | Attribute | Written on |
| ----- | --------- | ---------- |
| `NEXTCDF4_BACKEND_ATT` | `_Nextcdf4Backend` = `"NEXTCDF-4/1.0"` | root group |
| `NEXTCDF4_MODEL_ATT` | `_Nextcdf4Model` | root group (`NC_NETCDF4_MODEL` only) |
| `NEXTCDF4_DIMID_ATT` | `_Netcdf4Dimid` | dimension scale |
| `NEXTCDF4_VARID_ATT` | `_Nextcdf4Varid` | variable dataset |
| `NEXTCDF4_VARDIMIDS_ATT` | `_Netcdf4Coordinates` | variable dataset |
| `NEXTCDF4_DIMCLASS` | `CLASS` = `"DIMENSION_SCALE"` | dimension scale |

Dimensions are stored as HDF5 **dimension scales** — datasets marked
with `CLASS = "DIMENSION_SCALE"` and attached to the variable datasets
that use them. `_Netcdf4Coordinates` records a variable's dimids so
open can map ids fast; dimension-scale matching is the fallback for
files written without it.

## Provenance attributes

`nxt4provenance.c` reproduces upstream's provenance surface so
third-party tools cannot tell a NEXTCDF-4 file from an upstream
NetCDF-4 file:

- `_NCProperties` is built at create
  (`version=2,netcdf=<libvers>,hdf5=<libvers>,nextcdf4=<NEP_VERSION>`)
  and stored on the root group at `_enddef`/`nc_sync`/`nc_close` —
  upstream's deferred-write point — as a fixed-length ASCII string of
  `strlen` bytes with `H5T_STR_NULLTERM` padding. `_nc3_strict` = 1 is
  stored alongside it for `NC_CLASSIC_MODEL` files. Neither is ever
  overwritten, and only `created_by_us` files are stamped: an updated
  file keeps its original stamp, and a stamp-less foreign file stays
  unstamped.
- `_IsNetcdf4` and `_SuperblockVersion` are never stored.
  `NEXTCDF4_inq_att`/`NEXTCDF4_inq_attid`/`NEXTCDF4_get_att`
  intercept those names (plus `_NCProperties`, answered from the
  in-memory `file->ncproperties`) on `NC_GLOBAL` lookups and call
  `nx4_get_att_special`, mirroring upstream's `nc4_get_att_special`:
  `NC_INT`/len 1, integer-memtype conversion, `NC_EATTMETA` for attnum
  inquiries, `NC_ENOTATT` when the file has no `_NCProperties`. The
  names never appear in `nc_inq_natts`/`nc_inq_attname` results —
  upstream's `NAMEONLYFLAG` semantics.
- `_IsNetcdf4`'s value follows upstream's `NC4_isnetcdf4`: 1 when
  `_nc3_strict` or `_NCProperties` exists at root (or the file was
  created this session), else an `H5Ovisit` walk that requires at
  least two objects carrying NetCDF-4 marker attributes.
  `_SuperblockVersion` reads the FCPL `super` field via
  `H5Pget_version` — the equivalent of upstream's exported
  `NC4_hdf5get_superblock`, reimplemented because upstream's walker
  depends on upstream-private struct layouts.

## Metadata lifecycle

A NEXTCDF-4 file is in **define mode** while dimensions, variables,
attributes and types are being defined, and in **data mode** while
data is read or written. `nc_enddef`/`nc_redef` switch between them
through `NEXTCDF4__enddef`/`NEXTCDF4_redef` in `nxt4file.c`.

- **Create** (`nxt4create.c`): the HDF5 file is opened immediately,
  but datasets and attributes are only materialized when `_enddef`
  runs — the metadata writer walks the `NC_*_INFO_T` tree and creates
  the HDF5 objects (`nxt4meta.c`).
- **Open** (`nxt4open.c`, `nxt4meta.c`): `NEXTCDF4_load_metadata`
  walks the HDF5 group/dataset/attribute tree and rebuilds the
  `NC_*_INFO_T` tree, reading the hidden attributes back into dimids
  and varids.
- **I/O** (`nxt4io.c`): `get/put_vara`, `get/put_vars` select a
  hyperslab on the variable's dataset and run `H5Dread`/`H5Dwrite`
  with a memory datatype.

## User-defined types

Type ids at or above `NC_FIRSTUSERTYPEID` (32) are user-defined types;
lower ids are atomic or NEP extended types. `nxt4type.c` provides the
context-aware plumbing that every consumer routes through:

- `nx4_find_type(grp, xtype)` resolves a type id by walking the group
  and its ancestors — types are group-scoped and visible to
  descendants, matching netCDF's visibility rules.
- `nx4_type_size`, `nx4_file_type` and `nx4_memory_type` resolve the
  element size, the committed HDF5 file datatype and a rebuilt
  native-layout memory datatype for any type, atomic or user-defined.
  For a committed type `nx4_file_type` returns the committed `hid_t`
  itself (refcount-bumped) rather than a `H5Tcopy`, so datasets and
  attributes share the named datatype's object identity; copies would
  produce anonymous stored types that break reload resolution.

Committed datatypes are written at `_enddef` (`NEXTCDF4_write_types`);
variables defined over a not-yet-committed type defer dataset creation
until then, and attributes over them queue similarly. At open, each
group's committed datatypes are loaded **before** variables and
attributes (`load_types`/`load_one_type` in `nxt4meta.c`), so a
dataset or attribute whose datatype is a shared committed type
resolves it back to the registered `NC_TYPE_INFO_T` by object token
(`nx4_hdf_type_match`, with an `H5Tequal` structural fallback for
anonymously stored types).

User-type I/O is matching-type or untyped only: `var_io` rejects a
typed request for a different memtype with `NC_EBADTYPE` and never
routes user types through `nx4_convert_type`. The memory datatype is
rebuilt field-by-field for compounds (native-endian members at the
recorded offsets), as `H5Tenum_create` over the integer base for
enums, and as a plain `H5T_OPAQUE` for opaque types, so HDF5 handles
byte-order conversion while the element layout the caller sees is the
type's exact `nc_inq_*` layout.

### Variable-length types and ownership

Vlen types (`NEXTCDF4_def_vlen`) wrap `H5Tvlen_create` over any atomic
or user-defined base, including strings and other vlens; the type graph rejects
`NC_STD_REF`, unknown bases and cycles, and commits
bases before dependents via `commit_type_tree`. `NC_TYPE_INFO_T::
varsized` is maintained recursively — a vlen is variable-sized, and a
compound is variable-sized when any field (including fields of array
fields) is — so I/O, attribute and fill paths can tell flat from
dynamic values. String leaves make a compound dynamic; `NC_CHAR` is not
pointer-bearing even though its synthetic metadata class is `NC_STRING`.
Sprint 1P's `nx4_copy_data` recursively initializes copies and unwinds
partial failures, including nested strings. `nx4_storage_memory_type`
preserves stored string character-set tags while retaining native memory
sizes, member offsets and array shapes. String fill caches hold an owned
pointer-sized element buffer, never a bare character buffer. Recognized
attribute-load failures propagate instead of silently dropping data.
Dynamic filled datasets allocate early so read-only missing-chunk reads
do not attempt vlen-heap writes in HDF5 2.1.1; this increases sparse-file
storage costs. Scalar datasets omit `_Netcdf4Coordinates` rather than
writing an empty attribute that upstream cannot parse. Clipped unlimited
reads select real values into their full logical memory positions and
supply each missing element independently, including strided selections.

Elements are `nc_vlen_t {size_t len; void *p}` descriptors:

- `NEXTCDF4_put_vlen_element` **borrows** the caller's payload: it
  stores `len` and `p` verbatim and never copies or frees `data`. The
  payload must stay valid until the subsequent `nc_put_var*` call
  completes; the caller still owns and frees it.
- `NEXTCDF4_get_vlen_element` copies `len *` resolved base-memory-size
  bytes into caller storage (upstream's hard-coded 4-byte size is a
  bug for doubles, compounds and most bases and is not reproduced).

Read results (`nc_get_var*`, `nc_get_att`) are **owned deep copies**:
HDF5's read buffer is handed to the caller and must be freed with
`nc_reclaim_data` (recursive over nested vlens and dynamic compound
fields; does not free the top-level buffer) or `nc_reclaim_data_all`
(frees it too). The legacy `nc_free_vlen`/`nc_free_vlens`/
`nc_free_string` helpers are flat-only. Reclaim must happen before
`nc_close` — `NC_reclaim_data` needs valid file and type context, and
on the 64-slot UDF scheme the controller resolves the UDF9 dispatch
correctly (10-slot headers could abort, which is why the build
requires the 64-slot scheme). On a failed variable read any
HDF5-allocated payloads are reclaimed recursively before the error is
returned.

Attribute and fill-value metadata that lives in memory is stored
**deep-copied** (`nc_copy_data` into caller-independent storage for
dynamic types), so a `nc_put_att` caller may free or reuse its payload
immediately and every `nc_get_att` returns a fresh independent copy.
netcdf-c's `nc4_rec_grp_del_att_data`/`var_free` paths reclaim this
owned storage at `nc_close`. `_FillValue` for variable-sized types
takes the same deep-copy path both when written as an attribute and
when mirrored into `var->fill_value`.

## Session controls and persistent group links

`NEXTCDF4_set_fill` updates shared `NC_FILE_INFO_T.fill_mode` without entering define mode or rewriting existing datasets. Definitions capture the default before eager creation; user-defined, string and reference fill rules are preserved. Opens start at `NC_FILL`. Per-variable fill overrides remain DCPL setters, distinct from this session default.

`nx4_var_dapl` maps netCDF bytes/slots/float-preemption to HDF5 slots/bytes/double-preemption. Dataset creation and deferred rebuild use it; loaded variable handles are reopened with global NetCDF defaults. `nx4_reopen_var_cache` finds owned variable/scale aliases by object token, allocates its path/property lists before mutation, flushes writable files, closes all aliases, then reopens with the requested DAPL. On failure it attempts to restore the old access properties; no datasets or dimension attachments are recreated. Getters query live chunked DAPLs; pending/nonchunked variables retain their requests. Existing full-suite normal and sanitizer CI covers the `nextcdf4_controls` focused tests.

`NEXTCDF4_rename_grp` validates normalized names and parent variable/group/type collisions, moves the HDF5 link, updates the parent child index and flushes. Open descendant IDs and object identities remain valid, and full-name inquiry derives new paths from the metadata tree. Native and NetCDF-4-model groups are supported; classic mode remains root-only. Unsupported memory, sharing and parallel flags are rejected by `NEXTCDF4_check_mode` before filesystem side effects; image files remain deferred. Native serial SWMR writers use `NC_SWMR_WRITE` or `nc_start_swmr_write`; NEP reader refresh remains Sprint 1V5. Optional MPI-IO sessions use the Sprint 1V1 file plumbing and Sprint 1V2 per-variable access described below.

## SWMR writer lifecycle

`NEXTCDF4_FILE_INFO_T.swmr_requested` records deferred activation and
`swmr_active` records successful `H5Fstart_swmr_write`. Flagged creates activate
only after enddef materializes the schema; flagged writable opens activate after
metadata load. Explicit `nc_start_swmr_write` preflights and commits definitions,
then uses the same activation helper. `nx4_swmr_validate` checks native serial
state, actual superblock v3, SWMR VFD capability and the complete schema tree:
no declared user types or string/reference/user-defined variables. Open attributes
and committed datatypes cannot survive HDF5 activation; the restricted schema
avoids adding a committed-type handle lifecycle. An inactive requested session
closes as an ordinary file, never activating solely to close.

Active sessions cannot re-enter define mode or mutate schema/attributes. The
common write-define guard and direct type/group/reference mutator guards return
`NC_EPERM`; committed dataset setters return `NC_ELATEDEF`. Variable data I/O and
cache-alias reopen remain available without creating objects. Provenance is
materialized before activation and is not rewritten by active sync/close.
`nx4_swmr_flush` walks variable datasets, then dimension scales, then calls
`H5Fflush`; alias handles stay owned and may safely be flushed twice. Public
`nc_swmr_flush`, active `nc_sync` and close share this path. No writer refresh,
record transaction or physical durability guarantee is introduced. See
[NEXTCDF-4](nextcdf4.md) for the public writer contract and deferred reader support.

## Parallel variable transfers

`NEXTCDF4_var_par_access` records an independent/collective preference in `NEXTCDF4_VAR_INFO_T.parallel_access`, not in file metadata. It is initialized independent at definition/open and preserved by dataset rebuilds and cache-handle reopens. Filter setters force collective in parallel sessions; independent filtered access is rejected. Reopened filtered variables require explicit collective selection.

`var_io` owns a per-call DXPL configured by `H5Pset_dxpl_mpio`, closes it on every exit, and keeps serial transfers unchanged. Empty or fill-only collective ranks use none-selected memory/file spaces rather than skipping HDF5. Collective unlimited writes reduce proposed extents with `MPI_MAX` over the file's owned communicator, extend the variable and dimension scales consistently and preserve longer shared dimensions. Independent extension returns `NC_ECANTEXTEND` before mutation.

Fixed-size standard/user types and NEP small floats use the existing native-memory conversion engine; parallel dynamic/string/reference variable transfers return `NC_ENOTBUILT` before automatic enddef or mutation. Completed conversion range warnings stay rank-local and do not skip a collective call. The API requires valid arguments and matching collective order rather than providing a general rank-local failure-consensus layer. HDF5 may fall back internally from requested collective I/O; no physical MPI algorithm is guaranteed. Four-rank Open MPI tests cover the contract under `nextcdf4_parallel`; no new build option or packaging requirement is introduced.

## Filters

Variable filter pipelines are stored on `NC_VAR_INFO_T::filters` — an
`NClist` of `struct NC_HDF5_Filter` — managed with netcdf-c's
exported `NC4_hdf5_*` helpers (`NC4_hdf5_addfilter`,
`NC4_hdf5_filter_lookup`, `NC4_hdf5_filter_freelist`), exactly as the
built-in HDF5 backend does in `libhdf5/hdf5filter.c`. All filter code
lives in `nxt4filter.c`:

- **Define**: `NEXTCDF4_def_var_filter`, `NEXTCDF4_def_var_deflate`
  and `NEXTCDF4_def_var_fletcher32` append to the list (upstream's
  `nc_def_var_szip`, `nc_def_var_bzip2`, `nc_def_var_zstandard` and
  the quantize path all funnel through the generic entry) and mark
  the variable dirty. Because HDF5 creation properties are immutable,
  `_enddef` then recreates each dirty dataset once — detaching
  dimension scales and dependents before `H5Ldelete` and re-attaching
  them after — with the collected pipeline.
- **Apply**: `nx4_apply_filters` translates the list into DCPL calls:
  `H5Pset_fletcher32`/`H5Pset_shuffle`/`H5Pset_deflate`/`H5Pset_szip`
  for the built-ins, `H5Pset_filter(..., H5Z_FLAG_OPTIONAL, ...)` for
  everything else.
- **Open**: `nx4_load_filters` reads every DCPL filter — including
  unknown plugin filters — into `var->filters` with its parameters.
  A filter `H5Zfilter_avail` cannot resolve sets
  `NC_HDF5_FILTER_MISSING` on the spec and
  `NC_HDF5_VAR_FILTER_MISSING` on `NEXTCDF4_VAR_INFO_T::flags`, so
  metadata stays readable while `var_io` rejects reads and writes
  with `NC_ENOFILTER`.
- **Inquire**: `NEXTCDF4_inq_var_filter_ids`,
  `NEXTCDF4_inq_var_filter_info` and `NEXTCDF4_inq_filter_avail`
  (the last replacing `NC_NOOP_inq_filter_avail`, which reported every
  filter present) sit in the dispatch table behind
  `nc_inq_var_filter_ids`, `nc_inq_var_filter_info`,
  `nc_inq_var_filter` and `nc_inq_filter_avail`.
- **Close**: `close_group_hdf` in `nxt4file.c` frees the list with
  `NC4_hdf5_filter_freelist` (netcdf-c's `var_free` does not touch
  `format`-owned state) and frees `NEXTCDF4_VAR_INFO_T` itself.

`NC_CLASSIC_MODEL` files reject every filter and storage setter with
`NC_EINVAL`; scalars reject filters; and committed variables reject
every dataset-property setter — filters, chunking, fill, endianness,
quantization — with `NC_ELATEDEF`, matching upstream's
`nc_def_var_extra`.

## Compatibility modes

| Mode flag | Superblock | Data model |
| --------- | ---------- | ---------- |
| (none) | v3 | full NEXTCDF-4, all extended types |
| `NC_CLASSIC_MODEL` | v3 | NetCDF-3 model only: root group, one unlimited dim, classic atomic types |
| `NC_NETCDF4_MODEL` | v1 | enhanced model, no NEXTCDF-4-only types; file readable by upstream netcdf-c on HDF5 1.10+ |

The flags are mutually exclusive. `NEXTCDF4_check_atomic_type()`
enforces the per-mode type restrictions at `nc_def_var` time.

## Errors and logging

Functions use a `ret` variable plus a `fail:` cleanup label
(`nxt4err.h`):

- `EBAIL(e)` — log, set `ret = e`, `goto fail`. For non-HDF5 errors.
- `EBAIL2(e)` — same, plus `H5Eprint2()` to dump the HDF5 error
  stack. Use after a failed HDF5 call.

`ELOG((level, fmt, ...))` writes to stderr at levels 0 (errors) to 3
(metadata ops), controlled by `NEP_LOG_LEVEL` or
`nep_set_log_level()`; `-DNEP_ENABLE_LOGGING=OFF` compiles it out.

## The conversion engine

`nxt4convert.c` contains NEP's own numeric conversion engine
`nx4_convert_type()`, used for variable, attribute and
quantized-write conversions. It reports overflow and integer
range errors as `NC_ERANGE` *after* completing the I/O — the data
still lands — and treats precision loss and fractional truncation as
non-errors. IEEE float16 joined the engine in Sprint 1J (v4.3.0);
the only remaining exclusion is the raw-bit small-float types
(FP8/FP6/FP4) which are untyped-only. The `nc_*_bfloat16` and
`nc_*_float16` families perform NEP round-to-nearest-even conversion
between `NC_BFLOAT16`/`NC_FLOAT16` storage and `NC_FLOAT`/`NC_DOUBLE`
buffers.

@author Edward Hartnett
@date 2026-11-20
@copyright Intelligent Data Design, Inc. All rights reserved.

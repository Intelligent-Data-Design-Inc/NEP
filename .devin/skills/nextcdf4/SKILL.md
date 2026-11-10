---
name: nextcdf4
description: Working on the NEXTCDF-4 backend in src/nextcdf4/ — the clean-room NetCDF-4/HDF5 rewrite delivered as a NEP UDF slot-9 expansion pack. Covers dispatch table wiring, per-file/per-group/per-var state, hidden marker attributes, compatibility modes, new atomic types, and the ELOG/EBAIL diagnostics conventions.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-11-10"
---

# NEXTCDF-4 Skill

This skill covers the NEXTCDF-4 backend: a clean-room rewrite of netcdf-c's `libhdf5/` backend, implemented in `src/nextcdf4/` and delivered as a User Defined Format (UDF) expansion pack occupying netcdf-c UDF slot 9 (`NC_UDF9`). Use it when reading, modifying, or debugging any code under `src/nextcdf4/`, or when reasoning about HDF5 storage behavior of NEXTCDF-4 files.

> NetCDF-C previously provided 10 UDF slots (UDF0–UDF9). As of netcdf-c `main` (in the next release after 4.10.x) this was expanded to **64 UDF slots (UDF0–UDF63)**. NEP's slot assignments are unchanged — NEXTCDF-4 remains on UDF9 — but slots 10–63 are now available for new formats.

**Status: experimental and under active development.** NEXTCDF-4 is only partially implemented — not all NetCDF-4 features work yet (see Known Limitations). Its headline new features beyond upstream netcdf-c are:

- **HDF5 reference types** — `NC_REF_OBJECT` and `NC_REF_REGION` variables storing object/dataset-region references as opaque tokens, plus `nc_ref_object`/`nc_deref_object`/`nc_ref_region` APIs.
- **New floating-point types** — `NC_FLOAT16`, `NC_BFLOAT16`, and the HDF5 2.x small floats (`NC_FLOAT8_E4M3/E5M2`, `NC_FLOAT6_E2M3/E3M2`, `NC_FLOAT4_E2M1`).

Related skills: `hdf5` (HDF5 feature details — superblock v3, small floats, references, bitfields), `netcdf-architecture` (dispatch tables and format plumbing), `netcdf` (filters, chunking, quantization).

## Architecture

### Source Layout

| File | Role |
|------|------|
| `src/nextcdf4/nxt4dispatch.c` | Static `NC_Dispatch` table; `NC_NEXTCDF4_initialize()` registers it via `nc_def_user_format(NEP_UDF_NEXTCDF4, ...)`. |
| `src/nextcdf4/nxt4create.c` | `NEXTCDF4_create` — file creation, mode validation, superblock/libver setup. |
| `src/nextcdf4/nxt4open.c` | `NEXTCDF4_open` — opens the HDF5 file, reads markers, triggers metadata load. |
| `src/nextcdf4/nxt4file.c` | Per-file state (`NEXTCDF4_add_file`/`get_file`/`free_file`), close/abort/sync, `_enddef`/`redef`, marker attributes. |
| `src/nextcdf4/nxt4meta.c` | Metadata loader and writer — the largest file (~3.3k lines). Walks HDF5 groups/datasets/attributes into `NC_*_INFO_T` structures; materializes pending metadata on `_enddef`. |
| `src/nextcdf4/nxt4group.c` | Group operations: `def_grp`, `inq_ncid`, `inq_grps`, group name/parent inquiry. |
| `src/nextcdf4/nxt4type.c` | User-defined types: compound, vlen, enum, opaque; atomic type mapping (`NEXTCDF4_map_hdf_type`). |
| `src/nextcdf4/nxt4io.c` | `get/put_vara`, `get/put_vars`, `get/put_varm` hyperslab I/O. |
| `src/nextcdf4/nxt4internal.h` | Private state structs, hidden-attribute name macros, and all internal prototypes. |
| `src/nextcdf4/nxt4err.h` | `EBAIL`/`EBAIL2` error macros (see below). |
| `include/nextcdf4dispatch.h` | Public dispatch header: `NC_NEXTCDF4_initialize`, `NC_NEXTCDF4_finalize`. |
| `include/nep.h` | Defines `NC_NEXTCDF4` (= `NEP_UDF_NEXTCDF4` = `NC_UDF9`) and reference APIs. |

### Dispatch Wiring

`NEXTCDF4_dispatcher` implements the full NetCDF dispatch table. Functions not yet specific to NEXTCDF-4 are delegated to netcdf-c's shared `NC4_*` helpers (`NC4_inq`, `NC4_inq_dimid`, `NC4_inq_att*`, `NC4_inq_varids`, etc.) and `NC_NOTNC4_*`/`NC_NOOP_*` stubs for unsupported operations (`var_par_access`, `rename_grp`, chunk cache, `inq_filter_avail`). `NC_RO_set_fill` is used for `set_fill`.

When adding a dispatch entry, follow the table's positional order exactly — the struct has no designated initializers.

### Per-Object State

NEXTCDF-4 hangs format-private state off the netcdf-c `NC_*_INFO_T` structures (see `nxt4internal.h`):

- `NEXTCDF4_FILE_INFO_T` — `hdfid`, `rootid`, `path`, `mode`, `no_write`, `define_mode`, `netcdf4_model`, `backend_marked`. Allocated by `NEXTCDF4_add_file`, resolved by `NEXTCDF4_get_file`.
- `NEXTCDF4_GRP_INFO_T` — open `hid_t` for the HDF5 group.
- `NEXTCDF4_DIM_INFO_T` — `hid_t` of the dimension-scale dataset.
- `NEXTCDF4_VAR_INFO_T` — `hid_t` of the variable dataset plus cached `deflate_level`, `shuffle`, `fletcher32`.
- `NEXTCDF4_TYPE_INFO_T` — committed HDF5 datatype `hid_t`.

### Hidden Attributes

Defined in `nxt4internal.h`; filtered from user-visible attribute lists:

| Macro | Attribute | Location |
|-------|-----------|----------|
| `NEXTCDF4_BACKEND_ATT` | `_Nextcdf4Backend` = `"NEXTCDF-4/1.0"` | root group |
| `NEXTCDF4_MODEL_ATT` | `_Nextcdf4Model` | root group (`NC_NETCDF4_MODEL` mode only) |
| `NEXTCDF4_DIMID_ATT` | `_Netcdf4Dimid` | dimension scale |
| `NEXTCDF4_VARID_ATT` | `_Nextcdf4Varid` | variable |
| `NEXTCDF4_VARDIMIDS_ATT` | `_Netcdf4Coordinates` | variable |
| `NEXTCDF4_DIMCLASS` | `CLASS` = `"DIMENSION_SCALE"` | dimension scale |

Dimensions are stored as HDF5 dimension scales; `_Netcdf4Coordinates` provides fast dimid mapping at open, with dimension-scale matching as fallback for files lacking it.

## Backend Selection and Modes

- **Explicit selection only**: NEXTCDF-4 does not register the HDF5 magic number (it belongs to netcdf-c's built-in backend). Files must be created/opened with `nc_create`/`nc_open` passing `NC_NEXTCDF4`.
- `.ncrc` autoload is supported via `NETCDF.UDF9.LIBRARY`/`NETCDF.UDF9.INIT`, but only avoids calling `NC_NEXTCDF4_initialize()` — `NC_NEXTCDF4` is still required.
- Requires HDF5 2.1.1+ at build time (`-DNEP_ENABLE_NEXTCDF4=ON`); availability advertised via `NEP_HAS_NEXTCDF4` in `nep_meta.h`.

### Compatibility Modes

| Mode | Flag | Superblock |
|------|------|------------|
| Native | `NC_NEXTCDF4` | v3 |
| Classic model | `NC_NEXTCDF4 \| NC_CLASSIC_MODEL` | v3 |
| NetCDF-4 model | `NC_NEXTCDF4 \| NC_NETCDF4_MODEL` | v1 |

- `NC_CLASSIC_MODEL`: NetCDF-3 data model only — root group, one unlimited dim, classic atomic types, no chunking/compression/filters/user types.
- `NC_NETCDF4_MODEL`: full enhanced model but **no** NEXTCDF-4-specific types; produces files readable by upstream netcdf-c linked against HDF5 1.10+.
- The two flags are mutually exclusive. Enforce type restrictions via `NEXTCDF4_check_atomic_type`.

### Atomic Types

Standard `NC_BYTE`…`NC_UINT64`, `NC_FLOAT`, `NC_DOUBLE`, `NC_CHAR`, `NC_STRING` plus native-only types: `NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_E4M3/E5M2`, `NC_FLOAT6_E2M3/E3M2`, `NC_FLOAT4_E2M1`, `NC_COMPLEX`, `NC_DOUBLECOMPLEX`, `NC_BITFIELD8/16/32/64`, `NC_REF_OBJECT`, `NC_REF_REGION`. See the `hdf5` skill for the full `nc_type` → HDF5 type mapping table.

Small floats and bitfields move as raw bit patterns through untyped calls; `NC_FLOAT16`/`NC_BFLOAT16` additionally support typed float/double calls via HDF5 conversion (see `docs/NEXTCDF4_float16.md`). Typed calls on FP8/FP6/FP4 vars, and text/integer calls on small floats, return `NC_EBADTYPE`.

Reference APIs (`nc_ref_object`, `nc_deref_object`, `nc_ref_region`) are declared in `nep.h`; references round-trip as opaque tokens.

## Coding Conventions

### Error Handling — `nxt4err.h`

Functions use the local `ret` + `fail:` cleanup-label style (not upstream's BAIL directly):

- `EBAIL(e)` — logs the error, sets `ret = e`, `goto fail`. For non-HDF5 errors.
- `EBAIL2(e)` — same, plus `H5Eprint2(H5E_DEFAULT, stderr)` to dump the HDF5 error stack. Use after a failed HDF5 call. With logging disabled it degrades to `EBAIL`.

Every function using these macros must declare `int ret = NC_NOERR;` (or appropriate) and end with a `fail:` label before `return ret;`.

### Logging — `ELOG(())`

NEXTCDF-4 uses `ELOG((level, fmt, ...))` (distinct from netcdf-c's `LOG`), defined via `nep_logging.h`. Levels: `0` errors only, `1` major messages, `2` lifecycle, `3` metadata operations. Controlled by `NEP_LOG_LEVEL` env var (read in `NC_NEXTCDF4_initialize`) or `nep_set_log_level()`; `-1` disables. Output goes to `stderr`. Compiled in by default; `-DNEP_ENABLE_LOGGING=OFF` removes it (`NEP_HAS_LOGGING` in `nep_meta.h`).

Typical usage patterns in the code:

```c
ELOG((2, "%s", __func__));                       /* lifecycle */
ELOG((3, "%s: name=%s ncid=%d", __func__, name, ncid));  /* metadata ops */
```

### Comment/File Header Style

Doxygen file headers with `@file`, `@brief`, `@author Edward Hartnett`, `@date`, `@copyright Intelligent Data Design, Inc.` Match existing files when editing.

## Building and Testing

- CMake: `cmake -B build -DNEP_ENABLE_NEXTCDF4=ON` (use the `build/` directory per AGENTS.md; delete an existing `build` first).
- Requires netcdf-c ≥ 4.10.1 with UDF plugin/`NC_Dispatch` support and HDF5 ≥ 2.1.1. netcdf-c `main` (next release) expands UDF slots to 64 (UDF0–UDF63); older releases only have UDF0–UDF9.
- Examples live in `examples/nextcdf4/` (bfloat16 I/O and performance benchmarks).
- Feature test macros in `nep_meta.h`: `NEP_HAS_NEXTCDF4`, `NEP_HAS_FLOAT16`, `NEP_HAS_BITFIELD`, `NEP_HAS_REF_TYPES`, `NEP_HAS_COMPLEX`, `NEP_HAS_BFLOAT16`, `NEP_HAS_FLOAT8/6/4`, `NEP_HAS_NETCDF4_MODEL`, `NEP_HAS_SUPERBLOCK_V3`.

## Known Limitations

- `nc_put_varm`/`nc_get_varm` not implemented (mapped I/O deprecated).
- Reference-typed vars cannot be coordinate variables or nested in compounds; no deref API beyond `nc_deref_object`/`nc_deref_region`.
- `NC_FLOAT6_E2M3`/`NC_FLOAT4_E2M1` have no default fill value.
- `NC_BFLOAT16` rejects `_FillValue`; `nc_def_var_fill` value not recovered on reopen (issue #377).
- NEXTCDF-4 does not return `NC_ERANGE` for out-of-range conversions (issue #381).
- Def-var setters (`def_var_fill`, `chunking`, `deflate`, `endian`, `fletcher32`) recreate the HDF5 dataset, leaving an orphaned dataset object (issue #382).

## Documentation

- `docs/nextcdf4.md` — user-facing backend doc (modes, types, reference API, limitations).
- `docs/NEXTCDF4_float16.md` — float16/bfloat16 user guide and benchmarks.
- `docs/plan/NEXTCDF4_plan.md` — full architecture, UDF slot assignment, compatibility contract.
- `docs/roadmap.md` — implementation sprints and acceptance criteria.

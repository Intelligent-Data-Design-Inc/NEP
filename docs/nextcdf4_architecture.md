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
- **Stubs** — unsupported operations get `NC_NOTNC4_*`,
  `NC_NOOP_*`, or `NC_RO_set_fill`.

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
| `NEXTCDF4_FILE_INFO_T` | `NC_FILE_INFO_T` | `hdfid` (the HDF5 file), `rootid`, `path`, `mode`, `no_write`, `define_mode`, `netcdf4_model`, `backend_marked` |
| `NEXTCDF4_GRP_INFO_T` | `NC_GRP_INFO_T` | open `hid_t` for the HDF5 group |
| `NEXTCDF4_DIM_INFO_T` | `NC_DIM_INFO_T` | `hid_t` of the dimension-scale dataset |
| `NEXTCDF4_VAR_INFO_T` | `NC_VAR_INFO_T` | `hid_t` of the variable dataset plus cached `deflate_level`, `shuffle`, `fletcher32` |
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

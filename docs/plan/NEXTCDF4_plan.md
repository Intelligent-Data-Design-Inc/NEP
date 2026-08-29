# NEXTCDF-4 Plan

## Overview

NEXTCDF-4 is a clean rewrite from scratch of netcdf-c's `libhdf5` HDF5 backend, implemented in a new `src/nextcdf4/` directory. No code will be reused from the existing `libhdf5/` implementation, although it may be consulted as a reference. The rewrite takes advantage of new features in HDF5 1.14.x and HDF5 2.1.1+, such as Superblock v3, float16, and the new small floating-point types. It adds support for HDF5 reference types, and also fixes some long-standing bugs relating to renaming vars and dims.

The goals are:

- Correct handling of HDF5 features that the current implementation either avoids or maps poorly (references, float16, compound types, superblock v3).
- Maintain full read compatibility with existing NetCDF-4/HDF5 files.
- Produce files that netcdf-c can still open when the `NC_NETCDF4_MODEL` compatibility flag is used.

## Goals and Non-Goals

### Goals

- Write a new version of the HDF5 backend in a `src/nextcdf4/` directory with a modern, layered design aligned with the `NC_FILE_INFO_T` metadata model.
- Default to HDF5 Superblock v3 for all newly created NEXTCDF-4 files; use Superblock v1 for files created with the `NC_NETCDF4_MODEL` compatibility flag.
- Add native support for 16-bit floating point (`H5T_IEEE_F16LE` / `H5T_IEEE_F16BE`).
- Add read and write support for HDF5 reference types (object references and region references).
- Add native support for complex number types via HDF5 compound types.
- Fix the long-standing renaming bug for dimensions and variables.
- Expose all new C APIs in the Fortran API in the same release.

### Non-Goals

- Changing the public NetCDF-C API for classic NetCDF-3 files.
- Removing support for classic-model NetCDF-4 files.
- Modifying the NcZarr, DAP, or NetCDF-3 backends.
- Reusing code from the existing netcdf-c `libhdf5/` implementation. It may be consulted as a reference, but `src/nextcdf4/` will be written from scratch.

## NEXTCDF-4 as a NEP UDF Expansion Pack

NEXTCDF-4 is delivered as a NetCDF-C **User-Defined Format (UDF)** expansion pack. It does not modify any netcdf-c source files, including `libdispatch/dfile.c`. Activation and backend selection happen entirely through the existing UDF mechanism and mode flags.

### UDF Slot Assignment

NEXTCDF-4 occupies the remaining NEP UDF slot:

```c
/* include/nep.h */
#define NEP_UDF_NEXTCDF4 NC_UDF9   /* UDF slot 9 (NC_UDF9 = 0x00480040) */
#define NC_NEXTCDF4      NEP_UDF_NEXTCDF4
```

`NC_UDF9` is the only unassigned NEP slot and is the permanent slot for NEXTCDF-4.

NEXTCDF-4 autoloads via `.ncrc` like the other NEP handlers, using the `NETCDF.UDF9.LIBRARY` and `NETCDF.UDF9.INIT` keys. This relies on the expanded UDF support in netcdf-c main (64 slots, `dudfplugins.c` autoloading), which is a prerequisite for the NEXTCDF-4 work.

### Create-Time Backend Selection

To create a file with NEXTCDF-4, pass `NC_NEXTCDF4`:

```c
nc_create(path, NC_NEXTCDF4 | NC_CLOBBER, &ncid);
```

`nc_create` routes to the NEXTCDF-4 dispatch table because `NC_NEXTCDF4` is the `NC_UDF9` mode flag. Passing `NC_NETCDF4 | NC_NEXTCDF4` is also accepted but the `NC_NETCDF4` part is redundant. If `NC_NEXTCDF4` is not set, `nc_create` uses the built-in netcdf-c `libhdf5` backend, so the two backends coexist.

### Open-Time Backend Selection

Because HDF5 files share a single fixed magic number already claimed by the built-in NetCDF-4/HDF5 backend, automatic magic-based dispatch cannot route a file to NEXTCDF-4 without modifying netcdf-c. Therefore, opening a file with the NEXTCDF-4 backend requires passing `NC_NEXTCDF4`:

```c
nc_open(path, NC_NEXTCDF4, &ncid);
```

Files created without `NC_NEXTCDF4` continue to open with the legacy backend when `nc_open` is called without `NC_NEXTCDF4`.

### Compatibility Mode Flag

The `NC_NETCDF4_MODEL` create flag is also defined in `nep.h` because netcdf-c itself does not yet know about it:

```c
/* include/nep.h — bit 26, chosen to avoid all netcdf-c mode flags */
#define NC_NETCDF4_MODEL 0x04000000
```

`NC_NETCDF4_MODEL` is only meaningful when `NC_NEXTCDF4` is also set. It is passed through `nc_create`'s mode bits to the NEXTCDF-4 dispatch create function, which enforces the compatibility restrictions described below.

### Stored Markers

NEXTCDF-4 writes hidden root-group attributes so later opens can verify provenance and compatibility mode:

| Attribute | Condition | Purpose |
|-----------|-----------|---------|
| `_Nextcdf4Backend` | `NC_NEXTCDF4` was used at create | Records that NEXTCDF-4 created the file. Value is a version string, e.g. `"NEXTCDF-4/1.0"`. |
| `_Nextcdf4Model` | `NC_NETCDF4_MODEL` was also set | Records that the file must remain in NetCDF-4-model compatibility mode. Value `1`. |

These attributes use the underscore-prefix convention already followed by netcdf-c's hidden attributes. Unmodified netcdf-c releases do not know to filter them, so older netcdf-c versions may expose `_Nextcdf4Backend`/`_Nextcdf4Model` as user global attributes on `NC_NETCDF4_MODEL` files. This is a documented cosmetic compatibility caveat; it does not affect data readability.

## Backward Compatibility

### `NC_CLASSIC_MODEL`

NEXTCDF-4 will support the existing `NC_CLASSIC_MODEL` create flag. The data-model restrictions match upstream netcdf-c, but the file will be written with HDF5 Superblock v3 and therefore requires HDF5 1.14.x or later to read. When this flag is set:

- The file is restricted to the classic NetCDF-3 data model.
- Only the root group may be used; no subgroups may be created.
- No user-defined types (compound, enum, opaque, vlen) may be defined.
- Only one unlimited dimension is allowed, and it must be the first (slowest-varying) dimension.
- Only classic atomic types are permitted (`NC_BYTE`, `NC_CHAR`, `NC_SHORT`, `NC_INT`, `NC_FLOAT`, `NC_DOUBLE`). All NEXTCDF-4-specific types are forbidden.
- Chunking, compression, filters, and enhanced NetCDF-4 features are unavailable, matching upstream netcdf-c behavior.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` are mutually exclusive.

### `NC_NETCDF4_MODEL` Compatibility Flag

A new create flag `NC_NETCDF4_MODEL` is introduced in `include/nep.h` (value `0x04000000`, bit 26) and is only meaningful when `NC_NEXTCDF4` is also set. It behaves like the existing `NC_CLASSIC_MODEL` flag, but for the enhanced NetCDF-4 data model:

- When `NC_NETCDF4_MODEL` is set, the file must follow the same data-model restrictions as files created by the current netcdf-c `libhdf5` implementation.
- Such files are always written using Superblock v1 so they remain readable by upstream netcdf-c and HDF5 1.10.x or later.
- No NEXTCDF-4-specific types may be used, so the resulting file can be opened by upstream netcdf-c when it is linked against HDF5 1.10.x or later.
- When `NC_NETCDF4_MODEL` is **not** set, NEXTCDF-4 is free to use the new types described below.

### Restrictions in Compatibility Mode

In `NC_NETCDF4_MODEL` mode, the following features are forbidden at create time:

- `NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_*`, `NC_FLOAT6_*`, and `NC_FLOAT4_*` datatypes.
- Complex number compound types (`NC_COMPLEX`, `NC_DOUBLECOMPLEX`).
- HDF5 reference types (`NC_REF_OBJECT`, `NC_REF_REGION`).
- Bitfield types (`NC_BITFIELD8/16/32/64`).

Attempts to use these features with `NC_NETCDF4_MODEL` will return `NC_EINVAL` or `NC_ENOTNC4` as appropriate.

## Feature Detection: `nep_meta.h`

Before any other NEXTCDF-4 work begins, NEP will add a `nep_meta.h` build-generated header, following the same pattern netcdf-c already uses for `include/netcdf_meta.h` (generated from `include/netcdf_meta.h.in` via `configure_file(... @ONLY)` in `CMakeLists.txt`).

This addresses the need for downstream code (`nextcopy`, `nextdump`, third-party applications, and NEP's own test suite) to feature-detect NEXTCDF-4 capabilities at compile time, rather than guessing based on HDF5 or NEP version numbers alone, since availability of individual features depends on the HDF5 version NEP was built against (1.14.x vs. 2.1.1+) as well as whether NEXTCDF-4 was enabled at all.

### Why This Comes First

- Every later phase (new types, reference support, bitfields, tools) needs a place to advertise "is this feature compiled in," both for NEP's own conditional compilation (`#ifdef`) and for external code that links against NEP.
- Introducing it after other features exist would mean retrofitting feature macros into already-written code and risks inconsistent naming; defining the full macro surface up front keeps naming consistent across all of Phase 1–4.
- It is a small, low-risk, purely additive change (one `.h.in` file plus one `configure_file()` call), making it a low-cost first step that unblocks everything else.

### File Contents

`include/nep_meta.h.in` will be added alongside the existing `include/netcdf_meta.h.in`, generated the same way into `${netCDF_BINARY_DIR}/include/nep_meta.h`:

```c
#ifndef NEP_META_H
#define NEP_META_H

/* NEXTCDF-4 rewrite present at all (vs. classic libhdf5 backend). */
#define NEP_HAS_NEXTCDF4        @NEP_HAS_NEXTCDF4@

/* HDF5 version NEP was built against, for feature gating. */
#define NEP_HDF5_VERSION_MAJOR  @HDF5_VERSION_MAJOR@
#define NEP_HDF5_VERSION_MINOR  @HDF5_VERSION_MINOR@
#define NEP_HDF5_VERSION_PATCH  @HDF5_VERSION_PATCH@

/* Superblock v3 / HDF5 1.14.x+ features. */
#define NEP_HAS_SUPERBLOCK_V3   @NEP_HAS_SUPERBLOCK_V3@
#define NEP_HAS_FLOAT16         @NEP_HAS_FLOAT16@
#define NEP_HAS_REF_TYPES       @NEP_HAS_REF_TYPES@
#define NEP_HAS_BITFIELD        @NEP_HAS_BITFIELD@
#define NEP_HAS_COMPLEX         @NEP_HAS_COMPLEX@

/* HDF5 2.1.1+ small floating-point types. */
#define NEP_HAS_BFLOAT16        @NEP_HAS_BFLOAT16@
#define NEP_HAS_FLOAT8          @NEP_HAS_FLOAT8@
#define NEP_HAS_FLOAT6          @NEP_HAS_FLOAT6@
#define NEP_HAS_FLOAT4          @NEP_HAS_FLOAT4@

/* NC_NETCDF4_MODEL compatibility flag support. */
#define NEP_HAS_NETCDF4_MODEL   @NEP_HAS_NETCDF4_MODEL@

/* nextcopy / nextdump tools built. */
#define NEP_HAS_NEXTCOPY        @NEP_HAS_NEXTCOPY@
#define NEP_HAS_NEXTDUMP        @NEP_HAS_NEXTDUMP@

#endif /* NEP_META_H */
```

Each `@NEP_HAS_*@` macro is set to `1` or `0` by the CMake configure step based on detected HDF5 version and enabled NEP build options (mirroring how `netcdf_meta.h.in` sets `NC_HAS_HDF5`, `NC_HAS_DAP2`, etc.), so all of them are always defined (as `0` or `1`), never simply absent — callers can write `#if NEP_HAS_FLOAT16` unconditionally without an `#ifdef` guard first.

### Build System Integration

- Add `include/nep_meta.h.in` next to the existing `include/netcdf_meta.h.in`.
- Add a `configure_file(${netCDF_SOURCE_DIR}/include/nep_meta.h.in ${netCDF_BINARY_DIR}/include/nep_meta.h @ONLY)` call in the top-level `CMakeLists.txt`, immediately following the existing `netcdf_meta.h` generation step.
- NEP is CMake-only, so no Autotools generation is required.
- Install `nep_meta.h` alongside `netcdf_meta.h` in the public include directory.
- This is a Phase 0 / prerequisite task, completed before Phase 1 ("Foundation") begins, so that every subsequent phase can gate its own new code behind the appropriate `NEP_HAS_*` macro from day one.

## Use Superblock v3

NEXTCDF-4 will use HDF5 Superblock v3 for files by default, and Superblock v1 for `NC_NETCDF4_MODEL` compatibility files. Superblock v3 was introduced in HDF5 1.14.x and provides:

- Larger address and length fields (up to 64-bit throughout).
- Better support for huge datasets and high-throughput storage.
- A cleaner internal layout that avoids some legacy Superblock v0/v2 limitations.

### Requirements

- Build-time HDF5 version must be 1.14.0 or later; HDF5 2.1.1+ is the recommended modern target. If the detected HDF5 version is older than 1.14.x, NEXTCDF-4 is not built.
- All files created by NEXTCDF-4 are written with Superblock v3 by default. Files created with the `NC_NETCDF4_MODEL` compatibility flag are written with Superblock v1 and remain readable by HDF5 1.10.x and later; all other files require HDF5 1.14.x or later to read.

### Implementation Notes

- Use `H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST)` when creating files in native and `NC_CLASSIC_MODEL` modes.
- Use `H5Pset_libver_bounds(fapl_id, H5F_LIBVER_V110, H5F_LIBVER_V110)` when creating files with `NC_NETCDF4_MODEL`, which produces a Superblock v1 file compatible with older netcdf-c/HDF5 releases.
- At NEP configure time, check that the detected HDF5 version is 1.14.x or better. If it is not, disable NEXTCDF-4 entirely (`NEP_HAS_NEXTCDF4=0`).
- At create time, assert that the linked HDF5 library is 1.14.x or later and refuse to create files if it is too old to write Superblock v3. This is a defensive runtime check in addition to the configure-time gate.

## Correct Renaming of Dims and Vars

The current `libhdf5` implementation has subtle bugs when renaming dimensions and variables, especially when:

- A dimension and a variable share the same name.
- A variable is a coordinate variable.
- Dimension scales are attached to multiple variables.

NEXTCDF-4 will:

- Rename the HDF5 dataset or group atomically where possible.
- Update the in-memory `NC_DIM_INFO_T` and `NC_VAR_INFO_T` metadata consistently.
- Re-attach dimension scales when a rename changes the relationship between a coordinate variable and its dimension.
- Preserve dimids across renames so that existing variable references remain valid.
- Every rename will cause a flush to disk with the update. NEXTCDF-4 will not attempt to manage name changes in memory only, and then write everything at once. Each name change will be atomic, and flush to disk. The next name change, if there is one, will face a file on disk and memory info that match and are complete.

### Design Rationale: Flush-Per-Rename

Nearly all of the existing rename bugs in `libhdf5` stem from attempts to batch or defer rename bookkeeping (dimension scales, `DIMENSION_LIST`/`REFERENCE_LIST`, hidden `_Netcdf4Coordinates`/`_Netcdf4Dimid` attributes) and reconcile it later, which breaks down because users can issue renames in arbitrary, adversarial orders (e.g. swapping two names, renaming a dimension to a name a variable currently holds, then renaming that variable away, etc.).

NEXTCDF-4 deliberately avoids this class of bug by design rather than by handling every possible ordering:

- Each `nc_rename_dim`/`nc_rename_var` call performs its HDF5-level rename, updates all dependent in-memory and on-disk metadata (dimension scale names, hidden attributes, `DIMENSION_LIST`/`REFERENCE_LIST`), and flushes to disk **before returning to the caller**.
- Because every rename starts from a state where memory and disk are already fully consistent, each rename only ever has to solve one simple, well-defined problem: "given a consistent file, apply this one rename correctly." It never has to reason about a queue of pending, possibly-conflicting renames.
- This trades rename throughput for correctness: a script issuing thousands of renames in a loop will incur a flush per call. This is an accepted, intentional cost — rename operations are not expected to be a hot path, and the correctness guarantee is considered more valuable than batched rename performance.
- This behavior is not configurable; there is no "deferred flush" mode for renames, precisely because reintroducing deferred/batched updates is what caused the original bugs.
- Applications that need to perform many renames efficiently should be advised (in user-facing documentation) to expect O(1) disk flushes per rename call, and to avoid rename-heavy workloads in performance-sensitive code paths.
- Renaming a dimension or variable never invalidates an existing `NC_REF_OBJECT`/`NC_REF_REGION` reference to it: HDF5 renames (`H5Lmove`) only change the group *link* pointing at an object, not the object's underlying address/token that references actually store. See "Reference Validity Across File Changes" under "Support for HDF5 Reference Types" for the operations that *do* invalidate references.

## Dimension Scales and Dimension Mapping

NEXTCDF-4 must correctly implement NetCDF-4/HDF5 dimension storage using both HDF5 dimension scales and the hidden attributes that record dimension mappings.

### HDF5 Dimension Scales

NetCDF-4 dimensions are stored as HDF5 dimension scales — 1-D datasets marked with `CLASS = "DIMENSION_SCALE"`. Variables attach to these scales via `H5DSattach_scale()`. The relevant HDF5 attributes are:

- `CLASS = "DIMENSION_SCALE"` on the dimension dataset.
- `REFERENCE_LIST` on the dimension dataset: lists variables that use this scale.
- `DIMENSION_LIST` on the variable dataset: lists attached dimension scales.

### Hidden Dimension Mapping Attributes

Reading and matching dimension scales alone is slow for complex files and cannot represent every NetCDF semantic. Modern netcdf-c therefore writes two hidden attributes:

- `_Netcdf4Coordinates` on a variable: stores the list of `dimids` for that variable, allowing fast variable-to-dimension mapping without full dimscale traversal.
- `_Netcdf4Dimid` on a dimension scale dataset: stores the dimension's `dimid`.

### NEXTCDF-4 Behavior

- Write `_Netcdf4Coordinates` on every variable for fast dimension mapping.
- Continue to create and attach dimension scales so the file remains valid for tools that read them directly.
- When opening a file, prefer `_Netcdf4Coordinates`; fall back to dimscale matching when it is absent.
- Preserve `_Netcdf4Dimid` so that dimension IDs remain stable across renames and reopens.
- In `NC_NETCDF4_MODEL` mode, write the same hidden attributes that upstream netcdf-c produces.

## Support for Float16

NEXTCDF-4 will support the IEEE 754 half-precision (16-bit) floating point type as a first-class NetCDF type.
- Will be called `NC_FLOAT16`.
- `NC_FLOAT16` is the IEEE 754 **binary16** format (1 sign bit, 5 exponent bits, 10 mantissa bits). This is the same 16-bit format that C implementations call `float16`, `_Float16`, or `__fp16` on platforms that support it.
- `NC_FLOAT16` is **not** `bfloat16` (1 sign bit, 8 exponent bits, 7 mantissa bits); `bfloat16` is a separate format described below.
- The C API for `NC_FLOAT16` will use `_Float16 *` on C23 compilers that provide `_Float16`. If the compiler does not support `_Float16`, it will fall back to `uint16_t *` raw bits. Conversions to and from `float`/`double` follow IEEE 754 binary16 rules.

### HDF5 Mapping

- Native type: `H5T_IEEE_F16LE` or `H5T_IEEE_F16BE` depending on endianness.
- The NetCDF type constant will be `NC_FLOAT16`.
- Memory representation is the standard IEEE 754 binary16 format.

### API Additions

- Standard `nc_def_var` with `NC_FLOAT16` xtype.
- `nc_put_var_float16` and `nc_get_var_float16`, using `_Float16 *` when the compiler supports `_Float16` and `uint16_t *` otherwise.
- The generic `nc_put_var` / `nc_get_var` family continues to take `void *`.

### Small Floating-Point Types

In addition to `NC_FLOAT16`, NEXTCDF-4 will support the non-IEEE small floating-point types added in HDF5 2.x:

```c
#define NC_BFLOAT16    26  /* H5T_FLOAT_BFLOAT16, 1-8-7 layout */
#define NC_FLOAT8_E4M3 27  /* H5T_FLOAT_F8E4M3, 1-4-3 layout   */
#define NC_FLOAT8_E5M2 28  /* H5T_FLOAT_F8E5M2, 1-5-2 layout   */
#define NC_FLOAT6_E2M3 29  /* H5T_FLOAT_F6E2M3, 1-2-3 layout   */
#define NC_FLOAT6_E3M2 30  /* H5T_FLOAT_F6E3M2, 1-3-2 layout   */
#define NC_FLOAT4_E2M1 31  /* H5T_FLOAT_F4E2M1, 1-2-1 layout   */
```

Memory representation is the raw bit pattern:

- `NC_BFLOAT16` → `uint16_t`
- `NC_FLOAT8_*` → `uint8_t`
- `NC_FLOAT6_*`, `NC_FLOAT4_*` → `uint8_t` (HDF5 stores 6-bit and 4-bit floats in 1-byte datatypes)

These types are software-emulated in HDF5 and implicit conversions are unreliable, so callers must use the matching raw type for I/O.

### Compatibility

- All new floating-point types (`NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_*`, `NC_FLOAT6_*`, `NC_FLOAT4_*`) are only available when `NC_NETCDF4_MODEL` is **not** set.
- `NC_FLOAT16` requires HDF5 1.14.x or later.
- `NC_BFLOAT16` and the FP8/FP6/FP4 types require HDF5 2.1.1 or later.
- Reading an existing float16, bfloat16, or FP8/FP6/FP4 dataset from a non-NEXTCDF-4 HDF5 file is supported.
- These types cannot be coordinate variables.

### Default Fill Values

Each new floating-point type has a documented default fill value, following the same design principle as the existing classic-type defaults (`NC_FILL_FLOAT`, `NC_FILL_DOUBLE`, etc.): a finite, large-magnitude value that is unlikely to occur in real data and is clearly distinguishable from normal results, but that does not risk overflowing to infinity during ordinary arithmetic.

| NetCDF Type      | Default Fill Value | Bit Pattern | Rationale |
|------------------|--------------------:|-------------|-----------|
| `NC_FLOAT16`     | `57344.0`           | `0x7B00`    | Second-highest exponent (`11110`) with mantissa `1100000000`; well below the max finite value (`65504`, `0x7BFF`) and far from `+Inf` (`0x7C00`), so ordinary rounding/arithmetic on nearby data cannot produce or collide with it. |
| `NC_BFLOAT16`    | `9.969209968386869e+36` | `0x7CF0`    | The upper 16 bits of the `NC_FILL_FLOAT` bit pattern (`0x7CF00000`), since `bfloat16` is defined as a truncation of IEEE `binary32`. This keeps the fill value numerically consistent with `NC_FILL_FLOAT` under widening conversion. |
| `NC_FLOAT8_E4M3` | `224.0`             | `0x76`      | Exponent `1110` (second-highest finite exponent), mantissa `110`; avoids the reserved all-ones pattern (`0x7F`/`0xFF`) that E4M3 uses for NaN. |
| `NC_FLOAT8_E5M2` | `28672.0`           | `0x77`      | Exponent `11101` (one below the max finite exponent `11110`), mantissa `11`; avoids the reserved exponent `11111` used for `Inf`/`NaN` in E5M2. |
| `NC_FLOAT6_E2M3` | *(no default; see below)* | — | — |
| `NC_FLOAT4_E2M1` | *(no default; see below)* | — | — |

`NC_FLOAT6_E2M3` and `NC_FLOAT4_E2M1` have too few representable values (at most 32 and 8 finite magnitudes, including sign, respectively) for any reserved "unlikely" sentinel to be meaningful — any fill value chosen is likely to collide with legitimate data. NEXTCDF-4 therefore:

- Does **not** define a default fill value for `NC_FLOAT6_E2M3` or `NC_FLOAT4_E2M1`.
- Creates variables of these types with fill mode enabled per the caller's request via `nc_def_var_fill`, exactly like any other type, but leaves the underlying HDF5 dataset without an implicit fill value unless the caller supplies one explicitly.
- Returns `NC_EINVAL` from `nc_def_var_fill` if the caller does not supply an explicit fill value and also does not disable fill (`NC_NOFILL`) for a variable of one of these two types, so silent reliance on an unlikely-to-be-meaningful default is impossible.

For `NC_COMPLEX` and `NC_DOUBLECOMPLEX` (see "Support for Complex Numbers" below), the default fill value is the compound `{ NC_FILL_FLOAT, NC_FILL_FLOAT }` or `{ NC_FILL_DOUBLE, NC_FILL_DOUBLE }` respectively, applied independently to the real and imaginary members.

`NC_BITFIELD8/16/32/64` and `NC_REF_OBJECT`/`NC_REF_REGION` have no numeric interpretation, so no default fill value is defined for them (matching existing netCDF-4 behavior for `NC_OPAQUE`); files are created with `NC_NOFILL` semantics for these types unless the caller explicitly requests a fill pattern.

### Fill Value Validation

`nc_def_var_fill` will validate that a caller-supplied fill value for any of the new floating-point types is **exactly** representable in the target type's bit layout (no lossy rounding). If the requested value cannot be represented exactly, the call fails with `NC_EINVAL` rather than silently rounding to the nearest representable value. This avoids a class of bugs where a fill value that "looks right" in the API call actually reads back as a different value once written to disk.

## Support for HDF5 Reference Types

NEXTCDF-4 will support both HDF5 object references and region references as NetCDF variables.

### Types

- `H5T_STD_REF_OBJ` (object reference): an opaque 8-byte value pointing to an HDF5 object.
- `H5T_STD_REF_DSETREG` (region reference): an opaque value pointing to a dataspace region in a dataset.

### NetCDF Mapping

- Object references will be exposed as a new opaque NetCDF type `NC_REF_OBJECT`.
- Region references will be exposed as `NC_REF_REGION`.
- Variables of these types are read and written as opaque byte arrays.
- NEXTCDF-4 provides functions to create and resolve both object references and region references.
- Region references are created and dereferenced using NetCDF-style `start` / `count` / `stride` arrays, since NetCDF has no dataspace abstraction.
- Variables of type `NC_REF_REGION` support normal NetCDF strided access (`nc_get_vars` / `nc_put_vars`) to read or write subsets of the reference array; each element is an independent region-reference token.

### Object Reference API

NEXTCDF-4 will add functions to create an HDF5 object reference and to resolve an existing reference back to a NetCDF object id. The object type is passed as a parameter so the same functions work for variables, groups, and user-defined types.

Object reference tokens are represented by an opaque 8-byte struct type:

```c
typedef struct {
    unsigned char bytes[8];
} nc_ref_t;
```

```c
#define NC_REF_OBJ_VAR  1  /* NetCDF variable (HDF5 dataset) */
#define NC_REF_OBJ_GRP  2  /* NetCDF group (HDF5 group) */
#define NC_REF_OBJ_TYPE 3  /* NetCDF user-defined type (HDF5 committed datatype) */

/* Create a reference to the object identified by `objid` in group `ncid`.
 * `ref` receives the opaque 8-byte HDF5 object reference token.
 *   - obj_type == NC_REF_OBJ_VAR:  objid is a varid in group `ncid`.
 *   - obj_type == NC_REF_OBJ_GRP:  objid is a grpid (ncid); `ncid` is ignored.
 *   - obj_type == NC_REF_OBJ_TYPE: objid is a typeid in group `ncid`. */
int nc_ref_object(int ncid, int obj_type, int objid, nc_ref_t *ref);

/* Dereference `ref` and return the group and object ids.
 *   - NC_REF_OBJ_VAR:  *grpidp is the group containing the variable;
 *                      *idp is the varid valid in that group.
 *   - NC_REF_OBJ_GRP:  *grpidp and *idp both receive the grpid (ncid).
 *   - NC_REF_OBJ_TYPE: *grpidp is the group containing the type;
 *                      *idp is the typeid valid in that group. */
int nc_deref_object(int ncid, int obj_type, const nc_ref_t *ref,
                    int *grpidp, int *idp);
```

Both functions validate that the requested `obj_type` matches the actual HDF5 object type stored in the reference token and return `NC_EINVAL` on mismatch.

### Region Reference API

Region references are represented by an opaque struct large enough to hold an HDF5 dataset-region reference token:

```c
/* Region references are opaque and variable-sized in HDF5 1.12+.
 * The caller must allocate a buffer of at least
 * H5Tget_size(H5T_STD_REF_DSETREG) bytes and set `size` accordingly. */
typedef struct {
    size_t size;
    unsigned char *bytes;
} nc_region_ref_t;
```

```c
/* Create a region reference to a hyperslab selection in variable `varid`.
 * `start` and `count` arrays must have length equal to the variable's rank.
 * `stride` may be NULL to indicate a stride of 1 in all dimensions. */
int nc_ref_region(int ncid, int varid,
                  const size_t *start, const size_t *count,
                  const ptrdiff_t *stride,
                  nc_region_ref_t *ref);

/* Dereference a region reference and return the target variable and selection.
 * `start`, `count`, and `stride` arrays must have length equal to the
 * returned rank. */
int nc_deref_region(int ncid, const nc_region_ref_t *ref,
                    int *grpidp, int *varidp, int *ndimsp,
                    size_t *start, size_t *count, ptrdiff_t *stride);
```

Both functions translate between NetCDF-style `start` / `count` / `stride` arrays and the corresponding HDF5 dataspace selection. Only simple strided hyperslabs that can be represented by `start` / `count` / `stride` are supported; more complex HDF5 selections fail with `NC_EINVAL`.

### Reference Validity Across File Changes

HDF5 object and region references target an object's underlying address/token, not its path name, so most structural metadata edits are safe. NEXTCDF-4 documents the following guarantees and failure modes explicitly, rather than leaving them implicit:

**Safe (never invalidates an existing reference):**

- Renaming the referenced dimension, variable, or group (`nc_rename_dim`, `nc_rename_var`, group rename), including the atomic flush-per-rename behavior described under "Correct Renaming of Dims and Vars." HDF5 renames only change the link, not the object.
- Extending an unlimited dimension, as long as a region reference's stored hyperslab (`start`/`count`/`stride`) remains within the variable's (possibly grown) current shape.
- Adding, removing, or modifying attributes on the referenced object.

**Unsafe (invalidates or stales an existing reference):**

- Deleting the referenced variable or group, including the common "delete and recreate under the same name" pattern used to simulate retyping/reshaping. The recreated object has a new HDF5 address/token; old references to the original object become dangling.
- Shrinking a dimension (or otherwise reducing a variable's shape) such that a previously valid region reference's stored `start`/`count`/`stride` selection no longer fits within the variable's current dataspace.

**Enforcement**: `nc_deref_object` and `nc_deref_region` will detect both failure modes at dereference time rather than assuming success:

- If the referenced object no longer exists, both functions return `NC_EINVAL`.
- If a region reference's stored selection no longer fits the target variable's current shape, `nc_deref_region` returns `NC_EINVAL` rather than returning a truncated or out-of-bounds selection.

This is a purely reactive (dereference-time) check — NEXTCDF-4 does not track which variables have outstanding references against them, and does not block or warn on the destructive operations themselves at the time they are performed. Applications that create long-lived references to variables that may later be deleted, recreated, or shrunk are responsible for re-validating those references (e.g. by dereferencing them again) after such operations.

### Restrictions

- Reference-typed variables cannot be coordinate variables.
- Reference types cannot be used inside compound types in the initial implementation.
- Reference types are forbidden in `NC_NETCDF4_MODEL` mode.
- No default `_FillValue` is defined for reference types (see "Default Fill Values" above); variables are created with `NC_NOFILL` semantics unless the caller explicitly requests a fill pattern.

## Support for Complex Numbers

NEXTCDF-4 will support complex number types using HDF5 compound types.

### Type Definition

A complex number is represented as an HDF5 compound type with two members:

- `r` — real part.
- `i` — imaginary part.

For example, a single-precision complex number maps to the portable compound layout:

```c
H5Tcreate(H5T_COMPOUND, 2 * sizeof(float));
H5Tinsert(type_id, "r", 0, H5T_IEEE_F32LE);
H5Tinsert(type_id, "i", sizeof(float), H5T_IEEE_F32LE);
```

### NetCDF Mapping

- New base types: `NC_COMPLEX` (single precision), `NC_DOUBLECOMPLEX` (double precision).
- Layout in memory is the portable compound `{ float r; float i; }` (or `{ double r; double i; }`).
- On platforms with native C `_Complex` support, convenience conversions may be provided, but the file and wire format use the explicit `{r, i}` compound layout.

### Detecting Complex Types on Open

NEXTCDF-4 identifies a compound datatype as `NC_COMPLEX`/`NC_DOUBLECOMPLEX` using a purely structural rule, with no separate marker attribute or reserved committed-type name:

A compound datatype is treated as `NC_COMPLEX` (or `NC_DOUBLECOMPLEX`) if and only if it has:

- Exactly two members, in order.
- The first member named exactly `r`, the second named exactly `i` (case-sensitive).
- Both members of the same base type, either both `H5T_IEEE_F32LE`/`BE` (→ `NC_COMPLEX`) or both `H5T_IEEE_F64LE`/`BE` (→ `NC_DOUBLECOMPLEX`).
- No padding between or after the members (`i` immediately follows `r`, and the compound's total size is exactly `2 * sizeof(member type)`).

Any compound matching this shape — including one created by upstream netcdf-c or another tool with no knowledge of NEXTCDF-4 — is presented to the caller as `NC_COMPLEX`/`NC_DOUBLECOMPLEX`, not as a generic user-defined compound. This is a deliberate simplicity/compatibility trade-off:

- **Pro**: no hidden attributes or reserved committed-type names to keep in sync across renames, copies, or subsetting operations; any tool that writes the exact `{r, i}` layout interoperates automatically.
- **Con**: a pre-existing user-defined compound type that happens to match this exact shape (member names `r`/`i`, matching float/double members, no padding) will be reinterpreted as a complex number when opened by NEXTCDF-4, even if the original author intended a plain 2-field record. This is considered acceptable because the `r`/`i` naming convention for a 2-member float/double compound is already the de facto complex-number convention in the wider HDF5/NetCDF ecosystem (e.g. h5py, PyTables), so a genuine collision is expected to be rare in practice.
- This structural rule applies identically to compound types used for variables, attributes, and compound-type members nested inside other compounds.

### Compatibility

- Complex types are only available outside `NC_NETCDF4_MODEL`.
- Existing NetCDF-4 files with user-defined compound types that do **not** match the exact `{r, i}` structural shape above continue to be read as plain compound types, unchanged from current netcdf-4 behavior.
- Existing files whose compound types do match the structural shape (see "Detecting Complex Types on Open") will now be read as `NC_COMPLEX`/`NC_DOUBLECOMPLEX` rather than as a generic compound; this is a documented, intentional behavior change from upstream netcdf-c and will be called out in release notes.
- User-defined compound types are otherwise unchanged from netcdf-4.
- See "Default Fill Values" above for the default `_FillValue` applied to `NC_COMPLEX`/`NC_DOUBLECOMPLEX` variables.

## Support for HDF5 Bitfield Types

NEXTCDF-4 will support HDF5 bitfield (`H5T_BITFIELD`) datasets and attributes so that existing HDF5 files using bitfields can be read and new bitfield variables can be created.

### HDF5 Mapping

HDF5 bitfield types store raw bit patterns with no numeric semantics. Predefined sizes are 8, 16, 32, and 64 bits: `H5T_STD_B8LE/BE`, `H5T_STD_B16LE/BE`, `H5T_STD_B32LE/BE`, `H5T_STD_B64LE/BE`. They have the same on-disk layout as unsigned integers of the same size, so no data conversion is required.

### NetCDF Mapping

New base NetCDF types:

```c
#define NC_BITFIELD8  22
#define NC_BITFIELD16 23
#define NC_BITFIELD32 24
#define NC_BITFIELD64 25
```

Memory representation:

- `NC_BITFIELD8`  → `uint8_t`
- `NC_BITFIELD16` → `uint16_t`
- `NC_BITFIELD32` → `uint32_t`
- `NC_BITFIELD64` → `uint64_t`

The NetCDF API treats bitfield data as unsigned integer values; interpreting individual bits is the caller's responsibility.

### API

Bitfield variables are defined with the standard `nc_def_var` using an `NC_BITFIELD*` xtype. No separate type-definition function is needed because bitfields are atomic fixed-size types.

### Compatibility

- Bitfield types are only available when `NC_NETCDF4_MODEL` is **not** set.
- When reading an existing HDF5 file, `H5T_BITFIELD` is mapped to the matching `NC_BITFIELD*` type based on the datatype size.
- When writing, `NC_BITFIELD*` maps to the corresponding HDF5 bitfield datatype, preserving round-trip fidelity.
- No default `_FillValue` is defined for bitfield types (see "Default Fill Values" above); variables are created with `NC_NOFILL` semantics unless the caller explicitly requests a fill pattern.

## Type Mapping Summary

| NetCDF Type         | HDF5 Type (native)             | Compatibility Mode |
|---------------------|--------------------------------|----------------------|
| `NC_BYTE`           | `H5T_STD_I8LE/BE`              | Yes                  |
| `NC_UBYTE`          | `H5T_STD_U8LE/BE`              | Yes                  |
| `NC_CHAR`           | `H5T_C_S1`                     | Yes                  |
| `NC_SHORT`          | `H5T_STD_I16LE/BE`             | Yes                  |
| `NC_USHORT`         | `H5T_STD_U16LE/BE`             | Yes                  |
| `NC_INT`            | `H5T_STD_I32LE/BE`             | Yes                  |
| `NC_UINT`           | `H5T_STD_U32LE/BE`             | Yes                  |
| `NC_INT64`          | `H5T_STD_I64LE/BE`             | Yes                  |
| `NC_UINT64`         | `H5T_STD_U64LE/BE`             | Yes                  |
| `NC_FLOAT`          | `H5T_IEEE_F32LE/BE`            | Yes                  |
| `NC_DOUBLE`         | `H5T_IEEE_F64LE/BE`            | Yes                  |
| `NC_FLOAT16`        | `H5T_IEEE_F16LE/BE`            | No                   |
| `NC_BFLOAT16`       | `H5T_FLOAT_BFLOAT16LE/BE`      | No                   |
| `NC_FLOAT8_E4M3`    | `H5T_FLOAT_F8E4M3`             | No                   |
| `NC_FLOAT8_E5M2`    | `H5T_FLOAT_F8E5M2`             | No                   |
| `NC_FLOAT6_E2M3`    | `H5T_FLOAT_F6E2M3`             | No                   |
| `NC_FLOAT6_E3M2`    | `H5T_FLOAT_F6E3M2`             | No                   |
| `NC_FLOAT4_E2M1`    | `H5T_FLOAT_F4E2M1`             | No                   |
| `NC_COMPLEX`        | Compound `{r: f32, i: f32}`    | No                   |
| `NC_DOUBLECOMPLEX`  | Compound `{r: f64, i: f64}`    | No                   |
| `NC_REF_OBJECT`     | `H5T_STD_REF_OBJ`              | No                   |
| `NC_REF_REGION`     | `H5T_STD_REF_DSETREG`          | No                   |
| `NC_BITFIELD8`      | `H5T_STD_B8LE/BE`              | No                   |
| `NC_BITFIELD16`     | `H5T_STD_B16LE/BE`             | No                   |
| `NC_BITFIELD32`     | `H5T_STD_B32LE/BE`             | No                   |
| `NC_BITFIELD64`     | `H5T_STD_B64LE/BE`             | No                   |
| User compound       | `H5T_COMPOUND`                 | Yes                  |
| User enum           | `H5T_ENUM`                     | Yes                  |
| User opaque         | `H5T_OPAQUE`                   | Yes                  |
| User vlen           | `H5T_VLEN`                     | Yes                  |

## Implementation Phases

### Phase 0 — Feature Detection Header

- Add `include/nep_meta.h.in` and wire it into both the CMake and Autotools build systems (see "Feature Detection: `nep_meta.h`" above).
- This phase must land before any other NEXTCDF-4 code is written, so every subsequent phase can gate new functionality behind `NEP_HAS_*` macros from the start.

### Phase 1 — Foundation

- Create the new `src/nextcdf4/` directory structure.
- Implement create/open/close with Superblock v3 default (Superblock v1 for `NC_NETCDF4_MODEL` compatibility files).
- Add `NC_NETCDF4_MODEL` flag and compatibility path.
- Port existing variable and attribute I/O.
- Create HDF5 dimension scales and attach them to variable datasets.
- Write `_Netcdf4Coordinates` and `_Netcdf4Dimid` hidden attributes for fast dimension mapping.

### Phase 2 — Metadata and Renaming

- Implement dimension and variable rename correctly.
- Keep `_Netcdf4Coordinates` and `_Netcdf4Dimid` attributes consistent across renames.
- Add comprehensive tests for rename edge cases.

### Phase 3 — New Types

- Add float16, bfloat16, FP8, FP6, and FP4 support.
- Add complex number compound types.
- Add reference type support.
- Add bitfield type support (`NC_BITFIELD8/16/32/64`).

### Phase 4 — Tools

- Create `nextcopy`, a replacement for `nccopy` that understands all new NEXTCDF-4 types and can convert to/from classic NetCDF-4/HDF5 files.
- Create `nextdump`, a replacement for `ncdump` that can display metadata and data for `NC_FLOAT16`, `NC_BFLOAT16`, FP8/FP6/FP4, complex, reference, and bitfield variables.
- Ensure the tools degrade gracefully for `NC_NETCDF4_MODEL` compatibility files.
- Update Fortran and other language bindings to expose the new C APIs.

### Phase 5 — Validation

- Run the full NetCDF-C test suite against the new backend.
- Run `nextcopy` and `nextdump` against representative files with new types.
- Verify backward compatibility by reading files created with the old backend.
- Verify that `NC_NETCDF4_MODEL` files are readable by upstream netcdf-c.

## Testing Strategy

- Unit tests for each new datatype and HDF5 mapping.
- Round-trip tests that create, close, and re-read files.
- Small floating-point round-trip tests for `NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8/6/4` types.
- Default fill-value tests verifying the documented `NC_FILL_FLOAT16`/`NC_FILL_BFLOAT16`/`NC_FILL_FLOAT8_E4M3`/`NC_FILL_FLOAT8_E5M2` bit patterns round-trip exactly, that `NC_FLOAT6_E2M3`/`NC_FLOAT4_E2M1` variables require an explicit fill value or `NC_NOFILL`, and that `nc_def_var_fill` rejects inexact fill values with `NC_EINVAL`.
- Bitfield round-trip tests verifying `NC_BITFIELD8/16/32/64` and `H5T_BITFIELD` fidelity.
- Tests verifying `_Netcdf4Coordinates` and `_Netcdf4Dimid` are written and read correctly.
- Fallback tests that open files without hidden coordinates attributes using dimscale matching.
- Compatibility tests that open NEXTCDF-4 files with upstream netcdf-c (for `NC_NETCDF4_MODEL`).
- Compatibility tests that open upstream netcdf-c files with NEXTCDF-4.
- Rename regression tests covering coordinate variables and shared dimension scales.
- Reference-validity tests: create an object/region reference, then (a) rename the target and confirm the reference still dereferences correctly, (b) extend an unlimited dimension and confirm an in-bounds region reference still dereferences correctly, (c) delete-and-recreate the target under the same name and confirm `nc_deref_object`/`nc_deref_region` return `NC_EINVAL`, and (d) shrink a dimension so a region reference's selection no longer fits and confirm `nc_deref_region` returns `NC_EINVAL`.

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| HDF5 1.14.x not available on all target systems | Provide a configure-time fallback that disables NEXTCDF-4 and falls back to classic backend. |
| Reference types expose HDF5-specific semantics | Provide explicit `nc_ref_object`/`nc_deref_object` and `nc_ref_region`/`nc_deref_region` APIs; document that reference tokens are file-local and that region references are limited to strided hyperslabs. |
| Float16 precision surprises users | Document precision limits; consider fill-value and default-conversion behavior carefully. |
| Rename bug fixes break existing workarounds | Add tests for the exact bug scenarios and announce behavioral changes. |
| Complex compound layout differs from Fortran expectations | Define a single portable layout and provide Fortran wrappers that reorder if needed. |

## Open Questions

- Which HDF5 2.1.1+ features should be adopted beyond Superblock v3, float16, and reference types (e.g., FP4/FP6/FP8 datatypes, SWMR Fortran wrappers)?
- Should the rewrite use `H5VL` (virtual object layer) connectors for testing or layered I/O?

## `src/nextcdf4/` Source Organization

The NEXTCDF-4 backend is a clean rewrite in a new `src/nextcdf4/` directory. It is modelled on the split between `netcdf-c/libsrc4` (the in-memory NetCDF-4 metadata model) and `netcdf-c/libhdf5` (the HDF5-specific I/O and dispatch implementation). Existing code is used only as a reference, not copied.

The same NetCDF-4 `NC_Dispatch` interface is exposed, but the dispatch table lives in `src/nextcdf4/` and every file is written from scratch. Public entry points should use the existing `NC4_*` dispatch names where the semantics are unchanged, and `NEXTCDF4_*` names for helper routines that are internal to the new backend.

### Dispatch and build files

- `CMakeLists.txt` — Object library `netcdfnext4` that builds all files below, links `HDF5::HDF5`, and supplies the `NETCDF.UDF9.LIBRARY` / `NETCDF.UDF9.INIT` autoload keys.
- `nxt4dispatch.c` / `nxt4dispatch.h` — `NC_Dispatch` table and `NC_NEXTCDF4_initialize` / `NC_NEXTCDF4_finalize`, analogous to `hdf5dispatch.c` (`NC_HDF5_initialize` / `NC_HDF5_finalize`) and `nc4dispatch.c` (`NC4_initialize` / `NC4_finalize`).

### File life-cycle (analogous to `libhdf5/hdf5create.c`, `hdf5open.c`, `hdf5file.c`)

- `nxt4create.c`:
  - `NC4_create`, `NEXTCDF4_create_file`, `NEXTCDF4_H5Fcreate`
  - Set `H5Pset_libver_bounds` to `H5F_LIBVER_LATEST` by default and `H5F_LIBVER_V110` when `NC_NETCDF4_MODEL` is set.
- `nxt4open.c`:
  - `NC4_open`, `NEXTCDF4_open_file`, `NEXTCDF4_H5Fopen`
  - `NEXTCDF4_read_metadata`, `NEXTCDF4_read_var`, `NEXTCDF4_read_type`, `NEXTCDF4_read_att`, `NEXTCDF4_read_dimscales`, `NEXTCDF4_rec_read_metadata`
- `nxt4file.c`:
  - `NC4_close`, `NC4_sync`, `NC4_redef`, `NC4__enddef`, `NC4_set_fill`
  - `NC4_inq_format`, `NC4_inq_format_extended`
  - `NEXTCDF4_close_hdf5_file`, `NEXTCDF4_enddef_netcdf4_file`, `NEXTCDF4_sync_file`

### Variable I/O and chunking (analogous to `libhdf5/hdf5var.c` + `libsrc4/nc4var.c`)

- `nxt4var.c`:
  - `NC4_def_var`, `NC4_def_var_chunking`, `NC4_def_var_deflate`, `NC4_def_var_endian`, `NC4_def_var_fill`, `NC4_def_var_fletcher32`, `NC4_def_var_quantize`
  - `NC4_put_vara`, `NC4_get_vara`, `NC4_put_vars`, `NC4_get_vars`
  - `NC4_rename_var`, `NC4_var_par_access`, `NC4_HDF5_inq_var_all`
  - `NEXTCDF4_def_var_extra`, `NEXTCDF4_var_create_dataset`, `NEXTCDF4_set_par_access`, `NEXTCDF4_give_var_secret_name`

### Attributes (analogous to `libhdf5/hdf5attr.c` + `libsrc4/nc4attr.c`)

- `nxt4attr.c`:
  - `NC4_HDF5_put_att`, `NC4_HDF5_get_att`, `NC4_HDF5_del_att`, `NC4_HDF5_inq_att`, `NC4_HDF5_inq_attid`, `NC4_HDF5_inq_attname`, `NC4_HDF5_rename_att`
  - `NEXTCDF4_getattlist`, `NEXTCDF4_put_att`

### Dimensions and dimscales (analogous to `libhdf5/hdf5dim.c` + `libsrc4/nc4dim.c`)

- `nxt4dim.c`:
  - `NC4_def_dim`, `NC4_inq_dim`, `NC4_rename_dim`
  - `NC4_inq_dimid`, `NC4_inq_unlimdim`, `NC4_inq_unlimdims`
  - `NEXTCDF4_attach_dimscales`, `NEXTCDF4_detach_scales`, `NEXTCDF4_write_dim`, `NEXTCDF4_write_coord_dimids`, `NEXTCDF4_delete_dimscale_dataset`

### Groups and types (analogous to `libhdf5/hdf5grp.c`, `hdf5type.c` and `libsrc4/nc4grp.c`, `nc4type.c`)

- `nxt4grp.c`:
  - `NC4_def_grp`, `NC4_rename_grp`
  - `NC4_inq_ncid`, `NC4_inq_grps`, `NC4_inq_grpname`, `NC4_inq_grpname_full`, `NC4_inq_grp_parent`, `NC4_inq_grp_full_ncid`
  - `NC4_inq_varids`, `NC4_inq_dimids`, `NC4_inq_typeids`
- `nxt4type.c`:
  - `NC4_def_compound`, `NC4_insert_compound`, `NC4_insert_array_compound`, `NC4_inq_compound_field`, `NC4_inq_compound_fieldindex`
  - `NC4_def_vlen`, `NC4_put_vlen_element`, `NC4_get_vlen_element`
  - `NC4_def_enum`, `NC4_insert_enum`, `NC4_inq_enum_member`, `NC4_inq_enum_ident`
  - `NC4_def_opaque`, `NC4_inq_type`, `NC4_inq_typeid`, `NC4_inq_typeids`, `NC4_inq_user_type`, `NC4_inq_type_equal`
  - New type mapping helpers: `NEXTCDF4_get_hdf_typeid`, `NEXTCDF4_get_netcdf_type`, `NEXTCDF4_add_user_type`

### Core HDF5 metadata read/write (analogous to `libhdf5/nc4hdf.c`)

- `nxt4hdf.c`:
  - `NEXTCDF4_write_var`, `NEXTCDF4_write_attlist`, `NEXTCDF4_write_dim`, `NEXTCDF4_write_coord_dimids`, `NEXTCDF4_write_netcdf4_dimid`, `NEXTCDF4_write_quantize_att`
  - `NEXTCDF4_var_create_dataset`, `NEXTCDF4_var_exists`
  - `NEXTCDF4_commit_type`, `NEXTCDF4_create_group`
  - `NEXTCDF4_rec_write_metadata`, `NEXTCDF4_rec_write_groups_types`
  - `NEXTCDF4_rec_match_dimscales`, `NEXTCDF4_attach_dimscales`, `NEXTCDF4_rec_reattach_scales`, `NEXTCDF4_remove_coord_atts`
  - `NEXTCDF4_root_att_exists`, `NEXTCDF4_walk`, `NEXTCDF4_get_hdf_typeid`

### Filters and plugins (analogous to `libhdf5/hdf5filter.c`, `hdf5plugins.c` and `libsrc4/nc4filters.c`)

- `nxt4filter.c`:
  - `NC4_hdf5_def_var_filter`, `NC4_hdf5_inq_var_filter_ids`, `NC4_hdf5_inq_var_filter_info`, `NC4_hdf5_inq_filter_avail`
  - `NEXTCDF4_filter_lookup`, `NEXTCDF4_filter_remove`, `NEXTCDF4_filter_initialize`, `NEXTCDF4_filter_finalize`
- `nxt4plugins.c` (optional, if NEP plugin path support is required):
  - `NEXTCDF4_plugin_path_initialize`, `NEXTCDF4_plugin_path_finalize`, `NEXTCDF4_plugin_path_get/set`

### In-memory metadata model and utilities (analogous to `libhdf5/hdf5internal.c` and `libsrc4/nc4internal.c`)

- `nxt4internal.c`:
  - `NC_FILE_INFO_T` / `NC_GRP_INFO_T` / `NC_VAR_INFO_T` list management:
    - `NEXTCDF4_find_grp`, `NEXTCDF4_find_var`, `NEXTCDF4_find_dim`, `NEXTCDF4_find_type`, `NEXTCDF4_find_att`
    - `NEXTCDF4_file_list_add`, `NEXTCDF4_file_list_get`, `NEXTCDF4_file_list_del`
    - `NEXTCDF4_type_new`, `NEXTCDF4_type_free`, `NEXTCDF4_var_free`, `NEXTCDF4_dim_free`, `NEXTCDF4_att_free`
  - Name handling: `NEXTCDF4_check_name`, `NEXTCDF4_check_dup_name`, `NEXTCDF4_normalize_name`
  - `NEXTCDF4_hdf5_initialize`, `NEXTCDF4_hdf5_finalize`, `NEXTCDF4_hdf5_set_log_level`
  - `NEXTCDF4_show_metadata`, `NEXTCDF4_log_metadata_nc`, `NEXTCDF4_log_hdf5`

### Provenance / NCProperties (analogous to `libhdf5/nc4info.c`)

- `nxt4provenance.c`:
  - `NC4_new_provenance`, `NC4_clear_provenance`, `NC4_free_provenance`
  - `NC4_write_provenance`, `NC4_read_provenance`
  - `NC4_write_ncproperties`, `NC4_read_ncproperties`
  - `NEXTCDF4_build_propstring`, `NEXTCDF4_parse_provenance`, `NEXTCDF4_properties_getversion`

### In-memory images and debugging (optional/adapted)

- `nxt4image.c`:
  - `NC4_create_image_file`, `NC4_open_image_file`
- `nxt4debug.c` / `nxt4debug.h`:
  - `NEXTCDF4_hdf5breakpoint`, `NEXTCDF4_set_log_level`, `NEXTCDF4_log_hdf5`
  - Trace/logging macros (`TRACE0`, `TRACE1`, `TRACEEND`, `TRACEFAIL`, etc.)

### Private headers

- `nxt4internal.h` — shared internal data structures and function prototypes.
- `nxt4dispatch.h` — dispatch table and UDF init/final declarations.
- `nxt4debug.h` — logging macros, `nch5breakpoint`, and trace flags.

## Sprint 8: Correct Dimension and Variable Renaming

Sprint 8 adds full `nc_rename_dim` and `nc_rename_var` support for the NEXTCDF-4 backend.

- `NEXTCDF4_rename_dim` (new `nxt4dim.c` or `nxt4meta.c`):
  - Validates define mode/writable state and new-name constraints.
  - Locates the dimension's `NC_DIM_INFO_T` and `NEXTCDF4_DIM_INFO_T` HDF5 dataset.
  - Calls `H5Lmove` in the parent HDF5 group to rename the scale dataset link.
  - Calls `H5DSset_scale` with the new name so the dimension-scale `NAME` attribute is updated.
  - Updates the in-memory `dim->hdr.name`, frees the old string, and re-inserts the dimension into the group's dimension index under the new name.
  - If the dimension has a 1D coordinate variable (same name as the dim), also updates the associated `NC_VAR_INFO_T` name without a second `H5Lmove`.

- `NEXTCDF4_rename_var` (new `nxt4var.c` or `nxt4meta.c`):
  - Validates define mode/writable state and new-name constraints.
  - Locates the variable's `NC_VAR_INFO_T` and `NEXTCDF4_VAR_INFO_T` HDF5 dataset.
  - Calls `H5Lmove` in the parent HDF5 group to rename the variable dataset link.
  - Updates the in-memory `var->hdr.name` and re-inserts the variable into the group's variable index under the new name.
  - If the variable is a 1D coordinate variable (one dimension and same name as that dimension), also renames the dimension and scale `NAME` attribute with `H5DSset_scale` and updates `NC_DIM_INFO_T`.

- `nxt4dispatch.c`:
  - Replace the `NC_RO_rename_dim` and `NC_RO_rename_var` fallback entries with `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var`.

- `test/tst_nextcdf4_rename.c`:
  - Simple dimension rename.
  - Coordinate-variable/dimension mutual rename.
  - Variable rename that is a coordinate variable (dimension also renamed).
  - Shared-dimension rename with two dependent variables.
  - Reopen and verify persisted names.

- Acceptance criteria:
  - `nc_rename_dim` and `nc_rename_var` return `NC_NOERR` and update `nc_inq_dim`/`nc_inq_var` results.
  - Coordinate-variable/dimension name pairs stay synchronized.
  - Shared dimensions rename without corrupting dependent variables.
  - All existing `tst_nextcdf4_*` tests continue to pass.

**Sprint 8 is implemented.** The functions live in `src/nextcdf4/nxt4meta.c`, are wired into `nxt4dispatch.c`, and are covered by `test/tst_nextcdf4_rename.c`. All `tst_nextcdf4_*` C tests pass.

## Sprint 7: Open Existing and Populated Files

Sprint 7 is implemented and tested. The key changes are in `src/nextcdf4/nxt4meta.c`, `nxt4file.c`, `nxt4open.c`, `nxt4internal.h`, and `test/tst_nextcdf4_open.c`.

- `NEXTCDF4_load_metadata` now calls `load_group_metadata` recursively for the root group and every child group.
- `load_dimensions`, `load_variables`, and `load_group_attributes` accept an `NC_GRP_INFO_T *` and its `hdf_group` so that each group loads its own metadata.
- `load_one_var` resolves dimension ids from the `_Netcdf4Coordinates` hidden attribute first, then falls back to `H5DSget_num_scales`/`H5DSiterate_scales` and the `_Netcdf4Dimid`/`NAME` attributes.
- `find_var_cb` accepts coordinate variables, regular variables with attached dimension scales, and NEXTCDF-4 variables, but rejects bare dimension scales and arbitrary HDF5 datasets.
- `NEXTCDF4_read_markers` allows a missing `_Nextcdf4Backend` marker to support upstream `NC_NETCDF4` files while still rejecting unmarked, non-NetCDF-4 HDF5 files.
- `test/tst_nextcdf4_open.c` covers both NEXTCDF-4 reopen and upstream `NC_NETCDF4` open.

## Sprint 9: Add Small Floating-Point Types

Sprint 9 adds `NC_FLOAT16` and, when HDF5 2.1.1+ is available, `NC_BFLOAT16` and the FP8/FP6/FP4 small floating-point types to the NEXTCDF-4 backend.

- `include/nep.h`:
  - Define `NC_FLOAT16` (and optionally `NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, `NC_FLOAT4_E2M1`) with non-colliding `nc_type` values.

- `src/nextcdf4/nxt4type.c`:
  - Extend `NEXTCDF4_map_hdf_type` to map `NC_FLOAT16` to `H5T_IEEE_F16LE/BE` and the 2.1.1+ small floats to their HDF5 predefined datatypes.
  - Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name` for the new types.

- `src/nextcdf4/nxt4meta.c`:
  - Extend `map_nc_type` to recognize `H5T_FLOAT`/`H5T_BITFIELD` size 2 as `NC_FLOAT16` and the HDF5 2.1.1 small-float type/size pairs.
  - Update `set_var_type` and `NEXTCDF4_check_atomic_type` to permit the new types only in native NEXTCDF-4 mode.

- `src/nextcdf4/nxt4io.c`:
  - Ensure `var_io` uses the correct HDF5 memory/file type for the new small floats. For the initial implementation, require `memtype` to match the variable's file type to avoid incomplete HDF5 conversions.

- `test/tst_nextcdf4_float16.c`:
  - Round-trip `NC_FLOAT16` data using `nc_def_var`, `nc_put_vara`, and `nc_get_vara`.
  - Verify `nc_inq_var` reports `NC_FLOAT16`.
  - Verify `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` reject `NC_FLOAT16`.

- Acceptance criteria:
  - `NC_FLOAT16` round-trips in native NEXTCDF-4 files.
  - The type is rejected in compatibility modes.
  - Reopening a file with `NC_FLOAT16` variables correctly recovers the type.
  - Existing `tst_nextcdf4_*` tests continue to pass.

**Sprint 9 is implemented.** All seven small-floating-point types round-trip in `test/tst_nextcdf4_float16.c`, which passes. The relevant functions are in `nxt4meta.c`, `nxt4io.c`, and `nxt4internal.h`; constants are in `include/nep.h`.

## Sprint 10: Add Complex, Bitfield, and Reference Types

Sprint 10 adds built-in complex (`NC_COMPLEX`, `NC_DOUBLECOMPLEX`), bitfield (`NC_BITFIELD8/16/32/64`), and reference (`NC_REF_OBJECT`, `NC_REF_REGION`) types to the NEXTCDF-4 backend.

- `include/nep.h`:
  - Define the eight new `nc_type` constants.

- `src/nextcdf4/nxt4meta.c`:
  - Extend `NEXTCDF4_map_hdf_type` to create the matching HDF5 compound, bitfield, and reference datatypes.
  - Extend `map_nc_type` to detect `H5T_COMPOUND` with `r`/`i` float or double members (complex), `H5T_BITFIELD` by size, and `H5T_REFERENCE` by reference type.
  - Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name`.
  - Update `NEXTCDF4_check_atomic_type` to allow the new types only in native NEXTCDF-4 mode.
  - Update `set_var_type` to set the correct `nc_type_class` for each new type.

- `src/nextcdf4/nxt4io.c`:
  - Extend `memory_type` to return the matching in-memory HDF5 type for each new type and ensure whole-slab I/O for references.

- Tests:
  - `test/tst_nextcdf4_complex.c` round-trips `NC_COMPLEX` and `NC_DOUBLECOMPLEX`.
  - `test/tst_nextcdf4_bitfield.c` round-trips `NC_BITFIELD8/16/32/64`.
  - `test/tst_nextcdf4_ref.c` round-trips `NC_REF_OBJECT` and `NC_REF_REGION` as opaque byte arrays.
  - Rejection in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` for each new type.

- Acceptance criteria:
  - Each new type round-trips in native NEXTCDF-4 files.
  - `nc_inq_var` reports the correct `nc_type`.
  - Compatibility modes reject the new types.
  - Existing `tst_nextcdf4_*` tests continue to pass.

**Sprint 10 is implemented.** Complex, bitfield, and reference types round-trip in `test/tst_nextcdf4_complex.c`, `test/tst_nextcdf4_bitfield.c`, and `test/tst_nextcdf4_ref.c`. The relevant functions are in `nxt4meta.c` and `nxt4io.c`; constants are in `include/nep.h`.

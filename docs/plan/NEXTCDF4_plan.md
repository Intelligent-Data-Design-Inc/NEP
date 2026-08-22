# NEXTCDF-4 Plan

## Overview

NEXTCDF-4 is a clean rewrite from scratch of netcdf-c's `libhdf5` HDF5 backend, implemented in a new `nextcdf4/` directory. No code will be reused from the existing `libhdf5/` implementation, although it may be consulted as a reference. The rewrite takes advantage of new features in HDF5 1.14.x and HDF5 2.1.1+, like SWMR, float16, and compound types. It adds support for HDF5 reference types, and also fixes some long-standing bugs relating to renaming vars and dims.

The goals are:

- Correct handling of HDF5 features that the current implementation either avoids or maps poorly (references, float16, compound types, superblock v3).
- Maintain full read compatibility with existing NetCDF-4/HDF5 files.
- Produce files that netcdf-c can still open when the `NC_NETCDF4_MODEL` compatibility flag is used.

## Goals and Non-Goals

### Goals

- Write a new version of the HDF5 backend in a `nextcdf4/` directory with a modern, layered design aligned with the `NC_FILE_INFO_T` metadata model.
- Default to HDF5 Superblock v3 for all newly created NEXTCDF-4 files.
- Add native support for 16-bit floating point (`H5T_IEEE_F16LE` / `H5T_IEEE_F16BE`).
- Add read and write support for HDF5 reference types (object references and region references).
- Add native support for complex number types via HDF5 compound types.
- Fix the long-standing renaming bug for dimensions and variables.
- Expose all new C APIs in the Fortran API in the same release.

### Non-Goals

- Changing the public NetCDF-C API for classic NetCDF-3 files.
- Removing support for classic-model NetCDF-4 files.
- Modifying the NcZarr, DAP, or NetCDF-3 backends.
- Reusing code from the existing netcdf-c `libhdf5/` implementation. It may be consulted as a reference, but `nextcdf4/` will be written from scratch.

## Backward Compatibility

### `NC_CLASSIC_MODEL`

NEXTCDF-4 will support the existing `NC_CLASSIC_MODEL` create flag exactly as upstream netcdf-c does. When this flag is set:

- The file is restricted to the classic NetCDF-3 data model.
- Only the root group may be used; no subgroups may be created.
- No user-defined types (compound, enum, opaque, vlen) may be defined.
- Only one unlimited dimension is allowed, and it must be the first (slowest-varying) dimension.
- Only classic atomic types are permitted (`NC_BYTE`, `NC_CHAR`, `NC_SHORT`, `NC_INT`, `NC_FLOAT`, `NC_DOUBLE`). All NEXTCDF-4-specific types are forbidden.
- Chunking, compression, filters, and enhanced NetCDF-4 features are unavailable, matching upstream netcdf-c behavior.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` are mutually exclusive.

### `NC_NETCDF4_MODEL` Compatibility Flag

A new create flag `NC_NETCDF4_MODEL` will be introduced. It behaves like the existing `NC_CLASSIC_MODEL` flag, but for the enhanced NetCDF-4 data model:

- When `NC_NETCDF4_MODEL` is set, the file must follow the same restrictions as files created by the current netcdf-c `libhdf5` implementation.
- Such files will be written using Superblock v3, but without the new NEXTCDF-4 types.
- This ensures the resulting file can be opened by older versions of netcdf-c and by the current NetCDF-4 reference implementation, as long as HDF5-1.14.x or later is used.
- When `NC_NETCDF4_MODEL` is **not** set, NEXTCDF-4 is free to use the new types described below.

### Restrictions in Compatibility Mode

In `NC_NETCDF4_MODEL` mode, the following features are forbidden at create time:

- `NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_*`, `NC_FLOAT6_*`, and `NC_FLOAT4_*` datatypes.
- Complex number compound types (`NC_COMPLEX`, `NC_DOUBLECOMPLEX`).
- HDF5 reference types (`NC_REF_OBJECT`, `NC_REF_REGION`).
- Bitfield types (`NC_BITFIELD8/16/32/64`).

Attempts to use these features with `NC_NETCDF4_MODEL` will return `NC_EINVAL` or `NC_ENOTNC4` as appropriate.

## Use Superblock v3

NEXTCDF-4 will use HDF5 Superblock v3 for files by default. Superblock v3 was introduced in HDF5 1.14.x and provides:

- Larger address and length fields (up to 64-bit throughout).
- Better support for huge datasets and high-throughput storage.
- A cleaner internal layout that avoids some legacy Superblock v0/v2 limitations.

### Requirements

- Build-time HDF5 version must be 1.14.0 or later; HDF5 2.1.1+ is the recommended modern target.
- Files created by NEXTCDF-4 without `NC_NETCDF4_MODEL` will not be readable by HDF5 1.12.x or older.

### Implementation Notes

- Use `H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST)` when creating files in native mode.
- For `NC_NETCDF4_MODEL` files, use `H5F_LIBVER_EARLIEST` or the netcdf-c default bounds so the file remains broadly readable.
- At NEP build time, check that we have hdf5-1.14.x or better.

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
- Will be called NC_FLOAT16
- Will map to the C float16 type.

### HDF5 Mapping

- Native type: `H5T_IEEE_F16LE` or `H5T_IEEE_F16BE` depending on endianness.
- The NetCDF type constant will be `NC_FLOAT16`.
- Memory representation is the standard IEEE 754 binary16 format (1 sign bit, 5 exponent bits, 10 mantissa bits).

### API Additions

- Standard `nc_def_var` with `NC_FLOAT16` xtype.
- `nc_put_var_float16` and `nc_get_var_float16` variants.

### Small Floating-Point Types

In addition to `NC_FLOAT16`, NEXTCDF-4 will support the non-IEEE small floating-point types added in HDF5 2.x:

```c
#define NC_BFLOAT16    22  /* H5T_FLOAT_BFLOAT16, 1-8-7 layout */
#define NC_FLOAT8_E4M3 23  /* H5T_FLOAT_F8E4M3, 1-4-3 layout   */
#define NC_FLOAT8_E5M2 24  /* H5T_FLOAT_F8E5M2, 1-5-2 layout   */
#define NC_FLOAT6_E2M3 25  /* H5T_FLOAT_F6E2M3, 1-2-3 layout   */
#define NC_FLOAT6_E3M2 26  /* H5T_FLOAT_F6E3M2, 1-3-2 layout   */
#define NC_FLOAT4_E2M1 27  /* H5T_FLOAT_F4E2M1, 1-2-1 layout   */
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
#define NC_REF_REGION_SIZE 64

typedef struct {
    unsigned char bytes[NC_REF_REGION_SIZE];
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

### Restrictions

- Reference-typed variables cannot be coordinate variables.
- Reference types cannot be used inside compound types in the initial implementation.
- Reference types are forbidden in `NC_NETCDF4_MODEL` mode.

## Support for Complex Numbers

NEXTCDF-4 will support complex number types using HDF5 compound types.

### Type Definition

A complex number is represented as an HDF5 compound type with two members:

- `r` — real part.
- `i` — imaginary part.

For example, a single-precision complex number maps to:

```c
H5Tcreate(H5T_COMPOUND, sizeof(float _Complex));
H5Tinsert(type_id, "r", 0, H5T_IEEE_F32LE);
H5Tinsert(type_id, "i", sizeof(float), H5T_IEEE_F32LE);
```

### NetCDF Mapping

- New base types: `NC_COMPLEX` (single precision), `NC_DOUBLECOMPLEX` (double precision).
- Layout in memory follows the C `_Complex` ABI for the target platform.
- On platforms without native `_Complex` support, the user sees the compound `{r, i}` representation.

### Compatibility

- Complex types are only available outside `NC_NETCDF4_MODEL`.
- Existing NetCDF-4 files with user-defined compound types named like complex numbers will continue to be read as plain compound types.
- User-defined compound types are otherwise unchanged from netcdf-4.

## Support for HDF5 Bitfield Types

NEXTCDF-4 will support HDF5 bitfield (`H5T_BITFIELD`) datasets and attributes so that existing HDF5 files using bitfields can be read and new bitfield variables can be created.

### HDF5 Mapping

HDF5 bitfield types store raw bit patterns with no numeric semantics. Predefined sizes are 8, 16, 32, and 64 bits: `H5T_STD_B8LE/BE`, `H5T_STD_B16LE/BE`, `H5T_STD_B32LE/BE`, `H5T_STD_B64LE/BE`. They have the same on-disk layout as unsigned integers of the same size, so no data conversion is required.

### NetCDF Mapping

New base NetCDF types:

```c
#define NC_BITFIELD8  18
#define NC_BITFIELD16 19
#define NC_BITFIELD32 20
#define NC_BITFIELD64 21
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

## Type Mapping Summary

| NetCDF Type         | HDF5 Type (native)             | Compatibility Mode |
|---------------------|--------------------------------|----------------------|
| `NC_BYTE`           | `H5T_STD_I8LE/BE`              | Yes                  |
| `NC_UBYTE`          | `H5T_STD_U8LE/BE`              | Yes                  |
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

### Phase 1 — Foundation

- Create the new `nextcdf4/` directory structure.
- Implement create/open/close with Superblock v3 default.
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
- Bitfield round-trip tests verifying `NC_BITFIELD8/16/32/64` and `H5T_BITFIELD` fidelity.
- Tests verifying `_Netcdf4Coordinates` and `_Netcdf4Dimid` are written and read correctly.
- Fallback tests that open files without hidden coordinates attributes using dimscale matching.
- Compatibility tests that open NEXTCDF-4 files with upstream netcdf-c (for `NC_NETCDF4_MODEL`).
- Compatibility tests that open upstream netcdf-c files with NEXTCDF-4.
- Rename regression tests covering coordinate variables and shared dimension scales.

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

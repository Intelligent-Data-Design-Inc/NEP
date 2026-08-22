---
name: hdf5
description: Understanding HDF5 storage features used by NEXTCDF-4, including Superblock v3, float16, compound types, and reference types.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-08-22"
---

# HDF5 Skill

This skill covers the HDF5 features that NEXTCDF-4 depends on: Superblock v3, the half-precision float16 datatype, compound types, and HDF5 reference types. It is intended to guide implementation of the NEXTCDF-4 rewrite of netcdf-c's `libhdf5/` backend.

## Overview

**HDF5** is the storage format behind NetCDF-4. NEXTCDF-4 is a rewrite of the NetCDF-C HDF5 backend (`libhdf5/`) that targets HDF5 1.14.x and HDF5 2.1.1+. The rewrite exploits newer HDF5 capabilities while preserving read access to existing NetCDF-4 files.

Key areas covered here:

1. **Superblock v3** — the new file superblock introduced in HDF5 1.14.x.
2. **Float16** — the IEEE 754 half-precision floating point datatype.
3. **Compound types** — the HDF5 mechanism used for user-defined compound types and complex numbers.
4. **Reference types** — HDF5 object and region references.

## Superblock v3

### What It Is

The HDF5 superblock is the first object in an HDF5 file. It stores the file format version, free-space information, root group address, and driver information. Versions:

| Version | Introduced In | Notes |
|---------|---------------|-------|
| 0 | HDF5 1.0 | Original superblock; 8-byte signature at offset 0 (`89 HDF \r \n 1a \n`). |
| 1 | HDF5 1.6 | Minor changes to size fields. |
| 2 | HDF5 1.8 | Added checksum and extension mechanisms; used by classic netcdf-c. |
| 3 | HDF5 1.14.0 | Adds post-heap access, larger fields, and newer root group object header. |

### Why NEXTCDF-4 Uses It

Superblock v3 provides:

- Access to the **post-heap** for future format extensions.
- Cleaner evolution toward HDF5 2.x.
- Larger internal length fields that avoid some legacy v0/v2 limitations.

### HDF5 API

To create a Superblock v3 file, set the "latest" library version bounds on the file access property list:

```c
hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

hid_t file_id = H5Fcreate("file.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
```

To query the superblock version of an open file:

```c
unsigned superblock_version;
H5Fget_superblock(file_id, &superblock_version);  /* HDF5 1.14+ */
```

Or read the file superblock directly with `H5Fget_info2`:

```c
H5F_info2_t info;
H5Fget_info2(file_id, &info);
/* info.super.version contains the superblock version */
```

### Compatibility Implications

- Files created with `H5F_LIBVER_LATEST` (Superblock v3) require **HDF5 1.14.0 or later** to read.
- Older HDF5 1.12.x and 1.10.x libraries cannot open Superblock v3 files.
- NEXTCDF-4 therefore requires HDF5 1.14.x or newer when creating native-mode files.

### Compatibility Mode in NEXTCDF-4

Two compatibility flags are supported:

- `NC_CLASSIC_MODEL` — restricts the file to the classic NetCDF-3 data model, exactly as upstream netcdf-c does. Only the root group is allowed, no user-defined types, only one unlimited dimension (first and slowest-varying), and only classic atomic types. `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` are mutually exclusive.

- `NC_NETCDF4_MODEL` — allows the enhanced NetCDF-4 data model but forbids the new NEXTCDF-4-specific types. Files are written with Superblock v3, matching native-mode files, so they require HDF5 1.14.x or later to read. They remain readable by upstream netcdf-c when it is linked against HDF5 1.14.x or later.

### Implementation Guidance

- Set the file access property list bounds based on the create mode.
- For native NEXTCDF-4: `H5F_LIBVER_LATEST` for both lower and upper bound (Superblock v3).
- For `NC_NETCDF4_MODEL`: use `H5F_LIBVER_LATEST` to write Superblock v3 files, but do not create any of the new NEXTCDF-4 types.
- For `NC_CLASSIC_MODEL`: use bounds that ensure broad readability with upstream netcdf-c classic-model behavior.
- Add a runtime check that refuses to create native-mode files when the linked HDF5 library is too old.

## Float16

### What It Is

Float16 is the IEEE 754 binary16 half-precision floating point format:

- 1 sign bit
- 5 exponent bits
- 10 mantissa bits
- Total: 16 bits (2 bytes)

It provides reduced precision and range compared to `float32`, but uses half the storage and often improves I/O throughput.

### HDF5 Datatype

HDF5 1.14.x introduces native support for float16:

```c
hid_t f16le = H5Tcopy(H5T_IEEE_F16LE);  /* little-endian half precision */
hid_t f16be = H5Tcopy(H5T_IEEE_F16BE);  /* big-endian half precision */
```

You can also compose the type explicitly:

```c
hid_t f16 = H5Tcreate(H5T_FLOAT, 2);
H5Tset_fields(f16, 15, 10, 5, 0, 10);
H5Tset_size(f16, 2);
H5Tset_ebias(f16, 15);
H5Tset_precision(f16, 16);
H5Tset_order(f16, H5T_ORDER_LE);
```

### NetCDF Mapping

NEXTCDF-4 proposes a new NetCDF type:

```c
#define NC_FLOAT16 13  /* example value; assign official nc_type */
```

Memory layout is exactly the IEEE 754 binary16 representation. Users pass arrays of `uint16_t` (raw bits) or a platform half-precision type if available.

### API Additions

Convenience functions (optional):

```c
int nc_def_var_float16(int ncid, const char *name, int ndims,
                       const int *dimids, int *varidp);
int nc_put_var_float16(int ncid, int varid, const uint16_t *data);
int nc_get_var_float16(int ncid, int varid, uint16_t *data);
```

The standard path is to use `nc_def_var` with `NC_FLOAT16` and the generic `nc_put_var`/`nc_get_var` family.

### Implementation Notes

- Float16 variables cannot be coordinate variables (no meaningful ordering).
- Fill values for `NC_FLOAT16` should be representable as binary16 NaN or a chosen half value.
- When converting between memory types, avoid silent promotion to float32 unless the user requests it.
- Float16 is only allowed when `NC_NETCDF4_MODEL` is **not** set.

## Compound Types

### What They Are

An HDF5 compound type is a collection of named members, each with its own datatype and offset. It is the HDF5 equivalent of a C `struct`.

```c
typedef struct {
    int x;
    double y;
} point_t;
```

### Creating a Compound Type

```c
hid_t compound_type_id = H5Tcreate(H5T_COMPOUND, sizeof(point_t));
H5Tinsert(compound_type_id, "x", HOFFSET(point_t, x), H5T_STD_I32LE);
H5Tinsert(compound_type_id, "y", HOFFSET(point_t, y), H5T_IEEE_F64LE);
```

### Reading Compound Metadata

```c
int nmembers = H5Tget_nmembers(compound_type_id);
for (int i = 0; i < nmembers; i++) {
    char *name = H5Tget_member_name(compound_type_id, i);
    hid_t member_type = H5Tget_member_type(compound_type_id, i);
    size_t offset = H5Tget_member_offset(compound_type_id, i);
    /* ... */
    free(name);
    H5Tclose(member_type);
}
```

### NetCDF Mapping

NetCDF-4 user-defined compound types map directly to HDF5 compound types. NEXTCDF-4 preserves this behavior unchanged:

- Member names are preserved exactly and are case-sensitive.
- Nested compound types are supported.
- Fixed-size arrays inside a compound member are supported.
- Padding and alignment follow the HDF5 type definition.

### Complex Numbers as Compound Types

NEXTCDF-4 uses HDF5 compound types to represent complex numbers:

```c
/* Single-precision complex */
hid_t cplx = H5Tcreate(H5T_COMPOUND, 2 * sizeof(float));
H5Tinsert(cplx, "r", 0, H5T_IEEE_F32LE);
H5Tinsert(cplx, "i", sizeof(float), H5T_IEEE_F32LE);

/* Double-precision complex */
hid_t dcplx = H5Tcreate(H5T_COMPOUND, 2 * sizeof(double));
H5Tinsert(dcplx, "r", 0, H5T_IEEE_F64LE);
H5Tinsert(dcplx, "i", sizeof(double), H5T_IEEE_F64LE);
```

Proposed NetCDF type constants:

```c
#define NC_COMPLEX       14  /* float complex  */
#define NC_DOUBLECOMPLEX 15  /* double complex */
```

Memory layout is `{ float r; float i; }` (or double), matching the C `_Complex` layout on most platforms. On platforms without native complex support, expose the compound layout explicitly.

### Implementation Notes

- Use `H5Tpack` only when the user requests it; otherwise preserve the declared layout.
- When writing, verify that the memory type layout matches the file type layout.
- Compound types are allowed in `NC_NETCDF4_MODEL` mode, but complex-number compound types are not.

## Reference Types

### What They Are

HDF5 references are opaque values that point to objects or data regions inside an HDF5 file. They allow one dataset or attribute to refer to another without embedding paths.

There are two reference types:

1. **Object Reference** (`H5T_STD_REF_OBJ`): an 8-byte opaque token identifying an HDF5 object (group, dataset, committed type, etc.).
2. **Region Reference** (`H5T_STD_REF_DSETREG`): an opaque token identifying a hyperslab selection within a dataset.

### Object References

```c
hdf_ref_t ref;  /* opaque 8-byte type, H5R_OBJECT */
H5Rcreate(&ref, file_id, "/g1/dset", H5R_OBJECT, H5P_DEFAULT);
```

### Region References

```c
hdf_reg_ref_t ref;  /* opaque region reference type, H5R_DATASET_REGION */
hid_t space_id = H5Dget_space(dset_id);
hsize_t start[2] = {0, 0};
hsize_t count[2] = {10, 20};
H5Sselect_hyperslab(space_id, H5S_SELECT_SET, start, NULL, count, NULL);
H5Rcreate(&ref, file_id, "/g1/dset", H5R_DATASET_REGION, space_id);
```

### NetCDF Mapping

NEXTCDF-4 proposes two new opaque NetCDF types:

```c
#define NC_REF_OBJECT 16
#define NC_REF_REGION 17
```

- Variables of these types store HDF5 references as opaque byte arrays.
- The NetCDF API treats them as opaque types; it does **not** dereference them.
- Future API extensions may add dereferencing functions, but that is out of scope for the initial rewrite.

### Reading and Writing

Because references are file-local tokens, they cannot be copied meaningfully between files. This means:

- Reading references from one file and writing them to another is generally undefined.
- Reference-typed variables must be read and written as whole arrays; per-element hyperslab writes are allowed only within the same file.

### Restrictions

- Reference types cannot be coordinate variables.
- Reference types cannot appear inside compound types in the initial implementation.
- Reference types are not allowed in `NC_NETCDF4_MODEL` mode.
- Region references require the referenced dataset to remain open; keep the file open while reading region references.

### Implementation Guidance

- Detect `H5T_REFERENCE` with `H5Tget_class(type_id) == H5T_REFERENCE`.
- Use `H5Tget_ref_type(type_id)` to distinguish `H5R_OBJECT` from `H5R_DATASET_REGION`.
- Store references as opaque blobs in memory. Do not interpret the bytes.
- When writing, create the dataset with the exact HDF5 reference datatype.

## Bitfield Types

HDF5 provides atomic bitfield types (`H5T_BITFIELD`) for storing raw bit patterns without numeric interpretation. Predefined sizes are 8, 16, 32, and 64 bits: `H5T_STD_B8LE/BE`, `H5T_STD_B16LE/BE`, `H5T_STD_B32LE/BE`, `H5T_STD_B64LE/BE`.

Classic netcdf-c does not support bitfields. NEXTCDF-4 maps them to new base NetCDF types because they appear in existing HDF5 files (for example, packed quality flags and bitmasks in HDF-EOS and remote-sensing products):

```c
#define NC_BITFIELD8  18  /* maps to H5T_STD_B8LE/BE  */
#define NC_BITFIELD16 19  /* maps to H5T_STD_B16LE/BE */
#define NC_BITFIELD32 20  /* maps to H5T_STD_B32LE/BE */
#define NC_BITFIELD64 21  /* maps to H5T_STD_B64LE/BE */
```

Memory representation is the matching unsigned integer size (`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`). The NetCDF API treats the values as unsigned integers; decoding individual bits is the caller's responsibility.

### Implementation Guidance

- Detect bitfields with `H5Tget_class(type_id) == H5T_BITFIELD`.
- Map the HDF5 bitfield size to the matching `NC_BITFIELD*` type when reading.
- Write `NC_BITFIELD*` variables with the matching `H5T_STD_B*LE/BE` datatype to preserve round-trip fidelity.
- Bitfield types are **not** allowed in `NC_NETCDF4_MODEL` mode because upstream netcdf-c cannot create them.

## Dimension Scales and Dimension Mapping

NetCDF-4 stores dimensions and their relationship to variables using HDF5 dimension scales and a set of hidden attributes.

### HDF5 Dimension Scales

An HDF5 dimension scale is a special 1-D dataset that represents a dimension. A variable dataset can attach one or more dimension scales to its dataspace dimensions using `H5DSattach_scale()`.

Key attributes involved:

- `CLASS = "DIMENSION_SCALE"` on the dimension scale dataset.
- `NAME` on the dimension scale dataset (usually the dimension name).
- `REFERENCE_LIST` on the dimension scale dataset: lists variables that use this scale.
- `DIMENSION_LIST` on the variable dataset: lists attached dimension scales.

### NetCDF-4 Dimension Mapping Attributes

Because dimension scales alone are slow to match at file open and cannot represent every NetCDF semantic (for example, multi-dimensional coordinate variables), netcdf-c also stores dimension information in hidden attributes:

- `_Netcdf4Coordinates` — stores the list of dimension IDs (`dimids`) for a variable. Allows a variable to be mapped to its dimensions without reading all dimension scale metadata.
- `_Netcdf4Dimid` — stores the `dimid` on the dimension scale dataset itself.

Using `_Netcdf4Coordinates` for every variable:

- Avoids expensive dimension-scale matching at file open.
- Allows lazy reading of dimscale metadata.
- Applies only to newly created files; older files must fall back to dimscale matching.

### Implementation Guidance

- Write `_Netcdf4Coordinates` on every variable to preserve fast dimension mapping.
- Continue to create and attach dimension scales so the file remains valid for tools that read them directly.
- Read `_Netcdf4Coordinates` when present; fall back to dimension-scale matching when it is absent.
- Preserve `_Netcdf4Dimid` so that dimension IDs remain stable across renames and reopens.
- In `NC_NETCDF4_MODEL` mode, write the same hidden attributes that upstream netcdf-c produces.

## Type Mapping Summary

| NetCDF Type          | HDF5 Native Type                 | Allowed in `NC_NETCDF4_MODEL` |
|----------------------|----------------------------------|-------------------------------|
| `NC_BYTE`            | `H5T_STD_I8LE/BE`                | Yes                           |
| `NC_UBYTE`           | `H5T_STD_U8LE/BE`                | Yes                           |
| `NC_SHORT`           | `H5T_STD_I16LE/BE`               | Yes                           |
| `NC_USHORT`          | `H5T_STD_U16LE/BE`               | Yes                           |
| `NC_INT`             | `H5T_STD_I32LE/BE`               | Yes                           |
| `NC_UINT`            | `H5T_STD_U32LE/BE`               | Yes                           |
| `NC_INT64`           | `H5T_STD_I64LE/BE`               | Yes                           |
| `NC_UINT64`          | `H5T_STD_U64LE/BE`               | Yes                           |
| `NC_FLOAT`           | `H5T_IEEE_F32LE/BE`              | Yes                           |
| `NC_DOUBLE`          | `H5T_IEEE_F64LE/BE`              | Yes                           |
| `NC_STRING`          | `H5T_STRING`                     | Yes                           |
| User enum            | `H5T_ENUM`                       | Yes                           |
| User compound        | `H5T_COMPOUND`                   | Yes                           |
| User opaque          | `H5T_OPAQUE`                     | Yes                           |
| User vlen            | `H5T_VLEN`                       | Yes                           |
| `NC_FLOAT16`         | `H5T_IEEE_F16LE/BE`              | No                            |
| `NC_BFLOAT16`        | `H5T_FLOAT_BFLOAT16LE/BE`         | No                            |
| `NC_FLOAT8_E4M3`     | `H5T_FLOAT_F8E4M3`               | No                            |
| `NC_FLOAT8_E5M2`     | `H5T_FLOAT_F8E5M2`               | No                            |
| `NC_FLOAT6_E2M3`     | `H5T_FLOAT_F6E2M3`               | No                            |
| `NC_FLOAT6_E3M2`     | `H5T_FLOAT_F6E3M2`               | No                            |
| `NC_FLOAT4_E2M1`     | `H5T_FLOAT_F4E2M1`               | No                            |
| `NC_COMPLEX`         | `H5T_COMPOUND {r:f32, i:f32}`    | No                            |
| `NC_DOUBLECOMPLEX`   | `H5T_COMPOUND {r:f64, i:f64}`    | No                            |
| `NC_REF_OBJECT`      | `H5T_STD_REF_OBJ`                | No                            |
| `NC_REF_REGION`      | `H5T_STD_REF_DSETREG`            | No                            |
| `NC_BITFIELD8`       | `H5T_STD_B8LE/BE`                | No                            |
| `NC_BITFIELD16`      | `H5T_STD_B16LE/BE`               | No                            |
| `NC_BITFIELD32`      | `H5T_STD_B32LE/BE`               | No                            |
| `NC_BITFIELD64`      | `H5T_STD_B64LE/BE`               | No                            |

## Detecting HDF5 Version at Runtime

```c
unsigned major, minor, release;
H5get_libversion(&major, &minor, &release);

if (major < 1 || (major == 1 && minor < 14)) {
    /* reject Superblock v3 and NEXTCDF-4 native features */
}
```

## Common HDF5 Datatype Checks

```c
/* Is this a float16 type? */
if (H5Tget_class(type_id) == H5T_FLOAT && H5Tget_size(type_id) == 2) {
    /* float16 */
}

/* Is this a compound type? */
if (H5Tget_class(type_id) == H5T_COMPOUND) {
    /* compound */
}

/* Is this a reference type? */
if (H5Tget_class(type_id) == H5T_REFERENCE) {
    H5R_type_t rt = H5Tget_ref_type(type_id);
    switch (rt) {
        case H5R_OBJECT:        /* object reference */ break;
        case H5R_DATASET_REGION: /* region reference */ break;
        default: break;
    }
}

/* Is this a bitfield type? */
if (H5Tget_class(type_id) == H5T_BITFIELD) {
    size_t sz = H5Tget_size(type_id);  /* 1, 2, 4, or 8 */
    /* map to NC_BITFIELD8/16/32/64 */
}
```

## HDF5 2.1.1 Notes

HDF5 2.1.1 (released 2026-03-23) is a maintenance release in the HDF5 2.x line. For NEXTCDF-4 the most relevant additions are new small floating-point datatypes and build-time caveats.

### New Small Floating-Point Types

HDF5 2.x added predefined non-IEEE floating-point types. NEXTCDF-4 maps them to new base NetCDF types:

| HDF5 macro | NEXTCDF-4 type | Format | Memory |
|------------|----------------|--------|--------|
| `H5T_FLOAT_BFLOAT16LE/BE` | `NC_BFLOAT16` | 16-bit: 1 sign, 8 exponent, 7 mantissa | `uint16_t` |
| `H5T_FLOAT_F8E4M3` | `NC_FLOAT8_E4M3` | 8-bit: 1 sign, 4 exponent, 3 mantissa | `uint8_t` |
| `H5T_FLOAT_F8E5M2` | `NC_FLOAT8_E5M2` | 8-bit: 1 sign, 5 exponent, 2 mantissa | `uint8_t` |
| `H5T_FLOAT_F6E2M3` | `NC_FLOAT6_E2M3` | 6-bit: 1 sign, 2 exponent, 3 mantissa | `uint8_t` |
| `H5T_FLOAT_F6E3M2` | `NC_FLOAT6_E3M2` | 6-bit: 1 sign, 3 exponent, 2 mantissa | `uint8_t` |
| `H5T_FLOAT_F4E2M1` | `NC_FLOAT4_E2M1` | 4-bit: 1 sign, 2 exponent, 1 mantissa | `uint8_t` |

Important caveats:

- These are **predefined datatypes**, not native hardware types. HDF5 emulates conversions in software; no specialized hardware instructions are used.
- Native support for FP4/FP6/FP8/bfloat16 has **not** been added, so in-memory types must match the file type exactly to avoid slow or incomplete conversions.
- FP6 and FP4 are stored in 1-byte HDF5 datatypes even though their precision is 6 and 4 bits.
- Datatype conversions involving these non-IEEE types are currently incomplete; values may be converted incorrectly if the memory type differs.
- `_Float16` detection can fail on some systems (e.g., macOS 14); disable with `HDF5_ENABLE_NONSTANDARD_FEATURE_FLOAT16=OFF` if needed.
- These types require HDF5 2.1.1+ and are not allowed in `NC_NETCDF4_MODEL` mode.

### Build and CMake Changes in 2.1.x

- New `Findlibaec.cmake` module for locating libaec-built-with-Autotools when SZIP support is enabled.
- CMake export targets are split: static libraries use `${HDF5_EXPORTED_TARGETS}_static`, Java uses `${HDF5_EXPORTED_TARGETS}_java`, shared libraries continue using `${HDF5_EXPORTED_TARGETS}`.
- The old GitHub tag format `hdf5_Major_Minor_Patch` has been dropped in favor of plain `Major.Minor.Patch`.

## Important Caveats

1. **Superblock v3 requires HDF5 1.14+**; do not use it when `NC_NETCDF4_MODEL` is set.
2. **Float16** is a distinct HDF5 atomic type; do not confuse it with a 16-bit integer or a user-defined type.
3. **Compound types with complex-number member names** (`r`, `i`) should be recognized as complex only when the exact layout matches and the user type name or flag indicates a complex type.
4. **References are opaque tokens**; they are meaningful only within the file that created them.
5. **Region references** require the target dataset to exist and remain valid for the lifetime of the reference read.
6. **Non-IEEE floating-point types** (FP4/FP6/FP8) require exact in-memory type matching; implicit conversion is unreliable in current HDF5 releases.

## When to Use This Skill

Use this skill when:

- Implementing the NEXTCDF-4 rewrite in the `nextcdf4/` directory.
- Mapping NetCDF types to HDF5 types for float16, complex, compound, or reference data.
- Deciding whether a feature is allowed in `NC_NETCDF4_MODEL` compatibility mode.
- Debugging datatype-class detection (`H5T_FLOAT`, `H5T_COMPOUND`, `H5T_REFERENCE`).
- Choosing HDF5 file access property list version bounds for create and open paths.
- Evaluating HDF5 2.1.1+ features such as FP4/FP6/FP8 datatypes.

## References

- HDF5 1.14 Reference Manual: https://docs.hdfgroup.org/hdf5/v1_14/index.html
- HDF5 Superblock Format: https://docs.hdfgroup.org/hdf5/v1_14/_f_m_t3.html
- HDF5 Datatypes Reference: https://docs.hdfgroup.org/hdf5/v1_14/_h5_t__u_g.html
- HDF5 Reference Types: https://docs.hdfgroup.org/hdf5/v1_14/_h5_r__u_g.html
- IEEE 754 binary16: https://en.wikipedia.org/wiki/Half-precision_floating-point_format
- NEP NEXTCDF-4 Plan: `docs/plan/NEXTCDF4_plan.md`

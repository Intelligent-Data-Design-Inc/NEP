# NEXTCDF-4 Backend

NEXTCDF-4 is a clean-room rewrite of the NetCDF-4/HDF5 backend delivered as a NEP User Defined Format (UDF) expansion pack. It provides the full NetCDF-4 enhanced data model, variable I/O, storage features, and a set of new atomic types while coexisting with netcdf-c's built-in HDF5 backend.

**Explicit selection required**: because HDF5 files already belong to netcdf-c's built-in NetCDF-4 backend, NEXTCDF-4 does not register the HDF5 magic number. Open and create NEXTCDF-4 files by passing `NC_NEXTCDF4`.

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
- netcdf-c built with UDF plugin and `NC_Dispatch` support.
- HDF5 1.14.0 or newer (required).
- HDF5 2.1.1 or newer (recommended for small floating-point and complex types).

NEXTCDF-4 availability is advertised at compile time through `nep_meta.h`:

```c
#include <nep_meta.h>

#if NEP_HAS_NEXTCDF4
/* NEXTCDF-4 is compiled in. */
#endif
```

## Compatibility Modes

| Mode | Selection | Superblock | Purpose |
|------|-----------|------------|---------|
| Native | `NC_NEXTCDF4` | v3 | Full enhanced model and all new types |
| Classic model | `NC_NEXTCDF4 | NC_CLASSIC_MODEL` | v3 | NetCDF-3 data model on HDF5 |
| NetCDF-4 model | `NC_NEXTCDF4 | NC_NETCDF4_MODEL` | v1 | Files readable by upstream netcdf-c |

**Native mode** allows all NEXTCDF-4-specific types and HDF5 1.14+ features.

**`NC_CLASSIC_MODEL`** restricts the file to the classic NetCDF-3 data model: root group only, one unlimited dimension, classic atomic types, no chunking, compression, filters, or user-defined types.

**`NC_NETCDF4_MODEL`** produces files that upstream netcdf-c linked against HDF5 1.10+ can open. It allows the standard enhanced NetCDF-4 data model (groups, user-defined types, chunking, compression) but forbids NEXTCDF-4-specific atomic types (`NC_FLOAT16`, small floats, complex, bitfield, reference) so the file remains fully compatible with upstream.

`NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` are mutually exclusive.

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
| `NC_COMPLEX` | compound `{r: f32, i: f32}` | Native only |
| `NC_DOUBLECOMPLEX` | compound `{r: f64, i: f64}` | Native only |
| `NC_BITFIELD8/16/32/64` | `H5T_STD_B*LE/BE` | Native only |
| `NC_REF_OBJECT` | `H5T_STD_REF_OBJ` | Native only |
| `NC_REF_REGION` | `H5T_STD_REF_DSETREG` | Native only |

Small floating-point and bitfield types round-trip as raw bit patterns. Complex numbers use the memory layout `{ float r; float i; }` or `{ double r; double i; }`. Reference-typed variables are read and written as opaque tokens; no NetCDF dereferencing API is provided for them.

## Storage and Metadata

NEXTCDF-4 stores NetCDF dimensions as HDF5 dimension scales and attaches them to variable datasets. It writes the following hidden attributes for fast, reliable round-tripping:

| Attribute | Location | Purpose |
|-----------|----------|---------|
| `_Nextcdf4Backend` | root group | Identifies the file as created by NEXTCDF-4 |
| `_Nextcdf4Model` | root group | Present only in `NC_NETCDF4_MODEL` mode |
| `_Netcdf4Dimid` | dimension scale | Stores the stable dimension ID |
| `_Netcdf4Coordinates` | variable | Stores the variable's dimension IDs |

These attributes are filtered from user-visible attribute lists.

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

## Reference API

NEXTCDF-4 adds functions for creating and resolving HDF5 object and region references. These are only available in native mode and are declared in the NEP public headers.

```c
#include <nep.h>

nc_ref_t ref;

/* Create an object reference to a variable, group, or user-defined type */
nc_ref_object(ncid, NC_REF_OBJ_VAR, varid, &ref);

/* Resolve a reference back to object ids */
int grpid, id;
nc_deref_object(ncid, NC_REF_OBJ_VAR, &ref, &grpid, &id);
```

Region references identify a hyperslab selection inside a variable:

```c
nc_region_ref_t region;
size_t start[2] = {0, 0}, count[2] = {1, 10};

nc_ref_region(ncid, varid, start, count, NULL, &region);
```

## Known Limitations

- `NC_NEXTCDF4` must be passed explicitly; there is no magic-number dispatch for HDF5 files.
- `nc_put_varm` and `nc_get_varm` are not implemented in NEXTCDF-4 because the mapped I/O API is deprecated.
- Object and region references round-trip as opaque tokens; no NetCDF dereferencing API is provided beyond `nc_deref_object` and `nc_deref_region`.
- Reference-typed variables cannot be coordinate variables and cannot be nested inside compound types.
- Small floating-point types (`NC_FLOAT6_E2M3`, `NC_FLOAT4_E2M1`) have no default fill value because their value spaces are too small for a useful sentinel.

## Resources

- `docs/plan/NEXTCDF4_plan.md` — full architecture, compatibility contract, and type mapping.
- `docs/roadmap.md` — detailed v4.0.0 implementation sprints and acceptance criteria.
- NetCDF-C UDF documentation: https://docs.unidata.ucar.edu/netcdf/NUG/user_defined_formats.html

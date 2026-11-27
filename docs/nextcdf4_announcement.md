# NEXTCDF-4 Announcement — Draft

Draft announcement text for the netCDF mailing list and LinkedIn, describing
the NEXTCDF-4 backend in the NetCDF Expansion Pack (NEP) v4.3.0. Edit freely
before posting.

---

## Mailing List Post (netcdf mailing list)

**Subject:** ANN: NEXTCDF-4 — a clean-room NetCDF-4/HDF5 backend with new atomic types, SWMR, and parallel I/O (NEP v4.3.0, experimental)

Hello netCDF community,

I'd like to announce NEXTCDF-4, a new NetCDF-4/HDF5 backend shipping as part
of the NetCDF Expansion Pack (NEP) v4.3.0. NEXTCDF-4 is a clean-room rewrite
of the netcdf-c `libhdf5` backend, delivered as a User Defined Format (UDF)
expansion pack on the netcdf-c dispatch layer — it coexists with the built-in
NetCDF-4 backend rather than replacing it.

NEXTCDF-4 is experimental and off by default. It is being released now so the
community can try the new features and provide feedback.

### What it provides

- The full NetCDF-4 enhanced data model: groups, user-defined types
  (compound, enum, opaque, vlen — including nested and recursive
  combinations), `NC_STRING`, unlimited dimensions, chunking, deflate/szip
  compression, generic HDF5 filter plugins, quantization, endianness and
  fill control.
- A `NC_NETCDF4_MODEL` compatibility mode that writes files readable by
  unmodified upstream netcdf-c linked against HDF5 1.10+, plus a
  `NC_CLASSIC_MODEL` mode restricted to the classic NetCDF-3 data model.
  NEXTCDF-4 also reads ordinary upstream NetCDF-4 files.
- New atomic types in native mode, built on HDF5 2.1.1:
  - `NC_FLOAT16` and `NC_BFLOAT16` half-precision floats, with NEP's own
    round-to-nearest-even conversion engine and typed float/double I/O.
  - HDF5 2.x small floating-point types: `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`,
    `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, `NC_FLOAT4_E2M1` (relevant to AI/ML
    data interchange).
  - `NC_COMPLEX`/`NC_DOUBLECOMPLEX` and `NC_BITFIELD8/16/32/64`.
- HDF5 reference types: `NC_STD_REF` variables and attributes storing object,
  region, and attribute references, plus a new `nc_ref_*`/`nc_deref_*` API.
  References can point into *other files*: `nc_ref_open_target()` opens an
  external token's target as a caller-owned NEXTCDF-4 session and the
  `nc_deref_*` calls resolve variables, groups, committed datatypes and
  attributes against it. Resolution is by HDF5 object identity, so renames
  never break a reference.
- Remote references: a stored `(URL, object path)` convention lets
  `NC_STRING` variables/attributes point into datasets on other servers.
  `nc_ref_open_remote()`/`nc_ref_resolve()` resolve them through
  `nc_open()`, so any remote-capable NetCDF-C dispatch answers — OPeNDAP
  (DAP2/DAP4) and NCZarr `file://`/byte-range stores — returning an
  ordinary read-only upstream session. Optional, via
  `-DNEP_ENABLE_REMOTE_REFS=ON`.
- SWMR: a serial single-writer/multiple-reader interface for
  streaming/publishing use cases. Writers opt in with `NC_SWMR_WRITE` or
  `nc_start_swmr_write()` and publish appended records with
  `nc_swmr_flush()`/`nc_sync()`; independent readers open with
  `NC_SWMR_READ` and pull in new records with `nc_swmr_refresh()`.
  Multiple readers and either close order are supported.
- Parallel I/O: `nc_create_par`/`nc_open_par`/`nc_var_par_access` over MPI-IO
  with collective metadata, tested against both Open MPI and MPICH stacks.
- Fortran binding (`use nep_nextcdf4`) with mode/type constants and
  float16/bfloat16 I/O.

### Requirements

- netcdf-c with the 64-slot UDF scheme (`main`, or 4.10.2 once released)
- HDF5 2.1.1 or newer
- CMake build: `cmake -B build -DNEP_ENABLE_NEXTCDF4=ON`
  (add `-DNEP_ENABLE_NEXTCDF4_PARALLEL=ON` for MPI-IO)

Because the HDF5 magic number belongs to the built-in backend, NEXTCDF-4
files are selected explicitly by passing `NC_NEXTCDF4` to `nc_create`/`nc_open`.

```c
nc_create("example.nc", NC_NEXTCDF4 | NC_CLOBBER, &ncid);
```

### Status and feedback

NEXTCDF-4 is under active development; not all NetCDF-4 features are
implemented yet (notably `nc_put_varm`/`nc_get_varm`, in-memory/diskless
files, and some parallel paths for string/vlen/reference data). Bug reports,
test results, and feedback are very welcome.

- Repository: https://github.com/Intelligent-Data-Design-Inc/NEP
- Documentation: `docs/nextcdf4.md` (user guide), `docs/refs.md`
  (reference API), `docs/NEXTCDF4_float16.md` (half-precision guide)

Edward Hartnett
Intelligent Data Design, Inc.

---

## LinkedIn Post

Excited to share NEXTCDF-4 — a clean-room rewrite of the NetCDF-4/HDF5
backend, released experimentally in the NetCDF Expansion Pack (NEP) v4.3.0.

NEXTCDF-4 plugs into the netCDF-C dispatch layer as a User Defined Format,
coexisting with the built-in NetCDF-4 backend while adding capabilities the
format has never had:

- Half-precision and sub-byte floating point: NC_FLOAT16, NC_BFLOAT16, and
  the new HDF5 2.x 8/6/4-bit floats (FP8 E4M3/E5M2, FP6, FP4) — directly
  relevant to AI/ML data exchange.
- HDF5 object, region, and attribute references as a first-class netCDF
  type — including dereferenceable references into external files via
  nc_ref_open_target(), so datasets can link across file boundaries.
- Remote references into datasets on other servers: a stored
  (URL, object path) convention resolved over OPeNDAP (DAP2/DAP4) and
  NCZarr byte-range access — NetCDF files that link to remote data
  without copying it.
- SWMR (single-writer/multiple-reader) streaming for live data
  publication: one writer appends and flushes, independent readers
  refresh to see new records without reopening.
- MPI-IO parallel I/O with collective/independent access, tested on both
  Open MPI and MPICH.
- Full enhanced model support: groups, compound/enum/opaque/vlen types,
  strings, compression and filter plugins, quantization — plus a
  compatibility mode that produces files readable by unmodified netCDF-C.

Built on HDF5 2.1.1 and netcdf-c's new 64-slot UDF mechanism. It's
experimental — feedback, bug reports, and contributions welcome.

https://github.com/Intelligent-Data-Design-Inc/NEP

#netcdf #hdf5 #scientificdata #opensource #geoscience #dataformats

---

## Notes for the author

- The mailing-list post assumes the v4.3.0 tag exists; adjust the version if
  the announcement goes out before release.
- `nc_put_varm`/`nc_get_varm` (mapped I/O) are deliberately unimplemented —
  upstream has deprecated mapped I/O, so this may not need to be framed as a
  gap.
- If the target list is `netcdfgroup@unidata.ucar.edu`, keep the plain-text
  style; the markdown formatting above can be flattened for the email body.

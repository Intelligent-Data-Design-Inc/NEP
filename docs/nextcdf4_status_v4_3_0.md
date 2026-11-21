# NEXTCDF-4 Implementation Status — v4.3.0

Status of the NEXTCDF-4 backend as of the v4.3.0 development cycle: what is
implemented, what is stubbed or broken, and what must be done before the
backend can be called production-ready. Companion documents:

- `docs/plan/NEXTCDF4_plan.md` — architecture and design contract.
- `docs/nextcdf4.md` — user-facing reference.
- `docs/nextcdf4_architecture.md` — internals (`@page`).
- `docs/roadmap.md` — v4.3.0 sprints; `docs/roadmap_v3.md` — original v4.0.0
  NEXTCDF-4 sprints.

NEXTCDF-4 is **experimental and off by default** (`-DNEP_ENABLE_NEXTCDF4=ON`,
HDF5 2.1.1+, netcdf-c 4.10.1+ with UDF support). It occupies UDF slot 9
(`NC_UDF9`, exposed as `NC_NEXTCDF4`) and requires explicit mode-flag
selection for both create and open. Source lives in `src/nextcdf4/` (~8,700
lines across 11 C files plus `nxt4internal.h`/`nxt4err.h`).

## Implemented

### File lifecycle and modes

- `nc_create`/`nc_open`/`nc_close`/`nc_abort`/`nc_sync`/`nc_redef`/`nc_enddef`
  (`nxt4create.c`, `nxt4open.c`, `nxt4file.c`).
- Superblock v3 (`H5F_LIBVER_LATEST`) for native and `NC_CLASSIC_MODEL` files;
  Superblock v1 (`H5F_LIBVER_V110`) for `NC_NETCDF4_MODEL` files.
- Compatibility modes enforced by `NEXTCDF4_check_atomic_type()`
  (`nxt4meta.c`): classic model = 6 classic atomic types, root group only, one
  unlimited dim; netcdf-4 model = full enhanced model minus NEXTCDF-4-only
  types. Verified: `NC_NETCDF4_MODEL` files open under upstream netcdf-c
  (no `NC_NEXTCDF4` flag).
- Hidden markers `_Nextcdf4Backend`, `_Nextcdf4Model`, `_Netcdf4Dimid`,
  `_Nextcdf4Varid`, `_Netcdf4Coordinates`; reserved-name filtering covers
  upstream's `_NCProperties`, `_IsNetcdf4`, `_SuperblockVersion`, `_Format`,
  `_nc3_strict` as well.
- `.ncrc` autoload via `NETCDF.UDF9.LIBRARY`/`NETCDF.UDF9.INIT`;
  `nc_inq_format` reports `NC_FORMAT_NETCDF4`, `nc_inq_format_extended`
  reports `NC_FORMATX_UDF9` plus effective mode.

### Metadata

- Dimensions (fixed and unlimited), nested groups with all group inquiries,
  attributes (put/get/inq/rename/delete).
- HDF5 dimension scales with `_Netcdf4Coordinates` fast mapping and
  dimscale-iteration fallback for files lacking it (upstream `NC_NETCDF4`
  files with atomic variables open successfully).
- `nc_rename_dim`/`nc_rename_var` with the atomic flush-per-rename design
  from the plan (`nxt4meta.c`); `nc_rename_att`.

### Data I/O and storage

- `nc_put/get_vara`, `nc_put/get_vars` hyperslab I/O (`nxt4io.c`).
- Internal conversion engine `nx4_convert_type()` (`nxt4convert.c`): typed
  calls for all numeric conversions plus quantized writes, with
  complete-then-`NC_ERANGE` semantics (#381); NEP round-to-nearest-even for
  `NC_BFLOAT16`/`NC_FLOAT16`.
- Storage setters: `nc_def_var_chunking` (chunked/contiguous, `NC_COMPACT`
  rejected), `nc_def_var_deflate`, `nc_def_var_fletcher32`,
  `nc_def_var_szip`, `nc_def_var_filter` (generic plugin filters),
  `nc_def_var_endian`, `nc_def_var_fill`, `nc_def_var_quantize`
  (`nxt4meta.c`, `nxt4filter.c`, `nxt4io.c`).
- Filter pipeline on the shared `NC_VAR_INFO_T::filters` list via upstream's
  exported `NC4_hdf5_*` helpers; DCPL load at open marks variables with a
  missing mandatory plugin unreadable (`NC_ENOFILTER` on I/O);
  `nc_inq_var_filter_ids`/`nc_inq_var_filter_info`/`nc_inq_filter_avail`
  implemented (`nxt4filter.c`).

### Types

- All standard atomic types plus `NC_STRING` (see caveats below).
- Native-mode-only atomic types: `NC_FLOAT16`, `NC_BFLOAT16`,
  `NC_FLOAT8_E4M3/E5M2`, `NC_FLOAT6_E2M3/E3M2`, `NC_FLOAT4_E2M1`,
  `NC_COMPLEX`/`NC_DOUBLECOMPLEX` (native HDF5 2.x `H5T_COMPLEX_IEEE_F*` —
  not the compound `{r,i}` layout the plan describes; see doc deltas),
  `NC_BITFIELD8/16/32/64`, `NC_STD_REF` (`nxt4meta.c` `map_nc_type` /
  `NEXTCDF4_map_hdf_type`).
- Reference API (`nxt4ref.c`, declared in `nep.h`): `nc_ref_object` /
  `nc_deref_object`, `nc_ref_region` / `nc_deref_region` (regular strided
  hyperslabs), `nc_ref_attr` / `nc_deref_attr`, external-file creators
  `nc_ref_*_ext`, `nc_ref_get_type`/`nc_ref_get_file_name`/
  `nc_ref_get_obj_name`, `nc_ref_free`.
- User-defined types can be **defined and inquired**: `nc_def_compound` +
  `nc_insert_compound` (scalar fields), `nc_def_vlen`, `nc_def_enum` +
  `nc_insert_enum` + enum inquiries, `nc_def_opaque`, `nc_inq_user_type`,
  `nc_inq_typeid`, `nc_inq_typeids`, `nc_inq_type_equal`; committed
  datatypes are loaded at open (`load_one_type`). **But variables and
  attributes cannot use them — see gaps below.**
- `nc_*_bfloat16` / `nc_*_float16` typed APIs in `libnep`
  (`src/nep_bfloat16.c`, `src/nep_float16.c`).

### Diagnostics

- `ELOG(())` logging (`NEP_LOG_LEVEL` env / `nep_set_log_level()`, compiled
  out with `-DNEP_ENABLE_LOGGING=OFF`), `EBAIL`/`EBAIL2` error macros with
  HDF5 stack dump (`nxt4err.h`).

## Unimplemented — dispatch entries that error

| API | Dispatch entry | Return |
|-----|---------------|--------|
| `nc_get_varm`/`nc_put_varm` | `NEXTCDF4_get_varm`/`put_varm` (`nxt4io.c`) | `NC_ENOTBUILT` — deliberate; mapped I/O is deprecated upstream |
| `nc_insert_array_compound` | `NEXTCDF4_insert_array_compound` (`nxt4type.c`) | `NC_ENOTBUILT` |
| `nc_put_vlen_element`/`nc_get_vlen_element` | `nxt4type.c` | `NC_ENOTBUILT` |
| `nc_set_fill` | `NC_RO_set_fill` stub | `NC_EPERM` — file-level fill mode cannot be changed |
| `nc_rename_grp` | `NC_NOTNC4_rename_grp` stub | `NC_ENOTNC4` — group rename unimplemented (dims/vars do rename) |
| `nc_set_var_chunk_cache`/`nc_get_var_chunk_cache` | `NC_NOTNC4_*` stubs | `NC_ENOTNC4` |
| `nc_var_par_access` | `NC_NOTNC4_var_par_access` stub | `NC_ENOTNC4` — no parallel I/O |

## Broken / incomplete — verified against the built backend

Probed with a small program linked against `build/src/libncnextcdf4.so`
(HDF5 2.1.1, netcdf-c 4.10.x, 2026-11):

| # | Gap | Evidence |
|---|-----|----------|
| 1 | **Variables/attributes of user-defined types are unusable.** `nc_def_var` and `nc_put_att` call `NEXTCDF4_check_atomic_type`, which returns `NC_EBADTYPE` for any non-atomic typeid. Compound/vlen/enum/opaque types can be defined but no variable or attribute can use them. | `nc_def_var(ncid, "cv", compound_typeid, ...)` → `NC_EBADTYPE` |
| 2 | **Files containing user-typed or `NC_STRING` variables cannot be opened.** `load_one_var` resolves the dataset type through `map_nc_type`, which has no committed-type, vlen, or variable-length-string case → `NC_EBADTYPE` fails `nc_open`. This breaks the plan's "full read compatibility with existing NetCDF-4/HDF5 files" goal for a large class of upstream files. | NEXTCDF-4's own file with an `NC_STRING` var fails to reopen; an upstream `NC_NETCDF4` file with a compound or `NC_STRING` var fails to open; same file with only `NC_INT` opens fine |
| 3 | **`NC_STRING` attributes are silently lost on reopen.** File opens, but `nc_get_att_string` returns `NC_ENOTATT` — the variable-length-string attribute is not loaded. | probe: `put_att_string` → close → open → `get_att_string` = `NC_ENOTATT` |
| 4 | **Six tests disabled** (`test/nextcdf4/CMakeLists.txt`): `tst_nextcdf4_float16`, `tst_nextcdf4_complex`, `tst_nextcdf4_bitfield`, `tst_nextcdf4_ref`, `tst_explore` abort in netcdf-c `NC_reclaim_data` (`USEFILEINFO` assertion) after reopen; `tst_rename` (the ported upstream `nc_test4` test) disabled earlier. | #379; `docs/releases/v4.2.0.md` |
| 5 | **Def-var setters recreate the HDF5 dataset** (`nc_def_var_fill`/`chunking`/`deflate`/`endian`/`fletcher32`/`filter`), leaving an orphaned dataset object and a stale dimension-scale reference. The contiguous→chunked-deflate-0 half was fixed in Sprint 1C; the orphan remains. | #382 |
| 6 | **`NC_BFLOAT16` fill values don't work**: `_FillValue` attribute write rejected, `nc_def_var_fill` value not recovered on reopen. | #377 |
| 7 | **Installed `nextcdf4dispatch.h` is unusable downstream** — it includes `config.h` (not installed) and `ncdispatch.h`, so `NC_NEXTCDF4_initialize()` can't be declared by applications outside the tree. | #378; header unchanged |
| 8 | ~~**Small-float coordinate variables** — the design banned them but nothing enforces it.~~ Done in Sprint 1L1: allowed and documented; `tst_nextcdf4_var` covers bf16/f16 coord vars. | #380 |
| 9 | **Conversion reads take an extra full-buffer pass** (performance, not correctness). | #386 |
| 10 | **Mode flags not validated**: `NC_DISKLESS` is accepted but silently creates a file on disk; `NC_MPIIO`/`NC_MPIPOSIX`/`NC_SHARE`/`NC_PERSIST`/`NC_INMEMORY` are likewise neither honored nor rejected. | probe: `NC_NEXTCDF4|NC_DISKLESS` creates a real file |
| 11 | **No provenance attributes written** — upstream writes `_NCProperties`, `_IsNetcdf4`, `_SuperblockVersion`; NEXTCDF-4 filters them on read but never writes them (planned `nxt4provenance.c` doesn't exist). | `nxt4meta.c` reserved list |
| 12 | **`nc_inq_var_all` filter outputs never populated** (`idp`/`nparamsp`/`params` always empty) — noted in code as matching upstream's shared helper. | `nxt4meta.c` `NEXTCDF4_inq_var_all` |

Open-issue bookkeeping: #376 (typed bfloat16 read no-op — verified
resolved by `test_issue_376` in `tst_nextcdf4_conv.c`), #381 (`NC_ERANGE` —
implemented in Sprint 1H) and #372 (HDF5 detection — `cmake/NEPFindHDF5.cmake`,
imported targets, SONAME check, CMake 3.19, Spack gating all landed in Sprint 1)
were closed on 2026-11-21 with explanatory comments, as was #380
(small-float coordinate variables — Sprint 1L1). Still open and unresolved:
#374 (conda-forge submission is Sprint 2 work), #377, #378, #379, #382,
#386.

## Planned but not started

From `docs/plan/NEXTCDF4_plan.md` phases and `docs/roadmap_v3.md` v4.1.0
Sprint 1 (deferred when v4.1.0 shipped docs+logging instead):

- **`nextcopy`/`nextdump` tools** (Phase 4) — `nccopy`/`ncdump`
  replacements that understand the new types. No source exists.
- **Fortran bindings** — the plan's goal "expose all new C APIs in the
  Fortran API in the same release" is unmet: no Fortran wrappers for
  `nc_*_bfloat16`, `nc_*_float16`, `nc_ref_*`/`nc_deref_*`, or the new
  `nc_type` constants; `ftest/` has no NEXTCDF-4 test.
- **Full upstream `nc_test4` suite validation** (Phase 5) — only
  `nc_test4/tst_rename.c` was ported into `test/nextcdf4/nc_test4/`, and it
  is disabled.
- **Provenance/`_NCProperties`** (`nxt4provenance.c` in the plan's source
  layout) and **in-memory image files** (`nxt4image.c`, optional) — not
  implemented.
- **SWMR** — listed as an open question; not implemented.
- **CI coverage** — NEXTCDF-4 tests and benchmarks are not run in CI.

## Deliberate non-goals (document, don't fix)

- No magic-number dispatch: `NC_NEXTCDF4` must be passed explicitly.
- `nc_put_varm`/`nc_get_varm` (deprecated upstream).
- `nc_deref_*` do not cross into external files (`NC_ENOTNC4`); inspect with
  `nc_ref_get_file_name`/`nc_ref_get_obj_name`.
- Reference-typed vars can't be compound members; no default fill for
  `NC_STD_REF`, `NC_BITFIELD*`, `NC_FLOAT6_E2M3`, `NC_FLOAT4_E2M1`.
- A variable defined in a subgroup whose name matches an ancestor
  group's dimension is misdetected as a coordinate variable and fails
  `nc_def_var` with `NC_EHDFERR` — define coordinate variables in the
  dimension's own group (candidate fix for Sprint 1M).
- Typed calls on FP8/FP6/FP4 (HDF5 2.1.1 known problem) — raw-bits I/O only.
- Legacy `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG` datasets rejected at open.
- No changes to netcdf-c source; NEXTCDF-4 is a pure UDF expansion pack.

## Doc deltas to fix before release

- `docs/nextcdf4.md` and `NEXTCDF4_plan.md` describe `NC_COMPLEX` as the
  `{r,i}` compound layout; the code uses native HDF5 2.x `H5T_COMPLEX_*`.
  Decided 2026-11-21: keep native `H5T_COMPLEX_IEEE_F*` (already
  implemented); update the docs in Sprint 1N.
- `docs/design.md` claims compound/enum/opaque/vlen types and `NC_STRING`
  are implemented — only type *definition* is; variables/attributes and
  reopen paths are broken (gaps 1–3).
- Feature macros: `NEP_HAS_NEXTCDF4` is the only NEXTCDF-4 capability
  macro `nep_meta.h` needs; the larger per-feature set sketched in
  `docs/plan/NEXTCDF4_plan.md` is dropped (decided 2026-11-21).

## What remains before public release

Owned by v4.3.0 Sprints 1L1–1R (see `docs/roadmap.md`), ordered by severity.

**Blockers (correctness/interop):**

1. Reopen path for `NC_STRING` variables (gap 2) and silent loss of
   `NC_STRING` attributes (gap 3). → Sprint 1P
2. End-to-end user-defined types: let `nc_def_var`/`nc_put_att` accept
   user typeids, resolve committed datatypes in `load_one_var`, and
   implement `nc_insert_array_compound` + `nc_put/get_vlen_element`
   (gaps 1–2). Until then `NC_NETCDF4_MODEL` cannot honestly claim
   upstream-file compatibility. → Sprint 1N
3. Root-cause and fix the `NC_reclaim_data`/`USEFILEINFO` abort; re-enable
   the six disabled tests (#379). → Sprint 1M
4. Fix dataset-recreation orphans from def-var setters (#382) and the
   `NC_BFLOAT16` fill-value loss (#377). → Sprint 1M
5. Fix the installed `nextcdf4dispatch.h` (#378) so applications can call
   `NC_NEXTCDF4_initialize()`. → Sprint 1M
6. Validate or explicitly reject `NC_DISKLESS`/`NC_MPIIO`/`NC_SHARE`/
   `NC_INMEMORY`/`NC_PERSIST` mode flags instead of ignoring them (gap 10).
   → Sprint 1O

**Decisions — resolved 2026-11-21:**

7. #380 — resolved: small-float coordinate variables are allowed (Sprint
   1L1 ratifies and documents the code's behavior; FP8/FP6/FP4 decided
   consistently).
8. Complex storage — resolved: keep native `H5T_COMPLEX_IEEE_F*`; docs
   reconciled in Sprint 1N.
9. ~~Close #376 and #381~~ — done 2026-11-21, along with #372.
10. `nc_set_fill`/`nc_rename_grp`/per-var chunk cache — implement or
    document in Sprint 1O. Parallel I/O — deferred to the next release;
    mode flags rejected and deferral documented (Sprint 1O + release notes).
11. Provenance attributes — write `_NCProperties`/`_IsNetcdf4`/
    `_SuperblockVersion` like upstream (Sprint 1Q).

**Release-quality work:**

12. Performance: eliminate the extra conversion read pass (#386) — optional.
13. Fortran bindings for the new C APIs (Sprint 1R). `nextcopy`/`nextdump`
    (Phase 4 tools) deferred to a future release — recorded in the release
    notes (Sprint 2).
14. Port more of upstream `nc_test4` against the backend and get the
    NEXTCDF-4 suite into CI — currently owned by no sprint.
15. v4.3.0 Sprint 2 mechanics: tag, `docs/releases/v4.3.0.md`, Spack
    4.3.0, conda-forge, README/PRD/PRFAQ updates.

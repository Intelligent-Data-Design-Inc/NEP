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
selection for both create and open. Source lives in `src/nextcdf4/` (~12,100
lines across 12 C files plus `nxt4internal.h`/`nxt4err.h`).

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
  `_nc3_strict` as well. Sprint 1Q writes `_NCProperties`
  (`version=2,netcdf=...,hdf5=...,nextcdf4=...`) and `_nc3_strict` on
  classic-model files with upstream storage parity, deferred to
  `_enddef`/`nc_sync`/`nc_close` and never overwriting an existing stamp;
  `_IsNetcdf4`/`_SuperblockVersion` are virtual attributes synthesized on
  `NC_GLOBAL` name lookups like upstream's `nc4_get_att_special`.
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

- All standard atomic types including standalone and nested `NC_STRING` (Sprint 1P).
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
- Fixed-size user-defined types work end to end (Sprint 1N1):
  `nc_def_compound` + `nc_insert_compound` (scalar atomic fields),
  `nc_def_enum` + `nc_insert_enum` (integer bases) + enum inquiries,
  `nc_def_opaque`, `nc_def_vlen`, plus `nc_inq_user_type`, `nc_inq_typeid`,
  `nc_inq_typeids`, `nc_inq_type_equal`. Variables and attributes of
  compound/enum/opaque types are supported in native and
  `NC_NETCDF4_MODEL` files: untyped `nc_put/get_var*`, matching-type
  typed calls (numeric conversion of user types is rejected with
  `NC_EBADTYPE`), zero-length/user-typed attributes, fill values,
  group-scoped type visibility with parent-type inheritance, and full
  close/reopen via committed HDF5 datatypes loaded before variables
  (`load_types`/`load_one_type`, resolved by object token with an
  `H5Tequal` fallback). Files interchange with upstream netcdf-c in
  both directions (`tst_nextcdf4_user_types`). Sprint 1N2 added nested
  fixed-size compounds, enum and opaque members inside compounds,
  multidimensional fixed-size array fields, and arrays of compounds
  (`tst_nextcdf4_compound_arrays`): dependency-ordered committed-type
  writes (link creation order tracked so upstream reads types in
  dependency order), structural `nc_inq_type_equal`, cycle/duplicate/
  overlap rejection, and upstream interchange. Two upstream
  limitations exist under HDF5 2.1.1 (they are upstream bugs, not NEP
  bugs): upstream's own writer loses the last record's serialized tail
  when a compound contains nested compound members (serialized records
  need 84 bytes; storage holds n*80), and upstream's reader delivers
  the packed native layout for clean files. NEXTCDF-4 writes
  lossless declared-layout records; reads of foreign (anonymous-type)
  compound datasets go through the packed native datatype, matching
  upstream's own read capability. Sprint 1N3 added full vlen I/O and
  ownership (`tst_nextcdf4_vlen`): vlens of atomic and user-defined
  bases, nested vlens, compounds and array fields containing vlens,
  vlen variables and attributes in native and `NC_NETCDF4_MODEL`
  modes, `nc_put_vlen_element` (borrowed descriptors — the caller
  payload is stored, not copied) and `nc_get_vlen_element` (copies
  `len *` resolved base size into caller storage — upstream's
  hard-coded 4-byte get size is not reproduced), owned/deep-copied
  read results reclaimed recursively via
  `nc_reclaim_data`/`nc_reclaim_data_all` before `nc_close`,
  deep-copied attribute and fill-value storage for variable-sized
  types and upstream interchange in both directions. Sprint 1P extends
  recursive `varsized` tracking and ownership to string members/bases,
  fixes standalone string reopen, fill copies and partial-read cleanup,
  and covers bidirectional interchange plus updates to upstream input.
  New string storage is UTF-8-tagged without transcoding; legacy ASCII
  tags are supported. Dynamic filled datasets use early allocation to
  avoid read-only missing-chunk vlen-heap writes in HDF5 2.1.1, trading
  sparse storage for safe fills. `NC_STD_REF` inside type graphs and
  fixed-length HDF5 strings wider than one byte remain unsupported.
- Remote references (Sprint 1V7, `NEP_ENABLE_REMOTE_REFS` →
  `NEP_HAS_REMOTE_REFS`): stored `(URL, object path)` `NC_STRING`
  convention — no HDF5 token — resolved by
  `nc_ref_open_remote`/`nc_ref_resolve` (`nxt4remoteref.c`) through
  `nc_open`, answering DAP2/DAP4/NCZarr byte-range URLs and returning a
  caller-owned upstream session; `NC_ENOTBUILT` when the option is off.
  `tst_nextcdf4_remote_ref` under `nextcdf4_ref`; live URLs only via
  `NEP_REMOTE_TEST_URL`/`NEP_REMOTE_TEST_PATH`, never in CI.
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
| `nc_var_par_access` | `NEXTCDF4_var_par_access` | Session-only per-variable collective/independent selection (Sprint 1V2); serial sessions return `NC_ENOPAR`; filtered independent access returns `NC_EINVAL` |

Sprint 1O replaced the fill/cache/group-rename stubs: `nc_set_fill` controls a definition-time session default, cache setters/getters use real HDF5 DAPLs including live coordinate-alias reopen, and group creation/rename work in native and `NC_NETCDF4_MODEL` modes. See [NEXTCDF-4 controls](nextcdf4.md). Focused `tst_nextcdf4_fill_mode`/`tst_nextcdf4_cache` plus extended file/rename tests cover these paths in normal/sanitizer full CTest. Subgroup-only upstream files are accepted and descendant full paths are no longer truncated at `NC_MAX_NAME`.

## Broken / incomplete — verified against the built backend

Probed with a small program linked against `build/src/libncnextcdf4.so`
(HDF5 2.1.1, netcdf-c 4.10.x, 2026-11):

| # | Gap | Evidence |
|---|-----|----------|
| 1 | ~~**Variables/attributes of user-defined types are unusable**~~ — fixed in Sprint 1N1: `nc_def_var`/`nc_put_att` resolve user typeids through group-scoped lookup (`nx4_find_type`), uncommitted-type variables defer dataset creation until `_enddef`, and I/O uses the type's exact memory/file layout with matching-type or untyped access only. | Sprint 1N1 |
| 2 | ~~**Files containing `NC_STRING` variables cannot be opened**~~ — fixed in Sprint 1P: detect variable-length HDF5 strings before one-byte `NC_CHAR`, load standalone and recursive string types, preserve character-set tags, and deep-copy/reclaim string fill/read buffers. | `tst_nextcdf4_string`: native/model reopen, upstream interchange and updates, legacy ASCII, clipped reads and repeated failed-open cleanup |
| 3 | ~~**`NC_STRING` attributes are silently lost on reopen**~~ — fixed in Sprint 1P: standalone/nested string attributes load on groups and variables, recognized read failures propagate, and attribute/fill caches own independent payloads. | Focused regressions and two-mode `tst_strings`, `tst_strings2`, `tst_atts_string_rewrite` ports |
| 4 | ~~**Four tests disabled**~~ — fixed in Sprint 1M: the abort was upstream netcdf-c's `FILEINFOFLAGS` covering only UDF slots 0–1, fixed upstream by the 64-slot `UDFMODEL` scheme (netcdf-c `main`, shipping in 4.10.2). `NEP_ENABLE_NEXTCDF4` now requires 64-slot headers at configure time (`NC_UDF` + `NC_MAX_UDF_FORMATS >= 64`, `FATAL_ERROR` on 10-slot) and all four tests are re-enabled unconditionally. Note: the flag-value ABI differs between the two schemes — a NEP build runs only against the scheme it was built for. | #379 |
| 5 | ~~**Def-var setters recreate the HDF5 dataset**~~ — fixed in Sprint 1M: setters (`nc_def_var_fill`/`chunking`/`deflate`/`endian`/`fletcher32`/`filter`/`szip` via `nx4_mark_dcpl_dirty`; quantization is applied at data-conversion time and needs no rebuild) only update in-memory metadata and mark the variable dirty; `_enddef` rebuilds each dirty dataset once — detaching scales/dependents before `H5Ldelete`, re-attaching after — and rewrites user attributes in index order. Committed variables reject all dataset-property setters with `NC_ELATEDEF` (upstream parity), so written data can no longer be silently destroyed; a rebuilt var leaves one bounded orphaned object header. | #382 |
| 6 | ~~**`NC_BFLOAT16` fill values don't work**~~ — fixed in Sprint 1M: `nx4_dcpl_fill` no longer widens the fill message through `H5Tget_native_type` (which returned a 4-byte float for bf16, causing a heap over-read and storing 0); `_FillValue` loads as a normal attribute on reopen (`nc_inq_att` finds it) while `var->fill_value` is recovered from the DCPL via `NEXTCDF4_memory_type`-sized buffers. | #377 |
| 7 | ~~**Installed `nextcdf4dispatch.h` is unusable downstream**~~ — fixed in Sprint 1M: the installed header no longer includes `config.h`/`ncdispatch.h` and now declares only `NC_FORMATX_NEXTCDF4`, the create/open/abort/close/sync/inq-format entries, `NC_NEXTCDF4_initialize`/`finalize`, and `NEXTCDF4_dispatch_table` against `netcdf.h`/`netcdf_dispatch.h`/`nep.h`; private declarations moved to `src/nextcdf4/nxt4internal.h`. Sibling installed dispatch headers share the original defect — tracked in #389. | #378 |
| 8 | ~~**Small-float coordinate variables** — the design banned them but nothing enforces it.~~ Done in Sprint 1L1: allowed and documented; `tst_nextcdf4_var` covers bf16/f16 coord vars. | #380 |
| 9 | **Conversion reads take an extra full-buffer pass** (performance, not correctness). | #386 |
| 10 | ~~**Mode flags not validated**~~ — fixed in Sprint 1O: create/open reject `NC_DISKLESS`, `NC_INMEMORY`, `NC_MPIIO`/`NC_MPIPOSIX`, `NC_SHARE`, `NC_PERSIST` and `NC_MMAP` with `NC_EINVAL` before side effects. Upstream may preempt invalid combinations before dispatch. | `tst_nextcdf4_file`: missing-file and sentinel-preservation checks; memory opens use `nc_open_mem` to supply the image upstream requires |
| 11 | ~~**No provenance attributes written**~~ — fixed in Sprint 1Q: `nxt4provenance.c` stores `_NCProperties` and `_nc3_strict` with upstream's deferred-write/no-overwrite semantics and synthesizes `_IsNetcdf4`/`_SuperblockVersion` on name lookups; `tst_nextcdf4_provenance` covers stored-byte and API-level parity. | Sprint 1Q |
| 12 | **`nc_inq_var_all` filter outputs never populated** (`idp`/`nparamsp`/`params` always empty) — noted in code as matching upstream's shared helper. | `nxt4meta.c` `NEXTCDF4_inq_var_all` |
| 13 | ~~**`nc_put_att` on `_FillValue` rejected**~~ — fixed in Sprint 1L2: `_FillValue` is exempted from the reserved-att rule and validated upstream-style (len 1 → `NC_EINVAL`, type mismatch → `NC_EBADTYPE`, after data write → `NC_ELATEFILL`), mirrored into `var->fill_value`, and written to the DCPL (default `NC_FILL_*` when unset, so unwritten elements read back as fill). | Sprint 1L2 |
| 14 | ~~**Missing argument validation → crash instead of `NC_EINVAL`**~~ — fixed in Sprint 1L2: NULL `name`/`dimidsp` guards return `NC_EINVAL` before `NC_check_name`/`nc4_find_dim`. | Sprint 1L2 |
| 15 | ~~**Attribute definition order not preserved on reopen**~~ — fixed in Sprint 1L2: file/group/dataset/dimscale creation property lists get `H5Pset_attr_creation_order(TRACKED|INDEXED)`, att load iterates `H5_INDEX_CRT_ORDER`, and att replace/rename rewrite the object's attributes in index order so positions are preserved. | Sprint 1L2 |
| 16 | ~~**Rename semantics diverged from upstream**~~ — fixed in Sprint 1L2: `NEXTCDF4_rename_dim`/`rename_var` implement upstream break/reform (a renamed-away coordinate variable gets its own dimscale; renaming onto a matching name reforms the link), replacing the old name-sync divergence. `tst_nextcdf4_rename` was updated to assert upstream semantics. | Sprint 1L2 |
| 17 | ~~**Auto-mode transitions missing**~~ — fixed in Sprint 1L2: data writes auto-`enddef`; metadata calls (`def_dim`/`def_var`/`put_att`/renames) auto-`redef` like upstream (classic model still `NC_ENOTINDEFINE`); def-var *setters* stay strict `NC_ENOTINDEFINE`; reads in define mode keep NEP's `NC_EINDEFINE`. | Sprint 1L2 |
| 18 | ~~**Upstream `netCDF-4` files without `_Netcdf4Dimid` fail to open**~~ — fixed in Sprint 1L2: `find_dim_cb` accepts markerless `DIMENSION_SCALE` datasets and `load_dimensions` assigns free dimids, so upstream reference files (e.g. `ref_tst_dims.nc`) load. Also fixed: dimension-scale datasets now grow with unlimited-dim writes; coordinate variables require the dim in the same group (`dim->container`). | Sprint 1L2 |
| 19 | ~~**`H5T_STD_REF` datasets corrupted by fill value**~~ — fixed in Sprint 1L2: writing a fill message on a reference dataset makes it unreadable by `H5Dopen2`; `nx4_dcpl_fill` now skips `NC_STD_REF`. | Sprint 1L2 |

Open-issue bookkeeping: #376 (typed bfloat16 read no-op — verified
resolved by `test_issue_376` in `tst_nextcdf4_conv.c`), #381 (`NC_ERANGE` —
implemented in Sprint 1H) and #372 (HDF5 detection — `cmake/NEPFindHDF5.cmake`,
imported targets, SONAME check, CMake 3.19, Spack gating all landed in Sprint 1)
were closed on 2026-11-21 with explanatory comments, as was #380
(small-float coordinate variables — Sprint 1L1). #377, #378, #379 and
#382 were fixed in Sprint 1M; #389 (same `config.h` defect in the six
other installed dispatch headers) was filed from that sprint. Still open
and unresolved: #374 (conda-forge submission is Sprint 2 work), #386,
#389.

## Planned but not started

From `docs/plan/NEXTCDF4_plan.md` phases and `docs/roadmap_v3.md` v4.1.0
Sprint 1 (deferred when v4.1.0 shipped docs+logging instead):

- **`nextcopy`/`nextdump` tools** (Phase 4) — `nccopy`/`ncdump`
  replacements that understand the new types. No source exists.
- **Broader Fortran bindings** — Sprint 1R supplies installed `module
  nep_nextcdf4` / `libnepnextcdf4f` (`NEP::nepnextcdf4f`): initialization,
  mode/new-type constants and scalar–rank-7 float16/bfloat16 bit-buffer
  generics with optional start/count/stride. Three `nextcdf4_fortran`
  tests cover types/modes, I/O/reopen and an installed consumer in normal
  NEXTCDF-4 CI. NetCDF-Fortran >=4.6.2 and compatible compiler modules are
  required when both options are ON. The original all-new-C-APIs-in-the-same-release
  goal is explicitly narrowed: references and general enhanced-type value
  interfaces are deferred beyond v4.3.0, as is Fortran sanitizer-stack expansion.
  See [Fortran binding](nextcdf4_fortran.md).
- **Full upstream `nc_test4` suite validation** (Phase 5) — fifteen
  programs ported into `test/nextcdf4/nc_test4/` (`tst_rename` plus
  Sprint 1L2's `tst_camrun`, `tst_atts3`, `tst_dims`, `tst_vars`,
  `tst_atts1`, kept near-verbatim via the local
  `nc_tests.h`/`err_macros.h` shims). Those first six pass in native `NC_NEXTCDF4`
  mode as of Sprint 1L2; `tst_dims` also reads the shipped upstream
  fixture `ref_tst_dims.nc`. Sprint 1N4 added the six user-type ports
  `tst_enums`, `tst_opaques`, `tst_compounds`, `tst_compounds2`,
  `tst_vl` and `tst_vlen_unlim` — each registered twice (native and
  `NC_NETCDF4_MODEL`) under the `nextcdf4_user_types` CTest label,
  selected by `NEP_NC_TEST4_MODEL` through
  `nep_nc_test4_create_mode()`, with per-mode working directories —
  plus the upstream fixture `ref_tst_compounds.nc` (byte-for-byte from
  netcdf-c 5baa62e6, read through `srcdir`). Landing them required
  three backend fixes: an empty enum now fails `_enddef`/`nc_close`
  with `NC_EINVAL` (file stays open and `nc_redef` works),
  `nc_redef` in define mode is a no-op outside classic model
  (upstream parity), upstream dimscales named "This is a netCDF
  dimension but not a netCDF variable." resolve by link name, and
  reads past a dataset's physical extent on unlimited dims are
  clipped to the extent with missing elements deep-copied from the fill
  value (vlen datasets carry a zeroed-descriptor fill message).
  Sprint 1P corrects multidimensional/strided fill placement and adds
  `tst_strings`, `tst_strings2`, `tst_atts_string_rewrite` in both modes,
  plus `tst_nextcdf4_string`, under `nextcdf4_strings`. Other string,
  fill and mixed-feature ports remain future suite expansion; this is
  not full upstream-suite completion.
- **In-memory image files** (`nxt4image.c`, optional) — not implemented.
  (Provenance landed in Sprint 1Q: `nxt4provenance.c`.)
- **SWMR** — native serial writers and readers implemented in Sprints 1V4–1V5: `NC_SWMR_WRITE` or `nc_start_swmr_write`, whole-file `nc_swmr_flush`/active `nc_sync`, fixed-size atomic variables, frozen schema/attributes. No declared user types, strings/reference variables, writer compatibility modes or MPI-IO sessions. Readers open with `NC_SWMR_READ` and call `nc_swmr_refresh` to see published appends and grown dimension lengths; model-mode files qualify when their superblock is v3. Separate-process raw-HDF5 and NEP readers verify live publication, multi-reader and close ordering in normal/sanitizer full suites.
- **CI coverage** — benchmarks are not run in CI. NEXTCDF-4 tests run
  in the `nextcdf4` job (build, full CTest, docs) and, since Sprint
  1N3, in a dedicated `nextcdf4-sanitizers` job that builds
  instrumented HDF5/netcdf-c/NEP under ASan+UBSan with leak detection
  (`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`,
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`) and runs the
  full suite serially. Sprint 1S added a `nextcdf4-coverage` job
  (`nextcdf4.yml`): a `--coverage` build with Fortran enabled, full
  ctest run, and an `lcov`/`genhtml` HTML report artifact —
  report-only, warning below 50%.

## Gap analysis — remaining for the NEXTCDF-4 release

All v4.3.0 Sprint 1 sub-sprints (1 through 1S) have landed; the backend
is feature-complete for this cycle. What remains falls into three
groups.

### Release mechanics (Sprint 2, `docs/roadmap.md`)

- Tag v4.3.0 and write `docs/releases/v4.3.0.md` covering the HDF5
  detection change, CMake 3.19 minimum, small-float/bfloat16 work,
  generic filter support, reference types, and the `NC_REF_OBJECT`/
  `NC_REF_REGION` → `NC_STD_REF` breaking change.
- Spack: add `version("4.3.0")` with sha256; update
  spack/spack-packages#5557 in place.
- Conda (#374 part 2): update staged-recipes#34309 or the feedstock
  depending on merge state; move the `conda.yml` lint step to the 4.3.0
  tarball and set `context.version` to `4.3.1.dev0`.
- README/prd/prfaq/conda-plan wording updates for GeoTIFF in 4.3.0;
  document the deferrals in the release notes; close #374.

### Open issues (none release-blocking)

- #386 — conversion reads take an extra full-buffer pass (performance
  only).
- #389 — `config.h` leak in the six non-NEXTCDF-4 installed dispatch
  headers (sibling defect, not this backend).
- `nc_inq_var_all` filter outputs never populated — matches upstream's
  shared helper (gap #12 above).

### Deliberate deferrals beyond v4.3.0

- `nextcopy`/`nextdump` tools (Phase 4); `nccopy`/`ncdump` remain the
  tools for these files.
- Parallel dynamic/string/reference variable data I/O (`NC_ENOTBUILT`);
  fixed-size collective/independent access landed in Sprint 1V2 under
  `NEP_ENABLE_NEXTCDF4_PARALLEL`. Preferences reset at reopen; filtered
  writes and unlimited growth require collective access, including empty
  ranks; independent filtered reads are permitted (HDF5 parallel filter
  rule, Sprint 1V3).
- SWMR on MPI-IO or with dynamic/user/reference types, and schema changes
  visible to readers; native writer support landed in 1V4, NEP reader
  open/refresh in 1V5.
- In-memory image files (`nxt4image.c`), mapped I/O
  (`get_varm`/`put_varm`, deprecated upstream).
- `NC_STD_REF` inside user-defined type graphs; fixed-length HDF5
  strings wider than one byte.
- Remaining upstream `nc_test4` ports (string/fill/mixed-feature) —
  suite expansion continues post-release.
- Fortran: reference wrappers and general enhanced-type value
  interfaces, F77 bindings, half attributes, sanitizer-stack expansion.

## Deliberate non-goals (document, don't fix)

- No magic-number dispatch: `NC_NEXTCDF4` must be passed explicitly.
- `nc_put_varm`/`nc_get_varm` (deprecated upstream).
- `nc_deref_*` do not cross into external files (`NC_ENOTNC4`); inspect with
  `nc_ref_get_file_name`/`nc_ref_get_obj_name`.
- Reference-typed vars can't be compound members; no default fill for
  `NC_STD_REF`, `NC_BITFIELD*`, `NC_FLOAT6_E2M3`, `NC_FLOAT4_E2M1`.

- Typed calls on FP8/FP6/FP4 (HDF5 2.1.1 known problem) — raw-bits I/O only.
- Legacy `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG` datasets rejected at open.
- No changes to netcdf-c source; NEXTCDF-4 is a pure UDF expansion pack.

## Doc deltas to fix before release

- ~~`docs/nextcdf4.md` and `NEXTCDF4_plan.md` describe `NC_COMPLEX` as the
  `{r,i}` compound layout~~ — reconciled in Sprint 1N: docs now state
  the native HDF5 2.x `H5T_COMPLEX_IEEE_F*` storage (decided
  2026-11-21). Ordinary `{r,i}` compounds are ordinary user compounds.
- ~~`docs/design.md` claims vlen types and `NC_STRING` variables are
  implemented~~ — reconciled in Sprint 1N3: vlen I/O and ownership now
  work; `NC_STRING` variables remain broken (gap 2, Sprint 1P).
- Feature macros: `NEP_HAS_NEXTCDF4` is the only NEXTCDF-4 capability
  macro `nep_meta.h` needs; the larger per-feature set sketched in
  `docs/plan/NEXTCDF4_plan.md` is dropped (decided 2026-11-21).

## What remains before public release

Owned by v4.3.0 Sprints 1L1–1R (see `docs/roadmap.md`), ordered by severity.

**Blockers (correctness/interop):**

1. Reopen path for `NC_STRING` variables (gap 2) and silent loss of
   `NC_STRING` attributes (gap 3). → Sprint 1P
2. ~~End-to-end user-defined types~~ — fixed-size types (scalar atomic
   compound fields, integer enums, opaque) done in Sprint 1N1, and
   nested compounds/array fields done in Sprint 1N2, including
   committed-type reload and upstream interchange in both directions.
   Vlen I/O/ownership done in Sprint 1N3 and the selected upstream
   user-type ports in Sprint 1N4 — Sprint 1N is complete.
3. ~~Root-cause and fix the `NC_reclaim_data`/`USEFILEINFO` abort; re-enable
   the disabled tests (#379).~~ — done in Sprint 1M.
4. ~~Fix dataset-recreation orphans from def-var setters (#382) and the
   `NC_BFLOAT16` fill-value loss (#377).~~ — done in Sprint 1M.
5. ~~Fix the installed `nextcdf4dispatch.h` (#378) so applications can call
   `NC_NEXTCDF4_initialize()`.~~ — done in Sprint 1M.
6. ~~Validate unsupported memory/parallel/sharing mode flags (gap 10).~~
   Done in Sprint 1O, including `NC_MMAP`; rejection preserves existing files.

**Decisions — resolved 2026-11-21:**

7. #380 — resolved: small-float coordinate variables are allowed (Sprint
   1L1 ratifies and documents the code's behavior; FP8/FP6/FP4 decided
   consistently).
8. Complex storage — resolved: keep native `H5T_COMPLEX_IEEE_F*`; docs
   reconciled in Sprint 1N.
9. ~~Close #376 and #381~~ — done 2026-11-21, along with #372.
10. `nc_set_fill`/`nc_rename_grp`/per-var chunk cache — implemented in
    Sprint 1O, including model-mode groups and effective live cache settings.
    Parallel file access via `nc_create_par`/`nc_open_par` landed in
    Sprint 1V1; per-variable fixed-size collective/independent selection
    landed in Sprint 1V2. Sprint 1V3 ported the eight upstream
    `nc_test4` parallel tests (each in native and `NC_NETCDF4_MODEL`
    mode), proved interchange both directions, added the MPICH CI stack,
    covered external zstd/bzip2 plugins in parallel, and added the
    `parallel_perf` disk-bound benchmark. Quantize mode is now persisted
    via the upstream `_Quantize*` dataset attributes, so
    `nc_inq_var_quantize` works after reopen and upstream interchange
    sees the setting. SWMR remains with Sprints 1V4–1V5. Sprint 2 owns
    release notes.
11. ~~Provenance attributes~~ — done in Sprint 1Q: `_NCProperties` and
    `_nc3_strict` stored like upstream; `_IsNetcdf4`/
    `_SuperblockVersion` synthesized on name lookups.

**Release-quality work:**

12. Performance: eliminate the extra conversion read pass (#386) — optional.
13. Fortran bindings for the new C APIs (Sprint 1R). `nextcopy`/`nextdump`
    (Phase 4 tools) deferred to a future release — recorded in the release
    notes (Sprint 2).
14. Port more of upstream `nc_test4` against the backend — Sprint 1L2
    delivered five ports (`tst_camrun`, `tst_atts3`, `tst_dims`,
    `tst_vars`, `tst_atts1`) plus `tst_rename`, and Sprint 1N4 added
    the six user-type ports (`tst_enums`, `tst_opaques`,
    `tst_compounds`, `tst_compounds2`, `tst_vl`, `tst_vlen_unlim`)
    each running in native and `NC_NETCDF4_MODEL` modes under the
    `nextcdf4_user_types` CTest label in both `nextcdf4` and
    `nextcdf4-sanitizers` CI jobs. String/mixed-feature ports remain
    future suite-expansion work.
15. v4.3.0 Sprint 2 mechanics: tag, `docs/releases/v4.3.0.md`, Spack
    4.3.0, conda-forge, README/PRD/PRFAQ updates.

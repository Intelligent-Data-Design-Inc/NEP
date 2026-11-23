# NEP Development Roadmap

### V4.3.0 - NEXTCDF4 Development

#### Sprint 1 - Fix Issues 372 and 374
- #372: find HDF5 without a version in new `cmake/NEPFindHDF5.cmake` (1.14.0 minimum enforced, NEXTCDF-4 2.1.1 check kept); link every target through FindHDF5's `hdf5::hdf5_hl`/`HDF5::HDF5` imported targets; CMake minimum 3.19.
- Configure fails if the `libhdf5.so.N` that `libnetcdf.so` needs differs from the HDF5 found (readelf SONAME check; skipped with a status line when undeterminable).
- Spack overlay: the `h5cc` workaround is gated to `@:4.2.0`; `depends_on("cmake@3.19:", when="@4.3.0:")`.
- #374 part 1: GeoTIFF enabled in `conda/recipe/` (`geotiff`/`libtiff` host deps, `libncgeotiff` and `.tif` `ncdump` tests), `context.version` `4.3.0.dev0`; `docs/conda-plan.md` updated, including #34309's current title.
- Verification is manual (no new CTest test or CI job); all existing workflows must pass.
- Out of scope (Sprint 2): tag, release notes, Spack 4.3.0 and #5557, conda-forge submission, user-facing conda wording.

Plan: [docs/plan/v4.3.0-sprint1-fix-issues-372-374.md](plan/v4.3.0-sprint1-fix-issues-372-374.md)

#### Sprint 1A - Add NEXTCDF4 float16 Examples and Documentation
- `NC_BFLOAT16` only. Commit `test/data/netCDF-4/gdas_sfc_sample.nc` (`.gitignore` exception) and its README, which points at nep-compress's `make_gdas_sample.sh`.
- New `examples/nextcdf4/`: `bfloat16.c` (tutorial, tmp2m), `bfloat16_perf.c` (4 GDAS fields x float/bfloat16 x contiguous/shuffle+deflate 1, min of 3, CSV with size, speed and error; fails if a contiguous bfloat16 file is not smaller), `plot_bfloat16.py` (size and speed PNGs).
- Conversion is round-to-nearest-even in user code; I/O uses `nc_put_vara`/`nc_get_vara` with `uint16_t`; no fill value.
- New `NEP_ZSTD_PLUGIN_DIR` gates the `nxt4_bfloat16` ctest, plus `nxt4_bfloat16_perf` under `NEP_ENABLE_BENCHMARKS`; no CI changes.
- Commit the reference CSV, machine record and plots; document in `docs/nextcdf4.md`, `examples/examples.dox` and `examples/README.md`.
- Out of scope: other small floats, the NEXTCDF-4 defects found (#376 typed `nc_get_vara_float` no-op, #377 bfloat16 fill not recovered, #378 `nextcdf4dispatch.h` includes `config.h`), re-enabling disabled reopen tests (#379), CI for these tests, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1A-bfloat16-examples.md](plan/v4.3.0-sprint1A-bfloat16-examples.md)

#### Sprint 1B - Fix Issue of Float Conversion
- #376 doesn't reproduce: HDF5 already converts typed calls. Define the behavior in `var_io()` (`check_memtype`): `NC_FLOAT16`/`NC_BFLOAT16` accept `NC_FLOAT`/`NC_DOUBLE` through HDF5 (exact reads; HDF5 write rounding documented). FP8/FP6/FP4 are untyped only (HDF5 2.1.1 Known Problem). Other memtypes return `NC_EBADTYPE`, and text returns `NC_ECHAR`.
- New enabled test `tst_nextcdf4_conv` (exhaustive 16-bit widening, pinned HDF5 write bits, the #376 reproduction, rejections).
- `bfloat16.c` cross-checks a typed read; `bfloat16_perf` adds `bfloat16_hdf5` rows (24 total); reference CSV, machine record and plots regenerated.
- Fix the Sprint 1A "typed calls are a no-op" text in `docs/nextcdf4.md`, `docs/NEXTCDF4_float16.md`, `NEXTCDF4_plan.md`, `prd.md` and `nep.h`. Comment on and close #376.
- New `libnep` functions `nc_{get,put}_{var,var1,vara,vars}_bfloat16` with `nc_bfloat16_t` (`__bf16` when available, else `uint16_t`): raw bits for `NC_BFLOAT16`, NEP round-to-nearest-even conversion for `NC_FLOAT`/`NC_DOUBLE` with `NC_ERANGE` on overflow; new test `tst_nep_bfloat16`. The bfloat16 examples use them; benchmarks rerun and plots regenerated.
- Out of scope: `NC_ERANGE` for standard typed calls (#381), typed FP8/FP6/FP4 I/O, NEP-side rounding for standard typed calls, complex/bitfield/reference typed calls, `varm`/attribute/Fortran bfloat16 functions, #377-#379, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1B-fix-float-conversion.md](plan/v4.3.0-sprint1B-fix-float-conversion.md)

#### Sprint 1C - Refine Performance Graphs
- Reference results come from a `-DCMAKE_BUILD_TYPE=RelWithDebInfo` (-O2) build. `record_machine.sh` records the build type and flags, and the benchmarks warn when built without optimization. Project defaults stay unchanged.
- `bfloat16_size.png`/`bfloat16_speed.png` show only float32 and bfloat16 (NEP). The new `bfloat16_hdf5_speed.png` adds the HDF5-conversion rows. The `bfloat16_perf` CSV is unchanged.
- New `test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc` (4 fields as `NC_BFLOAT16`, shuffle+zstd 19 applied by `h5repack`), generated once by `make_bfloat16_sample.c`/`.sh` and committed with a README and a `.gitignore` exception.
- New `bfloat16_io_perf.c` times write+read of float32 against data already in bfloat16, with no conversion timed (4 fields x contiguous/deflate1). `plot_bfloat16_io.py` makes `docs/images/bfloat16_io.png`. Its ctest runs under `NEP_ENABLE_BENCHMARKS`.
- Update `docs/float16.md` (new HDF5 and I/O sections) and `docs/NEXTCDF4_float16.md`, and commit the pending `float16.md` work.
- #382 (found here): def-var setters made contiguous variables chunked with deflate 0. `deflate_level` now starts at -1, with a test in `tst_nextcdf4_chunking`, and the benchmarks were rerun.
- Disk-bound timing: new `examples/perf_disk.h` (`nep_perf_sync_file`, `nep_perf_drop_cache`). Write time includes `fsync`, and each read starts after `posix_fadvise(DONTNEED)`. Each file holds 10 copies of the field (`NCOPIES`) to spread the fixed open/close cost, each configuration runs 10 times (`NREPS`), and speeds are in MB/s of float32 data. Both bfloat16 benchmarks are rerun, and `record_machine.sh` records the storage device.
- Out of scope: fsync/cache drop for the other timing programs (Sprints 1D-1E), dataset recreation on setters (rest of #382), NEXTCDF-4 `nc_def_var_filter`, #377/#380, CI for the benchmarks, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1C-refine-performance-graphs.md](plan/v4.3.0-sprint1C-refine-performance-graphs.md)

#### Sprint 1D - Disk-Bound Timing in the Compression Benchmarks
- `deflate.c`, `zstandard.c` and `szip.c` use `examples/perf_disk.h` (Sprint 1C): write time includes `fsync`, each read starts after `posix_fadvise(DONTNEED)`. Structure, CSV columns and one-measurement-per-config stay unchanged.
- `lossless.c`/`quantize.c` get the source edits but no rerun (targets disabled since v4.2.0).
- Rerun on `RelWithDebInfo`, regenerate committed CSVs, plot PNGs and `_metadata.txt` captions; add a disk-bound-timing note to `docs/examples.md`, `docs/prd.md` and `examples/README.md`/`examples.dox`.
- NEXTCDF-4 docs (`docs/nextcdf4.md`, `docs/mainpage.md`, `docs/prd.md`, `docs/prfaq.md`) state the backend is experimental with unimplemented features.
- Out of scope (Sprint 1E): `cache_tuning.c`, `chunking.c`, `endianness.c`, `fill_values.c`, `examples/netcdf-4/chunking_performance.c`. Also out: min-of-N repetitions, `NCOPIES` files, MB/s columns, re-enabling lossless/quantize, CI for benchmarks, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1D-disk-bound-timing.md](plan/v4.3.0-sprint1D-disk-bound-timing.md)

#### Sprint 1E - Disk-Bound Timing in the Storage-Behavior Benchmarks
- `cache_tuning.c`, `chunking.c`, `endianness.c`, `fill_values.c` (in `examples/performance/`) and `examples/netcdf-4/chunking_performance.c` get the same `perf_disk.h` fsync + page-cache-drop timing as Sprint 1D.
- Rerun on `RelWithDebInfo`, regenerate committed CSVs, plot PNGs and `_metadata.txt` captions; `cache_tuning_validate` and the `cache_test.nc` lifecycle are preserved.
- `chunking_performance.c` keeps its stdout tutorial output; nothing committed.
- Out of scope: min-of-N repetitions, `NCOPIES` files, MB/s columns, CI for benchmarks, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1E-disk-bound-timing-storage.md](plan/v4.3.0-sprint1E-disk-bound-timing-storage.md)

#### Sprint 1F - Doxygen and Code Documentation for src/nextcdf4
- Convert every `/*` doc block in `src/nextcdf4/*.c` to `/**` with complete `@param`/`@return`; `nxt4meta.c` is the bulk (~81 blocks, ~58 functions).
- Audit `nxt4internal.h`/`nxt4err.h`; add targeted in-body "why" comments for HDFdon't wio5 idioms, ownership rules, and format quirks.
- Add the `src/` files missing from `docs/Doxyfile.in` `INPUT` (dicom, pdb, mmcif readers, `nep_bfloat16.c`, jpeglossless codec, `nep_logging.h`) and fix any warnings that surfaces; document the codec's real functions (`dicom_jpeg_lossless_precision`, `dicom_ll_*`). `#else` stubs and `#if 0` dead code stay undocumented.
- Docs build and the `ci.yml` warning grep stay warning-free; `ctest` unchanged.
- Out of scope: documenting top-level `src/*.c`, `include/`, `test/`, `examples/` beyond `INPUT`-surfaced warnings; `EXTRACT_ALL`/`WARN_IF_UNDOCUMENTED` enforcement; release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1F-doxygen-nextcdf4.md](plan/v4.3.0-sprint1F-doxygen-nextcdf4.md)

#### Sprint 1G - Convert Data Types in One Function
- Temporary CMake option `NEP_INTERNAL_CONVERT` defaults OFF (existing behavior); `NEP_HAS_INTERNAL_CONVERT` reports the effective build setting.
- ON routes supported variable/attribute numeric conversions and quantized writes through private, NEP-owned `nx4_convert_type()`, modeled on netCDF-C's `libsrc4/nc4var.c`; standard bfloat16 calls use NEP round-to-nearest-even.
- IEEE float16 conversion remains an explicit HDF5 exception until Sprint 1I; raw-bit I/O, FP8/FP6/FP4 restrictions and the independent `nc_*_bfloat16` API stay intact.
- Add enabled mode-aware conversion tests and dedicated NEXTCDF-4 OFF/ON CI; update affected API/build/rounding docs and keep CTest passing and Doxygen warning-free.
- Out of scope (Sprint 1H): #381, all benchmark reruns/artifacts, the permanent conversion-path decision, removal of the temporary switch and deferred Spack review. No temporary Spack variant or recipe changes.

Plan: [docs/plan/v4.3.0-sprint1G-convert-data-types.md](plan/v4.3.0-sprint1G-convert-data-types.md)

#### Sprint 1H - Continued Conversion Changes
- Fix #381 in variable, attribute and quantized-write conversions, including retained HDF5/float16 paths: complete I/O, then return `NC_ERANGE` for overflow/integer range errors; precision loss, fractional truncation and floating underflow are not range errors.
- Compare corrected OFF/ON engines using both GDAS benchmarks plus representative integer, float/double and quantized-write cases; optimized, disk-bound runs with separate CSVs, machine records and charts, preserving historical reference artifacts.
- Ed selected the internal engine after the OFF/ON review: `NEP_INTERNAL_CONVERT`, its capability marker and the OFF code/test/docs/CI branches are removed; setting the option now fails at configure time. Regression coverage and warning-free docs are retained.
- Close the deferred Spack review: no temporary variant; change the overlay only for an approved actual packaging-contract change, version-gated for old releases and verified with audit/spec/upstream style checks. Upstream changes ship with Sprint 2's version bump.
- Out of scope: internal IEEE float16 (Sprint 1I), independent bfloat16 API changes, other small-float conversions and unrelated defects (backlog), release notes/publication (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1H-continued-conversion-changes.md](plan/v4.3.0-sprint1H-continued-conversion-changes.md)

#### Sprint 1I - Better Documentation
- New `docs/nextcdf4_architecture.md` `@page` (UDF mechanism as in current netcdf-c main with 64 slots — no mention of the old 10-slot scheme — slot 9, dispatch table, `NEXTCDF4_*_INFO_T` state, hidden attributes, metadata/`_enddef` lifecycle, compatibility modes, EBAIL/ELOG, conversion engine), added to `INPUT` and `@ref`'d from function docs.
- Expand every `/**` block in all 9 `src/nextcdf4/*.c` files (incl. `nxt4convert.c`) and `nxt4internal.h`/`nxt4err.h` from one-liners to end-user prose: lifecycle role, brief concept explanations, `@ref` links; fix skeleton blocks (e.g. `is_reserved_att`) and stale params.
- Plain `/*` comments: a walkthrough paragraph atop each function body plus targeted "why" comments (HDF5 idioms, ownership/refcount, format quirks, `nxt4convert.c` rounding/aliasing); nothing on self-evident lines.
- `include/nextcdf4dispatch.h` and the NEXTCDF-4 parts of `include/nep.h` (`NC_NEXTCDF4`, reference API, `nc_*_bfloat16`) get the same end-user depth.
- `docs/nextcdf4.md` gains a "How it works" section; `docs/NEXTCDF4_float16.md` and `docs/mainpage.md` get a first-use-of-terms pass and pointers to the architecture page.
- Docs build stays warning-free (`WARN_AS_ERROR`); comment-only changes, `ctest` unaffected. No Spack/conda recipe changes.
- Out of scope: `test/`/`examples/` docs, other UDF readers' deep docs, `EXTRACT_ALL`/`WARN_IF_UNDOCUMENTED` enforcement, code changes, IEEE float16 (Sprint 1J), release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1I-better-documentation.md](plan/v4.3.0-sprint1I-better-documentation.md)

#### Sprint 1J - Support IEEE 754 Float16
- `nx4_convert_type()` drops the HDF5 float16 exception: NEP RNE conversion both directions, corrected boundary ((65504, 65520) → `0x7bff`, ≥ 65520 finite → ±inf + `NC_ERANGE`), single rounding from double; `nx4_conversion_dxpl()`/`nx4_conversion_exception()` removed.
- New `libnep` API `nc_{get,put}_{var,var1,vara,vars}_float16` with `nc_float16_t` (`_Float16` when `__FLT16_MAX__`, else `uint16_t`), mirroring `nc_*_bfloat16`; raw bits on `NC_FLOAT16`, NEP RNE + `NC_ERANGE` on `NC_FLOAT`/`NC_DOUBLE`; new test `tst_nep_float16`, boundary pins updated in `tst_nextcdf4_conv`/`_range`/`_internal_convert`.
- Full bfloat16 example mirror: `float16.c`/`float16_conv.h`/`float16_perf.c` (float32 / `nc_*_float16` / typed x contiguous / deflate1, disk-bound), `make_float16_sample` + committed `gdas_sfc_sample_float16.nc`, `float16_io_perf.c`, plots and committed reference artifacts. `prate_ave` subnormals pass a combined rel/abs error gate.
- Docs updated across `NEXTCDF4_float16.md`, `float16.md`, `nextcdf4.md`, `nextcdf4_architecture.md`, `prd.md`, `NEXTCDF4_plan.md`; ctest stays green and docs warning-free. No Spack/conda changes.
- Out of scope: #377-#380, #382, #386, `tst_nextcdf4_float16` re-enablement, typed FP8/FP6/FP4, float16 attributes/varm/Fortran, benchmark CI, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1J-ieee-float16.md](plan/v4.3.0-sprint1J-ieee-float16.md)

#### Sprint 1K - Do Filters work with NEXTCDF4?
- Yes: `nc_def_var_filter()` implemented on `var->filters` via netcdf-c's exported `NC4_hdf5_*` helpers in new `nxt4filter.c`; deflate/fletcher32 setters moved there with upstream parity (no removal, level-0 rule, `NC_ELATEDEF` on created vars, fletcher32-first pipeline order). Free szip/bzip2/zstd/`nc_inq_var_filter` wrappers; real `inq_filter_avail` replaces `NC_NOOP`.
- Open path loads every DCPL filter (any param count) and marks vars unreadable when a mandatory plugin is missing; `var_io` returns `NC_ENOFILTER`; `NC_CLASSIC_MODEL` rejects all filter/storage setters. Filters allowed on coord vars (ordering caveat → #382).
- New enabled `tst_nextcdf4_filter` (plugin cases runtime-gated, missing plugin via `H5Zunregister`); docs updated. No Spack/conda/CI changes.
- Out of scope: rest of #382, #377/#379/#380/#386, parallel/chunk-cache stubs, nccopy/Fortran filter APIs, filter examples, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1K-def-var-filter.md](plan/v4.3.0-sprint1K-def-var-filter.md)

#### Sprint 1L - Reference Types
- Remove `NC_REF_OBJECT`/`NC_REF_REGION` (deprecated HDF5 types) and all mentions; files holding `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG` datasets then fail `nc_open` with `NC_EBADTYPE`.
- New `NC_STD_REF` (=30) maps to `H5T_STD_REF` — 64-byte `H5R_ref_t` tokens as opaque `nc_std_ref_t`; native mode only, `NC_OPAQUE` class, allowed on variables and attributes; still no coord vars or compound members.
- New API in `src/nextcdf4/nxt4ref.c`, declared in `nep.h`: `nc_ref_object`/`nc_deref_object`, `nc_ref_region`/`nc_deref_region` (regular strided hyperslabs only), `nc_ref_attr`/`nc_deref_attr`, external-file creators `nc_ref_*_ext`, `nc_ref_get_type`/`nc_ref_get_file_name`/`nc_ref_get_obj_name`, and `nc_ref_free` (tokens hold open file references); `nc_deref_*` return `NC_ENOTNC4` for external targets.
- Rewrite `tst_nextcdf4_ref` (object/region/attr/external round-trips, token inquiry, mode rejections) and update `tst_nextcdf4_internal_convert`.
- Update `docs/nextcdf4.md`, `docs/design.md`, `docs/prd.md`, `docs/prfaq.md`, `NEXTCDF4_plan.md` and the hdf5/nextcdf4 skills; `roadmap_v3.md` and `releases/v4.0.0.md` stay historical.
- Out of scope: external deref via NetCDF, legacy ref datasets, refs in compounds/coord vars, varm/Fortran/dump tools, release notes (Sprint 2), Spack/conda.

Plan: [docs/plan/v4.3.0-sprint1L-reference-types.md](plan/v4.3.0-sprint1L-reference-types.md)

#### Sprint 1L1 - Allow float16/bfloat16 Coord Vars
Ratify the code's current behavior — small-float coordinate variables are allowed — resolving #380 by lifting the design ban rather than enforcing it. Verified by probe: fixed `NC_BFLOAT16` and unlimited `NC_FLOAT16` coord vars already define/write/reopen bit-exactly; no code change is needed for the feature itself.

- FP8/FP6/FP4 documented as allowed (untested); the `NC_STD_REF` coord-var doc ban is removed the same way.
- Fix found in-session staleness: `NEXTCDF4_rename_var` detects the coord var *after* updating its name (dead dim-rename block) and `NEXTCDF4_rename_dim` leaves the in-memory var name stale; make inquiry match post-reopen state.
- Extend `tst_nextcdf4_var` (bf16 fixed + dependent var, f16 unlimited append/reopen, `NC_ENOTNC4` in both compat modes) and add in-session rename checks to `tst_nextcdf4_rename`.
- Remove the ban from `NEXTCDF4_plan.md`, `NEXTCDF4_float16.md`, `float16.md`, `nextcdf4.md`, `refs.md`, the status doc, and both Sprint 9/10 copies in `roadmap.md`/`roadmap_v3.md`; close #380.
- Out of scope: FP8/FP6/FP4/complex/bitfield coord-var testing, the subgroup name-collision edge (→ 1M candidate), #377/#379/#382/#386, CI, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1L1-small-float-coord-vars.md](plan/v4.3.0-sprint1L1-small-float-coord-vars.md)

#### Sprint 1L2 - Port More nc_test4 Tests
- Port 5 of the biggest upstream `nc_test4` tests to NEXTCDF-4 (native mode only): `tst_camrun` (7.4k-line CAM metadata stress), `tst_atts3`, `tst_dims`, `tst_vars`, `tst_atts1`; kept near-verbatim via new `test/nextcdf4/nc_test4/` `err_macros.h`/`nc_tests.h` shims, and `tst_rename.c` migrates to them.
- Runtime failures surfaced real backend gaps; they were fixed in-sprint (upstream-compatible validation, mode transitions, `_FillValue`, attribute ordering, coord-var break/reform, markerless-scale loading). All six ports pass.
- Enabled ports run in `nextcdf4.yml` CI automatically; `docs/nextcdf4_status_v4_3_0.md` updated.
- Out of scope: all other nc_test4 ports incl. the feature-blocked set (user types → 1N, `nc_set_fill`/`nc_rename_grp`/var chunk cache → 1O, `NC_STRING` → 1P), failure fixes, #379 re-enablement (1M), parallel/`.cdl`-diff/perf ports, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1L2-port-nc-test4.md](plan/v4.3.0-sprint1L2-port-nc-test4.md)



#### Sprint 1M - Issues
Fix the four open NEXTCDF-4 defect issues:

- #378: split `include/nextcdf4dispatch.h` — installed part keeps `NC_NEXTCDF4_initialize`/`finalize`/`NC_FORMATX_NEXTCDF4` and includes only `<netcdf.h>`, `<netcdf_dispatch.h>`, `nep.h`; dispatch entries + `NEXTCDF4_dispatch_table` move to `nxt4internal.h`.
- #382: setters mark a `dcpl_dirty` flag instead of recreating; `_enddef` rebuilds each dirty var once (detach scales/dependents, recreate, rewrite atts, re-attach). `var->created` set at `_enddef`; all DCPL setters + quantize then return `NC_ELATEDEF` — fixes verified att-wipe and silent data-loss paths.
- #377: root cause is `H5Tget_native_type` widening NEP types (bf16→f32): `nx4_dcpl_fill`/`load_one_var` use the dataset's own HDF5 type for `H5Pset/get_fill_value`; `_FillValue` loads as a normal visible attribute on datasets.
- #379: abort traced to upstream `FILEINFOFLAGS` covering only UDF0/UDF1 (fixed in `5a81fb791`, 64-slot rework). NEXTCDF-4 requires the 64-slot scheme at configure time (`NC_MAX_UDF_FORMATS>=64` compile check → `FATAL_ERROR`); re-enable `tst_nextcdf4_float16`, `tst_nextcdf4_complex`, `tst_nextcdf4_bitfield`, `tst_explore` unconditionally. Spack overlay gains `depends_on("netcdf-c@4.10.2:", when="+nextcdf4 @4.3.0:")`.
- New `tst_nextcdf4_setters` + `tst_nextcdf4_var` fill coverage; status doc and docs updated; #377/#378/#379/#382 closed.
- Out of scope: #386 (performance only), other installed dispatch headers' `config.h` defect (#389), project-wide netcdf-c ≥4.10.2 floor once that release ships (it has not — Sprint 2 owns the bump), no freespace manager, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1M-fix-nextcdf4-issues.md](plan/v4.3.0-sprint1M-fix-nextcdf4-issues.md)

#### Sprint 1N1 - Fixed-Size User Types
- Context-aware user-type resolution; compound (scalar atomic fields), enum and opaque variables/attributes work through I/O, complete inquiries and reopen.
- Load committed types before values; preserve type identity, native memory layouts, ownership, fill behavior and deferred setter rebuilds.
- Native and `NC_NETCDF4_MODEL` coverage, classic-mode rejections and bidirectional upstream interchange; focused enabled tests.
- Keep native HDF5 complex storage and correct stale compound-layout docs; ordinary `{r,i}` compounds remain user types. No packaging changes.
- Out of scope: nesting/array fields (1N2), vlens/sanitizer CI (1N3), upstream ports (1N4), strings (1P), reference members (unsupported), release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1N1-fixed-size-user-types.md](plan/v4.3.0-sprint1N1-fixed-size-user-types.md)

#### Sprint 1N2 - Nested Types and Array Fields
- Implement `nc_insert_array_compound` and fixed-size nesting, sharing scalar insertion, recursive layout/visibility checks and dependency-ordered type commit/load.
- Preserve complete member metadata, rank/shapes, enum/opaque members and arrays of compounds; reject cycles, invalid shapes/offsets and nested references.
- Focused tests in native and `NC_NETCDF4_MODEL` modes, including upstream interchange; update type docs. No packaging/CI changes.
- Out of scope: dynamic nesting/vlens (1N3), upstream ports (1N4), strings (1P), reference members (unsupported), release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1N2-nested-types-and-array-fields.md](plan/v4.3.0-sprint1N2-nested-types-and-array-fields.md)

#### Sprint 1N3 - Vlen I/O and Ownership
- Vlen variables/attributes and full non-string nesting (vlens of user types, nested vlens, compounds/arrays containing vlens) in native and `NC_NETCDF4_MODEL` modes.
- Implement `nc_put_vlen_element`/`nc_get_vlen_element` with resolved base sizes; define borrowed descriptors versus owned read/deep-copy results and recursive reclamation.
- Focused fill, unlimited/strided I/O, reopen, ownership and upstream-interchange tests; update docs. No packaging changes.
- New `nextcdf4-sanitizers` CI job builds instrumented dependencies and NEP with ASan/UBSan plus leak detection; normal full CTest/docs checks retained.
- Out of scope: selected upstream ports (1N4), all string-value work (1P), nested references (unsupported), release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1N3-vlen-io-and-ownership.md](plan/v4.3.0-sprint1N3-vlen-io-and-ownership.md)

#### Sprint 1N4 - Upstream User-Type Tests
- Port pinned upstream `tst_enums`, `tst_opaques`, `tst_compounds`, `tst_compounds2`, `tst_vl`, `tst_vlen_unlim` using existing shims; preserve UCAR notices and the upstream compound fixture.
- Run each port in native and `NC_NETCDF4_MODEL` modes with isolated working directories, in normal and sanitizer CI; fix in-scope failures without weakening or disabling assertions.
- Update the status doc with exact two-mode coverage; full CTest remains green and docs warning-free. No packaging changes.
- Out of scope: remaining/mixed-feature upstream ports (future suite expansion), strings (1P), reference nesting (unsupported), release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1N4-upstream-user-type-tests.md](plan/v4.3.0-sprint1N4-upstream-user-type-tests.md)

#### Sprint 1O - Fill Mode, Cache Control, Renaming Groups
- Implement `nc_set_fill` as a session default for subsequently defined eligible variables; preserve per-variable overrides, dynamic-type fill safety and existing data.
- Implement real per-variable HDF5 chunk-cache settings on create/open and live-dataset reopen, including coordinate-handle aliases; no successful no-op.
- Implement flush-per-rename `nc_rename_grp`; enable group creation and rename in native and `NC_NETCDF4_MODEL` modes, retaining classic-model rejection.
- Reject unsupported memory/sharing/parallel create/open flags before side effects (`NC_DISKLESS`, `NC_INMEMORY`, `NC_MPIIO`/`NC_MPIPOSIX`, `NC_SHARE`, `NC_PERSIST`, `NC_MMAP`).
- Add focused regressions to existing normal/sanitizer CI; update behavior/status docs, keep CTest green and docs warning-free. No packaging changes.
- Out of scope: upstream test ports (future suite expansion), strings (1P), provenance (1Q), Fortran (1R), release notes (Sprint 2); parallel I/O, image files and SWMR move beyond v4.3.0.

Plan: [docs/plan/v4.3.0-sprint1O-fill-mode-cache-control-renaming-groups.md](plan/v4.3.0-sprint1O-fill-mode-cache-control-renaming-groups.md)

#### Sprint 1P - NC_STRING
- Fix standalone string variable/attribute reopen and full string nesting (compound/array members, vlens of strings and recursive combinations) in native and `NC_NETCDF4_MODEL` modes.
- Unify owned string/fill buffers, empty-string defaults, deep-copy/reclaim and partial-read cleanup; retain classic-model rejection and reject standalone string `NC_NOFILL`.
- Require bidirectional upstream interchange plus NEXTCDF-4 updates to upstream-created string files; preserve legacy ASCII and upstream UTF-8 string storage compatibility.
- Add focused string regressions and two-mode ports of `tst_strings`, `tst_strings2`, `tst_atts_string_rewrite`; existing normal/sanitizer CI runs them. Update behavior/status docs, keep CTest green and docs warning-free. No packaging changes.
- Out of scope: wider fixed-length HDF5 strings (beyond v4.3.0), other upstream ports (future suite expansion), reference nesting (unsupported), provenance (1Q), Fortran (1R), release notes/publication (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1P-nc-string.md](plan/v4.3.0-sprint1P-nc-string.md)

#### Sprint 1Q - Provenance Attributes (_NCProperties/_IsNetcdf4/_SuperblockVersion)
- New `src/nextcdf4/nxt4provenance.c`: deferred `_NCProperties` write at `_enddef`/`nc_sync`/`nc_close` (never overwriting an existing stamp) as `version=2,netcdf=...,hdf5=...,nextcdf4=4.3.0`, plus `_nc3_strict`=1 on `NC_CLASSIC_MODEL` files — upstream storage parity.
- `_IsNetcdf4`/`_SuperblockVersion` stay virtual: synthesized in `inq_att`/`inq_attid`/`get_att` on `NC_GLOBAL` name lookups, mirroring `nc4_get_att_special` (`NC_EATTMETA` attnum, `NC_INT`/len 1, memtype conversion).
- Foreign files without `_NCProperties` are left unstamped; `_Format` needs no backend work (ncdump synthesizes it). New `tst_nextcdf4_provenance` under `nextcdf4_controls`; no packaging changes.
- Out of scope: Fortran (1R), `nextcopy`/`nextdump`, parallel/SWMR/in-memory files, user-injected property keys, remaining nc_test4 ports, release notes (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1Q-provenance-attributes.md](plan/v4.3.0-sprint1Q-provenance-attributes.md)

#### Sprint 1R - Fortran Wrappers
- Install `module nep_nextcdf4` / `libnepnextcdf4f` (`NEP::nepnextcdf4f`) when NEXTCDF-4 and Fortran are enabled; require NetCDF-Fortran >=4.6.2 for that combination.
- Export initialization, mode/new-type constants and `nf90_{get,put}_var_{float16,bfloat16}` generics (scalar–rank 7, optional start/count/stride); raw `integer(c_int16_t)` bits, 1-based Fortran indices and dimension order.
- Add focused create/types/I/O/reopen tests and an installed consumer; enable Fortran in normal NEXTCDF-4 CI, retaining C-only sanitizer coverage and warning-free docs.
- Update the Spack overlay's conditional dependency/install checks using existing variants; upstream copy ships with Sprint 2's version-bump PR. Conda is unchanged.
- Out of scope: reference wrappers and general enhanced-type value interfaces (beyond v4.3.0), varm/F77/half attributes, Fortran sanitizer expansion, release notes/publication (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1R-fortran-wrappers.md](plan/v4.3.0-sprint1R-fortran-wrappers.md)

#### Sprint 1S - CI and Docs
- New `nextcdf4-coverage` job in `nextcdf4.yml` (geotiff-coverage pattern): `--coverage` build of the full enabled suite, full ctest run, `lcov`/`genhtml` HTML artifact; report-only, warn below 50%. No CMake coverage option, no Codecov.
- Rewrite `README.md`, `docs/prd.md`, `docs/prfaq.md`, `docs/design.md` for 4.3.0: drop dated press-release framing, format list (Space: CDF/FITS/PDS4; Earth: GeoTIFF/GRIB2; Medical: DICOM; Bio: PDB/mmCIF) plus NEXTCDF-4 summary (drop-in netCDF-4 replacement, 16-bit floats, `NC_STD_REF`, latest superblocks, performance); compression reduced to a nep-compress pointer in the README; dependency floors corrected (netcdf-c ≥4.10.2, HDF5 2.1.1).
- `src/nextcdf4/nxt4meta.c`: comment-only pass — walkthrough paragraphs and deeper `/**` blocks for the metadata load, `_enddef` dirty-var rebuild, att rewrite, and dim-scale re-attach machinery.
- Out of scope: coverage gates/trends/Codecov, other `src/nextcdf4/*.c` files, code changes, release notes/tag/Spack bump/conda wording (Sprint 2).

Plan: [docs/plan/v4.3.0-sprint1S-ci-and-docs.md](plan/v4.3.0-sprint1S-ci-and-docs.md)

#### Sprint 1T - Example Programs
- Both book repos verified on GitHub: `captainkirk99/NetCDF_Developers_Handbook` holds all eight moved example dirs (`docs/nep-handoff.md` lists every file); `Earth_Observation_in_Practice` holds the mission examples removed in v4.1.0 Sprint 3.
- Delete `examples/{classic,netcdf-4,nczarr,opendap,parallelIO,f_classic,f_netcdf-4}` plus `generate_all_cdl.sh` and all `expected_output/` files except `README.md`, `cache_tuning_expected.cdl`, and the four `chunking_performance_*_expected.cdl` files.
- Keep `examples/performance/` (NEP holds the authoritative Sprint 1C-1E `perf_disk.h` benchmark sources; the book repo's copies are older); move `netcdf-4/chunking_performance.c` into it. Delete dead `lossless.c`/`quantize.c` (LZ4/BZIP2 API moved to nep-compress in v4.2.0) and their artifacts.
- Remove `NEP_ENABLE_OPENDAP_EXAMPLES` (emit `NEP_HAS_OPENDAP 0`); keep `NEP_ENABLE_BENCHMARKS`, `NEP_ENABLE_PARALLEL_TESTS`, `HAVE_NCZARR` for non-example users.
- Delete `docs/ch3-fill-values.md` and `docs/background/NetCDF_Developers_Handbook_1_10.{md,sdi}`; rewrite `README.md`, `docs/examples.md`, `prd.md`, `prfaq.md`, `design.md`, `mainpage.md`, `NEP_abstract.md`, `examples/README.md`, `nep_skill.md`, `.devin/skills/nep-examples.md`, `.devin/workflows/edit.md` to point at the handbook repo.
- No Spack/conda recipe changes; remaining dirs: `nextcdf4/`, `dicom/`, `pdb/`, `viz/`, `performance/`.

Plan: [docs/plan/v4.3.0-sprint1T-example-programs.md](plan/v4.3.0-sprint1T-example-programs.md)

#### Sprint 1U - Fix Issue 389
- Split all eight format dispatch headers the #378 way: six installed (`grib2`, `fits`, `pds4`, `dicom`, `pdb`, `mmcif`) plus uninstalled `cdfdispatch.h`/`geotiffdispatch.h`, which share the `config.h`/`ncdispatch.h` defect.
- Installed headers keep only `NC_FORMATX_NC_<FMT>` (with the `NC_FORMATX_UDFn` fallbacks), `NC_<FMT>_initialize`, `NC_<FMT>_finalize`; includes limited to `<netcdf.h>`, `<netcdf_dispatch.h>`, `nep.h`.
- New `src/<fmt>internal.h` per backend holds the state structs, dispatch prototypes, `*_dispatch_table`, `*_INIT_*` macros and third-party includes; backend `.c` files and `test_cdf/`/`test_geotiff/` repoint to them.
- Verification: build, ctest, warning-free docs, plus a manual install-prefix compile probe per installed header; close #389. No Spack/conda or CI changes.
- Out of scope (Sprint 2): tag, release notes, Spack/conda version bumps; installing cdf/geotiff headers stays out entirely.

Plan: [docs/plan/v4.3.0-sprint1U-fix-issue-389.md](plan/v4.3.0-sprint1U-fix-issue-389.md)

#### Sprint 2 - 
- Tag v4.3.0 and write `docs/releases/v4.3.0.md` (HDF5 detection change, CMake 3.19 minimum, conda GeoTIFF, Sprint 1A NEXTCDF-4 bfloat16 examples and `NEP_ZSTD_PLUGIN_DIR`, Sprint 1B typed small-float I/O and the `nc_*_bfloat16` functions, #376, Sprint 1C optimized benchmark results, the bfloat16 GDAS sample and `bfloat16_io_perf`, the #382 contiguous-layout fix, the fix for untyped `nc_put_var`/`nc_get_var`/`var1`/`vars` failing with `NC_EBADTYPE` on NEXTCDF-4, and Sprint 1K generic HDF5 filter support (`nc_def_var_filter`, szip/bzip2/zstd plugins, missing-plugin `NC_ENOFILTER`), and Sprint 1L reference types (`NC_REF_OBJECT`/`NC_REF_REGION` removed — a breaking API change — replaced by `NC_STD_REF`/`H5T_STD_REF` and the `nc_ref_*`/`nc_deref_*` API).
- Spack: add `version("4.3.0")` with its sha256; update spack/spack-packages#5557 in place per `docs/spack.md` "Updating the upstream PR".
- Conda (#374 part 2): if #34309 is still open, regenerate the staged copy for 4.3.0, push it to `nep-3.0.0` and update the PR body; if merged, make the GeoTIFF change on the feedstock and drop the staged lint/micromamba steps. Move the `conda.yml` lint step to the 4.3.0 tarball and set `context.version` to `4.3.1.dev0`.
- Update the README "Conda Installation", `docs/prd.md` section 5.5, `docs/prfaq.md` and the `docs/conda-plan.md` Status table for GeoTIFF in 4.3.0.
- Document the deferrals in the release notes: `nextcopy`/`nextdump` (NEXTCDF-4 Phase 4 tools) and all NEXTCDF-4 parallel I/O move to a future release.
- Release notes also cover the Sprint 1T example removal: generic netCDF examples deleted (now maintained in `captainkirk99/NetCDF_Developers_Handbook`), `NEP_ENABLE_OPENDAP_EXAMPLES` removed, `examples/performance/` retained.
- Close #374 (#372 was closed 2026-11-21 — Sprint 1 work landed).


### V4.2.0 - Move Compression Filters to nep-compress

**Goal:** Move ownership of the LZ4 and BZIP2 compression APIs, HDF5 filter integration, tests, and documentation to `/fast/ed/nep-compress/`. NEP remains the home of the format UDF readers and NEXTCDF-4 backend. Complete and verify the new project before removing compression from NEP.
#### Sprint 1: Inventory and Define the Compatibility Contract
**Objective:** Establish exactly what moves and what must continue to work for existing compression users.

**Detailed Plan:** See `docs/plan/v4.2.0-sprint1-compression-inventory.md` for the sprint plan and `docs/plan/v4.2.0-sprint1-results.md` for the inventory, migration decisions, local baseline and open gates.

**Deliverables:**
- Inventory compression-specific code and build configuration: `src/nep.c`, `include/nep.h`, `fsrc/nep.F90`, `include/nep_meta.h.in`, root and subdirectory CMake files, `hdf5_plugins/LZ4/`, C/Fortran and HDF5 tests, compression examples, Spack recipes, CI, and docs. Separate shared logging, metadata, and format-reader code from compression-only code.
- Record the existing public C and Fortran API names, argument conventions, filter IDs, plugin names and discovery path, build defaults, and read/write behavior for existing LZ4/BZIP2 files. Decide how users migrate their includes, libraries, Fortran module, and build options without changing on-disk filter IDs or breaking file readability.
- Identify the BZIP2 plugin provider: NEP currently builds an LZ4 plugin but expects BZIP2 from the NetCDF/HDF5 installation. Specify whether nep-compress will build BZIP2 itself or require an external plugin, and make the requirement testable and documented.
- Inventory compression-related references in `README.md`, `docs/compression.md`, `docs/design.md`, `docs/prd.md`, `docs/prfaq.md`, Doxygen inputs, release notes, and examples. Distinguish historical release records from current installation instructions.

**Acceptance criteria:**
- A migration checklist identifies what will move, what stays in NEP, and the expected behavior when both projects are installed.
- No format-reader, NEXTCDF-4, or NEP logging dependency is assigned to nep-compress; logging statements in extracted compression code may be commented out for now without changing error handling.

#### Sprint 2: Build and Test nep-compress Locally
**Objective:** Make `/fast/ed/nep-compress/` independently buildable and testable before changing NEP.

**Detailed Plan:** See `docs/plan/v4.2.0-sprint2-nep-compress-build.md` for dependency gates, standalone targets, local verification, and bidirectional file tests. Local implementation status and remaining gates are in `docs/plan/v4.2.0-sprint2-results.md`.

**Deliverables:**
- Move the compression C APIs, Fortran wrappers, LZ4 plugin source and build rules, and compression-only tests into nep-compress. Provide standalone CMake options, dependency detection, install rules, and an HDF5 plugin installation path; handle BZIP2 according to the Sprint 1 decision. Do not copy NEP's unrelated UDF or NEXTCDF-4 code.
- Port C and Fortran tests for valid/invalid levels, filter inquiry, round-trip I/O, and reopening existing compressed files. Test both filters when their dependencies are available; clearly report unavailable optional dependencies instead of silently skipping required coverage.
- Build and run tests against `/usr/local/netcdf-c`, `/usr/local/netcdf-fortran`, and `/usr/local/hdf5-2.1.1`, using an isolated `build` directory and the built plugin directory in `HDF5_PLUGIN_PATH`. Check that library and module resolution uses those installations rather than system copies.
- Verify compressed files created by the original NEP setup can be read with nep-compress, and vice versa, without changing filter IDs or file contents.

**Acceptance criteria:**
- Standalone configure, build, install, and CTest pass for the enabled filters; Fortran tests pass when NetCDF-Fortran is enabled.
- No NEP checkout or NEP library is needed to build, test, or use nep-compress.

#### Sprint 3: Documentation and Independent CI
**Objective:** Make nep-compress usable and verifiable as a separate GitHub project.

**Detailed Plan:** See `docs/plan/v4.2.0-sprint3-docs-ci.md` for docs ownership, external BZIP2 provisioning, CI verification and open gates. Local implementation status is in `docs/plan/v4.2.0-sprint3-results.md`.

**Deliverables:**
- Transfer and correct compression documentation, including C/Fortran examples, filter discovery with `HDF5_PLUGIN_PATH`, dependencies, build options and defaults, existing-file compatibility, and BZIP2 plugin provisioning. Keep benchmark artifacts with their generating code or link to their maintained location.
- Write the nep-compress `README.md` to explain that compression was separated from NEP to make installation and use easier, with standalone build/test/install instructions and a pointer back to NEP for format readers.
- Add a basic GitHub Actions workflow that installs or builds required dependencies, configures and builds nep-compress, and runs CTest for compression code. Exercise both filters where available and report skipped optional coverage explicitly.

**Acceptance criteria:**
- A new user can build and test nep-compress from its README without NEP; CI passes independently on GitHub.
- Documentation and CI describe the same dependency and plugin-discovery behavior as the local build.

#### Sprint 4: Remove Compression Ownership from NEP
**Objective:** Leave NEP building and testing its format code, not shipping duplicate compression implementations.

**Prerequisite:** Sprints 2 and 3 pass, including the cross-project file compatibility tests; do not remove the working NEP path before this gate.

**Deliverables:**
- Remove compression-only C/Fortran APIs, module contents, plugin build and install rules, build options and generated feature flags, compression tests, and compression-specific examples from NEP. Retain `src/nep.c` and any shared library functionality still required by UDF handlers and NEXTCDF-4, or refactor those dependencies explicitly before removal.
- Update NEP's CMake targets, packaging/Spack recipes, workflows, Doxygen configuration, and active documentation so they no longer promise or build bundled filters. Remove stale `NEP_BUILD_LZ4`/`NEP_BUILD_BZIP2` flags from format-only CI configurations. NEP uses CMake; do not add Autotools work for this migration.
- Update `README.md`, `docs/design.md`, `docs/prd.md`, `docs/prfaq.md`, and release notes to point compression users to nep-compress and describe the packaging/API migration. Preserve useful historical release notes without leaving broken links.
- Run a clean NEP format-only build and CTest with the local NetCDF-C/HDF5 installations; verify enabled UDF readers and NEXTCDF-4 still build independently of nep-compress. Verify nep-compress remains usable alongside the installed NEP format libraries.

**Acceptance criteria:**
- NEP no longer builds or installs compression filters or compression-only APIs/tests, and its active docs no longer advertise bundled compression.
- NEP's supported format tests pass without LZ4/BZIP2 development packages; existing compressed files remain readable through nep-compress, with migration instructions for former NEP users.

#### Sprint 6: Spack
- Prerequisite: tag v4.2.0. Recipe offers only `main` and `4.2.0`; no compression variants; HDF5 `@1.14:`.
- Move the recipe to the overlay `spack/repos/spack_repo/nep/`. Variants: `fits`, `geotiff`, `grib2`, `pds4`, `pdb`, and new `mmcif` and `nextcdf4`. `dicom` is removed (no `gdcm` package in Spack); `cdf` is a commented-out variant waiting on spack/spack-packages#2797.
- Rewrite `spack.yml` on the nep-compress pattern: pinned `setup-spack`, the `spack/ci` environment, `install --test=root` on push and PR, and upstream ruff/vermin style checks.
- Update spack/spack-packages#5557 in place to 4.2.0, removing 1.3.0. It keeps `netcdf-c@4.10.1:` and depends on #6569; CI uses `^netcdf-c@main` until #6569 merges.
- Update the Spack sections of the README, `docs/spack.md`, `docs/prd.md` and `docs/design.md`.
- Out of scope: Spack DICOM/`gdcm`, enabling `cdf`, the local `spack/cdf` recipe, and run-environment `.ncrc` wiring.

Plan: [docs/plan/v4.2.0-sprint6-spack.md](plan/v4.2.0-sprint6-spack.md)

#### Sprint 7: Conda
- Replace `conda/meta.yaml` with a v1 `conda/recipe/recipe.yaml` (rattler-build) on the nep-compress pattern, plus `conda/make_staged_recipe.py` and `docs/conda-plan.md`.
- Linux-only, format-only 4.2.0 package with PDB, mmCIF, FITS and PDS4 readers against conda-forge `libnetcdf` 4.10.1 (nompi, HDF5 1.14.6); maintainer `captainkirk99`.
- Readers are registered by appending `$CONDA_PREFIX/share/.ncrc` to `~/.ncrc`; there is no activation script.
- Replace `ci-conda.yml` with `conda.yml`: rattler-build with conda-forge pinnings, package tests that `ncdump` each reader, and `conda-smithy` lint of the staged copy.
- Update staged-recipes#34309 in place to `Add nep` (4.2.0); update the README, `docs/prd.md`, `docs/prfaq.md` and `docs/formats.md`.
- Out of scope: GeoTIFF (4.2.0 lacks `4d920c5`; enable in the next release), DICOM (`gdcm` is built per Python version), GRIB2/CDF (no packages), NEXTCDF-4 (HDF5 pin 1.14.6), macOS, MPI builds.

Plan: [docs/plan/v4.2.0-sprint7-conda.md](plan/v4.2.0-sprint7-conda.md)

### V4.1.0 - NEXTCDF4 Docs and Logging

#### Sprint 1: Docs for NEXTCDF4
**Objective:** Create complete user-facing and developer documentation for the NEXTCDF-4 backend, including a dedicated reference page, Doxygen main-page integration, and updates to design, PRD, FAQ, and release notes so users and downstream developers can understand how to build, select, and use NEXTCDF-4 files and new types.

**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md` for architecture, compatibility contract, type mapping, and source layout.

**Prerequisites:**
- v4.0.0 NEXTCDF-4 backend implementation complete and buildable with `NEP_ENABLE_NEXTCDF4`.
- `NEP_HAS_NEXTCDF4` and related feature macros are stable in `nep_meta.h`.
- `NC_NEXTCDF4`, `NC_NETCDF4_MODEL`, and new atomic type constants (`NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_*`, etc.) are public API in `include/nep.h`.
- Existing format-reader documentation (`docs/geotiff.md`, `docs/grib2.md`, `docs/fits.md`, etc.) establishes the page style and section order to follow.

**Deliverables:**
1. **`docs/nextcdf4.md`** — dedicated user reference page covering:
   - Overview and relationship to the built-in NetCDF-4/HDF5 backend.
   - Build configuration (`NEP_ENABLE_NEXTCDF4`), HDF5 version requirements (2.1.1+ required), and `nep_meta.h` feature macros.
   - Creating and opening files with `NC_NEXTCDF4` and the `.ncrc` UDF9 autoload path.
   - Compatibility modes: native NEXTCDF-4, `NC_CLASSIC_MODEL`, and `NC_NETCDF4_MODEL`.
   - Supported atomic types table (classic, small floating-point, complex, bitfield, reference) and their HDF5 mappings.
   - Dimension, variable, attribute, group, and user-defined-type behavior, including hidden markers (`_Nextcdf4Backend`, `_Nextcdf4Model`, `_Netcdf4Coordinates`, `_Netcdf4Dimid`).
   - New reference APIs: `nc_ref_object`, `nc_deref_object`, `nc_ref_region`, `nc_deref_region`.
   - Limitations: explicit `NC_NEXTCDF4` selection only (no magic-number dispatch), deprecated `nc_put_varm`/`nc_get_varm`, reference restrictions, and `NC_NETCDF4_MODEL` type restrictions.
   - Code examples for create/read/reopen, compatibility-mode files, and reference usage.

2. **`docs/mainpage.md`** — add a NEXTCDF-4 section to the Doxygen landing page with a brief description, UDF9 / `NC_NEXTCDF4` mention, and a link to `docs/nextcdf4.md`.

3. **`docs/formats.md`** — add NEXTCDF-4 as a UDF9 backend entry (distinct from the read-only format readers) with a link to `docs/nextcdf4.md`; preserve the existing UDF slot table and autoload examples.

4. **`docs/design.md`** — update the architecture and project-structure sections:
   - Remove "(planned)" from the `src/nextcdf4` entry.
   - Expand the NEXTCDF-4 paragraph to cover the full v4.0.0 feature set (metadata, I/O, groups, types, rename, small floats, complex, bitfield, reference).
   - Clarify that UDF9 selection is explicit because HDF5 magic numbers are already owned by the built-in backend.

5. **`docs/prd.md`** — add a NEXTCDF-4 product-requirements section covering goals, build options, type support, compatibility contract, new reference APIs, and limitations.

6. **`docs/prfaq.md`** — add FAQ entries for NEXTCDF-4:
   - What is NEXTCDF-4 and why does NEP include it?
   - How do I create or open a NEXTCDF-4 file?
   - What new atomic types does NEXTCDF-4 support?
   - Can NEXTCDF-4 files be read by upstream netcdf-c?
   - Why is NEXTCDF-4 explicit selection only?
   - What are `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` compatibility modes?
   - Update the "What is the current version?" and "What formats does NEP support?" answers to mention NEXTCDF-4.

7. **`docs/releases/v4.1.0.md`** — create release notes documenting that v4.1.0 Sprint 1 delivered the NEXTCDF-4 documentation suite and pointing forward to the logging, tools, and language-binding sprints.

8. **Doxygen integration** — verify that `src/nextcdf4/*.c` and `src/nextcdf4/*.h` source files are included in Doxygen `INPUT` paths, that existing file-level `@file`/`@brief` comments render, and that any new documentation cross-references resolve correctly.

**Implementation sequence:**
1. Draft `docs/nextcdf4.md` using the established format-reader page template.
2. Update `docs/mainpage.md` with a NEXTCDF-4 section.
3. Update `docs/formats.md` with a NEXTCDF-4 backend entry.
4. Update `docs/design.md` architecture and project-structure sections.
5. Add a NEXTCDF-4 section to `docs/prd.md`.
6. Add NEXTCDF-4 FAQ entries to `docs/prfaq.md` and update version/format summary answers.
7. Create `docs/releases/v4.1.0.md` release notes.
8. Run the Doxygen build (`cmake --build build --target docs`) and fix any warnings or broken cross-references.

**Verification and acceptance criteria:**
- `docs/nextcdf4.md` exists, follows the existing format-reader style, and covers every deliverable topic listed above.
- `docs/mainpage.md` links to `nextcdf4.md` and mentions `NC_NEXTCDF4` / UDF9.
- All internal cross-references in new and updated docs resolve without broken links.
- `cmake --build build --target docs` completes with zero Doxygen warnings.
- New and updated documentation is consistent with `docs/plan/NEXTCDF4_plan.md`, the detailed v4.0.0 sprint descriptions in this roadmap, and the actual `src/nextcdf4/` implementation and public headers.
- No code changes are introduced; only documentation is added or updated.

**Out of scope for Sprint 1:**
- Implementation of `nextcopy` / `nextdump` tools (v4.1.0 Sprint 3).
- Fortran or other language bindings for NEXTCDF-4 (later v4.1.0 sprints).
- New NEXTCDF-4 functionality beyond documenting the existing v4.0.0 behavior.
- Autotools documentation parity; NEP is CMake-only, so Autotools examples are not required.


#### Sprint 2: Logging in NEP
**Objective:** Add an independent NEP logging system for the NEXTCDF-4 backend and future NEP handlers. It uses the distinct `ELOG(())` macro, patterned after netcdf-c's `LOG(())` in `libsrc4` and `libhdf5`, and routes output through NEP's own `nep_log()` and is controlled by a public `nep_set_log_level()` API. Logging is enabled by default and does not depend on whether the host NetCDF-C was built with logging.

**Detailed Plan:**
- Add a CMake option `NEP_ENABLE_LOGGING` (default ON). When ON, define the `NEP_LOGGING` preprocessor macro; when OFF, `ELOG(...)` expands to nothing and `nep_set_log_level()` is a no-op.
- Expose the capability as `NEP_HAS_LOGGING` in `include/nep_meta.h.in`.
- Create `include/nep_logging.h` with the `ELOG(())`, `EBAILLOG`, `EBAIL`, and `EBAIL_QUIET` macros.
- Add the public function `nep_set_log_level(int new_level)` to `include/nep.h` and implement it, along with `nep_log()` and the `nep_log_level` global, in `src/nep.c`.
- Link the NEXTCDF-4 backend (`ncnextcdf4`) against the core `nep` library so it can use the shared logging state.
- Add `src/nextcdf4/nxt4err.h` with `EBAIL`/`EBAIL2` macros adapted to the local `ret`/`fail:` style. `EBAIL2` dumps the HDF5 error stack directly with `H5Eprint2()` so it does not require NetCDF-C's `nc_log_hdf5()`.
- Instrument NEXTCDF-4 lifecycle and metadata functions, replacing temporary `fprintf(stderr, ...)` debug prints with `ELOG(...)` calls.
- In `NC_NEXTCDF4_initialize()`, read `NEP_LOG_LEVEL` and call `nep_set_log_level()`.
- Add `test/nextcdf4/tst_nextcdf4_logging.c` to verify output at level 3, suppression at `-1`, and HDF5 failure logging.
- Update `docs/nextcdf4.md` and `docs/releases/v4.1.0.md`.

**Verification and acceptance criteria:**
- A default NEP build sets `NEP_HAS_LOGGING=1`; `nep_set_log_level()` controls NEXTCDF-4 diagnostic output.
- With logging enabled, `ELOG((severity, fmt, ...))` calls produce output only when `severity <= nep_log_level`, and output is suppressed after `nep_set_log_level(-1)`.
- With logging disabled (`-DNEP_ENABLE_LOGGING=OFF`), `ELOG(...)` expands to nothing, the logging test skips, and the full C test suite still passes.
- The temporary `fprintf(stderr, ...)` debug statements in `nxt4create.c`, `nxt4open.c`, and `nxt4file.c` are removed; no unguarded debug output remains in the NEXTCDF-4 backend.
- `EBAIL2` on an injected HDF5 failure emits a severity-0 NetCDF error line and dumps the HDF5 error stack.
- `cmake --build build` produces no new warnings.

**Out of scope for Sprint 2:**
- Instrumenting the read-only format handlers (GeoTIFF, GRIB2, FITS, PDS4, DICOM, CDF, PDB, mmCIF) beyond making the shared macro header available; those may be instrumented in later maintenance sprints.
- Per-rank log files for parallel builds; NEP does not yet exercise parallel NEXTCDF-4 I/O.
- Replacing every existing error path in NEXTCDF-4 with `EBAIL` macros; the goal is coverage of the lifecycle and metadata paths, not a wholesale control-flow refactor.

#### Sprint 3: Remove Space Mission Examples
**Objective:** Remove the standalone Earth-observation mission examples and companion articles from NEP so that this repository remains focused on the NEP C/Fortran library, UDF readers, compression filters, NEXTCDF-4 backend, and examples that directly exercise those capabilities. Direct users interested in satellite-data workflows to the dedicated *Earth Observation in Practice* repository and book.

**Background:** The NISAR, SWOT, GOES-R ABI, and Sentinel-3 OLCI examples are self-contained Python applications for products that are already NetCDF-4/HDF5 compatible. They do not exercise NEP dispatch handlers or other NEP library functionality. These examples now live in a dedicated repository where mission-specific code, data-access instructions, figures, and articles can evolve independently of NEP.

**Removal inventory:**
- Remove the standalone example trees `examples/nisar/`, `examples/swot/`, `examples/abi/`, and `examples/olci/`, including Python packages, requirements files, mission-specific READMEs, fetch helpers, committed figures, and local `.gitignore` files.
- Remove the companion articles `docs/netCDF_with_NISAR.md`, `docs/netCDF_with_SWOT.md`, `docs/netCDF_with_ABI_CMIP.md`, and `docs/netCDF_with_Sentinel3_OLCI.md`.
- Remove active links and descriptions for these examples from the top-level `README.md`, `examples/README.md`, Doxygen inputs/navigation, generated-documentation indexes, and any other current user-facing documentation.
- Audit `CMakeLists.txt`, `examples/CMakeLists.txt`, `examples/viz/CMakeLists.txt`, CTest registration, install rules, packaging manifests, and CI workflows for mission-example references. Remove any references found; document that examples which were already standalone had no build integration rather than adding replacement build logic.
- Audit project-maintenance configuration for mission-specific example support, including `.devin/skills/nep-satellite-example/` and any mission-specific skills whose only purpose is maintaining the removed examples. Remove or retire those files so project guidance does not instruct contributors to recreate examples in NEP.

**README replacement:**
- Replace the in-repository mission-example list with a concise "Earth Observation Examples" pointer to [Earth Observation in Practice](https://github.com/captainkirk99/Earth_Observation_in_Practice).
- Mention the book *Earth Observation in Practice: A Mission-by-Mission Guide to Reading Satellite Data with NetCDF and Python* and link to its [Amazon page](https://www.amazon.com/Earth-Observation-Practice-Mission-Mission/dp/B0HJRZ9F7L/).
- Explain that the external repository contains the maintained mission-by-mission examples, data-access instructions, and Python visualizations formerly hosted in NEP.
- Keep the replacement text informational; NEP must not acquire a build-time, run-time, test-time, or packaging dependency on the external repository or book resources.

**Documentation handling:**
- Preserve historical release records where useful, but remove links that would become broken after deleting the example trees and companion articles. Historical text may state that the examples were introduced in earlier releases and moved out in v4.1.0.
- Update `docs/releases/v4.1.0.md` with a Sprint 3 migration note listing the removed examples and the new external destination.
- Do not remove format-reader examples under `examples/viz/` that directly exercise NEP UDF handlers merely because their sample data originates from a satellite. The removal target is the standalone mission-oriented applications and articles, not GeoTIFF, GRIB2, DICOM, PDS4, PDB, mmCIF, or other NEP format demonstrations.
- Do not remove general NetCDF, NcZarr, OPeNDAP, parallel-I/O, performance, compression, or NEXTCDF-4 examples.

**Implementation sequence:**
1. Inventory all references to NISAR, SWOT, ABI/GOES-R, OLCI/Sentinel-3, `Earthdata`, and the four `examples/<mission>/` paths across source, build files, documentation, CI, packaging, and project skills.
2. Remove the four standalone mission example directories and four companion articles.
3. Remove active documentation, Doxygen, packaging, build, and CI references to the deleted paths.
4. Replace the satellite-example section in `README.md` with links to the dedicated repository and book.
5. Update `examples/README.md` so its inventory and Python dependency guidance describe only examples that remain in NEP.
6. Update v4.1.0 release notes and repair historical references that would otherwise point to deleted files.
7. Remove or retire project skills dedicated solely to adding or maintaining standalone satellite examples in NEP.
8. Search the repository again for stale path references and classify any remaining mission names as intentional historical records or errors to fix.
9. Perform a clean configure, build, CTest run, and Doxygen build to prove that deleting the examples did not leave broken dependencies or links.

**Verification and acceptance criteria:**
- `examples/nisar/`, `examples/swot/`, `examples/abi/`, and `examples/olci/` no longer exist in NEP.
- The four `docs/netCDF_with_*.md` mission articles no longer exist.
- No active CMake, CTest, install, packaging, CI, Doxygen, README, or documentation link references a deleted file or directory.
- The top-level README contains working links to the dedicated Earth-observation repository and the book, and clearly tells users where the migrated examples are maintained.
- `examples/README.md` accurately lists only examples still present in the repository and no longer asks users to install mission-specific Python requirements.
- Remaining references to NISAR, SWOT, ABI, or OLCI are limited to intentional historical release/roadmap records and the v4.1.0 migration notice; none instruct users to run removed code.
- A clean build in `build/` succeeds, all enabled CTest tests pass, and the Doxygen target completes without warnings or unresolved references.
- No NEP library API, ABI, UDF dispatch behavior, test data, or format-reader functionality changes in this sprint.

**Out of scope for Sprint 3:**
- Copying or synchronizing files into the external repository; that repository is treated as the maintained destination.
- Adding new satellite missions, download clients, credentials, sample granules, plots, or replacement Python packages to NEP.
- Removing visualization scripts that directly test NEP format readers under `examples/viz/`.
- Changing the NEP C/Fortran implementation, NEXTCDF-4 backend, UDF slot allocation, or compression-filter behavior.

### V4.0.0 - NEXTCDF4

NEXTCDF-4 is a clean-room rewrite of the NetCDF-4/HDF5 backend delivered as a NEP UDF expansion pack. The complete architecture, compatibility contract, datatype roadmap, and proposed source layout are defined in `docs/plan/NEXTCDF4_plan.md`.

#### Sprint 1: Prepare the NEXTCDF-4 Foundation
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "NEXTCDF-4 as a NEP UDF Expansion Pack," "Implementation Phases," and "`src/nextcdf4/` Source Organization."

**Objective:** Establish a buildable, testable NEXTCDF-4 plugin skeleton and prove UDF9 registration before implementing HDF5 file operations. This sprint creates the integration boundary that later sprints will fill in; it does not create, open, modify, or read files.

**Prerequisites:**
- Complete the v3.5.0 `nep_meta.h` work so NEXTCDF-4 availability can be advertised as `NEP_HAS_NEXTCDF4`.
- Build against a netcdf-c version that provides `NC_UDF9`, supports UDF plugin autoloading, and exposes the `NC_Dispatch` interface required by the backend.
- Detect HDF5 2.1.1 or newer. If the dependency is unavailable, keep NEXTCDF-4 disabled without affecting the existing NEP handlers or the built-in NetCDF-4 backend.

**Deliverables:**
- Reserve UDF9 permanently in `include/nep.h` with `NEP_UDF_NEXTCDF4` and the public alias `NC_NEXTCDF4`; update the UDF allocation documentation so the slot is no longer described as reserved.
- Add an opt-in CMake option for NEXTCDF-4, disabled by default, and gate it on compatible netcdf-c and HDF5 versions. Keep existing configurations and installations unchanged when the option is off.
- Create the initial `src/nextcdf4/` build target and private headers. Limit the first source set to dispatch, initialization/finalization, and the minimum shared definitions needed by later sprints.
- Implement `NC_NEXTCDF4_initialize()` and `NC_NEXTCDF4_finalize()` with repeatable initialization, safe cleanup, and UDF9 registration through `nc_def_user_format()`.
- Define the NEXTCDF-4 `NC_Dispatch` table with the correct model identifier and complete ABI-compatible layout for the supported netcdf-c version. File and metadata callbacks remain intentional stubs that return an appropriate NetCDF "not supported/not implemented" error rather than succeeding or dereferencing null pointers.
- Add `.ncrc`/autoload metadata for `NETCDF.UDF9.LIBRARY` and `NETCDF.UDF9.INIT`, following the same install and plugin-discovery conventions as existing NEP UDF handlers.
- Install any public declarations needed to initialize and feature-detect NEXTCDF-4, while keeping implementation headers private.
- Update architecture, build, and UDF-slot documentation to describe NEXTCDF-4 as an optional backend that coexists with netcdf-c's built-in HDF5 backend.

**Verification and acceptance criteria:**
- A default build with NEXTCDF-4 disabled continues to configure, build, and pass the full existing unit test suite.
- An enabled build with supported dependencies compiles and links the NEXTCDF-4 plugin without unresolved dispatch symbols or compiler warnings introduced by this sprint.
- A focused unit test initializes the plugin, confirms UDF9 registration, verifies repeated initialization/finalization is safe, and confirms a representative unimplemented operation returns the documented error without creating a file.
- Autoload verification confirms the installed plugin can be discovered through the UDF9 `.ncrc` keys without an explicit initialization call.
- Configuration with HDF5 older than 2.1.1 fails with a clear message only when the user explicitly enables NEXTCDF-4 (`-DNEP_ENABLE_NEXTCDF4=ON`); with NEXTCDF-4 off (the default) all unrelated NEP features remain buildable.
- Public UDF constants, generated feature macros, installed configuration, and user-facing documentation agree on whether NEXTCDF-4 is available.

**Out of scope for Sprint 1:**
- HDF5 `create`, `open`, `close`, or metadata discovery.
- Superblock selection, `_Nextcdf4Backend`/`_Nextcdf4Model` markers, and `NC_NETCDF4_MODEL` behavior.
- Dimensions, variables, attributes, groups, user-defined types, filters, data I/O, renaming fixes, Fortran APIs, `nextcopy`, and `nextdump`.
- Automatic magic-number selection for HDF5 files; NEXTCDF-4 selection remains explicit through `NC_NEXTCDF4`.

#### Sprint 2: Implement the File Lifecycle
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Create-Time Backend Selection," "Open-Time Backend Selection," "Stored Markers," "Use Superblock v3," and "File life-cycle."

**Objective:** Replace the Sprint 1 file-operation stubs with a complete lifecycle for empty NEXTCDF-4 files. When this sprint is complete, `nc_create()`, `nc_open()`, and `nc_close()` will work for NEXTCDF-4 files selected explicitly with `NC_NEXTCDF4`; metadata definition and data I/O remain later work.

**Prerequisites and constraints:**
- Build on the Sprint 1 UDF9 dispatch registration, `.ncrc` autoload configuration, dependency checks, and `NEP_HAS_NEXTCDF4` feature macro.
- Continue using `/usr/local/netcdf-c` and `/usr/local/hdf5-2.1.1` for local development, with Fortran disabled for this sprint.
- Keep NEXTCDF-4 external to netcdf-c: do not modify netcdf-c's dispatch, model-inference, or HDF5 backend sources.
- Treat the implementation as a clean rewrite. Existing netcdf-c `libsrc4` and `libhdf5` code may be studied for required behavior and ABI contracts but must not be copied.
- Require explicit `NC_NEXTCDF4` selection for both create and open; do not register the shared HDF5 magic number for UDF9.

**Lifecycle design:**
- Add a private `nxt4internal.h` containing the minimum per-file state for this sprint: the owning `NC`/ncid relationship, normalized mode, path, HDF5 file and root-group identifiers, read-only/define-mode state, and ownership flags used for reliable cleanup.
- Split lifecycle code out of `nxt4dispatch.c` into `nxt4create.c`, `nxt4open.c`, and `nxt4file.c`, leaving dispatch registration and table wiring in `nxt4dispatch.c`.
- Attach NEXTCDF-4 state to the netcdf-c `NC` handle through the supported dispatch interface, initialize the minimum root `NC_FILE_INFO_T`/`NC_GRP_INFO_T` metadata required by existing `NC4_*` inquiry callbacks, and remove all registered state on close or failed initialization.
- Centralize HDF5 identifier cleanup so partial create/open failures close resources exactly once and never leave a usable ncid, leaked identifier, or half-created in-memory file entry.
- Translate HDF5 failures to stable NetCDF error codes and suppress expected HDF5 diagnostic-stack output on handled error paths.

**Create path (`nxt4create.c`):**
- Implement `NEXTCDF4_create()` and internal helpers to validate the path and create-mode flags before allocating persistent state.
- Accept `NC_NEXTCDF4 | NC_CLOBBER` and `NC_NEXTCDF4 | NC_NOCLOBBER`; accept redundant `NC_NETCDF4 | NC_NEXTCDF4` as specified by the NEXTCDF-4 plan.
- Reject incompatible format-selection flags and reject `NC_CLASSIC_MODEL | NC_NETCDF4_MODEL` together with `NC_EINVAL`.
- Add the public `NC_NETCDF4_MODEL` flag defined by the plan, ensuring its bit does not overlap the installed netcdf-c mode flags.
- Check the linked HDF5 runtime version before creating a file and fail cleanly if it is older than the configured minimum.
- Create an HDF5 file-access property list and select library-version bounds according to the plan: `H5F_LIBVER_LATEST` for native NEXTCDF-4 and `NC_CLASSIC_MODEL`, and `H5F_LIBVER_V110` for `NC_NETCDF4_MODEL` compatibility files.
- Map `NC_CLOBBER`/`NC_NOCLOBBER` to `H5F_ACC_TRUNC`/`H5F_ACC_EXCL` and preserve the expected NetCDF errors for an existing file, invalid path, or permission failure.
- Create/open the root group and write `_Nextcdf4Backend` on every new file; write `_Nextcdf4Model = 1` only when `NC_NETCDF4_MODEL` was requested. Flush these markers before returning success.
- Return a valid ncid in define mode with zero dimensions, variables, and user attributes visible through the basic NetCDF inquiry API.

**Open path (`nxt4open.c`):**
- Implement `NEXTCDF4_open()` and internal helpers using `H5F_ACC_RDONLY` for `NC_NOWRITE` and `H5F_ACC_RDWR` for `NC_WRITE`.
- Require a valid HDF5 file and `_Nextcdf4Backend`; reject non-HDF5 inputs and unmarked HDF5/legacy NetCDF-4 files rather than silently claiming files owned by the built-in backend.
- Read and validate `_Nextcdf4Backend` and optional `_Nextcdf4Model`, restore the mode recorded by the file, and reject malformed or unsupported marker values.
- Reconstruct only the root and empty-file metadata needed in this sprint. Opening files that contain dimensions, variables, subgroups, user-defined types, or unsupported NEXTCDF-4 metadata must fail explicitly rather than returning incomplete metadata.
- Honor read-only mode throughout the in-memory state so later modifying calls cannot accidentally write through an `NC_NOWRITE` handle.

**Close, abort, and basic inquiry path (`nxt4file.c`):**
- Implement `NEXTCDF4_close()` to flush writable files, close the root group and HDF5 file, free path and metadata allocations, detach dispatch state, and return the first meaningful cleanup error.
- Implement `NEXTCDF4_abort()` so a newly created define-mode file can be abandoned without leaking HDF5 or netcdf-c resources. Sprint 2 retains the valid empty file, matching the close path, so it can be reopened explicitly with `NC_NEXTCDF4`; test this behavior.
- Implement the minimum `sync`, `inq_format`, and `inq_format_extended` behavior needed for a valid lifecycle. `nc_inq_format()` reports NetCDF-4 semantics, while `nc_inq_format_extended()` identifies the UDF9/NEXTCDF-4 backend and returns the effective mode.
- Make repeated close impossible through normal ncid validation and ensure operations on a closed ncid return the standard bad-ID error.

**Implementation sequence:**
1. Define private lifecycle structures, ownership rules, and shared cleanup/error helpers.
2. Implement create for an empty native NEXTCDF-4 file, then verify the HDF5 signature, superblock version, and backend marker directly with HDF5.
3. Implement close/abort and verify that all HDF5 object counts return to their pre-operation values.
4. Implement read-only open/close of a file created in step 2, including marker validation and empty root metadata reconstruction.
5. Add read/write open, clobber/no-clobber handling, runtime-version checks, and failure-path cleanup.
6. Add `NC_NETCDF4_MODEL`, compatibility-mode property bounds, and marker restoration.
7. Replace lifecycle stubs in the dispatch table, then expand direct-registration and `.ncrc` autoload tests to exercise the public `nc_*` API.
8. Update design and user documentation with selection flags, compatibility behavior, and the intentional empty-file limitation for Sprint 2.

**Verification and acceptance criteria:**
- `nc_create(path, NC_NEXTCDF4 | NC_CLOBBER, &ncid)` succeeds, returns a valid ncid, and `nc_close(ncid)` produces a valid marked HDF5 file.
- `nc_open(path, NC_NEXTCDF4 | NC_NOWRITE, &ncid)` and `nc_open(path, NC_NEXTCDF4 | NC_WRITE, &ncid)` both succeed for a Sprint 2 file and close cleanly.
- `nc_inq()` on a newly created or reopened file reports zero dimensions, zero variables, zero visible global attributes, and no unlimited dimension.
- Native and `NC_CLASSIC_MODEL` files use the planned latest-format bounds; `NC_NETCDF4_MODEL` files use the planned compatibility bounds. Tests inspect the resulting superblock rather than assuming the property-list request succeeded.
- `_Nextcdf4Backend` survives close/reopen, `_Nextcdf4Model` is present only in compatibility mode, and neither marker is counted as a user-visible global attribute.
- `NC_NOCLOBBER` preserves an existing file and returns the expected NetCDF error; `NC_CLOBBER` replaces it.
- Explicit open rejects an ordinary unmarked HDF5/NetCDF-4 file, a non-HDF5 file, a missing file, and malformed marker metadata with stable errors and no leaked HDF5 identifiers.
- Repeated create/open/close cycles pass under HDF5 leak checks; injected or naturally triggered failures at each allocation/open step leave no registered file state behind.
- Direct UDF initialization and `.ncrc` autoload paths both pass the same public lifecycle tests.
- A clean CMake build with `NEP_ENABLE_NEXTCDF4=ON` and `NEP_ENABLE_FORTRAN=OFF` passes the full C test suite against `/usr/local/netcdf-c` and `/usr/local/hdf5-2.1.1`.

**Out of scope for Sprint 2:**
- Defining, reading, or writing dimensions, variables, user attributes, groups, filters, or user-defined datatypes.
- Opening populated NetCDF-4/HDF5 files or files produced by the built-in netcdf-c HDF5 backend.
- `redef`, `enddef`, fill behavior beyond lifecycle defaults, parallel I/O, diskless/in-memory files, and non-default virtual file drivers.
- Rename fixes, dimension scales, `_Netcdf4Coordinates`, `_Netcdf4Dimid`, new numeric/reference/bitfield types, Fortran APIs, `nextcopy`, and `nextdump`.

#### Sprint 3: Build the Core Metadata Model
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Dimension Scales and Dimension Mapping," "Backward Compatibility," "Implementation Phases," and the type-mapping summary.

**Objective:** Replace the core metadata stubs with a persistent root-group metadata implementation for dimensions, variables, and attributes. Sprint 3 supports complete create/define/inquire/close/reopen round trips for metadata built from the existing fixed-size NetCDF atomic types; variable data I/O and dimension-scale attachment remain Sprint 4 work.

**Scope decisions:**
- Persist Sprint 3 metadata to HDF5 and reconstruct it through NEXTCDF-4 on reopen rather than limiting the sprint to an in-memory model or HDF5-only inspection.
- Support both global and variable attributes, including definition, retrieval, inquiry, rename, and deletion.
- Create basic HDF5 dimension-scale datasets and stable `_Netcdf4Dimid` values in this sprint. Attaching scales to variable datasets and writing `_Netcdf4Coordinates` remain in Sprint 4.
- Support the fixed-size standard atomic types allowed by the active model: `NC_BYTE`, `NC_CHAR`, `NC_SHORT`, `NC_INT`, `NC_FLOAT`, `NC_DOUBLE`, `NC_UBYTE`, `NC_USHORT`, `NC_UINT`, `NC_INT64`, and `NC_UINT64`. Defer `NC_STRING`, user-defined types, and all NEXTCDF-4-specific types.

**Prerequisites and constraints:**
- Build on the Sprint 2 file lifecycle, root `NC_FILE_INFO_T`/`NC_GRP_INFO_T` state, HDF5 identifiers, mode validation, markers, synchronization, and cleanup behavior.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use netcdf-c's common NetCDF-4 metadata structures and inquiry helpers where their contracts fit, while keeping all new HDF5 persistence and discovery code in `src/nextcdf4/`.
- Restrict Sprint 3 to the root group. Group creation and nested metadata remain Sprint 5 work.
- Enforce `NC_NOWRITE`, define-mode, duplicate-name, invalid-ID, invalid-name, and active-model restrictions with standard NetCDF error codes.
- Preserve Sprint 2's hidden backend/model markers and ensure they never appear as user attributes.

**Metadata architecture:**
- Extend private NEXTCDF-4 state with the ownership and lookup information needed to keep HDF5 objects synchronized with `NC_FILE_INFO_T`, `NC_GRP_INFO_T`, `NC_DIM_INFO_T`, `NC_VAR_INFO_T`, and `NC_ATT_INFO_T` metadata.
- Add focused source modules for dimensions, variables, attributes, metadata loading, and shared type/name helpers rather than growing lifecycle or dispatch files into general metadata implementations.
- Allocate stable dimension and variable IDs in definition order and rebuild the same IDs when reopening a file.
- Represent each dimension with a basic HDF5 dimension-scale dataset carrying `_Netcdf4Dimid`. Support one or more unlimited dimensions in enhanced mode, but enforce the classic-model single-unlimited-dimension and ordering rules.
- Create variable datasets with the correct rank, extents, maximum extents, and fixed-size HDF5 atomic datatype. Dataset creation in this sprint establishes persistent variable metadata but does not expose variable data reads or writes.
- Store user attributes on the root group or variable dataset as appropriate and distinguish them from HDF5 and NEXTCDF-4 internal attributes during inquiry and open.
- Load dimensions before variables and variables before their attributes when reopening, validating malformed, duplicate, unsupported, or inconsistent metadata instead of exposing a partial file.

**Dispatch and API coverage:**
- Implement `redef` and `_enddef`, including legal mode transitions, read-only rejection, metadata materialization, flush behavior, and rollback/cleanup after a failed transition.
- Implement dimension definition and inquiry callbacks: define, lookup by name, lookup by ID, unlimited-dimension inquiry, and root-group dimension-ID enumeration. Dimension rename remains Sprint 8 work.
- Implement variable definition and inquiry callbacks: define, lookup by name, lookup by ID, complete variable inquiry, and root-group variable-ID enumeration. Variable rename and all `get_var*`/`put_var*` operations remain later work.
- Implement global and variable attribute put/get/inquiry/name/ID/rename/delete callbacks for the supported atomic types, including zero-length attributes and type conversion behavior provided by the supported NetCDF API contract.
- Keep unsupported callbacks wired to explicit `NC_ENOTBUILT`, `NC_ENOTNC4`, or read-only errors as appropriate; no callback may succeed without implementing its promised state change.
- Update `sync`, `close`, and `open` so pending metadata is materialized and flushed, populated Sprint 3 files can be reconstructed, and partial-load failures release every HDF5 identifier and netcdf-c metadata allocation.

**Implementation sequence:**
1. Define metadata ownership, ID-allocation, HDF5 naming, internal-attribute filtering, and supported-type mapping rules; add shared helpers and focused tests for those rules.
2. Implement define-mode transitions and root metadata transaction boundaries without changing variable data I/O.
3. Implement dimensions, basic dimension-scale creation, `_Netcdf4Dimid`, dimension inquiry, and classic/enhanced unlimited-dimension validation.
4. Implement fixed-size atomic variable definitions and inquiries, creating correctly shaped HDF5 datasets without attaching scales or enabling public data access.
5. Implement global and variable attribute mutation and inquiry, including filtering of all backend, dimension-scale, and NetCDF hidden attributes.
6. Implement populated-file metadata discovery in dependency order and verify stable IDs and equivalent inquiries after close/reopen.
7. Wire the completed callbacks into the dispatch table, retain intentional stubs for later sprints, and expand direct-registration and autoload test coverage.
8. Update NEXTCDF-4 design and user documentation to describe the supported Sprint 3 metadata subset and its intentional I/O limitations.

**Verification and acceptance criteria:**
- A file can define multiple fixed and unlimited dimensions, fixed-size atomic variables of ranks zero and higher, and global and variable attributes, then return matching metadata through all relevant inquiry calls.
- Closing and reopening through `NC_NEXTCDF4` preserves names, IDs, lengths, unlimited status, variable rank/type/dimension IDs, and user attributes.
- Direct HDF5 inspection confirms each dimension has a dimension-scale dataset and stable `_Netcdf4Dimid`, variable datasets have the expected datatype and dataspace, and internal attributes are not counted as user attributes.
- `nc_enddef()` and `nc_redef()` enforce valid transitions; repeated or invalid transitions, metadata mutation outside define mode where prohibited, and all mutation through `NC_NOWRITE` handles return the expected errors.
- Classic-model files reject enhanced atomic types, multiple unlimited dimensions, and an unlimited dimension in a disallowed variable position. Enhanced and `NC_NETCDF4_MODEL` files accept only the types allowed by their respective contracts.
- Duplicate names, invalid names, bad IDs, unsupported types, malformed on-disk metadata, and injected HDF5 failures return stable errors without leaking HDF5 identifiers or leaving partially registered metadata.
- Global and variable attribute put/get/rename/delete operations round-trip all supported types, lengths, and zero-length cases while hidden attributes remain invisible.
- Variable data access continues to return the documented not-implemented error, proving Sprint 3 does not accidentally claim Sprint 4 behavior.
- Direct initialization and `.ncrc` autoload run the same metadata round-trip tests, and a clean NEXTCDF-4-enabled build passes the full C test suite.

**Out of scope for Sprint 3:**
- Variable data reads or writes, dimension-scale attachment, `DIMENSION_LIST`/`REFERENCE_LIST`, and `_Netcdf4Coordinates`; these belong to Sprint 4.
- Nested groups, `NC_STRING`, compound, enum, opaque, or vlen types; these belong to Sprint 5.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior; these belong to Sprint 6.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by Sprint 3; broader interoperability belongs to Sprint 7.
- Dimension and variable rename behavior, which remains Sprint 8 work. Attribute rename is included because it does not alter dimension-scale relationships.
- New floating-point, complex, bitfield, and reference types, tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 4: Implement Variable I/O and Dimension Scales
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Dimension Scales and Dimension Mapping," "Use Superblock v3," "Backward Compatibility," and the type-mapping summary.

**Objective:** Add read and write access for standard atomic variables, including scalar, array, strided, and unlimited-dimension data. Attach the basic HDF5 dimension scales created in Sprint 3, write `_Netcdf4Coordinates` and `DIMENSION_LIST`/`REFERENCE_LIST`, and reconstruct complete variable-to-dimension mappings when files are reopened. The legacy mapped variable I/O calls (`nc_get_varm`/`nc_put_varm`) are intentionally out of scope and will remain unimplemented in NEXTCDF-4 because they are deprecated.

**Scope decisions:**
- Support the full generic variable I/O surface except the deprecated mapped (`varm`) variants: `nc_put_var`/`nc_get_var`, `nc_put_var1`/`nc_get_var1`, `nc_put_vara`/`nc_get_vara`, and `nc_put_vars`/`nc_get_vars` for all fixed-size standard atomic types.
- Do not implement `nc_put_varm`/`nc_get_varm`; keep the dispatch table entries returning `NC_ENOTBUILT` and document that mapped I/O is deprecated in NEXTCDF-4.
- Support full read and extend behavior for unlimited dimensions, including appending record data and re-opening to report the updated length.
- Treat a variable that shares a dimension's name and has the same one-dimensional shape as the basic coordinate variable: it is stored using the same HDF5 dimension-scale dataset as the dimension and is identified during reopen and inquiry.
- Keep strided (`vars`) read and write in Sprint 4; leave the mapped (`varm`) calls unimplemented.

**Prerequisites and constraints:**
- Build on the Sprint 3 metadata model: `NC_FILE_INFO_T`/`NC_GRP_INFO_T`/`NC_DIM_INFO_T`/`NC_VAR_INFO_T`/`NC_ATT_INFO_T`, stable IDs, `_Netcdf4Dimid`, basic dimension scales, hidden attribute filtering, and root-group metadata persistence.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use only public HDF5 APIs and NEP/netcdf-c internal metadata helpers; do not copy netcdf-c `libhdf5` code.
- Restrict variable I/O to the fixed-size standard atomic types supported in Sprint 3. `NC_STRING`, user-defined types, and all NEXTCDF-4-specific types remain later work.
- Preserve `NC_NOWRITE`, define-mode, compatibility-mode, and type restrictions with standard NetCDF error codes.
- Honor the classic model and `NC_NETCDF4_MODEL` storage restrictions defined in the NEXTCDF-4 plan.

**Dimension scale and coordinate architecture:**
- After defining a variable in `nc_def_var`, attach each dimension's existing HDF5 dimension-scale dataset to the variable's HDF5 dataset using the appropriate dimension-scale API.
- Maintain the `DIMENSION_LIST` attribute on the variable and the `REFERENCE_LIST` attribute on each used dimension scale to preserve standard HDF5 dimension-scale semantics.
- Write `_Netcdf4Coordinates` on every variable as a little-endian integer array of the variable's `dimids`, enabling fast variable-to-dimension mapping on reopen.
- When opening a file, prefer `_Netcdf4Coordinates` for variable-to-dimension recovery; retain the dimension-scale matching path as a fallback for files without the hidden attribute.
- Treat a variable whose name matches a dimension and whose shape is one dimension of that length as the coordinate variable for that dimension. The coordinate variable's data and the dimension-scale dataset share the same HDF5 dataset, not a separate one.
- For unlimited coordinate variables, ensure the dimension scale and coordinate dataset extend together when record data is appended.

**Variable I/O architecture:**
- Implement contiguous and chunked HDF5 dataset access in new or extended source modules. Fixed-size variables with no unlimited dimensions use contiguous storage; variables with at least one unlimited dimension use chunked storage with default chunk sizes.
- Implement memory-to-disk and disk-to-memory type conversion using the supported netcdf-c helper for the standard atomic types.
- Implement scalar, one-element, hyperslab, and strided access through a common hyperslab selection helper that maps NetCDF `start`/`count`/`stride` arrays to HDF5 dataspace selections.
- For unlimited variables, support appending record data by extending the dataset along the unlimited dimension. Validate that writes stay within the current+extended shape and that reads do not exceed the current extent.
- Update `sync`, `close`, and `open` so dimension-scale attachments, `_Netcdf4Coordinates`, and dataset extensions are materialized and flushed. Ensure partial-write and partial-load failures release every HDF5 identifier and netcdf-c metadata allocation.

**Dispatch and API coverage:**
- Implement `get_vara` and `put_vara` for all supported standard atomic types, scalar and array variables, and unlimited-dimension extension.
- Implement `get_var1` and `put_var1` as single-element `get_vara`/`put_vara` calls.
- Implement `get_var` and `put_var` as full-variable reads and writes using the variable's current shape.
- Implement `get_vars` and `put_vars` for strided hyperslab access. `get_varm` and `put_varm` remain unimplemented and return `NC_ENOTBUILT`.
- Update `inq_var_all` so `contiguousp`, `no_fill`, `fill_valuep`, and `endiannessp` reflect actual dataset storage when applicable.
- Ensure all read and write calls enforce `NC_NOWRITE`, `NC_EBADID`, `NC_EINVAL` for malformed hyperslabs, `NC_EEDGE` for out-of-bounds accesses, and `NC_EBADTYPE` for unsupported in-memory types.
- Keep unsupported variable controls (chunking, compression, filters, checksums, endianness, quantization) wired to explicit `NC_ENOTBUILT` or `NC_ENOTNC4` errors as appropriate.

**Implementation sequence:**
1. Add the dimension-scale attachment step to `nc_def_var`, write `_Netcdf4Coordinates`, `DIMENSION_LIST`, and `REFERENCE_LIST`, and add focused tests verifying the attachments persist after close/reopen.
2. Implement a shared HDF5 dataspace selection and I/O helper for `vara` access; cover scalar, fixed-shape, and unlimited-dimension cases.
3. Implement `put_vara`/`get_vara` and the generic `put_var`/`get_var` wrappers for supported atomic types, including memory type conversion and fill-value handling.
4. Implement `put_var1`/`get_var1` as single-element wrappers.
5. Implement `put_vars`/`get_vars` for strided access using the same selection helper.
6. Implement unlimited-dimension dataset extension and coordinate-variable extension, with tests for append and reopen.
7. Wire the completed I/O callbacks into the dispatch table, keep `put_varm`/`get_varm` as `NC_ENOTBUILT` stubs, and expand direct-registration and autoload test coverage.
8. Update NEXTCDF-4 design and user documentation to describe the supported I/O subset, the deprecated `varm` calls, and the coordinate-variable behavior.

**Verification and acceptance criteria:**
- Scalar, fixed-array, and unlimited variables can be written and read back with matching values for all fixed-size standard atomic types.
- `nc_put_var1`/`nc_get_var1` write and read a single element; `nc_put_vara`/`nc_get_vara` write and read arbitrary contiguous hyperslabs; `nc_put_vars`/`nc_get_vars` write and read strided selections.
- `nc_put_varm`/`nc_get_varm` return `NC_ENOTBUILT` with a documented deprecation note.
- Variables with multiple dimensions use the correct `start`/`count`/`stride` ordering; out-of-bounds and malformed selections return `NC_EEDGE` or `NC_EINVAL` without writing partial data.
- Unlimited variables can be extended by writing beyond the current length; `nc_inq_dimlen` reports the new length; reopening the file preserves and reports the extended length.
- Coordinate variables share the dimension-scale dataset; reading and writing the coordinate variable updates the dimension values, and reopening identifies the variable as the coordinate for that dimension.
- Direct HDF5 inspection confirms `DIMENSION_LIST` on variables, `REFERENCE_LIST` on dimension scales, `_Netcdf4Coordinates` on every variable, and `_Netcdf4Dimid` on every dimension scale.
- `nc_get_var`/`nc_put_var` round-trip all supported atomic types for scalar, fixed, and unlimited variables.
- Read-only, non-existent variable, bad ID, and unsupported type cases return stable errors without leaking HDF5 identifiers.
- Direct initialization and `.ncrc` autoload run the same variable I/O tests, and a clean NEXTCDF-4-enabled build passes the full C test suite.

**Out of scope for Sprint 4:**
- `nc_put_varm`/`nc_get_varm` are intentionally unimplemented because they are deprecated in NEXTCDF-4.
- User-defined types, `NC_STRING`, compound, enum, opaque, vlen, and all NEXTCDF-4-specific types; these belong to Sprints 5, 9, and 10.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior beyond default fill values; these belong to Sprint 6.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by NEXTCDF-4; broader interoperability belongs to Sprint 7.
- Dimension and variable rename behavior, which remains Sprint 8 work.
- New floating-point, complex, bitfield, and reference types, tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 5: Add Groups and User-Defined Types
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially "Correct Renaming of Dims and Vars" for group/type interactions, "Backward Compatibility," the type-mapping summary, and the enhanced-data-model contract.

**Objective:** Extend NEXTCDF-4 from a single root group to the full enhanced NetCDF-4 data model with arbitrarily nested groups, plus user-defined types and string variables. Implement definition, inquiry, attribute, and basic variable I/O for `NC_STRING` and for compound, enum, opaque, and vlen types, while enforcing classic-model and `NC_NETCDF4_MODEL` restrictions.

**Scope decisions:**
- Support arbitrary-depth nested groups, including group creation, parent/child inquiry, full-name and full-path lookup, and per-group namespaces for dimensions, variables, types, attributes, and subgroups.
- Implement all user-defined types that are already part of the classic enhanced NetCDF-4 model: compound, enum, opaque, and vlen, plus the base `NC_STRING` variable-length string type. NEXTCDF-4-specific types (float16 variants, complex, bitfield, reference) remain Sprints 9 and 10.
- Support both fixed-size `NC_CHAR` arrays and true variable-length `NC_STRING` strings; `NC_STRING` is represented with HDF5 variable-length `H5T_C_S1` and is forbidden in classic model and `NC_NETCDF4_MODEL` mode.
- Support basic read and write for variables of all types introduced in this sprint, including scalar, fixed-array, and unlimited-dimension access, using the existing hyperslab I/O machinery. Compound, enum, opaque, vlen, and string attribute I/O is also included.

**Prerequisites and constraints:**
- Build on the Sprint 4 metadata and I/O foundation: stable IDs, in-memory `NC_FILE_INFO_T`/`NC_GRP_INFO_T`/`NC_DIM_INFO_T`/`NC_VAR_INFO_T`/`NC_ATT_INFO_T` trees, dimension scales, `_Netcdf4Coordinates`, hidden attribute filtering, and atomic type I/O.
- Continue to require explicit `NC_NEXTCDF4` selection and keep all changes within NEP; do not modify netcdf-c or its built-in HDF5 backend.
- Use only public HDF5 APIs and the existing NEP/netcdf-c internal metadata helpers; do not copy netcdf-c `libhdf5` code.
- Enforce the classic model and `NC_NETCDF4_MODEL` restrictions: no nested groups, no user-defined types other than those allowed, no `NC_STRING`, and only classic atomic variables in `NC_CLASSIC_MODEL`; in `NC_NETCDF4_MODEL` no user-defined types and no `NC_STRING`.
- Preserve stable object IDs across dimensions, variables, groups, and types so that reopening reconstructs a consistent namespace.

**Group architecture:**
- Move from a single hard-coded root group to a recursive `NC_GRP_INFO_T` tree under `NC_FILE_INFO_T`/`root_grp`.
- Implement `def_grp` to create an HDF5 subgroup and a matching `NC_GRP_INFO_T` child, allocating stable group IDs and recording parent/child relationships.
- Implement group lookup by full name, parent group, direct and recursive child enumeration, `ncid` derivation, and `inq_ncid`/`inq_grps`/`inq_grpname`/`inq_grpname_full`/`inq_grp_parent`/`inq_grp_full_ncid`.
- Distinguish group-local dimensions and variables from those in parent or sibling groups; dimension IDs remain file-scoped and must be resolvable from any group that contains them.
- Keep group-hidden attributes (`_Nextcdf4Backend`, `_Nextcdf4Model`) on the root group and add a group-marker attribute only if needed for fast root-group identification of subgroups.
- Maintain a single `_Nextcdf4VarDimids` per variable and `_Nextcdf4Dimid` per dimension scale regardless of the group that owns them.

**User-defined type architecture:**
- Add a type-creation and commit pipeline for compound, enum, opaque, and vlen, storing each committed HDF5 type inside its owning group and registering it in the group `NC_TYPE_INFO_T` index.
- Track type references (variables and attributes) so that types cannot be deleted while in use; for this sprint, committed types are immutable after creation.
- Implement compound member insertion with fixed-size atomic base types and, later, nested compounds. Keep the `r`/`i` structural rule for `NC_COMPLEX` detection out of this sprint; plain compounds are created with user-supplied member names.
- Map enum base types to the fixed-size signed and unsigned integers supported by the active model; store values in little-endian committed HDF5 `H5T_ENUM` datatypes.
- Map opaque types to `H5T_OPAQUE` with the declared size; provide round-trip `put`/`get` as raw bytes.
- Map vlen types to `H5T_VLEN` over a fixed-size atomic or `NC_CHAR`/`NC_STRING` base type; support the standard `put`/`get` vlen element helper APIs.
- Map `NC_STRING` to HDF5 variable-length `H5T_C_S1` with native memory layout `hvl_t`.
- Support attribute read/write for all new types, including compound, enum, opaque, vlen, and `NC_STRING`.
- Load all committed types before variables when opening a file so that variable and attribute datatypes can be resolved during variable discovery.

**Variable I/O extension:**
- Extend the existing `var_io` helper to dispatch on `var->type_info->nc_type_class` and, for user-defined types, to use the committed HDF5 type as the file datatype and a correctly shaped memory buffer.
- For compound and opaque variables, treat the memory buffer as raw bytes of the declared size and rely on HDF5 for endian conversion.
- For enum variables, support read and write using the enum's base type memory buffer; conversions between the numeric base and the enum are not yet required to be lossy-checked.
- For vlen and `NC_STRING` variables, use `hvl_t` or `char **` memory buffers, respectively, and free all returned elements on close or after `nc_free_vlens`.
- Extend coordinate-variable detection and dimension-scale attachment to handle variables whose dimensions are in the same group or an ancestor group.
- Keep `nc_put_varm`/`nc_get_varm` unimplemented and deprecated.

**Dispatch and API coverage:**
- Implement `def_grp`, `inq_ncid`, `inq_grps`, `inq_grpname`, `inq_grpname_full`, `inq_grp_parent`, and `inq_grp_full_ncid`.
- Implement `inq_typeids`, `inq_user_type`, `inq_typeid`, `inq_type_equal`, `inq_compound_field`, and `inq_compound_fieldindex`.
- Implement `def_compound`, `insert_compound`, `insert_array_compound`, `def_vlen`, `put_vlen_element`, `get_vlen_element`, `def_enum`, `insert_enum`, `inq_enum_member`, `inq_enum_ident`, and `def_opaque`.
- Update `def_var`, `put_att`, `get_att`, and `inq_var_all` so they accept and report the new types and groups correctly.
- Keep `NC_COMPLEX`/`NC_DOUBLECOMPLEX`, `NC_BITFIELD*`, `NC_FLOAT16`, and reference types as `NC_ENOTBUILT` or `NC_EBADTYPE` until Sprints 9 and 10.

**Implementation sequence:**
1. Add recursive group creation and discovery to `nxt4meta.c` or a new `nxt4group.c`, including HDF5 group link creation, parent-child indexing, and stable group-ID allocation.
2. Extend dimension and variable lookup to support group-local and ancestor-group dimension resolution, and add tests for group scoping.
3. Add a type-commitment module for compound, enum, opaque, and vlen types, and extend `NC_TYPE_INFO_T` ownership, reference counting, and on-disk committed-type naming.
4. Extend `NEXTCDF4_map_hdf_type` and `set_var_type` to recognize the new base and user-defined types and load committed HDF5 types on open.
5. Extend the attribute and variable I/O paths to support `NC_STRING` and the user-defined types with appropriate memory layouts.
6. Update the dispatch table, group/type inquiry callbacks, and classic/enhanced compatibility checks.
7. Add a dedicated `tst_nextcdf4_group.c` and expand `tst_nextcdf4_meta.c` with type and string round-trip tests.
8. Update NEXTCDF-4 design and user documentation to describe the new groups, types, and classic/compat restrictions.

**Verification and acceptance criteria:**
- Files with nested groups can be created, closed, reopened, and navigated using full names and `inq_grp*` functions; duplicate and invalid group names return the expected errors.
- Variables and dimensions defined in subgroups are resolved only from their owning and ancestor groups, and IDs remain stable across reopens.
- Compound, enum, opaque, vlen, and `NC_STRING` types can be defined, used in variables and attributes, and round-tripped with `put`/`get` for scalar, fixed, and unlimited cases.
- `NC_CHAR` multi-dimensional arrays continue to work as fixed-size character arrays, and `NC_STRING` works as variable-length strings.
- Classic-model files reject groups, `NC_STRING`, and all user-defined types; `NC_NETCDF4_MODEL` files reject `NC_STRING` and user-defined types.
- Type inquiry functions return correct names, sizes, base types, enum members, and compound field offsets and types.
- Direct HDF5 inspection confirms committed datatypes in the correct group, dimension scales attached to variables in subgroups, and `_Netcdf4Coordinates` present on all variables.
- A clean NEXTCDF-4-enabled build passes the full C test suite including the new group and type tests.

**Out of scope for Sprint 5:**
- The `NC_COMPLEX`/`NC_DOUBLECOMPLEX` structural reinterpretation, which belongs to Sprint 10.
- `NC_BITFIELD*` types, which belong to Sprint 10.
- `NC_FLOAT16` and other small floating-point types, which belong to Sprint 9.
- Reference types and `nc_ref_object`/`nc_ref_region`, which belong to Sprint 10.
- Chunking controls, compression, filters, checksums, quantization, endianness controls, and detailed fill behavior beyond the defaults, which belong to Sprint 6.
- Dimension and variable renaming, which remains Sprint 8 work.
- General discovery of legacy or arbitrary upstream NetCDF-4/HDF5 layouts beyond the populated subset written by NEXTCDF-4; broader interoperability belongs to Sprint 7.
- Tools, language bindings, parallel I/O, diskless/in-memory files, and non-default HDF5 virtual file drivers.

#### Sprint 6: Add Variable Storage Features
**Detailed Plan**: See `docs/plan/NEXTCDF4_plan.md`, especially the "Variable I/O and chunking" section, the type-mapping summary, and the compatibility rules for `NC_NETCDF4_MODEL`.

**Objective:** Implement chunking, fill behavior, compression, filters, endianness, checksums, quantization, and related variable creation properties. Cover dataset extension and synchronization behavior needed for production use of unlimited dimensions and chunked data.

**Scope decisions:**
- Support both `NC_CHUNKED` (with caller-supplied or auto-computed chunk shapes) and `NC_CONTIGUOUS` storage. Default to chunked for variables with unlimited dimensions and to contiguous for small fixed-size variables unless the caller overrides.
- Implement `nc_def_var_chunking`, `nc_inq_var_chunking`, and the chunk-inquiry helpers.
- Implement fill mode and fill values for all current types: classic atomic, `NC_STRING`, compound, enum, opaque, and vlen. Support `nc_def_var_fill`, `nc_set_fill`, `nc_inq_var_fill`, and `NC_NOFILL`.
- Implement compression and checksum filters: `nc_def_var_deflate` (zlib level 0-9 plus optional shuffle), `nc_def_var_fletcher32`, and `nc_def_var_filter` for registering HDF5 filter IDs that the linked HDF5 supports.
- Implement `nc_def_var_endian` with `NC_ENDIAN_LITTLE`, `NC_ENDIAN_BIG`, and `NC_ENDIAN_NATIVE`.
- Implement `nc_def_var_quantize` with `NC_QUANTIZE_BITGROOM`, `NC_QUANTIZE_GRANULARBR`, and `NC_QUANTIZE_BITROUND` for numeric variables.
- Keep `nc_def_var_szip`, `nc_def_var_zstandard`, `nc_def_var_bzip2`, and other third-party filters out of this sprint unless they are already available as HDF5 plugins.
- Maintain `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions: no enhanced features in classic; `NC_NETCDF4_MODEL` must only use filter/chunking/fill settings that upstream netcdf-c can read back.

**Prerequisites and constraints:**
- Build on the Sprint 5 group/type and Sprint 4 I/O foundations: stable object IDs, recursive group metadata, type info, and the `NC_VAR_INFO_T`/`NEXTCDF4_VAR_INFO_T` structures.
- Use HDF5 Dataset Creation Property Lists (DCPL) and Dataset Access Property Lists (DAPL) only through public HDF5 APIs.
- Keep the existing `NC_NEXTCDF4` opt-in and do not modify netcdf-c or the built-in HDF5 backend.
- Preserve round-trip compatibility for `NC_NETCDF4_MODEL` files; write standard HDF5 chunking, fill, deflate, shuffle, and fletcher32 attributes.
- Ensure `nc_sync`, `nc_enddef`, and write operations correctly extend unlimited dimensions and flush chunked datasets.

**Variable storage architecture:**
- Introduce `src/nextcdf4/nxt4var.c` (or extend `nxt4io.c`) to centralize variable creation, DCPL construction, and the `def_var_chunking/deflate/fill/endian/fletcher32/quantize` dispatch functions.
- Build the DCPL with: chunk shape, fill value (`H5Pset_fill_value` or `NC_NOFILL`), deflate+shuffle filter (`H5Pset_deflate`/`H5Pset_shuffle`), fletcher32 (`H5Pset_fletcher32`), endianness (`H5Tset_order` on the file type), and quantization state.
- Store the file type with the chosen endianness in `var->type_info`/`format_type_info`; preserve the memory type as native.
- Apply quantization during write: round floating-point or integer data to the requested number of significant bits before calling `H5Dwrite`; on read, return the quantized (stored) values.
- Load chunking, fill, filter, endian, and quantization metadata when opening an existing file by reading the DCPL and dataset attributes; update `NC_VAR_INFO_T` accordingly.
- Continue to support `nc_put_vara`/`nc_get_vara`/`nc_put_vars`/`nc_get_vars` for chunked and unlimited variables; ensure `H5Dset_extent` is called when writing beyond the current unlimited dimension length.

**Dispatch and API coverage:**
- `nc_def_var_chunking`, `nc_inq_var_chunking`
- `nc_def_var_deflate`, `nc_inq_var_deflate`
- `nc_def_var_fill`, `nc_inq_var_fill`, `nc_set_fill`
- `nc_def_var_endian`, `nc_inq_var_endian`
- `nc_def_var_fletcher32`, `nc_inq_var_fletcher32`
- `nc_def_var_quantize`, `nc_inq_var_quantize`
- `nc_sync`, `nc__enddef`, and `nc_redef` already exist; extend them to flush/create datasets and extend unlimited dimensions for new variables.

**Implementation sequence:**
1. Add `nxt4var.c` skeleton and move existing `NEXTCDF4_def_var`/`var_create_dataset` logic into it or from `nxt4io.c`.
2. Implement DCPL builders for chunking, fill, deflate+shuffle, fletcher32, endian, and quantization.
3. Hook the DCPL into `NEXTCDF4_def_var` and `NEXTCDF4__enddef`.
4. Extend the I/O path to apply quantization before writing and to extend unlimited dimensions before `H5Dwrite`.
5. Update `NEXTCDF4_load_metadata` to read DCPL properties and reconstruct chunking/fill/filter/endian/quantize state.
6. Add `test/tst_nextcdf4_chunking.c` with scalar, fixed, unlimited, chunked, compressed, checksummed, big-endian, and quantized round-trip tests.
7. Update `docs/roadmap.md` and `NEXTCDF4_plan.md` to reflect the new capabilities.
8. Audit and bring all Doxygen comments in `src/nextcdf4/` up to date with the current implementation, and make Doxygen maintenance a required step in every future sprint.

**Verification and acceptance criteria:**
- Chunked, contiguous, and auto-chunked variables can be created, written, read, and reopened.
- Deflate/shuffle/fletcher32 round-trips produce the same data and report correct filter state.
- Fill values work for atomic, `NC_STRING`, compound, enum, opaque, and vlen variables; `NC_NOFILL` disables fill.
- `nc_def_var_endian` writes big-endian or little-endian datasets and reads them back as native.
- `nc_def_var_quantize` stores the specified number of significant bits and reads back the quantized values.
- Unlimited chunked variables can be extended and flushed with `nc_sync`/`nc_enddef`.
- `NC_CLASSIC_MODEL` rejects all enhanced storage calls; `NC_NETCDF4_MODEL` only uses upstream-compatible features and the resulting files are readable by netcdf-c.
- A clean NEXTCDF-4 build passes the full C test suite including the new chunking tests.

**Sprint 6 status:**
|- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `src/nextcdf4/nxt4dispatch.c`, and `src/nextcdf4/nxt4internal.h`.
|- `test/tst_nextcdf4_chunking.c` covers `nc_def_var_chunking`, `nc_def_var_deflate`, `nc_def_var_fletcher32`, `nc_def_var_fill`, `nc_def_var_endian`, `nc_def_var_quantize`, and their `nc_inq_var_*` counterparts, including close/reopen round-trips for chunking, filters, fill, and endian settings.
|- Quantization is applied to floating-point data before `H5Dwrite` using `nc4_convert_type`.
|- DCPL properties (chunking, fill, deflate/shuffle, fletcher32, endianness) are read back from the HDF5 dataset when an existing file is opened.
|- The NEXTCDF-4 C test suite passes; the remaining `run_fortran_tests` segfault is unrelated to the NEXTCDF-4 C backend.

**Out of scope for Sprint 6:**
- Szip, Zstandard, BZIP2, LZ4, and other third-party filters (except through `nc_def_var_filter` if already registered).
- Parallel I/O, advanced chunk cache tuning, and non-default HDF5 virtual file drivers.
- Chunking/fill/filter discovery in arbitrarily legacy files not created by NEXTCDF-4 (Sprint 7).
- Rename of chunked/compressed variables (Sprint 8).
- Float16, complex, bitfield, reference types (Sprints 9 and 10).

#### Sprint 7: Open Existing and Populated Files
**Objective:** Expand metadata discovery so NEXTCDF-4 can open populated files produced by both NEXTCDF-4 and the upstream NetCDF-4/HDF5 backend. Support hidden-coordinate fast paths, dimension-scale reference fallback, and the compatibility rules for native, classic-model, and `NC_NETCDF4_MODEL` files. Reading from legacy and third-party HDF5 layouts remains out of scope.

**Scope decisions:**
- `nc_open(..., NC_NEXTCDF4 | NC_NOWRITE)` must be able to open any file previously written by `NC_NEXTCDF4` and any standard `NC_NETCDF4` file produced by the upstream netcdf-c HDF5 backend.
- Autoload behavior (`.ncrc` / `NCRCENV_RC` magic matching) must also route these files to the NEXTCDF-4 backend when the user has not explicitly passed `NC_NEXTCDF4`.
- All groups, dimensions, variables, attributes, and user-defined types reachable from the root must be reconstructed in the same `NC_FILE_INFO_T`/`NC_GRP_INFO_T` hierarchy used by the rest of the backend.
- Dimension-to-variable mapping must first use the `_Netcdf4Coordinates` integer array (the `_Netcdf4Dimid` fast path already used for dimensions) for O(1) reconstruction.
- If `_Netcdf4Coordinates` is absent, fall back to the HDF5 `DIMENSION_LIST` / `REFERENCE_LIST` dimension-scale reference attributes and resolve them to `NC_DIM_INFO_T` entries.
- If neither hidden attribute is present, fall back to matching dimension scales by `NAME` and `CLASS` attributes and by the dimension length, tolerating minor naming/ordering differences where the match is unambiguous.
- Variable datatypes, shapes, chunking, fill, filters, endianness, and quantization state must be recovered from the DCPL and dataset attributes (reusing the Sprint 6 DCPL reader).
- Opened files are read-only with respect to legacy HDF5 layout details: rewriting or re-chunking files not created by NEXTCDF-4 is out of scope.

**Prerequisites and constraints:**
- Build on Sprint 5 recursive group/type loading and Sprint 6 DCPL recovery.
- Use only public HDF5 and HDF5 high-level APIs (`H5Gopen2`, `H5DSget_num_scales`, `H5DSget_scale_name`, `H5Aopen`, `H5Dget_create_plist`, etc.).
- Keep `NC_NEXTCDF4` opt-in and do not modify the upstream netcdf-c or built-in HDF5 backend.
- Preserve `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions when opening files: reject classic-model files that contain unsupported enhanced features, and allow only features that upstream netcdf-c can read back for `NC_NETCDF4_MODEL` files.

**Metadata discovery architecture:**
- Refactor `load_dimensions` and `load_variables` to accept an `NC_GRP_INFO_T *` (and its `hdf_group`) so that every group loads its own dimensions and variables before child groups are processed.
- Update `load_one_dim` to read existing HDF5 dimension-scale datasets, extract `_Netcdf4Dimid`, `CLASS`, and `NAME` attributes, and determine unlimited status from the dataspace max extent.
- Update `load_one_var` to open the variable dataset in the correct group, read `_Netcdf4Coordinates` if available, and otherwise resolve dimensions through `DIMENSION_LIST` references or by scale-name matching.
- Introduce a `resolve_var_dimids` helper that maps a variable's dimension-scale references or scale names to the `dimids` already loaded into the parent `NC_GRP_INFO_T`.
- Ensure `load_global_attributes` and variable attribute loading run after all dimensions and variables for the group have been created, so that user attributes attach to the right container.
- Update `NEXTCDF4_load_metadata` to walk the group tree recursively: load dimensions, variables, and attributes for the root, then repeat for each child group loaded by `load_children`.

**Dispatch and API coverage:**
- `NEXTCDF4_open` is the entry point; the work is in `NEXTCDF4_load_metadata` and the per-group loaders.
- `nc_open` with `NC_NEXTCDF4` and the autoload path must both route to the NEXTCDF-4 backend.
- `nc_inq_*`, `nc_get_var*`, and, for files NEXTCDF-4 created, `nc_put_var*` must work on opened files without redefinition errors.

**Implementation sequence:**
1. Refactor `load_dimensions`/`load_variables` to take a group pointer and operate on `grp->format_grp_info->hdf_group`.
2. Update `load_one_var` to accept the target group and its HDF5 group, and to read `_Netcdf4Coordinates` as the primary dimid source.
3. Add a `DIMENSION_LIST` / `REFERENCE_LIST` reader and a dimension-scale reference resolver.
4. Add a scale-name fallback for files that only expose `CLASS`/`NAME` dimension-scale attributes.
5. Update `load_one_dim` to handle unlimited dimension discovery from existing scale datasets and to validate `_Netcdf4Dimid`.
6. Make `NEXTCDF4_load_metadata` recurse into child groups in the correct dependency order (dims, vars, atts, children).
7. Add compatibility checks for `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` open paths.
8. Add `test/tst_nextcdf4_open.c` with cases for: (a) opening a NEXTCDF-4-written file without `NC_NEXTCDF4` (autoload), (b) opening an upstream `NC_NETCDF4` file with `NC_NEXTCDF4`, and (c) a file without `_Netcdf4Coordinates` that requires dimension-scale fallback.
9. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md` to reflect the new discovery capabilities.
10. Audit and update Doxygen comments in the modified `src/nextcdf4/` files.

**Verification and acceptance criteria:**
- A NEXTCDF-4-created file can be closed and reopened with `nc_open(..., NC_NEXTCDF4)` and all groups, dimensions, variables, attributes, and storage properties are recovered.
- The same file can be opened by autoload (no explicit format flag) when `.ncrc` / `NCRCENV_RC` points to the build `.ncrc`.
- An upstream `NC_NETCDF4` file with `_Netcdf4Coordinates` and `_Netcdf4Dimid` attributes opens and matches the original `nc_inq` results.
- A file with only `DIMENSION_LIST` / `REFERENCE_LIST` attributes opens and produces the same variable-to-dimension mapping.
- A file with only `CLASS`/`NAME` dimension scales opens and falls back to name-based matching.
- Data read back from an opened file (`nc_get_var_*`) matches the stored values for atomic types, chunked or contiguous variables, and unlimited variables.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` compatibility checks reject unsupported files and allow supported ones.
- A clean NEXTCDF-4 build passes the full C test suite including the new open tests.

**Sprint 7 status:**
|- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4file.c`, `src/nextcdf4/nxt4open.c`, `src/nextcdf4/nxt4internal.h`, `test/tst_nextcdf4_open.c`, and `test/CMakeLists.txt`.
|- `load_dimensions`, `load_variables`, `load_group_attributes`, and `load_group_metadata` now operate per-group and recurse through the group tree.
|- `load_one_var` resolves dimensions from `_Netcdf4Coordinates`, attached HDF5 dimension scales (`H5DS`), or `_Netcdf4Dimid`/`NAME` fallbacks.
|- `find_var_cb` and `find_dim_cb` distinguish NEXTCDF-4-created files, upstream `NC_NETCDF4` files, and arbitrary HDF5 datasets.
|- `NEXTCDF4_read_markers` treats a missing backend marker as an upstream NetCDF-4/HDF5 file while still rejecting arbitrary unmarked HDF5 files.
|- `test/tst_nextcdf4_open.c` covers reopening a NEXTCDF-4 file and opening an upstream `NC_NETCDF4` file via `NC_NEXTCDF4`.
|- All NEXTCDF-4 C tests pass; the unrelated `run_fortran_tests` segfault remains unchanged.

**Out of scope for Sprint 7:**
- Writing, re-chunking, or redefining files not originally created by NEXTCDF-4.
- Arbitrary non-NetCDF-4 HDF5 files that do not follow the dimension-scale convention.
- Dimension and variable renaming (Sprint 8).
- Float16, complex, bitfield, and reference types (Sprints 9 and 10).

#### Sprint 8: Correct Dimension and Variable Renaming
**Objective:** Implement atomic, flush-per-operation dimension and variable renaming. Keep in-memory metadata, HDF5 links, dimension-scale relationships, and hidden dimension-mapping attributes consistent across coordinate-variable and shared-dimension edge cases.

**Scope decisions:**
- `nc_rename_dim` and `nc_rename_var` with `NC_NEXTCDF4` must update the HDF5 link, the `NC_DIM_INFO_T`/`NC_VAR_INFO_T` names, and all indexes used for name-based lookup.
- A coordinate variable (a 1D variable whose name equals its dimension's name) shares the same HDF5 dataset with the dimension scale. Renaming one must rename the other and keep the `NAME` dimension-scale attribute in sync.
- A shared dimension used by many variables must only be renamed once; `DIMENSION_LIST`, `REFERENCE_LIST`, and `_Netcdf4Coordinates` references continue to work because they use object references or integer dimids.
- Renaming must work in define mode on a writable file. Read-only files and files in data mode must return `NC_EINVAL`.
- `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` restrictions are enforced by the existing checks; renaming is allowed as long as the resulting names are valid for the active model.

**Prerequisites and constraints:**
- Build on Sprint 6 DCPL and Sprint 7 metadata loading.
- Use public HDF5 APIs: `H5Lmove` for link renaming, `H5DSset_scale` for updating the scale `NAME` attribute, and `H5Aopen`/`H5Awrite` for hidden attributes if `H5DSset_scale` is insufficient.
- Only one HDF5 link move is required for a coordinate-variable/dimension pair.
- Do not rewrite dataset data or dimension-scale attachments; only the link name and the in-memory metadata change.
- Keep `NC_NEXTCDF4` opt-in and do not affect the upstream netcdf-c backend.

**Metadata and dispatch architecture:**
- Implement `NEXTCDF4_rename_dim` in `src/nextcdf4/nxt4dim.c` (or `nxt4meta.c` if a dim file does not yet exist). It finds the `NC_DIM_INFO_T`, validates the new name, moves the dimension-scale HDF5 link with `H5Lmove`, updates the `NAME` attribute with `H5DSset_scale`, updates the `NCindex` entry for dimensions, and updates the associated coordinate variable's `NC_VAR_INFO_T` name if present.
- Implement `NEXTCDF4_rename_var` in `src/nextcdf4/nxt4var.c` (or `nxt4meta.c`). It finds the `NC_VAR_INFO_T`, validates the new name, moves the variable HDF5 link with `H5Lmove`, updates the `NCindex` entry for variables, and updates the associated dimension and scale `NAME` attribute if the variable is a 1D coordinate variable.
- Add `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` to the dispatch table in `nxt4dispatch.c` in place of the `NC_RO_*` fallbacks.
- Use `NC_DIM_INFO_T` to detect the coordinate-variable relationship: a 1D variable with one dimension whose name equals the variable name.
- Ensure `nc4_find_dim`/`nc4_find_var` still work after rename by re-inserting the object into the group dimension/variable index with the new name.

**Implementation sequence:**
1. Implement `NEXTCDF4_rename_dim`, including the `H5Lmove`, `H5DSset_scale`, in-memory name update, and coordinate-variable name sync.
2. Implement `NEXTCDF4_rename_var`, including the `H5Lmove`, in-memory name update, and coordinate-variable dimension rename.
3. Wire `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` into the `NC_DISPATCH` table in `nxt4dispatch.c`.
4. Add `test/tst_nextcdf4_rename.c` covering: simple dimension rename, coordinate-variable dimension rename (one link move, both names update), variable rename that is also a coordinate variable (dimension also renames), rename of a shared dimension used by two variables, and reopen with verification.
5. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md` with the Sprint 8 details.
6. Run all `tst_nextcdf4_*` tests and the full C test suite.

**Verification and acceptance criteria:**
- A NEXTCDF-4 file can have a dimension renamed and the new name is returned by `nc_inq_dim` after `nc_enddef`.
- Renaming a coordinate variable (1D var with same name as its dim) also renames the underlying dimension.
- Renaming a dimension that has a coordinate variable also renames the variable.
- A shared dimension can be renamed without breaking `nc_inq_var` or `nc_get_var` for variables that use it.
- Reopening the file shows the new dimension and variable names.
- All NEXTCDF-4 C tests, including `tst_nextcdf4_rename`, pass.

**Sprint 8 status:**
- Implementation is complete in `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4internal.h`, `src/nextcdf4/nxt4dispatch.c`, `test/tst_nextcdf4_rename.c`, and `test/CMakeLists.txt`.
- `NEXTCDF4_rename_dim` and `NEXTCDF4_rename_var` are wired into the dispatch table and use `H5Lmove` and `H5DSset_scale` to rename HDF5 links and scale names.
- Both functions update in-memory `hdr.name` and rebuild the dimension/variable `NCindex` with `ncindexrebuild`.
- Coordinate variables (1D variables whose name matches their dimension) stay synchronized when either the dimension or the variable is renamed.
- `test/tst_nextcdf4_rename.c` covers simple dim rename, coordinate-variable rename in both directions, shared-dimension rename, and simple variable rename.
- All 9 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 8:**
- Renaming user-defined types, groups, or attributes (attribute rename is already implemented).
- Renaming in files not created by NEXTCDF-4 or in `NC_NETCDF4_MODEL` files where it would violate the classic model.
- Re-chunking, redefining, or data movement as part of a rename.

#### Sprint 9: Add Small Floating-Point Types
**Objective:** Add `NC_FLOAT16` and, when supported by HDF5, bfloat16 and FP8/FP6/FP4 types. Define their public APIs, HDF5 mappings, conversion rules, fill-value behavior, compatibility restrictions, and round-trip coverage.

**Scope decisions:**
- Primary deliverable is `NC_FLOAT16` (IEEE 754 binary16). It maps to `H5T_IEEE_F16LE/BE` and is stored as 2-byte `uint16_t` in memory.
- Because NEXTCDF-4 requires HDF5 2.1.1 or later, also add `NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, and `NC_FLOAT4_E2M1` with the mappings and memory types from the HDF5 2.x small-floating-point datatypes.
- Small floating-point types are atomic NetCDF types, not user-defined types. They use the same `nc_def_var`/`nc_put_vara`/`nc_get_vara` dispatch path as other atomic types.
- `NC_FLOAT16` and the other small floats are only allowed in native NEXTCDF-4 mode; they are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- They cannot be used as coordinate variables because they do not support a total ordering suitable for dimension coordinates. (Superseded by v4.3.0 Sprint 1L1: small floats may be coordinate variables in native mode.)
- The canonical in-memory representation is the raw bit pattern (`uint16_t` for 16-bit, `uint8_t` for 8/6/4-bit types). Users are responsible for producing valid encodings; no automatic conversion to/from `float`/`double` is guaranteed for the non-IEEE formats.

**Prerequisites and constraints:**
- Build on `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, and `NEXTCDF4_check_atomic_type`.
- NEXTCDF-4 requires HDF5 2.1.1 or later, which provides all the small-float predefined datatypes (`H5T_IEEE_F16LE`, `H5T_FLOAT_BFLOAT16LE`, etc.), so no per-type HDF5 version checks are needed.
- `ncindex` and `NC_TYPE_INFO_T` do not need to change; the new types are atomic.

**API and type-model architecture:**
- Define `NC_FLOAT16` (and the optional small-float constants) in `include/nep.h` with values that do not collide with existing `nc_type` constants.
- Add `nc_put_vara_float16`/`nc_get_vara_float16` or document that callers use `nc_put_vara`/`nc_get_vara` with `memtype = NC_FLOAT16` and a `uint16_t` buffer.
- Extend `NEXTCDF4_map_hdf_type` to create `H5T_IEEE_F16LE/BE` (and the HDF5 2.x small-float types) for the new `nc_type` values.
- Extend `map_nc_type` in `nxt4meta.c` to recognize `H5T_FLOAT` with size 2 as `NC_FLOAT16`, and the HDF5 2.x small-float class/type pairs as the other new types.
- Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name` for each new type.
- Update `NEXTCDF4_check_atomic_type` to accept the new types only when `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` are not set.
- Ensure `set_var_type` in `nxt4meta.c` accepts the new `nc_type` values and sets `var->nc_typeid` correctly.
- `var_io` in `nxt4io.c` must use the correct HDF5 memory and file types when the variable or `memtype` is one of the new small floats. For now, require `memtype` equal to the file type to avoid conversion edge cases.

**Implementation sequence:**
1. Add the `NC_FLOAT16` and small-float constants to `include/nep.h`.
2. Update `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, and `NEXTCDF4_type_name` for `NC_FLOAT16`.
3. Update `NEXTCDF4_check_atomic_type` and `set_var_type` to permit `NC_FLOAT16`.
4. Add `test/tst_nextcdf4_float16.c` that creates a `NC_FLOAT16` variable, writes/reads `uint16_t` values with `nc_put_vara`/`nc_get_vara`, and verifies round-trip.
5. Repeat steps 2-4 for bfloat16/FP8/FP6/FP4.
6. Add documentation notes in `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md`.
7. Run the full `tst_nextcdf4_*` test suite and the full C test suite.

**Verification and acceptance criteria:**
- `nc_def_var` with `NC_FLOAT16` succeeds in native NEXTCDF-4 mode.
- `nc_put_vara` and `nc_get_vara` round-trip `uint16_t` values without corruption when `memtype == NC_FLOAT16 == var->nc_typeid`.
- `nc_inq_var` reports `xtype == NC_FLOAT16` for a float16 variable.
- `nc_def_var` with `NC_FLOAT16` fails with `NC_EBADTYPE` in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- Reopening a file with a float16 variable recovers the type correctly.
- bfloat16 and FP8/FP6/FP4 pass the same round-trip tests.
- All existing NEXTCDF-4 C tests continue to pass.

**Sprint 9 status:**
- Implementation is complete in `include/nep.h`, `src/nextcdf4/nxt4internal.h`, `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `test/tst_nextcdf4_float16.c`, and `test/CMakeLists.txt`.
- `NC_FLOAT16` and all HDF5 2.1.1 small floats (`NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, `NC_FLOAT4_E2M1`) are defined and round-trip through `nc_def_var`, `nc_put_vara`, and `nc_get_vara`.
- `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, `NEXTCDF4_check_atomic_type`, `set_var_type`, and `nxt4io.c` `memory_type` all handle the new types.
- `test/tst_nextcdf4_float16.c` covers round-trip for all seven types and rejection in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- All 10 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 9:**
- Automatic conversion between `NC_FLOAT16`/small floats and `NC_FLOAT`/`NC_DOUBLE`; users must supply the correct in-memory type.
- Complex numbers, bitfields, and reference types (Sprint 10).
- Coordinate variables and unlimited dimensions using small floats. (Superseded by v4.3.0 Sprint 1L1 — now allowed.)
- Support for HDF5 versions older than 2.1.1.

#### Sprint 10: Add Complex, Bitfield, and Reference Types
**Objective:** Add complex-number compounds, fixed-width HDF5 bitfields, and object and region references. Include structural type detection, reference creation and dereference APIs, validity checks, storage restrictions, and interoperability with existing HDF5 files.

**Scope decisions:**
- Add `NC_COMPLEX` (single-precision complex) and `NC_DOUBLECOMPLEX` (double-precision complex) as built-in compound-like atomic types. They map to HDF5 compound types `{ float r; float i; }` and `{ double r; double i; }`. In-memory layout is a `float[2]` or `double[2]` where the first element is the real part and the second is the imaginary part.
- Add `NC_BITFIELD8`, `NC_BITFIELD16`, `NC_BITFIELD32`, and `NC_BITFIELD64` as built-in atomic types. They map to `H5T_STD_B*LE/BE` bitfield datatypes and are held in memory as `uint8_t`, `uint16_t`, `uint32_t`, and `uint64_t`. The NetCDF API treats them as unsigned integers; bit decoding is the caller's responsibility.
- Add `NC_REF_OBJECT` and `NC_REF_REGION` as opaque reference types. They map to `H5T_STD_REF_OBJ` and `H5T_STD_REF_DSETREG`. Variables of these types store HDF5 reference tokens as opaque byte arrays. No dereferencing API is exposed in this sprint; callers read and write the opaque tokens.
- All new types are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` because they are not part of the classic NetCDF data model.
- Complex and bitfield variables may be used as regular variables. Reference-typed variables cannot appear inside compounds or attributes and cannot be copied meaningfully between files. (The coordinate-variable ban was lifted in v4.3.0 Sprint 1L1.)

**Prerequisites and constraints:**
- Build on `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, `NEXTCDF4_check_atomic_type`, `set_var_type`, and `nxt4io.c` `memory_type` from Sprints 8 and 9.
- `NC_TYPE_INFO_T` for the new atomic types must carry the right `nc_type_class` (`NC_COMPOUND` for complex, `NC_INT` for bitfields, `NC_OPAQUE` for references) without being confused with user-defined types.
- Complex detection on read relies on `H5T_COMPOUND` with exactly two members named `r` and `i` whose base types are `H5T_FLOAT`/size 4 or `H5T_FLOAT`/size 8. Bitfield detection uses `H5Tget_class == H5T_BITFIELD` and size 1/2/4/8. Reference detection uses `H5Tget_class == H5T_REFERENCE` and `H5Tget_ref_type`.
- Reference variables must be read and written as whole arrays to avoid invalid token states; per-element hyperslab writes are allowed only within the same file.

**API and type-model architecture:**
- Define `NC_COMPLEX`, `NC_DOUBLECOMPLEX`, `NC_BITFIELD8`, `NC_BITFIELD16`, `NC_BITFIELD32`, `NC_BITFIELD64`, `NC_REF_OBJECT`, and `NC_REF_REGION` in `include/nep.h`.
- Extend `NEXTCDF4_map_hdf_type` to create:
  - `NC_COMPLEX`/`NC_DOUBLECOMPLEX` as `H5T_COMPOUND` with two `r`/`i` members.
  - `NC_BITFIELD*` as `H5T_STD_B*LE`.
  - `NC_REF_OBJECT` as `H5T_STD_REF_OBJ` and `NC_REF_REGION` as `H5T_STD_REF_DSETREG`.
- Extend `map_nc_type` to detect and map the corresponding HDF5 datatypes back to the new `nc_type` values.
- Extend `NEXTCDF4_type_size` and `NEXTCDF4_type_name` for each new type.
- Update `NEXTCDF4_check_atomic_type` to allow the new types only in native NEXTCDF-4 mode.
- Update `set_var_type` so `NC_COMPLEX`/`NC_DOUBLECOMPLEX` are recorded as compound-class types, `NC_BITFIELD*` as integer-class types, and `NC_REF_*` as opaque-class types.
- Extend `nxt4io.c` `memory_type` to return the matching in-memory HDF5 datatype (`H5T_NATIVE_FLOAT_COMPLEX`/`H5T_NATIVE_DOUBLE_COMPLEX` for complex, `H5T_NATIVE_B*` for bitfields, and a copy of `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG` for references).
- Ensure `var_io` performs whole-slab I/O for references and requires `memtype` to match the file type for the other new types to avoid unsupported HDF5 conversions.

**Implementation sequence:**
1. Add the new `nc_type` constants to `include/nep.h`.
2. Update `NEXTCDF4_map_hdf_type`, `map_nc_type`, `NEXTCDF4_type_size`, `NEXTCDF4_type_name`, and `NEXTCDF4_check_atomic_type` for complex, bitfield, and reference types.
3. Update `set_var_type` to set the correct `nc_type_class` and minimal `NC_TYPE_INFO_T` state for the new types.
4. Update `nxt4io.c` `memory_type`.
5. Add `test/tst_nextcdf4_complex.c` for complex round-trip and `nc_inq_var` type recovery.
6. Add `test/tst_nextcdf4_bitfield.c` for bitfield round-trip.
7. Add `test/tst_nextcdf4_ref.c` for reference-type write/read as opaque arrays.
8. Update `test/CMakeLists.txt` to build and register the three new tests.
9. Update `docs/roadmap.md` and `docs/plan/NEXTCDF4_plan.md`.
10. Run the `tst_nextcdf4_*` test suite.

**Verification and acceptance criteria:**
- `nc_def_var` with `NC_COMPLEX`/`NC_DOUBLECOMPLEX` succeeds and round-trips `float[2]`/`double[2]` data using `nc_put_vara` and `nc_get_vara`.
- `nc_inq_var` reports the correct `nc_type` for a variable of each new type.
- Bitfield variables round-trip raw unsigned integer patterns.
- Reference-typed variables can be written and read back as opaque byte arrays in the same file.
- `nc_def_var` with each new type fails with `NC_EBADTYPE` or `NC_ENOTNC4` in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- Reopening a file with the new types recovers the type information correctly.
- All existing `tst_nextcdf4_*` C tests continue to pass.

**Sprint 10 status:**
- Implementation is complete in `include/nep.h`, `src/nextcdf4/nxt4meta.c`, `src/nextcdf4/nxt4io.c`, `test/tst_nextcdf4_complex.c`, `test/tst_nextcdf4_bitfield.c`, `test/tst_nextcdf4_ref.c`, and `test/CMakeLists.txt`.
- `NC_COMPLEX` and `NC_DOUBLECOMPLEX` round-trip using the native HDF5 2.1.1 complex datatypes (`H5T_COMPLEX_IEEE_F32LE` and `H5T_COMPLEX_IEEE_F64LE`).
- `NC_BITFIELD8/16/32/64` round-trip as raw unsigned integer bit patterns using `H5T_STD_B*LE` and `H5T_NATIVE_B*`.
- `NC_REF_OBJECT` and `NC_REF_REGION` round-trip as opaque `hobj_ref_t`/`hdset_reg_ref_t` tokens using `H5T_STD_REF_OBJ`/`H5T_STD_REF_DSETREG`.
- All new types are rejected in `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL`.
- All 13 `tst_nextcdf4_*` C tests pass; the pre-existing `run_fortran_tests` segfault is unchanged.

**Out of scope for Sprint 10:**
- Dereferencing object or region references through the NetCDF API (e.g., `H5Rcreate`, `H5Rdereference`, `H5Rget_obj_type3`).
- Reference types inside user-defined compounds, enums, vlen, or attributes.
- Typed convenience functions such as `nc_put_var_complex` or `nc_put_var_bitfield`.
- Conversion between complex numbers and real/imaginary arrays.
- Support for opening arbitrary HDF5 files whose compound types happen to be named `r`/`i` but are not intended as complex numbers (detection is purely structural in this sprint).

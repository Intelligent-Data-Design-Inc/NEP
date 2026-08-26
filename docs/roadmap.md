# NEP Development Roadmap

### V3.5.0 - Add nep_meta.h with Build Info, and NISAR/SWOT Examples

#### Sprint 1: Create nep_meta.h
**Detailed Plan**: See `docs/plan/v3.5.0-sprint1-nep_meta.md`

- Create `include/nep_meta.h` from a committed `include/nep_meta.h.in` template using CMake `configure_file()` at build time; the generated `nep_meta.h` is added to `.gitignore` and never committed.
- Include version (major/minor/patch), build date, compiler name, and feature flags for enabled readers and compression filters.
- Add CMake logic to populate `nep_meta.h.in` placeholders using `version.txt` and the existing `HAVE_*` / `NEP_ENABLE_*` options.
- Update documentation to reference the new meta header and add an install rule for `nep_meta.h`.

**Clarified decisions:**
- `include/nep_meta.h.in` is the committed source of truth; `include/nep_meta.h` is generated into the build tree and added to `.gitignore`, matching the netcdf-c `netcdf_meta.h` convention.
- The build-info header is generated at configure time; build date is a configure-time timestamp, not a per-build dynamic value, to avoid unnecessary rebuilds.
- Feature flags reflect the state of `NEP_ENABLE_*` CMake options (PDB, MMCIF, CDF, GeoTIFF, GRIB2, FITS, PDS4, DICOM, LZ4, BZIP2, Fortran, etc.) as `0`/`1` macros.
- The generated header is installed alongside `nep.h` so downstream projects can inspect build capabilities.
- Acceptance is a compile-only C test `test/tst_meta.c` that includes `nep_meta.h` and validates the version macros.
- No runtime API for querying build info is added in this sprint.

**GitHub Issue:** [#353](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/353)

#### Sprint 2: NISAR Example
**Detailed Plan**: See `docs/plan/v3.5.0-sprint2-nisar-example.md`

- NISAR is a super-cool satellite!
- Let's add some example code which opens NISAR file and plots it in python.
- We want to take the example from /home/ed/nisar_play
- NISAR file is too big for repo, so user must provide path to it on the command line. There is one here: /home/ed/Downloads/NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001_QA_STATS.h5
- CI does not really need to test this, we just need a working example from the command line.
- README.md needs to clearly explain how to run NISAR example, and show the output graph for fun.
- We are interested in NISAR soil moisture for this example.

**Clarified decisions:**
- The example lives in a new top-level `examples/nisar/` directory (not `examples/viz/NISAR/`), since NISAR SME2 files are plain CF/netCDF-compliant HDF5 read directly with `xarray`/`netCDF4` and involve no NEP UDF dispatch code, unlike the per-format `examples/viz/<FORMAT>` scripts.
- The example is Python-only and standalone: not registered in CMake/CTest and not run in CI, matching the roadmap's "CI does not really need to test this" note.
- The CLI mirrors `nisar_play`'s `plot-sme2` command: a required positional NISAR SME2 `.h5` path, plus `--output-dir` and `--show` flags, porting `sme2.py`/`plots.py` logic with minimal changes.
- Dependencies match `nisar_play` exactly: `xarray`, `netCDF4`, `matplotlib`, `cartopy`, plus `earthaccess` for the bbox fetch path, declared in a new `examples/nisar/requirements.txt`.
- The Earthdata bbox-search/download capability (`fetch.py`, `--bbox` flag) is ported over in full, including `~/.netrc` / `EARTHDATA_USERNAME`/`EARTHDATA_PASSWORD` credential handling, matching `nisar_play` feature-for-feature.
- A pre-generated soil-moisture PNG (produced from the sample SME2 file) is committed under `examples/nisar/` and embedded in `examples/nisar/README.md` and referenced from the top-level `README.md`.
- No changes are made to NEP's C library, `include/nep.h`, or the UDF dispatch framework this sprint; this is a pure Python usage example.

**GitHub Issue:** [#355](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/355)

#### Sprint 3: SWOT Example
**Detailed Plan**: See `docs/plan/v3.5.0-sprint3-swot-example.md`

- Add a standalone Python example under `examples/swot/` that opens a SWOT gridded sea-surface-height product (NASA PO.DAAC `SWOT_L2_LR_SSH_*` or AVISO+ `SWOT_L3_LR_SSH` NetCDF) and plots sea surface height anomaly (`ssha`) with `xarray`/`netCDF4`, `matplotlib`, and `cartopy`.
- Mirror the `examples/nisar/` structure: a `swot_example/` package with reader, plots, CLI, and an optional Earthdata/AVISO+ fetch module; `requirements.txt`; `README.md`; and committed example output PNG(s).
- The CLI takes a local `.nc` file path or a `--bbox W S E N` fetch path; the fetch path uses `earthaccess` against the chosen collection, with credentials from `~/.netrc` or environment variables.
- Write `docs/netCDF_with_SWOT.md` as a companion paper mirroring `docs/netCDF_with_NISAR.md`: explain how SWOT gridded SSH products map to netCDF-4, show `ncdump`/`h5dump` excerpts, and reference the example code.
- Update top-level `README.md` and `examples/README.md` to list the new SWOT example.

**Clarified decisions:**
- Target product is a SWOT gridded sea-surface-height NetCDF (e.g., PO.DAAC `SWOT_L2_LR_SSH_D` Basic/Expert or AVISO+ `SWOT_L3_LR_SSH` Basic). The primary plotted variable is `ssha` (sea surface height anomaly), masked with the product `quality_flag` when present; coordinates come from 2D `latitude`/`longitude` variables.
- The example is a standalone Python package (not a NEP UDF reader and not added to CMake/CTest/CI), following the NISAR Sprint 2 precedent.
- The fetch path uses `earthaccess` and NASA Earthdata/PO.DAAC credentials (`~/.netrc` or `EARTHDATA_USERNAME`/`EARTHDATA_PASSWORD`); if AVISO+ `L3_LR_SSH` is chosen, the fetch implementation switches to AVISO+ FTP/THREDDS access and documents AVISO+ credential setup.
- The reader is organized as `examples/swot/swot_example/l3_ssh.py`, `plots.py`, `fetch.py`, and `cli.py`, invoked via `python -m swot_example FILE` or `python -m swot_example --bbox W S E N`.
- Output PNGs (e.g., `ssha_map.png` and a `swath_footprint.png` overview) are committed under `examples/swot/figures/` and embedded in `examples/swot/README.md` and referenced from `docs/netCDF_with_SWOT.md`.
- No changes are made to NEP's C library, `include/nep.h`, the build system, or CI this sprint; this is documentation- and example-only.

**GitHub Issue:** [#357](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/357)

### V3.4.0 - More Proteins with mmCIF Format

#### Sprint 1: Setup
**Detailed Plan**: See `docs/plan/v3.4.0-sprint1-mmcif-setup.md`

- Refer to the `mmcif` skill file (`/.devin/skills/mmcif/SKILL.md`) and the `pdb-legacy` skill for the relationship between legacy PDB and mmCIF.
- Add the PDBx/mmCIF UDF8 slot definitions to `include/nep.h` (`NEP_UDF_MMCIF`, `NEP_MAGIC_MMCIF`, `NEP_FORMAT_NAME_MMCIF`).
- Add the `NEP_ENABLE_MMCIF` CMake option (default OFF) and the `HAVE_MMCIF`/`#cmakedefine HAVE_MMCIF` plumbing.
- Create a no-op dispatch skeleton (`include/mmcifdispatch.h`, `src/mmcifdispatch.c`, `src/mmciffile.c`) that builds `libncmmcif.so` and registers `"data_"` magic on UDF8.
- Ensure the existing `test/data/mmCIF/*.cif` files (from https://www.rcsb.org) are copied into the build tree.
- Add a C smoke test `test/tst_mmcif_udf.c` that opens and closes a real `.cif` file through `nc_open()`.
- Add a Fortran smoke test `ftest/ftst_mmcif_udf.F90` that opens and closes a real `.cif` file through `nf90_open()`.
- Add a dedicated `.github/workflows/ci-mmcif.yml` workflow that builds and tests with `NEP_ENABLE_MMCIF=ON`.

**Clarified decisions:**
- mmCIF is assigned **UDF8**; legacy PDB remains UDF7. The `mmcif` and `pdb-legacy` skill files already reflect this allocation.
- `NEP_ENABLE_MMCIF` defaults to **OFF** in this sprint, following the FITS/PDB precedent; it will flip to ON once the read-only dispatch layer is proven in Sprint 2.
- The new workflow is CMake-only because Autotools was removed in v3.1.0.
- No external parsing library is required; a custom STAR/CIF tokenizer will be added in Sprint 2.
- The smoke tests verify only `nc_open()`/`nc_close()` round-trips; no record parsing or PDBx category detection happens until Sprint 2.
- `NC_MMCIF_initialize()` registers the handler with the literal magic string `"data_"`. Avoiding false positives against generic (small-molecule) CIF files via a PDBx category check is deferred to Sprint 2.

**GitHub Issue:** TBD

#### Sprint 2: Dispatch Layer for mmCIF
**Detailed Plan**: See `docs/plan/v3.4.0-sprint2-mmcif-dispatch-layer.md`

- Fill in read-only dispatch layer.
- Build tests around files in /home/ed/NEP/test/data/mmCIF

**Clarified decisions:**
- Schema scope: parse `_atom_site` (coordinates and per-atom identity fields), `_cell`/`_symmetry` (unit cell), and single-row `_entry`/`_struct`/`_entity`/`_pdbx_database_status` categories as global attributes, matching the PDB Sprint 2 scope (CRYST1 + HEADER/TITLE/COMPND/SOURCE analogs). `_entity_poly_seq`-derived sequence data is deferred to a later sprint, mirroring PDB deferring `SEQRES` to Sprint 3.
- Coordinates are exposed as three separate `atom_site_Cartn_x/y/z` variables (`NC_DOUBLE`, shape `[model][atom]`), directly mirroring the PDB Sprint 2 `atom_site_Cartn_x/y/z` pattern (double instead of PDB's float, since mmCIF values carry more decimal precision).
- The `model` dimension is sized from distinct `_atom_site.pdbx_PDB_model_num` values (1 for all three current test files: `1J7W.cif`, `2W6V.cif`, `4HHB.cif`, all single-model X-ray structures); the `[model][atom]` code path exists but multi-model mmCIF is untested this sprint, a documented limitation until an NMR mmCIF file is acquired.
- The PDBx-vs-generic-CIF category check deferred from Sprint 1 is added now: after the `"data_"` magic match, `NC_MMCIF_open()` rejects files with no `_atom_site` category.
- `?` (unknown) and `.` (not-applicable) mmCIF placeholder values both map to the variable's type-appropriate NetCDF fill value; no separate mask/flag variable is added this sprint.
- `NEP_ENABLE_MMCIF` stays default **OFF** in `CMakeLists.txt`; only `ci-mmcif.yml` builds with it ON. Following the PDB precedent, the flip to default ON is reserved for a later "more testing" sprint.

**GitHub Issue:** [#350](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/350)

#### Sprint 3: Example Visualization
**Detailed Plan**: See `docs/plan/v3.4.0-sprint3-mmcif-visualizations.md`

- Add `examples/viz/mmCIF/` Python visualization scripts that open each mmCIF test file through the NetCDF UDF interface and produce publication-ready PNG artifacts, following the `examples/viz/PDB/` pattern.
- Wire the new scripts into `examples/viz/CMakeLists.txt` (`HAVE_MMCIF` guard, `_viz_artifacts` registration) and `examples/viz/README.md`.
- Update `docs/mmcif.md` (new file, if it does not already exist) with a Visualization section.
- Update `.github/workflows/ci-mmcif.yml` to build with `NEP_ENABLE_VIZ_EXAMPLES=ON` using the project virtual environment, so the new plots run in CI.

**Clarified decisions:**
- All three existing mmCIF test files (`1J7W.cif`, `2W6V.cif`, `4HHB.cif`) get their own visualization script/plot, maximizing parser regression coverage, matching the v3.1.0 Sprint 3 "visualize all test files" precedent rather than picking just one or two files.
- Plot content reuses the PDB Sprint viz pattern exactly: a 3D scatter of `atom_site_Cartn_x/y/z`, colored/marked by `atom_site_group_PDB` (`ATOM` vs `HETATM`), for consistency with `examples/viz/PDB/plot_pdb_xray_structure.py` and minimal new code, since the mmCIF and PDB schemas expose the same fields.
- `NEP_ENABLE_MMCIF` stays default **OFF** in `CMakeLists.txt`; this sprint is scoped to visualization only (unlike PDB Sprint 3, which combined "more testing" with the default flip). Flipping the default is deferred to a future "more testing" sprint.
- `ci-mmcif.yml` is extended to set up the project Python virtual environment and build with `-DNEP_ENABLE_VIZ_EXAMPLES=ON`, matching the DICOM Sprint 1 and PDB visualization precedent, so the new plots are exercised on every PR.

**GitHub Issue:** [#351](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/351)

### V3.3.0 - Proteins with PDB Format

#### Sprint 1: Setup
**Detailed Plan**: See `docs/plan/v3.3.0-sprint1-pdb-infrastructure.md`

- Look at PDB skill file.
- There are new test files in /test/data/PDB. They will be used in tests. Make sure they are available in build directory. (They are from https://www.rcsb.org)
- Create a no-op dispatch layer for PDB.
- Write a test for it.
- Update the CI so the new test is run.

**Clarified decisions:**
- Legacy PDB is assigned **UDF7** (next free slot; it ships before mmCIF, which now takes UDF8). The `pdb-legacy` and `mmcif` skill files have been updated to match.
- `NEP_ENABLE_PDB` / `--enable-pdb` defaults to **OFF** in this sprint, following the exact FITS Sprint 1/2 precedent (flip to ON once the dispatch layer is proven, in Sprint 2).
- A new dedicated `ci-pdb.yml` workflow is added (consistent with `ci-fits.yml`/`ci-dicom.yml`), even though PDB requires no external parsing library — no extra system packages beyond the standard HDF5/NetCDF-C/NetCDF-Fortran stack are needed.
- `test/tst_pdb_udf.c` only verifies an `nc_open()`/`nc_close()` round-trip on the real `.pdb` test files; no PDB record parsing or magic/format-detection testing happens until Sprint 2.
- `NC_PDB_initialize()` registers the format with `nc_def_user_format()` using the literal magic string `"HEADER"` (`NEP_MAGIC_PDB`), per the `pdb-legacy` skill. Files that do not start with a `HEADER` record are a documented limitation, not solved in Sprint 1.

**GitHub Issue:** [#342](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/342)

#### Sprint 2: Dispatch Layer for PDB
**Detailed Plan**: See `docs/plan/v3.3.0-sprint2-pdb-dispatch-layer.md`

- Look at PDB skill file.
- Write read-only PDB dispatch code.
- Write tests involving PDB files in test/data/PDB. Check metadata and some data.

**Clarified decisions:**
- Schema scope: parse `ATOM`/`HETATM` coordinates and per-atom identity fields, `CRYST1` unit-cell global attributes, and `HEADER`/`TITLE`/`COMPND`/`SOURCE` global attributes. `SEQRES`-derived sequence data is deferred to Sprint 3.
- `ATOM` and `HETATM` records share a single `atom` dimension in file order, distinguished by a per-atom `atom_site_group_PDB` variable (`"ATOM"`/`"HETATM"`), matching the `mmcif` skill's `_atom_site` mapping.
- `MODEL`/`ENDMDL` blocks are parsed now: the `model` dimension is sized from the number of `MODEL` blocks (1 if none are present, as in both current test files), and the coordinate variable is shaped `[model][atom]`. Untested against real multi-model data this sprint since neither `1J7W.pdb` nor `4HHB.pdb` contains `MODEL` records.
- Files with no `ATOM`/`HETATM` records at all are rejected with `NC_EINVAL`. Hybrid-36 encoded serial/residue numbers are a documented known limitation, not handled this sprint.

**GitHub Issue:** [#343](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/343)

#### Sprint 3: More Testing of PDB Reading
**Detailed Plan**: See `docs/plan/v3.3.0-sprint3-pdb-more-testing.md`

Acquire additional real-world legacy PDB test files and extend `test/tst_pdb_udf.c` to exercise features not covered by the existing `1J7W.pdb`/`4HHB.pdb` X-ray files. Flip `NEP_ENABLE_PDB` to default ON and add a Fortran smoke check once the reader is covered by the expanded C test suite.

**Clarified decisions:**
- Acquire one multi-model NMR ensemble from RCSB (to exercise the `MODEL`/`ENDMDL` path and the `[model][atom]` coordinate variables) and one single-model synthetic PDB file without `CRYST1` (AlphaFold DB direct download returned 404 for multiple UniProt IDs during acquisition).
- All new sample files are placed in `test/data/PDB/`, copied to the build tree by `test/CMakeLists.txt`, and documented with provenance and license in the sprint plan.
- `NEP_ENABLE_PDB` flips from OFF to default ON in `CMakeLists.txt` now that the dispatch layer parses real files end-to-end.
- `ftest/ftst_pdb_udf.F90` is added as an open/close smoke check on one real PDB file, matching FITS/DICOM Fortran coverage.
- New test assertions include: `model` dimension > 1, coordinates differ across models, and graceful handling of files with no `CRYST1` record (optional attributes omitted). `SEQRES`, hybrid-36 encoding, and multi-character chain IDs remain documented known limitations.
- Build system remains CMake-only (Autotools removed in v3.1.0); `ci-pdb.yml` is updated to exercise the default-ON reader.

**Acceptance Criteria:**
- At least two new PDB test files are present in `test/data/PDB/` with documented provenance.
- `model` dimension length is verified as > 1 for the NMR file, and `atom_site_Cartn_x/y/z` values for the same atom index differ across models.
- The synthetic `no_cryst1.pdb` file opens successfully; since it lacks `CRYST1`, no `cell_*` or `space_group_name_H-M` global attributes are required.
- `ftest/ftst_pdb_udf.F90` compiles and passes when `NEP_ENABLE_FORTRAN=ON`.
- `NEP_ENABLE_PDB` defaults to ON in `CMakeLists.txt`.
- All PDB tests pass with PDB enabled; the full suite remains green with PDB explicitly disabled.

**Testing:** Configure with `-DNEP_ENABLE_PDB=ON` (now default) and run `ctest --test-dir build -R pdb --output-on-failure`. Verify `tst_pdb_udf` and `ftst_pdb_udf` pass, then run the full CTest suite to confirm no regressions. Validate the updated `ci-pdb.yml` job is green.

**Build System Integration:** `CMakeLists.txt` (default option flip), `test/CMakeLists.txt` (copy new `test/data/PDB/` files and add them to `tst_pdb_udf`), `ftest/CMakeLists.txt` (add `ftst_pdb_udf`), and `.github/workflows/ci-pdb.yml` (remove explicit `-DNEP_ENABLE_PDB=ON` to test the default).

**Definition of Done:** PDB reader is enabled by default, the expanded C test suite covers multi-model NMR and non-RCSB single-model files, a Fortran smoke test is in place, CI exercises the default-ON configuration, and all tests pass with no regressions.

**GitHub Issue:** [#345](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/345)

#### Sprint 4: Documentation
**Detailed Plan**: See `docs/plan/v3.3.0-sprint4-pdb-documentation.md`

- Update all the docs.
- Add a section on PDB to doxygen docs.
- Write some examples/viz visulaizations.

**Clarified decisions:**
- Two visualization scripts are added under `examples/viz/PDB/`: a static 3D scatter plot of `ATOM`/`HETATM` Cartesian coordinates for the X-ray structure `4HHB.pdb`, and a multi-model overlay plot from the NMR ensemble `1GAB.pdb` showing coordinate variation across models. Both follow the existing per-format viz pattern (`plot_common.py`, `verify_viz_artifacts.py`, metadata sidecar files).
- A standalone C example `examples/pdb/pdb_read.c` is added, matching the style of `examples/dicom/dicom_read.c` (calls `NC_PDB_initialize()`, opens a PDB file, prints dims/vars and a slice of atom coordinates), and is registered in `examples/CMakeLists.txt` and `examples/README.md`.
- Full documentation parity with FITS/DICOM: a new `docs/pdb.md` format reference page (mapping, enabling, dependencies, resources, example, matching `docs/fits.md` structure), a new PDB row/section in `docs/formats.md`'s UDF slot table and format reference table, PDB entries in the top-level `README.md` (format table, CMake option table, Spack variant table), `docs/pdb.md` added to `docs/Doxyfile.in`'s `INPUT` list, and PDB mentions added to `examples/README.md` and `examples/viz/README.md`.
- No reader behavior changes in this sprint; it is documentation- and example-only.

**GitHub Issue:** [#347](https://github.com/Intelligent-Data-Design-Inc/NEP/issues/347)

### V3.2.0 - More DICOM

Organize the visualization examples and add more DICOM functionality.

#### Sprint 1: Organize examples/viz
**Detailed Plan**: See `docs/plan/v3.2.0-sprint1-organize-viz.md`

Reorganize format-specific visualization scripts into source directories for CDF, DICOM, FITS, GeoTIFF, GRIB2, and PDS4. CMake must mirror the same directory structure in the build tree while preserving all existing visualization test names, runtime environment, UDF guards, copied test data, artifact basenames, flat artifact output location, and verifier interface.

**Clarified decisions:**
- The source and CMake build trees mirror the six per-format directories.
- The `examples/viz/` root retains shared infrastructure only: `plot_common.py`, helper tests, the verifier, root CMake configuration, and the root README.
- DICOM's `_dicom_udf.py` is colocated with its DICOM plot scripts; format scripts must reliably import root shared utilities from their mirrored build-tree locations.
- Generated PNG and metadata artifacts remain in the existing flat visualization build directory. No artifact basenames, CTest names, fixture behavior, or verifier command-line interface changes.
- Update the root visualization README with the new layout and manual paths. Do not add per-format READMEs.

**Acceptance Criteria:**
- All format-specific visualization scripts are under their matching source and mirrored build-tree format directories.
- Every enabled visualization test executes its relocated script with unchanged runtime behavior and expected artifacts.
- The flat artifact output directory continues to pass `verify_viz_artifacts.py` using the existing basename list.
- The root README accurately documents the grouped layout and manual execution paths.
- No reader behavior, public CMake option, artifact content, artifact basename, or test data changes solely because of this reorganization.

**Testing:** Configure with visualization examples and applicable readers enabled, run `ctest --test-dir build -R viz --output-on-failure`, execute enabled format tests individually, validate all artifacts through the existing verifier interface, inspect the mirrored build-tree layout, and run a non-visualization configuration for regression coverage.

**Build System Integration:** `examples/viz/CMakeLists.txt`, relocated Python imports/path handling, and `examples/viz/README.md`. CMake remains the sole supported build system; no dependency, option, UDF-registration, or install-rule change is required.

**Definition of Done:** Format-specific visualization scripts are organized in matching source and build-tree directories, shared utilities remain at the visualization root, existing CTest and artifact behavior is preserved, the README is current, and visualization tests pass for every enabled reader.

**GitHub Issue:** #334

#### Sprint 2: More DICOM Functionality
**Detailed Plan**: See `docs/plan/v3.2.0-sprint2-dicom-sample-coverage.md`

Verify and document that the NEP DICOM UDF reader correctly reads all sample files already present in `test/data/DICOM`. Baseline investigation found the sprint's original test-only premise was wrong: two of the five OME samples (`CT-MONO2-16-chest.dcm`, `MR-MONO2-12-shoulder.dcm`) use JPEG Lossless transfer syntaxes (`1.2.840.10008.1.2.4.70` and `.4.57`), not "Explicit VR Little Endian" as originally documented, and are currently rejected. A third (`CR-MONO1-10-chest.dcm`) has no DICOM preamble/File Meta Information at all and is unrelated to JPEG. This sprint adds JPEG Lossless decode support and descopes the no-preamble file.

**Implementation scope:**
- Add a memory-based `jpeg_source_mgr` and per-precision (8/12/16-bit) lossless JPEG decode wrappers using GDCM's `gdcmjpeg8`/`gdcmjpeg12`/`gdcmjpeg16` libraries (IJG libjpeg 6b + the classic lossless/Process 14 patch, mangled symbol namespace; new optional build dependency `libgdcm-dev`).
- Detect the JPEG frame's data precision (SOF3 marker) at decode time to select the matching 8/12/16-bit codec.
- Recognize `1.2.840.10008.1.2.4.57` and `1.2.840.10008.1.2.4.70` as supported encapsulated transfer syntaxes in `src/dicomfile.c`, alongside existing JPEG Baseline support.
- Add CMake detection for `gdcmjpeg8`/`gdcmjpeg12`/`gdcmjpeg16` headers/libraries under `NEP_ENABLE_DICOM`.
- Add test blocks to `test/tst_dicom_udf.c` for `CT-MONO2-16-brain.dcm` and `MR-MONO2-16-head.dcm` (native, already working) and for `CT-MONO2-16-chest.dcm` and `MR-MONO2-12-shoulder.dcm` (new lossless decode path), including pixel reads.
- Add a test asserting `CR-MONO1-10-chest.dcm` is cleanly rejected (`NC_EINVAL`) and is out of scope for this sprint.
- Add a lightweight open/close Fortran smoke check to `ftest/ftst_dicom_udf.F90` for one of the confirmed-working native files.
- Correct `test/data/DICOM/README.md`'s transfer-syntax labels for all five OME samples to match their actual embedded UIDs, and update `docs/dicom.md`.
- Update `.github/workflows/ci-dicom.yml` to install `libgdcm-dev`, build NetCDF-Fortran, and set `-DNEP_ENABLE_FORTRAN=ON` so `ftst_dicom_udf` runs in the focused DICOM workflow.

**Clarified decisions:**
- JPEG Lossless (`.4.57`, `.4.70`) decode is implemented via GDCM's bundled IJG lossless-JPEG codec (`gdcmjpeg8/12/16`), a new optional dependency, rather than vendoring the codec source or implementing a decoder from scratch.
- `CR-MONO1-10-chest.dcm` (no preamble/File Meta Information) is explicitly descoped; it remains a clean-rejection case. No-preamble DICOM support is deferred to a future sprint if ever needed.
- MONOCHROME1 pixel data (if encountered) is exposed as-is; the `PhotometricInterpretation` attribute remains the contract consumers use to invert for display.
- Each sample gets its own explicit test block in `test/tst_dicom_udf.c`, matching the current per-file style.
- Fortran coverage adds one smoke check (open/close only) rather than full parity with the C test.

**Acceptance Criteria:**
- `test/tst_dicom_udf.c` opens, inspects, and reads pixel data from `CT-MONO2-16-brain.dcm`, `CT-MONO2-16-chest.dcm`, and `MR-MONO2-12-shoulder.dcm`; opens and inspects metadata for `MR-MONO2-16-head.dcm` (pixel-data read is skipped due to an independent libdicom limitation with this file's missing `NumberOfFrames` tag, documented in the sprint plan); and asserts a clean `NC_EINVAL` rejection for `CR-MONO1-10-chest.dcm`.
- `ftest/ftst_dicom_udf.F90` opens and closes at least one confirmed-working new sample without error.
- `docs/dicom.md` and `test/data/DICOM/README.md` accurately describe JPEG Lossless support, correct transfer-syntax labels, and the `CR-MONO1-10-chest.dcm` rejection.
- `.github/workflows/ci-dicom.yml` installs `libgdcm-dev`, builds NetCDF-Fortran, enables `NEP_ENABLE_FORTRAN`, and runs `ftst_dicom_udf` alongside `tst_dicom_udf`.
- All DICOM tests pass with DICOM enabled; the full suite remains regression-free with DICOM disabled.

**Testing:** Run `ctest --test-dir build -R dicom --output-on-failure` with `-DNEP_ENABLE_DICOM=ON -DNEP_ENABLE_FORTRAN=ON`; confirm `tst_dicom_udf` and `ftst_dicom_udf` both pass; confirm the updated `ci-dicom.yml` job is green.

**Build System Integration:** `src/CMakeLists.txt` (gdcmjpeg8/12/16 detection and linking), `test/CMakeLists.txt`, `ftest/CMakeLists.txt`, `.github/workflows/ci-dicom.yml` (libgdcm-dev install, NetCDF-Fortran build step, `NEP_ENABLE_FORTRAN=ON`).

**Definition of Done:** JPEG Lossless (`.4.57`/`.4.70`) DICOM files decode correctly through the DICOM UDF reader, all four newly-supported OME samples are validated by the C and Fortran test suites, `CR-MONO1-10-chest.dcm` is documented as a clean, expected rejection, documentation accurately reflects transfer-syntax support, `ci-dicom.yml` runs the expanded Fortran coverage with the new dependency, and the full test suite remains green with DICOM enabled and disabled.

**GitHub Issue:** #338

#### Sprint 3: Visualize All DICOM Files in test/data/DICOM
- We need to create new examples/viz/DICOM examples for the new files we can now read.

### V3.1.0 - DICOM Visualizations, CMake Only Builds

#### Sprint 1: Add Visualization of DICOM Test File
**Detailed Plan**: See `docs/plan/v3.1.0-sprint1-dicom-visualizations.md`

Add the first DICOM visualization examples to `examples/viz/`, generate two publication-ready grayscale PNG artifacts from the existing `MRBRAIN.DCM` and `0003.DCM` samples, and acquire 3–5 additional public-domain DICOM samples to broaden visual content and regression coverage.

**Implementation scope:**
- Add `examples/viz/plot_dicom_mrbrain.py` to open `test/data/DICOM/MRBRAIN.DCM` through the NetCDF UDF interface, read `pixel_data[0, :, :]`, normalize the 16-bit signed/unsigned samples to grayscale, and write `dicom_mrbrain_image.png` + `dicom_mrbrain_image_metadata.txt`.
- Add `examples/viz/plot_dicom_xa_montage.py` to open `test/data/DICOM/0003.DCM`, read all 17 encapsulated JPEG Baseline frames, build a compact grayscale montage, and write `dicom_xa_frame_montage.png` + `dicom_xa_frame_montage_metadata.txt`.
- Update `examples/viz/CMakeLists.txt` and `examples/viz/Makefile.am` to copy the new scripts, conditionally run them under `HAVE_DICOM`, and append the two artifact basenames to the visualization artifact list.
- Update `examples/viz/README.md` with DICOM visualization instructions and script descriptions.
- Add `HAVE_DICOM` to the `NEP_ENABLE_VIZ_EXAMPLES` UDF-reader guard in top-level `CMakeLists.txt` and `configure.ac`.
- Add a Visualization section to `docs/dicom.md` describing the new plots and required environment variables.
- Acquire 3–5 additional public-domain DICOM samples (e.g., CT, CR, or US images) for `test/data/DICOM/`, documenting their provenance, license, and any clinically sensitive content considerations.
- Update `.github/workflows/ci-dicom.yml` to enable visualization examples (`NEP_ENABLE_VIZ_EXAMPLES=ON` / `--enable-viz-examples`), install Python dependencies in the project virtual environment, and run the visualization tests.

**Clarified decisions:**
- Two visualization artifacts are produced in Sprint 1: `dicom_mrbrain_image` (single-frame 16-bit MR) and `dicom_xa_frame_montage` (17-frame XA montage).
- DICOM visualization output uses an external `_metadata.txt` with exactly `title`, `caption`, and `alt_text`, a caption of at most 75 words, and a figure size at most 8.0 by 6.1 inches.
- `MRBRAIN.DCM` is plotted by normalizing the 16-bit `pixel_data` range to 8-bit grayscale; signed `NC_SHORT` values are offset before scaling.
- `0003.DCM` frames are arranged in a rectangular montage; if any frame has color photometric interpretation, it is converted to luminance before tiling.
- Additional test files are read-only data; they do not change existing test logic unless they exercise a new supported Transfer Syntax, in which case the change is limited to a new regression case.
- New samples must be public-domain or CC-licensed and must not contain real patient identifiers.

**Acceptance Criteria:**
- `plot_dicom_mrbrain.py` and `plot_dicom_xa_montage.py` are present in `examples/viz/` and copied to the build tree by both build systems.
- CMake and Autotools configure successfully with `-DNEP_ENABLE_DICOM=ON -DNEP_ENABLE_VIZ_EXAMPLES=ON` and `--enable-dicom --enable-viz-examples`, respectively.
- `ctest -R viz --output-on-failure` and `make check` generate both PNG/metadata pairs and `verify_viz_artifacts.py` passes.
- Both PNGs are within the 8.0 by 6.1 inch / 150 DPI limits and have valid `_metadata.txt` files.
- `docs/dicom.md` and `examples/viz/README.md` describe the new visualization scripts.
- At least 3 new public-domain DICOM samples are added under `test/data/DICOM/` with provenance documented in the sprint plan.
- `.github/workflows/ci-dicom.yml` passes with visualization enabled.
- Existing DICOM tests (`tst_dicom_udf`) continue to pass with DICOM enabled; the full suite remains regression-free with DICOM disabled.

**Testing:** Run `ctest -R viz --output-on-failure` and `make check` in a DICOM-enabled build, inspect the generated `dicom_mrbrain_image.png` and `dicom_xa_frame_montage.png` artifacts, run `tst_dicom_udf` for regression coverage, and verify the DICOM-enabled `ci-dicom.yml` job is green.

**Build System Integration:** `examples/viz/CMakeLists.txt`, `examples/viz/Makefile.am`, top-level `CMakeLists.txt`, `configure.ac`, `examples/viz/README.md`, `docs/dicom.md`, `.github/workflows/ci-dicom.yml`.

**Definition of Done:** Two DICOM visualization scripts generate validated, publication-ready artifacts under both CMake and Autotools; the visualization guard conditions recognize DICOM; user-facing documentation describes the plots; 3–5 new public-domain DICOM samples are added with documented provenance; `ci-dicom.yml` passes with visualization enabled; existing tests remain green.

**GitHub Issue:** #330

#### Sprint 2: CMake-Only Build Migration
**Detailed Plan**: See `docs/plan/v3.1.0-sprint2-cmake-only-build.md`

Make CMake the sole supported NEP build system beginning with v3.1.0. Remove the complete Autotools implementation and generated artifacts, convert all CI workflows to retain equivalent CMake coverage, and remove current user-facing Autotools guidance.

**Implementation scope:**
- Delete root and nested Autotools inputs, generated artifacts, helper scripts, macros, and Autotools-only test runners, including `configure.ac`, `configure`, `Makefile.am`, `Makefile.in`, `aclocal.m4`, `config.h.in`, `autogen.sh`, `m4/`, and Autotools build material under `hdf5_plugins/`.
- Preserve shared C/C++/Fortran sources, test data, CMake files, CMake-generated configuration, and CMake test execution.
- Remove Autotools matrix entries, bootstrap/configure/build/test steps, variables, and wording from all GitHub Actions workflows; retain or add equivalent CMake coverage for every active configuration.
- Update `README.md`, active product/design documentation, Doxygen configuration and generated-documentation guidance, packaging guidance, and the v3.1.0 release notes to identify CMake as the only supported build path.
- Remove references to `./configure`, `autoreconf`, `make check`, and Autotools-only options from active user-facing documentation; do not rewrite frozen historical roadmap, plan, issue, or release documents.
- Retain the current CMake minimum version unless the implementation audit identifies an existing incompatibility.

**Clarified decisions:**
- CMake is the sole supported build system from v3.1.0 onward; no `./configure` compatibility wrapper or legacy directory will be retained.
- The removal includes generated Autotools files and Autotools-specific test runners, not only their authored inputs.
- All repository CI is CMake-only. Existing configuration breadth remains: default/minimal/compression, Fortran on/off, enabled format readers, DICOM, parallel I/O, visualization, documentation, Conda, and Spack coverage where applicable.
- CMake 3.9 remains the declared minimum version for this sprint.
- Active documentation and packaging guidance must remove Autotools instructions; historical records remain unchanged.

**Acceptance Criteria:**
- No Autotools inputs, generated files, macros, helper scripts, `Makefile.am`, `Makefile.in`, or Autotools-only test runners remain in the repository.
- A repository search finds no active CI or user-facing references to `./configure`, `autoreconf`, `autogen.sh`, `make check`, or Autotools.
- Every GitHub Actions workflow uses CMake only, with no Autotools job, matrix value, bootstrap, configure, build, or test step.
- The CMake default, minimal, DICOM-enabled, format-enabled, Fortran on/off, documentation, examples, visualization, and parallel configurations continue to configure, build, and test successfully as applicable.
- `README.md`, active Doxygen documentation, design/requirements/FAQ documentation, and packaging guidance describe CMake-only installation and testing accurately.
- Existing public CMake options, install behavior, UDF reader behavior, compression filters, and source compatibility remain unchanged.

**Testing:** Run clean CMake configure/build/CTest validation for the retained CI configurations, build Doxygen documentation without warnings, and search the tracked repository for removed Autotools files and active references. Confirm CMake installation and package-consumer checks where covered by CI.

**Build System Integration:** Root and nested CMake files, CMake test registration, `.github/workflows/`, `README.md`, `docs/Doxyfile.in`, active design/requirements/FAQ documentation, release notes, Conda recipe/build script, and Spack package guidance.

**Definition of Done:** The repository has one supported build path (CMake), all active CI coverage is CMake-based, user and package documentation provides only CMake instructions, all retained configurations pass their CMake validation, and no unsupported Autotools artifacts or active references remain.

**GitHub Issue:** #332

#### Sprint 3: More DICOM Visualizations
- There are additional DICOM files in test/data/DICOM.
- Let's get some visualizations!

### V3.0.0 - DICOM Reader

Add the ability to read files in DICOM format through a new NetCDF UDF handler that uses the `libdicom` C library. The reader exposes DICOM image pixel data and key metadata as NetCDF variables and attributes, supports native uncompressed single-frame images first, then encapsulated JPEG compressed and multi-frame images, and finally functional-group metadata, Fortran bindings, documentation, and CI.

#### Sprint 1: DICOM Reader Infrastructure
**Detailed Plan**: See `docs/plan/v3.0.0-sprint1-dicom-infrastructure.md`

Integrate `libdicom` into both build systems, assign DICOM to UDF slot 6, implement the dispatch skeleton, and read native uncompressed single-frame DICOM files through the NetCDF API.

**Implementation scope:**
- Add `NEP_UDF_DICOM`, `NEP_MAGIC_DICOM`, and `NEP_FORMAT_NAME_DICOM` to `include/nep.h`.
- Add `-DNEP_ENABLE_DICOM` / `--enable-dicom` build options (default OFF) and detect `libdicom`.
- Create `src/dicomdispatch.c`, `src/dicomfile.c`, and `include/dicomdispatch.h` following the FITS handler pattern.
- Implement `NC_DICOM_initialize()`, `NC_DICOM_open()`, `NC_DICOM_close()`, `NC_DICOM_abort()`, and format inquiry functions.
- Map DICOM Patient/Study/Series/Image Pixel module tags to NetCDF dimensions, variables, and attributes.
- Implement `NC_DICOM_get_vara()` for native (uncompressed) pixel data.
- Add `test/tst_dicom_udf.c` and a small uncompressed DICOM sample.
- Create `ci-dicom.yml` to validate the build and the new test.

**Clarified decisions:**
- DICOM occupies UDF slot 6 (`NC_UDF6`); UDF0-UDF5 are already assigned.
- The DICOM magic string is `DICM` at byte offset 128; NetCDF-C magic detection may need an offset-aware check or the file can be opened explicitly with the UDF mode flag.
- Compressed Transfer Syntaxes (including the existing `test/data/DICOM/0003.DCM`, which is JPEG Baseline) are explicitly rejected in Sprint 1.
- Pixel data type is derived from `BitsAllocated` and `PixelRepresentation`.
- `PlanarConfiguration` and `SamplesPerPixel` determine whether a `sample` dimension is created.

**Acceptance Criteria:**
- `include/nep.h` defines the new UDF slot, magic, and format-name macros.
- CMake and Autotools detect `libdicom`, define `HAVE_DICOM`, and link `-ldicom`.
- `NC_DICOM_initialize()` registers the UDF6 dispatch table.
- `nc_open()` succeeds on an uncompressed single-frame DICOM file after the handler is registered.
- Dimensions, variable type, and global attributes match the DICOM metadata.
- `nc_get_vara()` reads a subset of `pixel_data` correctly.
- `test/tst_dicom_udf.c` passes under CMake and Autotools.
- `ci-dicom.yml` passes for both build systems.
- Compressed DICOM files fail cleanly with a clear error.

**Testing:** Run `tst_dicom_udf` directly and via `ctest -R dicom` / `make check`; verify the uncompressed sample; verify that `test/data/DICOM/0003.DCM` is rejected rather than crashing.

**Build System Integration:** Top-level `CMakeLists.txt` and `configure.ac`; `src/CMakeLists.txt` and `src/Makefile.am`; `test/CMakeLists.txt` and `test/Makefile.am`; new `.github/workflows/ci-dicom.yml`.

**Definition of Done:** `libdicom` is integrated, the UDF6 dispatch skeleton works, uncompressed single-frame DICOM files open and expose metadata, pixel data is readable, the C smoke test passes, CI is green, and the sprint documentation is complete.

**GitHub Issue:** TBD

#### Sprint 2: Compressed Pixel Data and Multi-Frame Support
**Detailed Plan**: See `docs/plan/v3.0.0-sprint2-dicom-compressed-multiframe.md`

Extend the reader to decompress encapsulated JPEG pixel data and support multi-frame images. Use the existing `test/data/DICOM/0003.DCM` JPEG Baseline sample as the primary compressed test case.

**Implementation scope:**
- Detect encapsulated Transfer Syntaxes from `(0002,0010)`.
- Add `libjpeg`/`libjpeg-turbo` detection and decompression for JPEG Baseline frames.
- Use `libdicom` frame-level read functions (`dcm_filehandle_read_frame`) to obtain raw encapsulated fragments.
- Decompress each requested frame on demand inside `NC_DICOM_get_vara()`.
- Add a `frame` dimension for `NumberOfFrames > 1`.
- Support grayscale and RGB 8-bit decompressed output.
- Update `test/tst_dicom_udf.c` to verify the compressed sample.

**Clarified decisions:**
- Only JPEG Baseline (`1.2.840.10008.1.2.4.50`) is required for this sprint; other Transfer Syntaxes may be recognized and rejected or supported if straightforward.
- Decompressed frames are cached for the duration of a single `nc_get_vara()` call.
- JPEG decompression output is exposed as `NC_UBYTE`; 12-bit JPEG values are stored in 16-bit unsigned samples.
- `PlanarConfiguration` is normalized to interleaved RGB in the NetCDF view for JPEG color images.

**Acceptance Criteria:**
- `test/data/DICOM/0003.DCM` opens and exposes correct Rows, Columns, SamplesPerPixel, and PhotometricInterpretation.
- `nc_get_vara()` reads decompressed pixel values from the JPEG sample.
- Single-frame and multi-frame uncompressed images continue to work (Sprint 1 regression coverage).
- CMake and Autotools both link `libjpeg` when DICOM is enabled.
- `ci-dicom.yml` passes with DICOM + JPEG support.
- Unsupported Transfer Syntaxes produce a clear error.

**Testing:** Run `tst_dicom_udf` against the compressed sample; run the Sprint 1 uncompressed sample for regression; run `ctest -R dicom --output-on-failure` and `make check`.

**Build System Integration:** Add `find_package(JPEG)` (CMake) and `AC_CHECK_LIB([jpeg], ...)` (Autotools); link `JPEG_LIBRARIES` into the DICOM reader objects.

**Definition of Done:** JPEG Baseline encapsulated DICOM files open, decompress, and expose readable pixel data; multi-frame layout works; the compressed sample test passes; CI is green; and the sprint documentation is complete.

**GitHub Issue:** TBD

#### Sprint 3: Test Expansion, C/Fortran Examples, Documentation, Packaging, and CI
**Detailed Plan**: See `docs/plan/v3.0.0-sprint3-dicom-tests-examples-ci.md`

Use the new DICOM test files (`MRBRAIN.DCM`, `0003.DCM`, and the existing uncompressed sample) to expand regression coverage, add a Fortran smoke test, create a C example program, update all user-facing documentation, and wire DICOM support into the existing CI matrix and packaging recipes.

**Implementation scope:**
- Expand `test/tst_dicom_udf.c` to exercise all three sample files, including the 16-bit uncompressed MR image and the 17-frame JPEG Baseline XA image.
- Add `ftest/ftst_dicom_udf.F90` and update Fortran build rules.
- Add `examples/dicom/dicom_read.c` with compact output and the NetCDF Developer's Handbook Second Edition reference.
- Create `docs/dicom.md` and update `docs/formats.md`, `docs/design.md`, `docs/prd.md`, and `README.md`.
- Update Spack and Conda recipes with optional DICOM support.
- Integrate DICOM into `.github/workflows/ci.yml` in addition to the focused `ci-dicom.yml`.

**Clarified decisions:**
- The Fortran test calls `NC_DICOM_initialize()` and opens the uncompressed sample, matching the FITS Fortran test pattern.
- DICOM remains disabled by default in CMake, Autotools, Spack, and Conda.
- Example output follows the NEP compact style (`ERR(retval)`, `Done.`).
- The three current sample files do not contain Shared/Per-frame Functional Groups; functional-group metadata is deferred to Sprint 4.

**Acceptance Criteria:**
- `test/tst_dicom_udf.c` passes for all three sample files under both build systems.
- `ftest/ftst_dicom_udf.F90` compiles and passes under both build systems.
- `examples/dicom/dicom_read.c` builds, runs, and prints compact metadata ending with `Done.`.
- All user-facing documentation describes DICOM support accurately.
- Spack and Conda recipes include optional DICOM support.
- `ci.yml` builds and tests DICOM in at least one job.
- All DICOM tests pass with DICOM enabled; existing tests pass with DICOM disabled.

**Testing:** Run `ctest -R dicom --output-on-failure`, `make check`, `ftst_dicom_udf`, and `dicom_read`; verify the DICOM-enabled `ci.yml` job and `ci-dicom.yml` both pass.

**Build System Integration:** `examples/CMakeLists.txt` and `examples/Makefile.am`; `ftest/CMakeLists.txt` and `ftest/Makefile.am`; `spack/NEP/package.py`; `conda/meta.yaml` and `conda/build.sh`; `.github/workflows/ci.yml`.

**Definition of Done:** The DICOM reader has expanded regression coverage over all available sample files, a C example and Fortran smoke test, user-facing documentation, optional packaging support, and a CI matrix job, and the full test suite passes with DICOM enabled while remaining regression-free with DICOM disabled.

**GitHub Issue:** TBD

#### Sprint 4: Functional Groups and Enhanced Multi-frame Metadata
**Detailed Plan**: See `docs/plan/v3.0.0-sprint4-dicom-functional-groups.md`

Expose Shared and Per-frame Functional Group metadata from enhanced multi-frame DICOM objects as NetCDF variables or global attributes once a suitable enhanced multi-frame sample is available.

**Implementation scope:**
- Parse Shared Functional Groups Sequence `(5200,9229)` and Per-frame Functional Groups Sequence `(5200,9230)`.
- Expose `image_position_patient`, `image_orientation_patient`, `pixel_spacing`, and `slice_thickness` as NetCDF variables over the `frame` dimension or as global attributes when shared.
- Acquire a multi-frame enhanced DICOM sample containing functional groups for testing.

**Clarified decisions:**
- Functional-group variables are created only when the corresponding sequences are present.
- Shared values become scalar global attributes; per-frame values become `[frame]` variables.

**Acceptance Criteria:**
- Multi-frame enhanced DICOM files expose functional-group metadata variables when present.
- All DICOM tests pass with DICOM enabled; existing tests continue to pass with DICOM disabled.
- Unsupported Transfer Syntaxes and malformed files fail cleanly.

**Testing:** Run `ctest -R dicom --output-on-failure` and `make check` against a multi-frame enhanced sample containing functional groups; run the full suite with DICOM disabled to confirm no regressions.

**Build System Integration:** None beyond existing `HAVE_DICOM` conditionals.

**Definition of Done:** The DICOM reader exposes shared and per-frame functional-group metadata for enhanced multi-frame objects, and the full test suite passes with DICOM enabled while remaining regression-free with DICOM disabled.

**GitHub Issue:** TBD

### V3.0.1 Spack and Conda Catchup

#### Sprint 1: Spack Catchup
**Detailed Plan**: See `docs/plan/v3.0.1-sprint1-spack-catchup.md`

Bring the in-repository NEP Spack recipe, Spack CI, user documentation, and the existing upstream Spack pull request up to date through NEP v3.0.0.

**Implementation scope:**
- Add released versions 2.6.0, 2.6.1, 2.7.0, 2.7.1, 2.8.0, and 3.0.0 with verified tag-tarball checksums, in descending version order.
- Resolve the open review feedback in [spack/spack-packages#5557](https://github.com/spack/spack-packages/pull/5557): version ordering, BZIP2 plugin verification, and removal of the unsatisfiable CDF variant.
- Omit `+cdf` until NASA CDF has an upstream Spack recipe, so the in-repository package file remains directly submit-ready.
- Update the upstream pull request in place, rather than opening a replacement.
- Update Spack CI to use v3.0.0 as the stable release, cover DICOM and all supported reader combinations, and remove masked install failures.
- Update the README and release-status documentation for the current stable Spack package.

**Clarified decisions:**
- All published releases after 2.5.0 through 3.0.0 are added.
- Spack CI validates default, minimal, DICOM-enabled, and all-supported-reader concretizations, plus minimal and all-supported-reader installs.
- CDF Spack support is deferred until its dependency is accepted upstream.

**Acceptance Criteria:**
- The local Spack recipe contains the six new versions with verified checksums in newest-to-oldest order.
- The in-repository recipe omits `+cdf`, matching the submit-ready upstream-compatible variant set.
- `check_install()` verifies both LZ4 and BZIP2 plugins when enabled.
- The existing upstream PR resolves every outstanding review thread without exposing an unsatisfiable CDF variant.
- CI concretizes v3.0.0 default, minimal, DICOM, and all-upstream-supported-reader configurations, and installs minimal and all-upstream-supported-reader configurations without `|| true`.
- README Spack guidance identifies v3.0.0 as the latest stable release and omits unavailable CDF variant guidance.

**Testing:** Run `spack style`, `spack audit`, default/minimal/DICOM `spack spec` checks, and an all-upstream-supported-reader spec. Run minimal and all-upstream-supported-reader `spack install -v --fail-fast` commands and confirm the Spack CI jobs pass.

**Build System Integration:** No CMake or Autotools source changes. The package retains the existing mappings from Spack variants to `NEP_*` CMake options.

**Definition of Done:** The in-repository recipe supports every published NEP release through v3.0.0 and is directly submit-ready for spack-packages, upstream review feedback is resolved according to Spack policy, CI validates v3.0.0 without ignored install failures, documentation is current, and the sprint issue is linked below.

**GitHub Issue:** #325

#### Sprint 2: Conda
**Detailed Plan**: See `docs/plan/v3.0.1-sprint2-conda.md`

Update the NEP Conda recipe for the existing v3.0.0 release, establish a reliable minimal LZ4 compression package with Fortran wrappers, and submit it to conda-forge through staged-recipes.

**Implementation scope:**
- Update `conda/meta.yaml` from v2.7.1 to the v3.0.0 tag tarball with its verified checksum.
- Build LZ4 compression and Fortran wrappers only; disable BZIP2 because NEP does not provide a BZIP2 HDF5 plugin without changing local build behavior.
- Disable every UDF reader, including DICOM, and remove all reader-only dependencies from the recipe.
- Simplify `conda/build.sh` to pass existing compression-enable and reader-disable options for the Conda recipe only; do not modify CMake, Autotools, or their defaults.
- Validate recipe rendering/linting, build and installed artifacts, then install the built package into a clean Conda environment for a plugin smoke test.
- Submit the initial `nep` recipe to `conda-forge/staged-recipes` and address review through feedstock creation.
- Update installation and release-status documentation to describe the intentionally minimal feature set.

**Clarified decisions:**
- The first conda-forge release packages the existing v3.0.0 source release; no new NEP release is created.
- The initial upstream submission uses `conda-forge/staged-recipes`.
- Only LZ4 and Fortran compression wrappers are enabled. BZIP2 and all format readers, including DICOM, remain disabled.
- Core HDF5, NetCDF-C, NetCDF-Fortran, and LZ4 dependencies remain; BZIP2 and reader-specific dependencies are excluded. This selection applies only to the Conda recipe.
- CI requires recipe render/lint, an unmasked package build, installed-artifact checks, and a clean-environment install smoke test.
- Future Conda sprints may add reader support after the minimal package is established.

**Acceptance Criteria:**
- `conda/meta.yaml` packages v3.0.0 using the verified immutable source tag tarball.
- The recipe and build script enable LZ4 and Fortran only, with BZIP2 and all UDF readers disabled.
- The recipe contains no BZIP2 or reader-only dependency, including `libdicom` and `libjpeg-turbo`.
- The built package contains and tests the LZ4 HDF5 plugin and the Fortran module.
- Conda CI renders/lints, builds, tests, and clean-environment-installs the package without ignored failures.
- An initial conda-forge staged-recipes PR for `nep` is opened and passes its required checks.
- Documentation accurately states the minimal Conda feature set and future reader-expansion path.
- No NEP CMake or Autotools source/default file changes are made, and local source builds remain unchanged.

**Testing:** Run recipe render/lint validation, `conda build conda/ --no-anaconda-upload`, plugin and Fortran-module artifact checks, and a clean-environment install/smoke test. Confirm the Conda CI workflow, staged-recipes validation, and the generated feedstock build pass.

**Build System Integration:** No NEP CMake or Autotools source/default changes. `conda/build.sh` alone invokes CMake with existing `NEP_*` options for the Conda package; local source builds remain unchanged.

**Definition of Done:** A minimal v3.0.0 NEP Conda package with LZ4 and Fortran support is validated end to end and submitted through conda-forge's staged-recipes process. Documentation states that BZIP2 and format readers are deferred until they can be added without changing local builds. No NEP CMake, Autotools, runtime, or local-build behavior changes are made.

**GitHub Issue:** #327

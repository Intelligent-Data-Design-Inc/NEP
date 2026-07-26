# NEP Development Roadmap

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
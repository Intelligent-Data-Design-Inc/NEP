# NEP Development Roadmap
### V2.6.0 - Further Testing
#### Sprint 1: Update Spack Package File
**Detailed Plan**: See `docs/plan/v2.6.0-sprint1-spack-version-bump.md`

- Add `version("2.5.0", sha256="fbc160eb8333b2b34c49119c07947f5fa2f526c3ba747d482c8578be8b8c97ef", url="https://github.com/Intelligent-Data-Design-Inc/NEP/archive/v2.5.0.tar.gz")` to `spack/NEP/package.py`.
- Remove the `version("develop", branch="main")` alias; `version("main", branch="main")` is the canonical development branch name.
- Update `.github/workflows/spack.yml` to replace `nep@2.4.0` spec and install steps with `nep@2.5.0`.
- Open an upstream pull request against the [Spack builtin packages repo](https://github.com/spack/spack) to add the v2.5.0 `version()` entry to `var/spack/repos/builtin/packages/nep/package.py` (NEP is already present upstream as `nep`).
- Update `README.md` Spack section to show `spack install nep@2.5.0` as the latest stable.
- Update `docs/prd.md` release history to add the v2.5.0 entry.
- **Testing**: CI spec job targets `nep@2.5.0` and `nep@main`; install job installs `nep@2.5.0~docs~fortran` and `nep@main~docs~fortran` with the existing variant coverage.

**Clarified decisions:**
- Add the v2.5.0 release entry with SHA256 `fbc160eb8333b2b34c49119c07947f5fa2f526c3ba747d482c8578be8b8c97ef` so the spec/install CI jobs can target the real pinned release.
- Remove the `develop` alias (leftover from pre-v2.4.0 work); `main` is the correct branch name going forward.
- Replace `nep@2.4.0` with `nep@2.5.0` in CI — 2.5.0 supersedes 2.4.0 as the pinned stable release under test.
- Open an upstream Spack PR against `spack/spack`; NEP is already registered there as `nep` so only the new `version()` stanza needs adding.
- `README.md` and `docs/prd.md` updated together with the version bump — small, naturally co-located changes.

**GitHub Issue:** #278

#### Sprint 2: PDS4 Testing with Maven Data
**Detailed Plan**: See `docs/plan/v2.6.0-sprint2-maven-pds4-testing.md`

Both Maven NGIMS data files are already present in `test/data/PDS4/`:
- **L1B housekeeping** (`mvn_ngi_l1b_cal-hk-058943_20250101T023235_v01_r02.xml`): `Table_Delimited`, 324 fields, 26121 records, large sparse CSV (9.3 MB). Tests open/parse and verify field count, record count, and a light read of the `TIME` field (first 2 records, value > 0).
- **L3 science** (`mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml`): `Table_Delimited`, 15 fields, 2 records, compact CSV. Tests verify group name, field count, record count, and known data values for `T_UNIX`, `SCALE_HEIGHT`, and `TEMPERATURE`.

Add a `test_mission_maven_l1b()` and `test_mission_maven_l3()` function to `test/tst_pds4_udf.c`, following the pattern of the existing `test_mission_cassini_hrd()`, `test_mission_messenger_tnmap()`, and `test_mission_lcs_9p()` functions.

Resolve type-mapping gaps exposed by the Maven L3 label before writing tests:
- `ASCII_NonNegative_Integer` → `NC_INT64` (add to type table in `src/pds4file.c`)
- `ASCII_Date_Time` → `NC_STRING` (add to type table in `src/pds4file.c`)

Determine empirically (build + run) whether field names containing hyphens (e.g., `EXO-ALT`) are preserved as-is or normalized to underscores (`EXO_ALT`) by the reader; update `tst_pds4_udf.c` with the actual name found at runtime.

Both Maven XML + CSV file pairs added to `test/CMakeLists.txt` copy rules and `test/Makefile.am` `dist_check_DATA` lists, consistent with the existing PDS4 test data setup.

**Clarified decisions:**
- Both Maven files get test functions — L3 for tight value verification, L1B for large-file stress testing.
- L1B data read is light: open + parse, read `TIME` for first 2 records, verify > 0; no full 26121-row read.
- Field name normalization (hyphens) determined empirically before hardcoding test assertions.
- `ASCII_NonNegative_Integer` → `NC_INT64` and `ASCII_Date_Time` → `NC_STRING` added to the type table as part of this sprint.
- Both Maven file pairs copied to build directory (CMake and Autotools) following existing PDS4 data pattern.

**GitHub Issue:** #280

#### Sprint 3: More Maven Testing — IUVS Corona (FITS-backed PDS4)
**Detailed Plan**: See `docs/plan/v2.6.0-sprint3-maven-iuvs-testing.md`

Implement `Group_Field_Binary` support in the PDS4 reader and test with
`mvn_iuv_l2_corona-orbit00407-fuv_20141214T192758.xml` — a PDS4 label that
uses a FITS file as its binary data container. The XML + FITS file pair is
already in `test/data/PDS4/`.

**Implementation tasks:**
1. Add `Group_Field_Binary` parsing to `pds4_read_table()` in `src/pds4file.c`:
   - Detect `Group_Field_Binary` elements alongside `Field_Binary` in the Record loop.
   - Parse `<repetitions>`, `<group_location>`, `<group_length>`.
   - For each `Field_Binary` inside the group, create a 2D variable `[record, repetition]` with a trailing dimension of size `repetitions`.
   - Compute the correct per-repetition byte offset for data reading (group_location + rep × field_size + field_location - 1).
2. Update the binary table data reading path in `pds4_get_vara()` to handle 2D group fields (stride by `group_length` for each repetition within a record).
3. Add both `.xml` and `.fits` to build system (`test/CMakeLists.txt`, `test/Makefile.am`).
4. Add two test functions to `test/tst_pds4_udf.c`:
   - `test_mission_maven_iuvs_metadata()` — verifies group name, total nvars (scalar + group fields), confirms `COLUMN` has ndims=2 with trailing dim=2.
   - `test_mission_maven_iuvs_data()` — reads `TANGENT_ALT[0]` (scalar, verify finite > 0) and `COLUMN[0,0]` (group field, verify finite).

The label contains 8 `Table_Binary` sections with data types `IEEE754MSBSingle`,
`IEEE754MSBDouble`, `ASCII_String`, `SignedMSB2`. The `outbound_above_limb` table
has 14 scalar fields + 17 Group_Field_Binary (repetitions=2 or 3), making it the
key test case for the new capability.

**Clarified decisions:**
- `Group_Field_Binary` fields become 2D variables `[record, repetition]` — simple, flat model.
- Only handle `groups=0` (no nested groups) for now; nested groups return `NC_EINVAL`.
- Both `.xml` and `.fits` copied to build directory unconditionally (CMake and Autotools).
- Two test functions (`_metadata` and `_data`) to separate concerns.

**GitHub Issue:** #282

### V2.5.0 More Spack Improvements
#### Sprint 1: GeoTIFF Variant
**Detailed Plan**: See `docs/plan/v2.5.0-sprint1-geotiff-variant.md`

- Add `version("2.4.0", ...)` to `spack/NEP/package.py` so `nep@2.4.0` can be used for variant checks.
- Add variant `geotiff` (default off).
- Add `depends_on("libgeotiff", when="+geotiff")` and `depends_on("libtiff", when="+geotiff")`.
- Pass `NEP_ENABLE_GEOTIFF` in `cmake_args`.
- Add a `check_install` assertion for the GeoTIFF dispatch library when `+geotiff`.
- **Testing**: Add `spack spec -I nep@2.4.0+geotiff` and `spack spec -I nep@main+geotiff` to spec job; add `spack install -v nep@2.4.0+geotiff~docs~fortran` and `spack install -v nep@main+geotiff~docs~fortran` to install job.

**Clarified decisions:**
- Add the v2.4.0 release entry now (SHA256 `d4144894ed5f7f544bd68252fdd905b5e8f8b27f8829049a9a344f226278c84b`) so the spec/install jobs can target a real pinned release.
- Keep the `geotiff` variant default OFF to match `NEP_ENABLE_GEOTIFF=OFF` in CMake.
- Declare both `libgeotiff` and `libtiff` explicitly in Spack to match CMake's dual-library requirement.
- Keep the install-job style consistent with existing steps (`--fail-fast` wrapped with `|| true` and manual log tailing).
- Verify the GeoTIFF dispatch library (`libncgeotiff.so`) is installed when `+geotiff`.

**GitHub Issue:** #268

#### Sprint 2: GRIB2 Variant
**Detailed Plan**: See `docs/plan/v2.5.0-sprint2-grib2-variant.md`

- Add variant `grib2` (default off).
- Add `depends_on("g2c", when="+grib2")` — the Spack builtin package name for NCEPLIBS-g2c; transitive deps (Jasper, libpng) resolved by Spack automatically.
- Pass `NEP_ENABLE_GRIB2` in `cmake_args` via `self.define_from_variant("NEP_ENABLE_GRIB2", "grib2")`.
- Add `check_install` assertion for `libncgrib2.so` when `+grib2`, consistent with Sprint 1.
- **Testing**: Add `spack spec -I nep@2.4.0+grib2` and `spack spec -I nep@main+grib2` to spec job as a dedicated step; add `spack install -v nep@2.4.0+grib2~docs~fortran` and `spack install -v nep@main+grib2~docs~fortran` to install job with `|| true` and log tailing (Spack builds `nceplibs-g2c` from source).

**Clarified decisions:**
- `grib2` variant default OFF to match `NEP_ENABLE_GRIB2=OFF` in CMake and all other format variants.
- Use `g2c` as the Spack package name (Spack builtin name for NCEPLIBS-g2c; see `spack/spack-packages` repo).
- Transitive g2c dependencies (Jasper, libpng, zlib) are resolved by Spack through `g2c`; no explicit re-declaration needed.
- `check_install()` asserts `libncgrib2.so` exists when `+grib2`, consistent with the `libncgeotiff.so` check in Sprint 1.
- Dedicated spec step for `+grib2` (not combined with all-variants step) for easier CI triage.
- Install job includes `+grib2` steps with `|| true`; Spack builds `nceplibs-g2c` from source since it is not in standard apt repos.

**GitHub Issue:** #270

#### Sprint 3: CDF Variant
**Detailed Plan**: See `docs/plan/v2.5.0-sprint3-cdf-variant.md`

- Add variant `cdf` (default off).
- Add `depends_on("cdf", when="+cdf")` — references the in-repo `spack/cdf/package.py`; not a Spack builtin.
- Pass `NEP_ENABLE_CDF` in `cmake_args` via `self.define_from_variant("NEP_ENABLE_CDF", "cdf")`.
- Register the in-repo CDF package in `spack.yml` by copying `spack/cdf/package.py` into the same `nep-repo` local repository (add a `cdf/` subdirectory alongside `nep/`).
- Add `check_install` assertion for `libnccdf.so` when `+cdf` (library name confirmed from `src/CMakeLists.txt`: `set(CDF_LIB_NAME nccdf)`).
- Keep `spack-cdf.yml` separate; consolidation deferred to Sprint 5.
- **Testing**: Add dedicated spec step for `nep@2.4.0+cdf` and `nep@main+cdf`; add install steps with `|| true` and log tailing, consistent with sprints 1 and 2.

**Clarified decisions:**
- `cdf` variant default OFF to match `NEP_ENABLE_CDF=OFF` in CMake and all other format variants.
- CDF is an in-repo Spack package (`spack/cdf/package.py`), not a builtin; `spack.yml` must register it in the local repo before concretizing `+cdf`.
- Register by copying `spack/cdf/package.py` into `$HOME/nep-repo/packages/cdf/` in the "Add NEP package to Spack" step — one repo, one setup, no second `spack repo add`.
- `check_install()` asserts `libnccdf.so` when `+cdf` (CMake target name: `nccdf`), consistent with `libncgeotiff.so` and `libncgrib2.so` checks.
- `spack-cdf.yml` remains separate; merging into `spack.yml` deferred to Sprint 5.
- Dedicated spec/install CI steps following the sprint 1 and 2 pattern.

**GitHub Issue:** #272

#### Sprint 4: PDS4 + Parallel + Remaining Variants
**Detailed Plan**: See `docs/plan/v2.5.0-sprint4-remaining-variants.md`

- Add variants: `pds4` (default off), `parallel` (default off), `examples` (default off), `benchmarks` (default off).
- Add deps: `depends_on("libxml2", when="+pds4")`, `depends_on("mpi", when="+parallel")`.
- When `+parallel`, adjust `hdf5` dep to `+mpi` and `netcdf-c` dep to `+mpi` (parallel NEP requires both parallel HDF5 and parallel netcdf-c with `NC_HAS_PARALLEL4`).
- Pass all remaining `NEP_ENABLE_*` / `NEP_BUILD_*` flags in `cmake_args`: `NEP_ENABLE_PDS4`, `NEP_ENABLE_PARALLEL_TESTS`, `NEP_BUILD_EXAMPLES`, `NEP_ENABLE_BENCHMARKS`.
- Add `check_install` assertion for `libncpds4.so` when `+pds4`; add `libncfits.so` when `+fits` (missing from earlier sprints).
- **Testing**: Add dedicated spec steps for `nep@2.4.0+pds4`, `nep@main+pds4`, `nep@2.4.0+parallel`, and `nep@main+parallel` to spec job. Add `spack install -v nep@2.4.0+pds4~docs~fortran` and `spack install -v nep@main+pds4~docs~fortran` to install job with `|| true` and log tailing. Parallel install-test skipped in CI (spec-only); MPI builds are slow and unreliable on GitHub runners.

**Clarified decisions:**
- `parallel` variant maps to `NEP_ENABLE_PARALLEL_TESTS` in `cmake_args`; parallel I/O itself is automatic when netcdf-c is built with MPI — the variant ensures the parallel stack is concretized and tests are enabled.
- Split HDF5 dependency: base dep `depends_on("hdf5@1.12:+hl~mpi", when="~parallel")`, parallel dep `depends_on("hdf5@1.12:+hl+mpi", when="+parallel")`.
- Add `depends_on("netcdf-c +mpi", when="+parallel")` so Spack concretizes with parallel netcdf-c.
- `examples` and `benchmarks` default OFF, matching CMake defaults (`NEP_BUILD_EXAMPLES=OFF`, `NEP_ENABLE_BENCHMARKS=OFF`).
- `check_install()` asserts `libncpds4.so` when `+pds4` (CMake target: `ncpds4`), consistent with all other format variant checks.
- Also add missing `libncfits.so` check when `+fits` (CMake target: `ncfits`).
- Parallel: spec-only in CI (no install step) due to MPI build complexity on runners.

**GitHub Issue:** #274

#### Sprint 5: Polish, CI, and Integration Testing
**Detailed Plan**: See `docs/plan/v2.5.0-sprint5-polish-ci.md`

- No `conflicts()` declarations added — no CMake-level incompatibilities exist across format variants; Spack `depends_on` constraints already guard the parallel stack.
- Update `README.md` with a comprehensive Spack installation section: full variant table with descriptions, common combination examples (e.g., `nep+geotiff+grib2`), and step-by-step local repo setup for `+cdf` (non-obvious requirement).
- Add all-variants spec steps to `spack.yml` spec job: `spack spec -I nep@2.4.0+geotiff+grib2+cdf+fits+pds4+fortran+docs` and `spack spec -I nep@main+geotiff+grib2+cdf+fits+pds4+fortran+docs`. Keep `+parallel` in a dedicated separate spec step (mixing it into all-variants forces MPI on the entire tree).
- Add all-variants install steps to `spack.yml` install job as **hard failures** (no `|| true`): `spack install -v --fail-fast nep@2.4.0+geotiff+grib2+cdf+fits+pds4~docs~fortran` and `nep@main+geotiff+grib2+cdf+fits+pds4~docs~fortran`. Sprint 5 is the integration milestone; hard failures surface real breakage.
- No `spack test` method added — functional post-install testing deferred to a future sprint.
- **Testing**: All-variants spec check (both `2.4.0` and `main`); all-variants install (hard fail); `+parallel` spec-only check retained from Sprint 4.

**Clarified decisions:**
- No `conflicts()` added: all format variants are independent at the CMake level; the parallel stack is guarded by conditional `depends_on` constraints already in place.
- All-variants install uses hard `--fail-fast` without `|| true`; Sprint 5 is the final integration milestone for the Spack packaging work.
- `+parallel` kept as a dedicated spec step; it is not combined with the all-format-variants spec because it requires `hdf5+mpi` and `netcdf-c+mpi` throughout, which inflates the dependency tree.
- `README.md` Spack section is comprehensive: full variant table, common usage examples, and inline CDF local-repo setup steps.
- No `spack test` / `test()` method — functional post-install testing is out of scope for this sprint.

**GitHub Issue:** #276


### V2.4.0 Spack Improvements

#### Sprint 1: Foundation — Versions, Description, and netcdf-c Bump
**Detailed Plan**: See `docs/plan/v2.4.0-sprint1-spack-foundation.md`

- Add `version("2.3.0", ...)` release (only 2.3.0+ is supported; no earlier version).
- Update package description to reflect multi-format reader.
- Bump `depends_on("netcdf-c@4.10.1:", ...)`.
- Update `cmake_args` to pass all existing variants properly (`NEP_BUILD_LZ4`, `NEP_BUILD_BZIP2`, `NEP_BUILD_DOCUMENTATION`, `NEP_ENABLE_FORTRAN`, `NEP_ENABLE_FITS`).
- **Testing**: Spec job keeps `nep@develop` checks; install job keeps `nep@develop` step and adds `nep@2.3.0~docs~fortran` install step.

**Clarified decisions:**
- `version("2.3.0", sha256="ce6eb7640a44e4b068af4beef3a1b7c5a4772666fbea73db34d81d71b4144dde", url="https://github.com/Intelligent-Data-Design-Inc/NEP/archive/v2.3.0.tar.gz")` — actual release SHA256 from published GitHub tag.
- `cmake_args` uses `self.define_from_variant(...)` for all five variants so user choices propagate correctly.
- Spec job `nep@develop` checks remain unchanged; `develop` is the ongoing concretization target.
- Install job adds `nep@2.3.0~docs~fortran` as a second install step alongside the existing `develop` step.
- `spack-cdf.yml` not touched; deferred to Sprint 4.

#### Sprint 1.5: Fix CMake Options
**Detailed Plan**: See `docs/plan/v2.4.0-sprint1.5-fix-cmake-options.md`

- Rename all project-specific CMake options to use the `NEP_` prefix, following the netcdf-c and HDF5 conventions. Affected options: `NEP_ENABLE_FORTRAN`, `NEP_BUILD_LZ4`, `NEP_BUILD_BZIP2`, `NEP_BUILD_DOCUMENTATION`, `NEP_BUILD_EXAMPLES`, `NEP_ENABLE_CDF`, `NEP_ENABLE_GEOTIFF`, `NEP_ENABLE_FITS`, `NEP_ENABLE_PDS4`, `NEP_ENABLE_GRIB2`, `NEP_ENABLE_PARALLEL_TESTS`, `NEP_ENABLE_BENCHMARKS`, `NEP_ENABLE_OPENDAP_EXAMPLES`.
- Leave `BUILD_TESTING` and `DEBUG` unprefixed (`BUILD_TESTING` is managed by CTest; `DEBUG` is a generic debug flag).
- Keep internal `config.h.cmake.in` macros (`BUILD_LZ4`, `BUILD_BZIP2`, `HAVE_*`) unchanged; only user-facing CMake option names change.
- Remove old unprefixed option names with no deprecated aliases; update all in-repo callers in the same sprint.
- Update `spack/NEP/package.py` `cmake_args` to pass the new `NEP_*` option names.
- Update CI workflow files and `.devin/rules/local-build-command.md` to use the new option names.
- Update `README.md` and any other documentation that references the old CMake option names.
- **Testing**: Existing CI configure/build/test matrix exercises the renamed options. Old option names are removed entirely, so no regression test for them is added.

**Clarified decisions:**
- Hard rename with no backward-compatibility aliases; all in-repo callers updated in this sprint.
- `BUILD_TESTING` and `DEBUG` are excluded from the prefix.
- Internal `config.h` macros remain unchanged.
- Spack package, CI workflows, and local build rules updated together.

**GitHub Issue:** #265


### V2.3.0 Documentation and More Testing for PDS4 Read

#### Sprint 1: Doxygen Docs for PDS4 Reader
**Detailed Plan**: See `docs/plan/v2.3.0-sprint1-pds4-doxygen.md`

- Add a "PDS4 File Reader" section to `docs/mainpage.md` matching the style and depth of the existing FITS section (What is PDS4 / Key Characteristics / PDS4 Support in NEP / Enabling PDS4 Support / C API usage example).
- Expand the `@file` block in `src/pds4file.c` to add sections on the netCDF model mapping (groups, dimensions, variables, attributes), byte-order handling, and data file resolution. Add `@param`/`@return` Doxygen tags to all public functions missing them; leave internal helpers with their existing comments.
- Fix struct visibility in the Doxygen Data Structures page: remove the `@internal` tag from the `@file` block in `include/pds4dispatch.h` so `NC_PDS4_VAR_INFO_T` and `NC_PDS4_FILE_INFO_T` appear publicly, and add `@brief` descriptions to all struct fields.
- Add a PDS4 API subsection to `docs/prd.md` covering `NC_PDS4_initialize()`, `NC_PDS4_finalize()`, and `NC_PDS4_get_vara()`, consistent with the CDF/GeoTIFF/GRIB2/FITS sections already present.

**Clarified decisions:**
- `pds4file.c` expansion: public functions only (not `@internal` helpers); add `@param`/`@return` where missing.
- Struct fix: remove `@internal` from `pds4dispatch.h` `@file` tag (not `EXTRACT_PRIVATE=YES`, which would expose other internal entities globally).
- `docs/mainpage.md`: full PDS4 section with C usage example, matching FITS section depth.
- `docs/prd.md`: new PDS4 API subsection; no code changes.
- Sprint 1 is documentation-only; new tests deferred to Sprint 3.

#### Sprint 2: Require netcdf-c-4.10.1
**Detailed Plan**: See `docs/plan/v2.3.0-sprint2-netcdf-c-4.10.1.md`

- netcdf-c-4.10.1 just released. This is the new minimum required netcdf-c version for the NEP.
- CMake and autotools builds must require netcdf-c-4.10.1.
- Update all CIs to use this netcdf-c version.
- Document in `README.md` that this version is required, to support UDF numbers > 2.

**Clarified decisions:**
- Both build systems use a two-layer check: update the pkg-config minimum to `>= 4.10.1` and add a compile-time `NC_VERSION_*` sanity check.
- All CI workflows that build NetCDF-C from source move to 4.10.1: `ci.yml`, `ci-fits.yml`, `ci-formats.yml`, `ci-parallel.yml`, and `geotiff-test.yml`.
- No runtime version probe test is added; the build-system requirement is the verification mechanism.
- Local build guidance documents are updated only if they currently mention a specific NetCDF-C version.

#### Sprint 3: Move Diagrams
**Detailed Plan**: See `docs/plan/v2.3.0-sprint3-move-diagrams.md`

- Move the compression-related SVG plots (`docs/compression_performance.svg`, `docs/compression_performance_fast.svg`) from `docs/` to `docs/images/` and update all references in `README.md` and `docs/compression.md`.
- Add companion `metadata.txt` files for both SVGs per the NEP diagram rules (title, caption ≤ 75 words, alt_text).
- Expand `docs/compression.md` with the full performance discussion, comparison table, and key insights currently duplicated in `README.md`.
- Trim the README compression section to a brief summary that links to `docs/compression.md`.
- Verify Doxygen renders the compression page with the moved images and no new warnings.

**Clarified decisions:**
- Move both SVGs into `docs/images/` so all visual assets live in one directory.
- Create required `metadata.txt` files for each SVG as part of this move, since the diagrams are already being touched.
- `docs/compression.md` becomes the authoritative compression performance page; `README.md` keeps only a high-level summary with a cross-link.
- Sprint 3 is documentation-only; no code, build system, or test changes.

### V2.2.0 Read NASA/ESA Planetary Data System 4 Data

#### Sprint 1: Make Extended Data Formats Off by Default
**Detailed Plan**: See `docs/plan/v2.2.0-sprint1-formats-off-by-default.md`

- GeoTIFF, GRIB2, and FITS currently default to ON in both CMake and Autotools; CDF already defaults OFF. Change all four extended format readers to default OFF.
- Remove the hard requirement to install libgeotiff, libg2c/libjasper, and CFITSIO for a default NEP build.
- Keep the existing CDF/GRIB2 UDF slot 2 mutual exclusivity in place for this sprint.
- Add a new `.github/workflows/ci-formats.yml` workflow that tests GeoTIFF + GRIB2 + FITS together in one job and CDF alone in a second job, so every reader is exercised somewhere without triggering the slot conflict.
- Update `README.md` and `docs/prd.md` to reflect the new default-off status and the opt-in build flags.

**Clarified decisions:**
- Sprint 1 does **not** reassign UDF slots; slot reassignment is deferred to Sprint 2.
- Combined CI job uses GRIB2 (not CDF) alongside GeoTIFF and FITS.
- CDF is exercised in a separate job within `ci-formats.yml`.

#### Sprint 2: Reassign UDF Slot for CDF
**Detailed Plan**: See `docs/plan/v2.2.0-sprint2-udf-slot-reassignment.md`

- Move CDF from UDF slot 2 to a dedicated permanent slot (UDF4) using the expanded NetCDF-C 0–9 slot range.
- Remove the CMake/Autotools error that prevents CDF and GRIB2 from being enabled together.
- Upgrade `ci-formats.yml` to build and test GeoTIFF + GRIB2 + CDF + FITS simultaneously.
- Update `docs/design.md`, `include/nep.h`, and `.ncrc` generation to reflect the new permanent slot assignments.

**Clarified decisions:**
- Permanent slot assignments: GeoTIFF BigTIFF=UDF0, GeoTIFF standard=UDF1, GRIB2=UDF2, FITS=UDF3, CDF=UDF4.
- No PDS4 code in this sprint; PDS4 slot reserved in Sprint 3.

#### Sprint 3: Prepare for PDS4
**Detailed Plan**: See `docs/plan/v2.2.0-sprint3-pds4-prep.md`

- Study PDS4 and create skill files under `.devin/skills/pds4/`.
- Add `--enable-pds4` / `-DNEP_ENABLE_PDS4=ON` build options to both build systems; default OFF.
- Assign PDS4 to UDF5.
- Build the PDS4 UDF dispatch library with no-op read functions (`ncpds4dispatch.c`/`ncpds4file.c`) following the existing handler pattern.
- Add a C test that attempts to open a real PDS4 label file via `nc_open()`.
- Populate `test/data/PDS4/` with sample PDS4 files and copy them to build directories for testing.
- Add a PDS4-only job to `ci-formats.yml`.

**Clarified decisions:**
- PDS4 uses XML label files; parse labels with **libxml2** (`libxml2-dev` on Ubuntu).
- PDS4 is assigned **UDF5** slot.

#### Sprint 4: Read PDS4 Array Metadata
**Detailed Plan**: See `docs/plan/v2.2.0-sprint4-pds4-array-metadata.md`

- Parse the opened PDS4 XML label and map `Identification_Area` and `Observation_Area` contents to global attributes on the root group.
- For each `File_Area_Observational`, create a child group named from `File/file_name`.
- For each `Array` / `Array_2D_Image` inside a file area, create netCDF dimensions from `Axis_Array` entries and a netCDF variable from `Element_Array/data_type`.
- Replace no-op metadata inquiry dispatch functions with implementations that return the actual group/dimension/variable/attribute structure.
- Extend `test/tst_pds4_udf.c` to verify global attributes, group names, dimensions, variables, and types for the existing `test_image.xml` sample.
- Leave data reading (`NC_PDS4_get_vara`) returning `NC_EINVAL`; metadata-only this sprint.

**Clarified decisions:**
- `Axis_Array` sequence number determines dimension order; `axis_name` becomes the dimension name.
- `Last Index Fastest` maps to C-order netCDF dimensions (fastest-varying rightmost).
- Array element data types are mapped to the closest netCDF type; byte order is recorded for the data sprint.

#### Sprint 5: Read PDS4 Table Metadata
**Detailed Plan**: See `docs/plan/v2.2.0-sprint5-pds4-table-metadata.md`

- Add support for `Table_Binary`, `Table_Character`, and `Table_Delimited` inside `File_Area_Observational`.
- Create a row dimension for each table (length from `records` or computed from file size/record size).
- Create one netCDF variable per `Field_*` with name from `name`, type from `data_type`, and `units` attribute from `unit`.
- Add a sample table PDS4 label and data file to `test/data/PDS4/`.
- Update tests to verify table group/dimension/variable/attribute metadata.
- Keep data reading no-op.

**Clarified decisions:**
- Fixed-record binary/character tables use record count as row dimension.
- Delimited tables derive row count by parsing the file or from label `records`.
- Repeated/vector fields become additional trailing dimensions.

#### Sprint 6: Read PDS4 Data
**Detailed Plan**: See `docs/plan/v2.2.0-sprint6-pds4-data-read.md`

- Implement `NC_PDS4_get_vara()` for both arrays and table fields.
- Resolve referenced data file paths relative to the XML label directory.
- Read binary array data honoring PDS4 byte order and convert to requested `memtype`.
- Read table data: fixed-record binary/character fields by offset, delimited fields by parsing.
- Update tests to read subsets from the array and table samples and verify values.
- Update `docs/prd.md` and `README.md` to remove the “no data reading” limitation and document usage.

**Clarified decisions:**
- Array I/O maps netCDF 0-based `start[]/count[]` directly to PDS4 line/sample order, applying byte-swap when the label type is MSB on a little-endian host.
- Table field reads use the row dimension index as the record index; delimited tables scan lines on first access.
- ASCII numeric fields are parsed as the mapped netCDF type.

### V2.1.0 Updated Examples for Boook
- Examples and docs were updated in support of second edition of NetCDF Developer's Handbook.

### V2.0.0 Read-only FITS Layer
#### Sprint 1: Set up CI and Build Systems
**Detailed Plan**: See `docs/plan/v2.0.0-sprint1-fits-ci.md`

- Add a CI run which will test FITS reader.
- Needs to install CFITSIO library on github runner.
- Needs to install latest build main branch of netcdf-c from github repo (can be built with --disable-dap for this CI run)
- Will also need netcdf-fortran, built from latest main on github.
- Will also need HDF5-2.1.1 installed before netcdf-c in order to get netCDF-4
- All CI installs should be cached.
- Autotools and cmake build systems should be updated to include new option to build FITS reader (default on).
- For this sprint, the CI should then build NEP and run tests, but there is no FITS code or tests in NEP yet.

**Clarified decisions:**
- HDF5 2.1.1 (separate cache keys from ci.yml)
- NetCDF-C and NetCDF-Fortran from main branch
- CFITSIO via apt-get install libcfitsio-dev
- Separate workflow file: ci-fits.yml
- Build matrix: cmake × autotools, Fortran ON
- FITS assigned to UDF3 slot

#### Sprint 2: Set up FITS Dispatch Layer and Tests
**Detailed Plan**: See `docs/plan/v2.0.0-sprint2-fits-dispatch.md`

- Build the FITS UDF dispatch library (`libncfits`) with no-op read functions.
- Flip `NEP_ENABLE_FITS` / `--enable-fits` default to **ON**; add `-DNEP_ENABLE_FITS=OFF` / `--disable-fits` to the other CI workflows so only `ci-fits.yml` needs CFITSIO.
- Create `include/fitsdispatch.h`, `src/fitsdispatch.c`, and `src/fitsfile.c` following the GRIB2/CDF/GeoTIFF pattern.
- Generate the `.ncrc` UDF3 autoload block for FITS in both CMake and Autotools.
- Create `test/tst_fits_udf.c` that calls `NC_FITS_initialize()` and does an `nc_open()`/`nc_close()` round-trip on the real FITS file without reading CFITSIO metadata.
- Create `ftest/ftst_fits_udf.F90` and add `ftest/CMakeLists.txt` so the Fortran test is built in both CMake and Autotools.
- Ensure `test/data/WFPC2u5780205r_c0fx.fits` is copied to build directories for both tests.

**Clarified decisions:**
- `NEP_ENABLE_FITS` / `--enable-fits` default ON.
- C test: `nc_open()`/`nc_close()` round-trip on the real FITS file.
- Fortran test: `ftest/ftst_fits_udf.F90` with a new `ftest/CMakeLists.txt`.
- Primary HDU content goes in the root group; extension HDUs become child groups.
- WCS coordinate variables are deferred to a later release.

#### Sprint 3: Open a Real FITS File
**Detailed Plan**: See `docs/plan/v2.0.0-sprint3-fits-open.md`
git tag -d v2.1.0 2>/dev/null ; git push origin :refs/tags/v2.1.0 ; git tag -a v2.1.0 -m "NEP v2.1.0" && git push origin v2.1.0
- Tests (C and Fortran) now open the real FITS file.
- NEP tests open and close file, but nothing else yet.
- In this sprint, the real FITS file is opened, and closed, but no metadata or data are read yet.
- Modify `NC_FITS_open()` to call `fits_open_file()` and store the CFITSIO file handle.
- Update `NC_FITS_close()` to properly close the CFITSIO file handle.
- Tests remain unchanged - they still just verify successful open/close operations.

**Status**: In progress - need to implement actual CFITSIO integration in the dispatch layer.

#### Sprint 4a: Read Primary HDU Metadata
**Detailed Plan**: See `docs/plan/v2.0.0-sprint4a-fits-primary-metadata.md`

- Read all header keywords from the primary HDU and store them as global netCDF attributes on the root group.
- For the primary HDU image (if `NAXIS > 0`): create netCDF dimensions (reversed order from FITS) and a netCDF variable in the root group.
- Map `BITPIX` to the correct `nc_type`; map standard keywords (`BUNIT`→`units`, `BZERO`→`add_offset`, `BSCALE`→`scale_factor`, `BLANK`→`_FillValue`) to netCDF attributes.
- `nc_inq*` functions now return correct results for the primary HDU. C test is updated to confirm primary HDU dims, variable, and attributes for the test FITS file.
- No extension HDUs in this sprint.

#### Sprint 4b: Read Extension HDU Metadata
**Detailed Plan**: See `docs/plan/v2.0.0-sprint4b-fits-extension-metadata.md`

- Each extension HDU becomes a child group of the root group, named from `EXTNAME` (or `hdu_N` if absent).
- Image extension HDUs: same dim/variable/attribute mapping as 4a, applied to the child group.
- Binary and ASCII table HDUs: create a row dimension (`NAXIS2`), one netCDF variable per column (named from `TTYPEn`), with `TUNITn`→`units` attributes. Vector columns become 2D variables.
- All HDU header keywords stored as group-level attributes.
- C and Fortran tests updated to verify extension group names, dims, variables, and attributes for the test FITS file.
- By the end of this sprint, all FITS metadata will have been converted to the netCDF model.

#### Sprint 5: Read the FITS Data
**Detailed Plan**: See `docs/plan/v2.0.0-sprint5-fits-data.md`

- Implement `NC_FITS_get_vara()` to read actual pixel/column data via CFITSIO.
- For image variables (`col_num == 0`): convert netCDF 0-based `start[]/count[]` to
  FITS 1-based `fpixel[]/lpixel[]` with reversed dimension order, then call
  `fits_read_subset()`.
- For table column variables (`col_num > 0`): call `fits_read_col()` using
  `start[0]+1` as `firstrow` and `count[0]` as `nelements`; for 2D variables
  (vector or string columns) use `start[1]+1` as `firstelem`.
- Map netCDF `memtype` to the appropriate CFITSIO datatype constant.
- C test updated to read a small subset of the primary image and verify pixel values
  against known data, and to read one scalar column from the extension table.
- Fortran test updated to read a row of the `image` variable and verify at least one value.
- By the end of this sprint, the FITS reader will be fully functional.

**Clarified decisions:**
- Image data: use `fits_read_subset()` with dimension order reversal.
- Table data: use `fits_read_col()`; string columns read via `TSTRING` into a flat
  `char` buffer.
- CFITSIO performs `BSCALE`/`BZERO` and `TSCALn`/`TZEROn` scaling automatically when
  reading into floating-point types; no extra scaling step needed.
- Known pixel values extracted from the test file before implementation to anchor the test.

#### Sprint 6: Documentation and More Tests
**Detailed Plan**: See `docs/plan/v2.0.0-sprint6-fits-docs-tests.md`

- Add FITS reader section to `README.md` (Capability / Benefits / How It Works / Use Cases),
  consistent with the existing CDF, GeoTIFF, and GRIB2 sections.
- Add FITS to the format list in `docs/mainpage.md` (Doxygen landing page).
- Update the `@file` block in `src/fitsfile.c` to reflect the fully implemented state.
- Add moderate additional test coverage to `test/tst_fits_udf.c`:
  - Read a second image plane (`image[1,0,0]`) and verify against the known float value.
  - Read a non-zero-start hyperslab (`image[0,1,0:4]`) to exercise the offset path.
  - Read all 4 rows of the 2D string column `CTYPE1` and verify the first string.
- Add matching additional assertions to `ftest/ftst_fits_udf.F90`.
- By the end of this sprint, the FITS reader is fully documented and tested.


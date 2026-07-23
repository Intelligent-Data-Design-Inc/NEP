# NEP Development Roadmap

### V2.8.0 - Visualization

Add command-line Python visualization examples to NEP that read FITS, CDF, GeoTIFF, GRIB2, and PDS4 files through the NetCDF API and the NEP UDF dispatch layer, then generate static PNG plots with Matplotlib. The Python `netCDF4` library opens UDF files directly with `.ncrc` autoload; there are no CSV intermediates and no C extractors. Plots are black and white for book publication.

#### Sprint 1: Visualization Infrastructure
**Detailed Plan**: See `docs/plan/v2.8.0-sprint1-visualization-infrastructure.md`

Establish disabled-by-default Python visualization infrastructure for NEP, including common PNG and metadata output handling, strict dependency validation, equivalent CMake and Autotools integration, and a FITS smoke test proving that `netCDF4.Dataset` can open a NEP UDF file through build-tree `.ncrc` autoload.

**Implementation scope:**
- Create `examples/viz/` with `README.md`, `CMakeLists.txt`, `Makefile.am`, common plotting support, and infrastructure smoke tests.
- Add CMake option `NEP_ENABLE_VIZ_EXAMPLES` and Autotools option `--enable-viz-examples`, both defaulting to disabled.
- Require normal examples and at least one enabled UDF reader when visualization examples are enabled.
- Require the source-root `.venv` and detect Python 3, `netCDF4`, Matplotlib, and NumPy at configuration time; explicit visualization enablement fails with a clear diagnostic when the venv or a dependency is missing.
- Implement `plot_common.py` with `save_with_metadata()` to write a 150-DPI PNG and companion `_metadata.txt` containing `title`, `caption`, and `alt_text`, while enforcing the 8-inch width and 6.1-inch height limits.
- Set `NCRCENV_RC`, `NETCDF_RC`, and `LD_LIBRARY_PATH` for CMake and Autotools visualization tests so Python uses only the generated build-tree `.ncrc` and UDF libraries.
- Add a dummy plotting-helper test and a FITS UDF-open smoke test using `test/data/WFPC2u5780205r_c0fx.fits`.
- Keep generated PNG and metadata artifacts in the build tree for inspection; do not install them or write them into the source tree.
- Defer all CDF registration, conversion fallback, and visualization work to Sprint 2.

**Clarified decisions:**
- FITS is the only Sprint 1 UDF smoke-test format; CDF is explicitly out of scope.
- No CSV intermediate files and no C extractor programs.
- Plots are black and white; captions live in `_metadata.txt`, not inside PNGs.
- Python dependencies are installed from `requirements.txt` into the source-root `.venv`; `netCDF4` is built from source and must use the same `libnetcdf` installation as NEP.
- A missing project `.venv`, Python 3, `netCDF4`, Matplotlib, or NumPy is a configuration error when visualization examples are explicitly enabled.
- Visualization requires normal examples and at least one enabled UDF reader; tests are registered only for enabled readers.
- Successful test artifacts remain in the build tree and are not installed.

**Acceptance Criteria:**
- `examples/viz/` is present and conditionally included by both build systems.
- Both visualization options default to disabled and enforce equivalent prerequisites and dependency diagnostics.
- `plot_common.py` saves a size-compliant dummy PNG and companion metadata file with all required fields.
- CMake and Autotools tests provide build-tree `NCRCENV_RC`, `NETCDF_RC`, and `LD_LIBRARY_PATH` settings.
- With FITS enabled, `netCDF4.Dataset` opens and closes the existing WFPC2 FITS file without an explicit NEP initializer call.
- `ctest -R viz` and the Autotools visualization test subset pass in out-of-tree builds.
- Existing builds remain unchanged when visualization examples are disabled, and no CDF source or fallback changes are made.

**Testing:** Validate missing-dependency diagnostics; run out-of-tree CMake and Autotools builds with examples, visualization, and FITS enabled; run the visualization test subsets; verify PNG dimensions, metadata fields, build-tree artifact locations, and FITS `.ncrc` autoload; run existing tests with visualization disabled.

**Build System Integration:** Top-level `CMakeLists.txt` and `configure.ac`; conditional additions in `examples/CMakeLists.txt` and `examples/Makefile.am`; new `examples/viz/CMakeLists.txt` and `examples/viz/Makefile.am`; equivalent dependency checks, runtime environment, and test selection in both systems.

**Definition of Done:** Both build systems provide optional visualization infrastructure, Python dependencies are validated, the common helper produces a build-tree PNG and metadata file, FITS opens through Python `netCDF4.Dataset` and `.ncrc` autoload, visualization tests pass, CDF remains deferred, and the sprint documentation is complete.

**GitHub Issue:** #312

#### Sprint 2: FITS and CDF Examples
**Detailed Plan**: See `docs/plan/v2.8.0-sprint2-fits-cdf-examples.md`

Create the first two format-specific Python visualization examples for NEP: a grey-scale FITS image plot and a CDF 1-D time-series line plot. Both scripts use `netCDF4.Dataset` directly and write a black-and-white PNG plus a companion `_metadata.txt` through `plot_common.py`.

**Implementation scope:**
- `plot_fits_image.py`: open `test/data/WFPC2u5780205r_c0fx.fits`, read `image[0, :, :]`, write `fits_wfpc2_image.png` + `_metadata.txt`.
- `plot_cdf_var.py`: open `test/data/tst_cdf_simple.cdf`, read `temperature`, write `cdf_temperature.png` + `_metadata.txt`.
- Fix CDF UDF registration in `src/cdfdispatch.c` by adding the `nc_def_user_format()` call inside `NC_CDF_initialize()`, mirroring `NC_FITS_initialize()` and `NC_PDS4_initialize()`.
- Update `.ncrc-cdf.in` with the CDF magic line (hex-escaped or in the form NetCDF-C supports). If NetCDF-C cannot parse the binary magic from `.ncrc`, the explicit `nc_def_user_format()` call still registers the handler after `libnccdf` is loaded.
- Copy `test/tst_cdf_simple.cdf` to `test/data/tst_cdf_simple.cdf` and wire both build systems to copy it to the build tree.
- Add the two scripts as `viz_*` test targets in `examples/viz/CMakeLists.txt` and `Makefile.am`, conditionally on `HAVE_FITS` and `HAVE_CDF`.

**Clarified decisions:**
- FITS `image` is `NC_FLOAT` and 200 x 200; use `imshow` with the existing gray colormap.
- CDF `temperature` is a 1-D `NC_FLOAT` variable with 10 values (20.0–24.5); plot it against array index with a simple black line.
- CDF registration is fixed now; no temporary NetCDF-4 conversion fallback is used.
- The new scripts are black-and-white, keep captions outside the PNG, and stay within the 8 x 6.1 inch size limit.
- Existing visualization build options (`NEP_ENABLE_VIZ_EXAMPLES` / `--enable-viz-examples`) remain unchanged.

**Acceptance Criteria:**
- `NC_CDF_initialize()` registers the CDF dispatch table and ignores an already-registered `NC_EINVAL`.
- `.ncrc-cdf.in` contains the CDF magic reference.
- `test/data/tst_cdf_simple.cdf` reaches the build tree for out-of-tree builds.
- `plot_fits_image.py` and `plot_cdf_var.py` each produce a PNG and a `_metadata.txt`.
- `ctest -R viz` and `make check` run the two new tests successfully.
- PNGs are black and white, size-compliant, and have companion metadata files.
- No CSV intermediates or C extractor programs are introduced.

**Testing:** Run `plot_fits_image.py` and `plot_cdf_var.py` with `NETCDF_RC` set; visually inspect output PNGs and metadata files. Run `ctest -R viz --output-on-failure` and the Autotools visualization subset. Verify disabled-by-default builds remain unchanged.

**Build System Integration:** Extend `examples/viz/CMakeLists.txt` and `examples/viz/Makefile.am` to stage `plot_fits_image.py` and `plot_cdf_var.py` and register `viz_fits_image` and `viz_cdf_temperature` tests with the existing `NCRCENV_RC`, `NETCDF_RC`, `LD_LIBRARY_PATH`, and `MPLCONFIGDIR` environment. Update `test/CMakeLists.txt` and `test/Makefile.am` to copy the CDF test data to the build tree.

**Definition of Done:** FITS and CDF example PNGs are generated automatically, both pass in CI, CDF UDF registration is fixed, the CDF test data is available in `test/data/`, documentation is updated, and the sprint issue is linked.

**GitHub Issue:** #314

#### Sprint 3: GeoTIFF and GRIB2 Examples
**Detailed Plan**: See `docs/plan/v2.8.0-sprint3-geotiff-grib2-examples.md`

Add GeoTIFF and GRIB2 Python visualization examples that open existing test files directly through `netCDF4.Dataset` and the NEP UDF autoload configuration. Each example produces a black-and-white PNG and companion `_metadata.txt` in the build tree without CSV intermediates or extractor programs.

**Implementation scope:**
- Create `plot_geotiff_subset.py` to open `test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif`, read the root `data` raster, dynamically downsample the full grid to at most approximately 1,000 samples per axis, and write `geotiff_modis_flood.png` plus metadata.
- Use GeoTIFF `lon` and `lat` coordinate variables for the image extent when available; fall back to pixel indices when coordinate metadata is unavailable.
- Create `plot_grib2_grid.py` to open `test/data/gdaswave.t00z.wcoast.0p16.f000.grib2`, select the known `WIND` variable, preserve the shared `[y, x]` orientation, and write `grib2_wave.png` plus metadata.
- Respect masks returned by `netCDF4`, additionally mask values equal to the GRIB2 `_FillValue`, and fail clearly if no valid grid cells remain.
- Stage both scripts and required GeoTIFF/GRIB2 data for CMake and Autotools out-of-tree builds, then register conditional visualization tests under `HAVE_GEOTIFF` and `HAVE_GRIB2`.
- Reuse the existing visualization runtime environment and `plot_common.py`; keep generated artifacts in the build tree and leave user-facing visualization documentation for Sprint 6.

**Clarified decisions:**
- The GeoTIFF plot shows the full raster rather than a fixed central subset; a stride computed from the raster dimensions limits each plotted axis to approximately 1,000 samples.
- GeoTIFF axes use `lon`/`lat` coordinates when present and pixel indices otherwise.
- The GRIB2 example selects `WIND` explicitly rather than relying on variable order or scanning for an arbitrary populated product.
- GRIB2 missing data remains masked; fill values are not replaced with zero or plotted as valid observations.
- Scripts reject missing variables, unsupported dimensionality, and all-masked data with clear errors.
- The planned `h00v02` MODIS tile contains only fill value 255, so the visualization uses the companion populated `h00v03` tile and retains the all-masked-data rejection requirement.
- GeoTIFF standard format inquiry reports `NC_FORMAT_NETCDF4`, matching the existing FITS and GRIB2 handlers so Python `netCDF4.Dataset` accepts the UDF; extended inquiry continues to identify UDF1.
- Sprint 3 documentation changes are limited to the roadmap, detailed plan, and planning record; user-facing visualization documentation remains in Sprint 6.

**Acceptance Criteria:**
- Both scripts open their source files through `netCDF4.Dataset` with build-tree `.ncrc` autoload and no explicit NEP initializer call.
- `plot_geotiff_subset.py` produces a size-compliant black-and-white `geotiff_modis_flood.png` and metadata file from the downsampled full raster, using geographic axes when available.
- `plot_grib2_grid.py` produces a size-compliant black-and-white `grib2_wave.png` and metadata file from valid `WIND` cells with `_FillValue` cells masked.
- CMake and Autotools stage the scripts and input data and register tests only when the corresponding reader is enabled.
- `ctest -R viz` and the Autotools visualization test subset pass in out-of-tree builds with GeoTIFF and GRIB2 enabled.
- Existing FITS, CDF, helper, and UDF-open visualization tests continue to pass; visualization remains disabled by default.
- No CSV intermediates, extractor programs, source-tree output artifacts, or GeoTIFF raster/metadata behavior changes are introduced beyond the format-inquiry compatibility correction.

**Testing:** Run separate and combined out-of-tree CMake and Autotools configurations with visualization plus GeoTIFF and/or GRIB2 enabled; run the visualization test subsets; verify source data autoload, variable shapes, downsampling bounds, coordinate fallback, masked fill handling, nonempty valid data, PNG size and grayscale presentation, metadata fields, and build-tree artifact locations; run existing visualization tests for regression coverage.

**Build System Integration:** Extend `examples/viz/CMakeLists.txt` and `examples/viz/Makefile.am` to stage both scripts, make the required input files available under `build/test/data`, pass the existing `NCRCENV_RC`, `NETCDF_RC`, `LD_LIBRARY_PATH`, and `MPLCONFIGDIR` environment, and conditionally register `viz_geotiff_modis_flood` and `viz_grib2_wave` tests under `HAVE_GEOTIFF` and `HAVE_GRIB2`.

**Definition of Done:** GeoTIFF and GRIB2 files open through Python and NEP UDF autoload, both scripts generate compliant build-tree PNG and metadata artifacts with approved downsampling, coordinate, product-selection, and missing-data behavior, both build systems provide equivalent conditional tests and data staging, regressions pass, and sprint planning documentation is complete.

**GitHub Issue:** #316

#### Sprint 4: PDS4 Synthetic Examples
**Detailed Plan**: See `docs/viz_plan.md`, Sprint 4.

Add synthetic PDS4 examples that navigate groups and read a small image array and a binary table through `netCDF4`.

**Implementation scope:**
- `plot_pds4_image.py`: open `test/data/PDS4/general/test_image.xml`, enter group `test_image.img`, read the 4 x 4 `data` variable, write `pds4_test_image.png` + `_metadata.txt`.
- `plot_pds4_table.py`: open `test/data/PDS4/general/test_table_binary.xml`, enter group `test_table_binary.dat`, read `Timestamp` and `Temperature`, write `pds4_table_binary.png` + `_metadata.txt`.

**Clarified decisions:**
- Group navigation is done through `netCDF4` group objects.
- PDS4 synthetic image uses annotated pixel values; table uses a scatter plot.

**Acceptance Criteria:**
- Synthetic PDS4 image and table PNGs are generated.
- PDS4 group navigation and variable slicing through `netCDF4` are solid.

**Testing:** Run the two scripts and verify group paths, variable shapes, and output PNGs.

**Build System Integration:** Add scripts to `examples/viz/CMakeLists.txt` and `Makefile.am`.

**Definition of Done:** Synthetic PDS4 image and table examples produce PNG + metadata and pass in CI.

**GitHub Issue:** TBD

#### Sprint 5: PDS4 Mission Examples
**Detailed Plan**: See `docs/viz_plan.md`, Sprint 5.

Create example plots for four PDS4 mission products: MESSENGER TN map, Perseverance Mastcam-Z, MAVEN NGIMS L1B, and New Horizons Alice.

**Implementation scope:**
- `plot_pds4_messenger.py`: MESSENGER thermal neutron map (`NC_UBYTE` 360 x 720).
- `plot_pds4_perseverance.py`: Mastcam-Z 3-D image, band 0, apply `scaling_factor` string, downsample, plot radiance.
- `plot_pds4_maven_l1b.py`: MAVEN NGIMS L1B `TIME` vs one other 1-D numeric column.
- `plot_pds4_new_horizons.py`: New Horizons Alice 2-D spectrum or table column.

**Clarified decisions:**
- `scaling_factor` is parsed from the string attribute at runtime.
- Large arrays are downsampled with NumPy slicing before plotting.
- MAVEN L1B has 324 columns; the plotted column is chosen during this sprint.

**Acceptance Criteria:**
- All four mission PNGs are generated.
- Scripts handle group navigation, `NC_SHORT` scaling, and large-array downsampling.

**Testing:** Run all four mission scripts and inspect output PNGs and `_metadata.txt` files.

**Build System Integration:** Add the four scripts to `examples/viz/CMakeLists.txt` and `Makefile.am`.

**Definition of Done:** All PDS4 mission examples produce PNG + metadata and pass in CI.

**GitHub Issue:** TBD

#### Sprint 6: Documentation, CI, and Polish
**Detailed Plan**: See `docs/viz_plan.md`, Sprint 6.

Document the visualization examples, wire them into CI, verify metadata files, and add module-level docstrings.

**Implementation scope:**
- Update `docs/examples.md` with a "Visualization Examples" section.
- Update `examples/viz/README.md` with run instructions.
- Add `python3-netcdf4` and `python3-matplotlib` to the relevant CI workflow.
- Run viz examples as a CI step when `NEP_ENABLE_VIZ_EXAMPLES=ON`.
- Verify every generated PNG has a companion `_metadata.txt`.
- Add module-level docstrings to all Python scripts.

**Clarified decisions:**
- CI `netCDF4` must be built against the same NetCDF-C as NEP; avoid PyPI wheels.
- Plots remain black and white, and captions remain in `_metadata.txt`.

**Acceptance Criteria:**
- Docs updated.
- CI installs `netCDF4` and Matplotlib and generates all PNGs.
- Every PNG has a `_metadata.txt`; no captions inside PNGs.

**Testing:** Run the full viz CI job and visually spot-check generated PNGs and metadata files.

**Build System Integration:** Update `.github/workflows/ci-*.yml` or add `ci-viz.yml`.

**Definition of Done:** Documentation, CI, and metadata verification are in place; all visualization examples run end-to-end.

**GitHub Issue:** TBD

### V2.7.1 - Tidying Up
#### Sprint 1: Fix Docs
**Detailed Plan**: See `docs/plan/v2.7.1-sprint1-fix-docs.md`

This documentation sprint reorganizes format-specific content and removes duplicated PDS4 test details from the README.

**Implementation scope:**
- Remove the `### PDS4 Tests` section and its table from `README.md`.
- Create dedicated format pages (`docs/cdf.md`, `docs/geotiff.md`, `docs/grib2.md`, `docs/fits.md`) from the detailed content currently in `docs/formats.md`; keep the existing `docs/pds4.md` and `docs/compression.md` unchanged except as noted below.
- Rewrite `docs/formats.md` as an overview page that links to each per-format page.
- Add the detailed PDS4 test-function inventory from the old README section to a new `PDS4 Tests` section in `docs/pds4.md`.
- Verify that New Horizons appears in the `docs/pds4.md` tested-missions coverage.
- Make minimal PDS4 updates to `docs/design.md`, `docs/prd.md`, and `docs/prfaq.md`: add New Horizons to tested-mission references and update version/date metadata to v2.7.1.

**Clarified decisions:**
- New per-format pages are created only for CDF, GeoTIFF, GRIB2, and FITS (the readers whose details currently live in `docs/formats.md`). PDS4 already has a dedicated page; compression docs remain separate.
- The README `PDS4 Tests` table is removed entirely; its contents move to `docs/pds4.md`.
- Updates to `design.md`, `prd.md`, and `prfaq.md` are minimal: verify existing PDS4 capability text, add New Horizons where missing, and refresh version/date footers. No source code or build system changes are in scope.

**Acceptance Criteria:**
- `README.md` no longer contains a `PDS4 Tests` section.
- `docs/formats.md` contains only introductory text and links to per-format pages.
- `docs/cdf.md`, `docs/geotiff.md`, `docs/grib2.md`, and `docs/fits.md` exist and contain the detailed content previously in `docs/formats.md`.
- `docs/pds4.md` contains a `PDS4 Tests` section with the full test-function inventory from the old README section.
- New Horizons appears in `docs/pds4.md` tested-mission coverage.
- `docs/design.md`, `docs/prd.md`, and `docs/prfaq.md` mention New Horizons in PDS4 context and show v2.7.1 version/date metadata.
- No broken internal documentation links.

**Testing:** No code changes. Verify all internal markdown links and rendered formatting.

**Build System Integration:** None.

**Definition of Done:** README PDS4 Tests section removed, per-format pages created and linked from `docs/formats.md`, `docs/pds4.md` contains the full PDS4 test inventory including New Horizons, and design/prd/prfaq files are versioned for v2.7.1 with current PDS4/New Horizons references.

**GitHub Issue:** #301

#### Sprint 2: Move Some PDS4 Test Files
**Detailed Plan**: See `docs/plan/v2.7.1-sprint2-move-pds4-test-files.md`

Reorganize the PDS4 test-data tree so mission-specific and generic test files live in clearly named subdirectories, and update the build systems and tests to find the files in their new locations.

**Implementation scope:**
- Move all `mvn_*` files from `test/data/PDS4/` into `test/data/PDS4/maven/`.
- Move all `ZLF_*` files from `test/data/PDS4/` into `test/data/PDS4/perseverance/`.
- Move all `Table_*` and `test_*` files from `test/data/PDS4/` into `test/data/PDS4/general/`.
- Leave existing subdirectories `cassini_hrd/`, `lcs_9p/`, `messenger_tnmap/`, and `new_horizons/` unchanged.
- Update `test/CMakeLists.txt` copy rules for the new source and destination paths.
- Update `test/Makefile.am` `check-local` copy commands and `EXTRA_DIST` paths.
- Update the affected `#define PDS4_*_FILE` path constants in `test/tst_pds4_udf.c`.
- Add a brief directory-layout note to `docs/pds4.md`.

**Clarified decisions:**
- New directories are named exactly `maven/`, `perseverance/`, and `general/` to match the roadmap wording.
- Existing mission subdirectories (`cassini_hrd/`, `lcs_9p/`, `messenger_tnmap/`, `new_horizons/`) are out of scope and remain in place.
- All Maven files (NGIMS and IUVS) share a flat `maven/` directory; no per-instrument split.
- The general directory receives both `Table_Character_Example.*` and the `test_image.*` / `test_table_binary.*` files.
- No PDS4 reader or source-code changes are in scope; this is a build/test data reorganization only.

**Acceptance Criteria:**
- All listed files exist only in their new subdirectory locations in the source tree.
- CMake and Autotools out-of-tree builds copy all moved PDS4 files to the correct build-tree subdirectories.
- `test/tst_pds4_udf.c` opens every moved label using the updated path constants.
- `tst_pds4_udf` passes with PDS4 enabled under CMake and Autotools.
- `docs/pds4.md` mentions the new `maven/`, `perseverance/`, and `general/` layout.
- No source code or reader behavior changes are introduced.

**Testing:** Run `tst_pds4_udf` with PDS4 enabled under CMake and Autotools out-of-tree builds.

**Build System Integration:** `test/CMakeLists.txt` and `test/Makefile.am`; no new dependencies or configuration options.

**Definition of Done:** Files reorganized, build systems and tests updated, all PDS4 tests pass, and documentation reflects the new layout.

**GitHub Issue:** #303

#### Sprint 3: Add Conda
**Detailed Plan**: See `docs/plan/v2.7.1-sprint3-add-conda.md`

Create a conda-build `meta.yaml` recipe so NEP can be installed via `conda install`. The recipe targets conda-forge conventions and enables all format readers whose dependencies are available on conda-forge. A CI workflow validates the recipe on every PR.

**Implementation scope:**
- Create `conda/meta.yaml` using conda-build format, targeting conda-forge conventions.
- Enable LZ4, BZIP2, Fortran wrappers, and three format readers: GeoTIFF, FITS, PDS4. CDF and GRIB2 are excluded because their dependencies (NASA CDF library, NCEPLIBS-g2c) are not packaged on conda-forge.
- Build with CMake, mirroring the Spack recipe's `cmake_args()` option mapping.
- Host dependencies: `hdf5`, `libnetcdf >=4.10.1`, `netcdf-fortran`, `lz4-c`, `bzip2`, `geotiff`, `libtiff`, `cfitsio`, `libxml2`.
- Run dependencies: `hdf5`, `netcdf-c`, `netcdf-fortran`.
- Add `conda/build.sh` build script.
- Add `ci-conda.yml` GitHub Actions workflow: install Miniconda, run `conda build conda/` on PRs and pushes to main.
- Add a `### Conda Installation` section to `README.md` after the Spack section.
- Update `docs/prd.md` to mention Conda packaging capability.

**Clarified decisions:**
- Recipe uses `meta.yaml` (conda-build) format for conda-forge compatibility; `recipe.yaml` (rattler-build) is not yet required by conda-forge.
- All readers except CDF and GRIB2 are enabled by default. CDF omitted because no conda-forge package exists for the NASA CDF library; GRIB2 omitted because NCEPLIBS-g2c is not on conda-forge.
- Fortran wrappers are included (requires `netcdf-fortran` and `gfortran`, both available on conda-forge).
- This sprint creates the in-repo recipe and CI only. Upstream submission to `conda-forge/staged-recipes` is a separate follow-up task.
- The CI workflow uses `conda-build` and validates that the package builds; it does not publish to any channel.

**Acceptance Criteria:**
- `conda/meta.yaml` exists and is valid conda-build syntax.
- `conda/build.sh` configures NEP with CMake and the correct option mapping.
- `conda build conda/` succeeds on Ubuntu with all enabled readers.
- `.github/workflows/ci-conda.yml` runs on PRs and pushes to main.
- `README.md` contains Conda installation instructions.
- `docs/prd.md` mentions Conda packaging.
- No existing tests or build configurations are broken.

**Testing:** CI workflow runs `conda build conda/` end-to-end. The built package's tests (CTest) execute as part of the conda build.

**Build System Integration:** No changes to CMake or Autotools. The `conda/build.sh` script invokes CMake directly.

**Definition of Done:** In-repo `conda/meta.yaml` and `conda/build.sh` created, CI workflow validates the recipe, README and PRD updated with Conda instructions.

**GitHub Issue:** #305

### V2.7.0 - More PDS4 Testing - New Horizons Data
- In this release we will test on a bunch of data from the New Horizons mission.
"You cannot swim for new horizons until you have courage to lose sight of the shore." — William Faulkner

#### Sprint 1: New Horizons Alice PDS4 Product
**Detailed Plan**: See `docs/plan/v2.7.0-sprint1-new-horizons-pds4.md`

Test `test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx`, a New Horizons Alice PDS4 `Product_Observational` label paired with the FITS container `ali_0030420276_0x4b0_sci_1.fit`. The label contains four `Array_2D_Spectrum` objects and two `Table_Binary` objects.

**Implementation scope:**
- Extend the PDS4 file-area dispatch so `Array_2D_Spectrum` is handled by the existing generic N-axis array reader.
- Retain the PDS4 label as the authority for the NetCDF model and read the FITS payload through the existing PDS4 byte-offset and byte-order-conversion path. FITS-UDF delegation is out of scope.
- Add the `.lblx` / `.fit` pair to the CMake and Autotools out-of-tree test-data copy and distribution rules.
- Add a New Horizons mission test to `test/tst_pds4_udf.c` that checks the file-area group, representative array/table metadata, one array hyperslab read, and one binary-table-field read.
- Audit `tst_pds4_udf.c` so every PDS4 label used by the executable has at least one successful `nc_get_vara_*()` assertion.
- Update `docs/pds4.md` for `Array_2D_Spectrum` and the New Horizons test product; update `docs/formats.md` mission-test inventory.

**Clarified decisions:**
- `.lblx` is accepted as a PDS4 XML label through the existing XML magic and namespace validation; no extension-specific format detection is needed.
- `Array_2D_Spectrum` reuses generic PDS4-array handling rather than creating a specialized reader.
- PDS4 continues to access the FITS container as a byte-addressable payload described by label offsets; no CFITSIO dependency or PDS4-to-FITS-UDF delegation is added.
- Every PDS4 label tested by `tst_pds4_udf` must have at least one data-read assertion.
- The New Horizons regression covers both relevant structure classes: at least one `Array_2D_Spectrum` hyperslab and one `Table_Binary` field read.

**Acceptance Criteria:**
- `nc_open()` opens the `.lblx` label and exposes the `ali_0030420276_0x4b0_sci_1.fit` file-area group.
- `Array_2D_Spectrum` objects have label-defined dimensions, native type, units, raw scaling attributes when present, and successful hyperslab reads.
- A representative `Table_Binary` field reads successfully.
- The `.lblx` / `.fit` pair is available in CMake and Autotools out-of-tree builds and source distributions.
- All PDS4 labels exercised by `tst_pds4_udf` have a successful data-read assertion.
- Existing PDS4 tests continue to pass.

**Testing:** Run `tst_pds4_udf` with PDS4 enabled under CMake and Autotools; run the existing PDS4 CI configuration.

**Build System Integration:** `test/CMakeLists.txt` and `test/Makefile.am`; no new dependencies or configuration options.

**Definition of Done:** Generic `Array_2D_Spectrum` support, New Horizons metadata and data-read coverage, the per-label data-read invariant, build-system data staging, documentation updates, and passing PDS4 tests are in place.

**GitHub Issue:** #293

#### Sprint 2: Build Test around: ali_0400644769_0x4b2_sci.lblx
**Detailed Plan**: See `docs/plan/v2.7.0-sprint2-new-horizons-ali-0400644769.md`

Test `test/data/PDS4/new_horizons/ali_0400644769_0x4b2_sci.lblx`, a New Horizons Alice PDS4 `Product_Observational` label paired with the FITS container `ali_0400644769_0x4b2_sci.fit`. The label contains three `Array_2D_Spectrum` objects, one `Array_1D` object, and one `Table_Binary` object with 102 records and 117 fields.

**Implementation scope:**
- Confirm the existing generic PDS4 array reader and flat `Table_Binary` reader handle this label; apply minimal reader fixes if the build exposes a gap.
- Add the `.lblx` / `.fit` pair to the CMake and Autotools out-of-tree test-data copy and distribution rules.
- Add `test_mission_new_horizons_0400644769_metadata()` and `test_mission_new_horizons_0400644769_data()` to `test/tst_pds4_udf.c`, following the Maven/Perseverance split-function pattern.
- Metadata test: verify the file-area group, the three `Array_2D_Spectrum` variables (NC_FLOAT, dims `[32, 1024]`, units, raw scaling attributes), the `Array_1D` variable (NC_INT, dim 64), and a representative `Table_Binary` field.
- Data test: read `Observational Data[0,0:4]`, `Pulse Height Distribution (PHD)[0]`, and `SAFETY_ACTIVE[0]`; assert successful reads and label-consistent values.
- Update `docs/pds4.md` for the second New Horizons product and `docs/formats.md` mission-test inventory.

**Clarified decisions:**
- Reader fixes are allowed if the build reveals a gap, but the starting assumption is that the existing generic array and flat `Table_Binary` readers are sufficient.
- Test functions are split into `_metadata()` and `_data()` following the Maven/Perseverance pattern.
- Every structure class in this label gets a data-read assertion: one `Array_2D_Spectrum` hyperslab, the `Array_1D` element, and one `Table_Binary` field.
- Specific data reads: `Observational Data[0,0:4]`, `Pulse Height Distribution (PHD)[0]`, and `SAFETY_ACTIVE[0]`.
- Copy rules are per-file in CMake/Autotools to match the existing explicit pattern.
- Documentation updates for `docs/pds4.md` and `docs/formats.md` are in scope.

**Acceptance Criteria:**
- `nc_open()` opens the `.lblx` label and exposes the `ali_0400644769_0x4b2_sci.fit` file-area group.
- `Array_2D_Spectrum` objects have label-defined dimensions, native type, units, raw scaling attributes when present, and successful hyperslab reads.
- The `Array_1D` object has label-defined dimensions, native type, units, and a successful element read.
- A representative `Table_Binary` field reads successfully.
- The `.lblx` / `.fit` pair is available in CMake and Autotools out-of-tree builds and source distributions.
- Existing PDS4 tests continue to pass.

**Testing:** Run `tst_pds4_udf` with PDS4 enabled under CMake and Autotools; run the existing PDS4 CI configuration.

**Build System Integration:** `test/CMakeLists.txt` and `test/Makefile.am`; no new dependencies or configuration options.

**Definition of Done:** Reader verified/fixed as needed, New Horizons metadata and data-read coverage for all structure classes, build-system data staging, documentation updates, and passing PDS4 tests are in place.

**GitHub Issue:** #295

#### Sprint 3: Build Test around: ali_0284461348_0x4b2_eng.lblx
**Detailed Plan**: See `docs/plan/v2.7.0-sprint3-new-horizons-ali-0284461348.md`

Test `test/data/PDS4/new_horizons/ali_0284461348_0x4b2_eng.lblx`, a New Horizons Alice PDS4 `Product_Observational` label paired with the FITS container `ali_0284461348_0x4b2_eng.fit`. The label contains one `Array_2D_Spectrum` object, one `Array_1D` object, and one `Table_Binary` object with 31 records and 117 fields.

**Implementation scope:**
- Confirm the existing generic PDS4 array reader and flat `Table_Binary` reader handle this label; apply minimal reader fixes if the build exposes a gap.
- Add the `.lblx` / `.fit` pair to the CMake and Autotools out-of-tree test-data copy and distribution rules.
- Add `test_mission_new_horizons_0284461348_metadata()` and `test_mission_new_horizons_0284461348_data()` to `test/tst_pds4_udf.c`, following the Sprint 2 split-function pattern.
- Metadata test: verify the file-area group, the `Array_2D_Spectrum` variable `Observational Data` (`NC_INT`, dims `[32, 1024]`, units `"COUNT"`, raw `scaling_factor`/`value_offset` attributes), the `Array_1D` variable `Pulse Height Distribution (PHD) Array` (`NC_INT`, dim `64`), and a representative `Table_Binary` field such as `SAFETY_ACTIVE` (`NC_UBYTE`, `ndims=1`).
- Data test: read `Observational Data[0,0:4]`, `Pulse Height Distribution (PHD) Array[0]`, and `SAFETY_ACTIVE[0]`; assert successful reads and label-consistent value invariants.
- Update `docs/pds4.md` for the third New Horizons product and `docs/formats.md` mission-test inventory.

**Clarified decisions:**
- Reader fixes are allowed if the build reveals a gap, but the starting assumption is that the existing generic array and flat `Table_Binary` readers are sufficient.
- Test functions are split into `_metadata()` and `_data()` following the Sprint 2 / Maven/Perseverance pattern.
- `Observational Data` is `SignedMSB4` → `NC_INT`; `Pulse Height Distribution (PHD) Array` is `SignedMSB4` → `NC_INT`.
- The `Array_1D` variable name is asserted exactly as it appears in the label: `Pulse Height Distribution (PHD) Array`.
- Every structure class in this label gets a data-read assertion: one `Array_2D_Spectrum` hyperslab, the `Array_1D` element, and one `Table_Binary` field.
- Specific data reads: `Observational Data[0,0:4]`, `Pulse Height Distribution (PHD) Array[0]`, and `SAFETY_ACTIVE[0]`.
- Data-read assertions verify `NC_NOERR` plus lightweight invariants; exact known values are not required.
- The per-label data-read invariant is enforced: every PDS4 label used by `tst_pds4_udf` must have at least one successful data-read assertion.
- FITS `Header` elements remain outside the NetCDF model and are not exposed as variables or attributes.
- Copy rules are per-file in CMake/Autotools to match the existing explicit pattern.
- Documentation updates for `docs/pds4.md` and `docs/formats.md` are in scope.

**Acceptance Criteria:**
- `nc_open()` opens the `.lblx` label and exposes the `ali_0284461348_0x4b2_eng.fit` file-area group.
- `Array_2D_Spectrum` object has label-defined dimensions, native type (`NC_INT`), units, and raw scaling attributes when present.
- `Array_1D` object has label-defined dimension, native type, and units.
- A representative `Table_Binary` field reads successfully.
- The `.lblx` / `.fit` pair is available in CMake and Autotools out-of-tree builds and source distributions.
- Every PDS4 label exercised by `tst_pds4_udf` has at least one successful data-read assertion.
- Existing PDS4 tests continue to pass.

**Testing:** Run `tst_pds4_udf` with PDS4 enabled under CMake and Autotools; run the existing PDS4 CI configuration.

**Build System Integration:** `test/CMakeLists.txt` and `test/Makefile.am`; no new dependencies or configuration options.

**Definition of Done:** Reader verified/fixed as needed, New Horizons metadata and data-read coverage for all structure classes, build-system data staging, the per-label data-read invariant maintained, documentation updates, and passing PDS4 tests are in place.

**GitHub Issue:** #297

#### Sprint 4: Build Test around: ali_0002845457_0x4b2_sci_1.lblx
**Detailed Plan**: See `docs/plan/v2.7.0-sprint4-new-horizons-ali-0002845457.md`

Test `test/data/PDS4/new_horizons/ali_0002845457_0x4b2_sci_1.lblx`, a New Horizons Alice PDS4 `Product_Observational` label paired with the FITS container `ali_0002845457_0x4b2_sci_1.fit`. The label contains five `Array_2D_Spectrum` objects (`Observational Data`, `Error Image`, `Wavelength Image`, `Pulse Height Distribution (PHD) Image`, `Count Rate Image`) and one `Table_Binary` object (`Housekeeping (HK) Table`) with 21 records and 117 fields.

**Implementation scope:**
- Confirm the existing generic PDS4 array reader and flat `Table_Binary` reader handle this label; apply minimal reader fixes if the build exposes a gap.
- Add the `.lblx` / `.fit` pair to the CMake and Autotools out-of-tree test-data copy and distribution rules.
- Add `test_mission_new_horizons_0002845457_metadata()` and `test_mission_new_horizons_0002845457_data()` to `test/tst_pds4_udf.c`, following the Sprint 3 split-function pattern.
- Metadata test: verify the file-area group, all five `Array_2D_Spectrum` variables (`NC_FLOAT` for `IEEE754MSBSingle`, `NC_INT` for `SignedMSB4`), dimensions, units, and raw `scaling_factor`/`value_offset` attributes where present, plus representative `Table_Binary` fields `MET` (`NC_INT`) and `SAFETY_ACTIVE` (`NC_UBYTE`).
- Data test: read representative hyperslabs/elements from every array (`Observational Data[0,0:4]`, `Error Image[0,0:4]`, `Wavelength Image[0,0:4]`, `Pulse Height Distribution (PHD) Image[0,0:4]`, `Count Rate Image[0,0:4]`) and from `MET[0]` and `SAFETY_ACTIVE[0]`; assert successful reads and label-consistent value invariants.
- Update `docs/pds4.md` for the fourth New Horizons product and `docs/formats.md` mission-test inventory.

**Clarified decisions:**
- Reader fixes are allowed if the build reveals a gap, but the starting assumption is that the existing generic array and flat `Table_Binary` readers are sufficient.
- Test functions are split into `_metadata()` and `_data()` following the Sprint 3 pattern.
- All five `Array_2D_Spectrum` objects are asserted in metadata; data reads cover every array plus two representative table fields to maintain the per-label data-read invariant.
- `IEEE754MSBSingle` → `NC_FLOAT`; `SignedMSB4` → `NC_INT`; `UnsignedByte` → `NC_UBYTE`.
- `Pulse Height Distribution (PHD) Image` is asserted as a 2D `NC_INT` variable with dims `[1, 64]` because the label declares `Array_2D_Spectrum`.
- Variable names are asserted exactly as they appear in the label, including `Pulse Height Distribution (PHD) Image`.
- Data-read assertions verify `NC_NOERR` plus lightweight invariants (finite floats, non-negative integer counts, `SAFETY_ACTIVE <= 1`); exact known values are not required.
- The per-label data-read invariant is enforced: every PDS4 label used by `tst_pds4_udf` must have at least one successful data-read assertion.
- FITS `Header` elements remain outside the NetCDF model and are not exposed as variables or attributes.
- Copy rules are per-file in CMake/Autotools to match the existing explicit pattern.
- Documentation updates for `docs/pds4.md` and `docs/formats.md` are in scope.

**Acceptance Criteria:**
- `nc_open()` opens the `.lblx` label and exposes the `ali_0002845457_0x4b2_sci_1.fit` file-area group.
- Each `Array_2D_Spectrum` object has label-defined dimensions, native NetCDF type, units, and raw scaling attributes when present.
- The `Table_Binary` `record` dimension has length 21 and representative fields read successfully.
- The `.lblx` / `.fit` pair is available in CMake and Autotools out-of-tree builds and source distributions.
- Every PDS4 label exercised by `tst_pds4_udf` has at least one successful data-read assertion.
- Existing PDS4 tests continue to pass.

**Testing:** Run `tst_pds4_udf` with PDS4 enabled under CMake and Autotools; run the existing PDS4 CI configuration.

**Build System Integration:** `test/CMakeLists.txt` and `test/Makefile.am`; no new dependencies or configuration options.

**Definition of Done:** Reader verified/fixed as needed, New Horizons metadata and data-read coverage for all structure classes, build-system data staging, the per-label data-read invariant maintained, documentation updates, and passing PDS4 tests are in place.

**GitHub Issue:** #299

### V2.6.1 - No More Worlds to Conquer
#### Sprint 1: Fix Bugs in FITS/PDS4 ncrc Initialization
- Fix bugs in UDF self-initialization of FITS/PDS4 data.

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

#### Sprint 4: More Maven Data Testing — IUVS Periapse (Nested Groups)
**Detailed Plan**: See `docs/plan/v2.6.0-sprint4-maven-periapse-testing.md`

Test `mvn_iuv_l2_periapse-orbit00124_20141021T132108.xml` — a FITS-backed PDS4 label
with 7 `Table_Binary` sections and **nested `Group_Field_Binary`** elements (groups
within groups). This is the first periapse-orbit product and exercises deeper nesting
than the corona file tested in Sprint 3.

**File structure summary:**
- `SPECIES` table: 1 scalar `ASCII_String` field, 3 records.
- `DENSITY` table: 4 outer groups (reps=19, groups=1 each), each containing 1 inner group (reps=3, groups=0) with 1 `IEEE754MSBSingle` field. → 3D variables `[12, 19, 3]`.
- `TEMPERATURE` table: 3 scalar `IEEE754MSBSingle` fields + 4 outer groups (reps=19, groups=0) with 1 field each. → scalar 1D fields + 2D group fields `[12, 19]`.
- `GEOMETRY_RETRIEVAL` table: 11 scalar `IEEE754MSBDouble` fields (no groups), 12 records. Key test table.
- `EMISSION_FEATURES` table: 2 scalar fields + 1 group (reps=256, groups=0) with 1 `IEEE754MSBDouble` field, 29 records.
- `MODEL_RADIANCE` table: 2 scalar fields + nested groups (outer reps=65 containing inner reps=29). → 3D variables.
- `GEOMETRY_RADIANCE` table: 12 groups (reps=65, groups=0), 12 records. → 2D variables `[12, 65]`.
- `OBSERVATION` table: 13 scalar fields, 1 record.

**Implementation tasks:**
1. Extend `Group_Field_Binary` parsing in `pds4_read_table()` to support depth-2 nesting:
   - Detect `<groups>` count inside a `Group_Field_Binary` element.
   - If inner groups > 0 (depth-2 case): parse the inner `Group_Field_Binary` (must have groups=0; deeper nesting returns `NC_EINVAL`).
   - For a depth-2 field, create a 3D variable `[record, outer_rep, inner_rep]`.
   - Store outer_group_loc, outer_group_length, inner_group_loc, inner_group_length, inner_field_loc in `NC_PDS4_VAR_INFO_T`.
2. Extend `pds4_get_vara()` to handle 3D group fields:
   - `start/count` has 3 indices; per-element seek = `table_offset + rec*record_len + outer_group_loc-1 + outer_rep*outer_group_len + inner_group_loc-1 + inner_rep*inner_group_len + inner_field_loc-1`.
3. Implement table-prefixed variable naming for conflict resolution:
   - When multiple tables in one `File_Area_Observational` group share a field name, prefix each field with the table's `local_identifier` (e.g., `DENSITY_ALT`, `TEMPERATURE_ALT`).
   - Detect conflicts by checking existing variable names before registration; apply prefix only to conflicting names, or apply uniformly to all fields from tables that have any conflict.
4. Add both `.xml` and `.fits` to build system (`test/CMakeLists.txt`, `test/Makefile.am`).
5. Add `test_mission_maven_periapse_metadata()` to `test/tst_pds4_udf.c`:
   - Verify `nc_open()` succeeds.
   - Navigate to file group `mvn_iuv_l2_periapse-orbit00124_20141021T132108_v13_r01.fits`.
   - Verify `GEOMETRY_RETRIEVAL` has 11 variables, all `NC_DOUBLE`, `ndims=1`, dim length=12.
   - Verify `DENSITY_ALT` has `ndims=3`, dims `[12, 19, 3]`.
   - Verify `EMISSION_FEATURES_FIT_TEMPLATE` has `ndims=2`, dims `[29, 256]`.
6. Add `test_mission_maven_periapse_data()` to `test/tst_pds4_udf.c`:
   - Read `LAT[0]` from `GEOMETRY_RETRIEVAL` (scalar field) → finite double.
   - Read `DENSITY_ALT[0,0,0]` (3D nested group field) → finite float > 0.
7. Wire both test functions into `main()` after `test_mission_maven_iuvs_data()`.

**Clarified decisions:**
- Nested `Group_Field_Binary` (depth ≤ 2) fully supported; depth-3+ nesting returns `NC_EINVAL`.
- 3D variable model for depth-2: `[record, outer_rep, inner_rep]`.
- Conflicting field names prefixed with table `local_identifier` (e.g., `DENSITY_ALT` not `ALT`).
- Both `.xml` and `.fits` copied to build directory unconditionally (CMake and Autotools).
- Two test functions (`_metadata` and `_data`) following Sprint 3 pattern.

**GitHub Issue:** #284

#### Sprint 5: More PDS4 Testing — Perseverance Mastcam-Z
**Detailed Plan**: See `docs/plan/v2.6.0-sprint5-perseverance-testing.md`

Test `ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml` — a PDS4
`Product_Observational` label from the Mars 2020 Perseverance rover's Mastcam-Z
left camera (Sol 1738). The file area contains one `Array_3D_Image`
(Band=3, Line=1200, Sample=1648, `SignedMSB2`, offset=52736 bytes) backed by
a VICAR/ODL-prefixed binary `.IMG` file.

**Implementation tasks:**
1. Add `Array_3D_Image` to the dispatch list in `pds4_read_file_area()` in `src/pds4file.c`:
   - Extend the `if` branch at the `Array_2D_Image` / `Array` check to also match
     `Array_3D_Image` (and any other `Array_*` variants: `Array_1D`, `Array_3D`,
     `Array_2D`). Pass the node to `pds4_read_array()` unchanged — the function
     already handles N-axis arrays generically via `Axis_Array` children.
2. Both `.xml` and `.IMG` files are already in `test/data/PDS4/`; add copy rules to
   `test/CMakeLists.txt` and `test/Makefile.am` (`check-local` and `EXTRA_DIST`).
3. Add `#define PDS4_PERSEVERANCE_FILE` path constant to `test/tst_pds4_udf.c`.
4. Add `test_mission_perseverance_mastcamz_metadata()` to `test/tst_pds4_udf.c`:
   - Verify `nc_open()` succeeds.
   - Navigate to the file-area group named `ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG`.
   - Verify the variable named `data` exists (no `<name>` element in label → reader default).
   - Verify it is `NC_SHORT`, `ndims=3`.
   - Verify dim lengths: dim[0]=3 (Band), dim[1]=1200 (Line), dim[2]=1648 (Sample).
   - Verify `scaling_factor` string attribute equals `"5.0e-06"` (raw label value, per answer 3b).
5. Add `test_mission_perseverance_mastcamz_data()` to `test/tst_pds4_udf.c`:
   - Read pixel `[0,0,0]` as `NC_SHORT` via `nc_get_vara_short()`.
   - Verify the call returns `NC_NOERR` (data path works with the 52736-byte offset).
   - Verify the returned value is within the valid `SignedMSB2` range (−32768–32767);
     a zero value is acceptable (instrument may report zero for fill/invalid pixels).
6. Wire both test functions into `main()` after `test_mission_maven_periapse_data()`.

**Clarified decisions:**
- `Array_3D_Image` fix: extend the existing `if`/`else if` dispatch in `pds4_read_file_area()`;
  no changes to `pds4_read_array()` itself — it already handles arbitrary N-axis arrays.
- Variable name defaults to `"data"` because `Array_3D_Image` has no `<name>` child element;
  test asserts this exact name.
- `scaling_factor` attribute verified as raw string `"5.0e-06"` (answer 3b); no CF
  `scale_factor` mapping in this sprint.
- Data read verified for pixel `[0,0,0]`; value just checked for NC_NOERR + range validity.
- Both `.xml` and `.IMG` copied to build directory unconditionally (CMake and Autotools).
- Two test functions (`_metadata` and `_data`) following Sprint 3/4 pattern.

**GitHub Issue:** #287

#### Sprint 5: More PDS4 Testing — Perseverance Mastcam-Z
**Detailed Plan**: See `docs/plan/v2.6.0-sprint5-perseverance-testing.md`

Test `ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml` — a PDS4
`Product_Observational` label from the Mars 2020 Perseverance rover's Mastcam-Z
left camera (Sol 1738). The file area contains one `Array_3D_Image`
(Band=3, Line=1200, Sample=1648, `SignedMSB2`, offset=52736 bytes) backed by
a VICAR/ODL-prefixed binary `.IMG` file.

**Implementation tasks:**
1. Add `Array_3D_Image` to the dispatch list in `pds4_read_file_area()` in `src/pds4file.c`:
   - Extend the `if` branch at the `Array_2D_Image` / `Array` check to also match
     `Array_3D_Image` (and any other `Array_*` variants: `Array_1D`, `Array_3D`,
     `Array_2D`). Pass the node to `pds4_read_array()` unchanged — the function
     already handles N-axis arrays generically via `Axis_Array` children.
2. Both `.xml` and `.IMG` files are already in `test/data/PDS4/`; add copy rules to
   `test/CMakeLists.txt` and `test/Makefile.am` (`check-local` and `EXTRA_DIST`).
3. Add `#define PDS4_PERSEVERANCE_FILE` path constant to `test/tst_pds4_udf.c`.
4. Add `test_mission_perseverance_mastcamz_metadata()` to `test/tst_pds4_udf.c`:
   - Verify `nc_open()` succeeds.
   - Navigate to the file-area group named `ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG`.
   - Verify the variable named `data` exists (no `<name>` element in label → reader default).
   - Verify it is `NC_SHORT`, `ndims=3`.
   - Verify dim lengths: dim[0]=3 (Band), dim[1]=1200 (Line), dim[2]=1648 (Sample).
   - Verify `scaling_factor` string attribute equals `"5.0e-06"` (raw label value, per answer 3b).
5. Add `test_mission_perseverance_mastcamz_data()` to `test/tst_pds4_udf.c`:
   - Read pixel `[0,0,0]` as `NC_SHORT` via `nc_get_vara_short()`.
   - Verify the call returns `NC_NOERR` (data path works with the 52736-byte offset).
   - Verify the returned value is within the valid `SignedMSB2` range (−32768–32767);
     a zero value is acceptable (instrument may report zero for fill/invalid pixels).
6. Wire both test functions into `main()` after `test_mission_maven_periapse_data()`.

**Clarified decisions:**
- `Array_3D_Image` fix: extend the existing `if`/`else if` dispatch in `pds4_read_file_area()`;
  no changes to `pds4_read_array()` itself — it already handles arbitrary N-axis arrays.
- Variable name defaults to `"data"` because `Array_3D_Image` has no `<name>` child element;
  test asserts this exact name.
- `scaling_factor` attribute verified as raw string `"5.0e-06"` (answer 3b); no CF
  `scale_factor` mapping in this sprint.
- Data read verified for pixel `[0,0,0]`; value just checked for NC_NOERR + range validity.
- Both `.xml` and `.IMG` copied to build directory unconditionally (CMake and Autotools).
- Two test functions (`_metadata` and `_data`) following Sprint 3/4 pattern.

**GitHub Issue:** #287

#### Sprint 6: More Perseverance Testing
**Detailed Plan**: See `docs/plan/v2.6.0-sprint6-more-perseverance-testing.md`

Test the second Perseverance Mastcam-Z calibrated image product
`ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.xml` (Sol 1737), which
has the same `Array_3D_Image` structure as the Sprint 5 file: Band=3, Line=1200,
Sample=1648, `SignedMSB2`, 52736-byte VICAR/ODL-prefixed `.IMG` offset, and
`scaling_factor` of `5.0e-06`.

No PDS4 reader changes are required; Sprint 5 already added `Array_3D_Image`
dispatch and `scaling_factor`/`value_offset` attribute storage.

**Implementation tasks:**
1. Add a second Perseverance file constant and two test functions to
   `test/tst_pds4_udf.c`:
   - `test_mission_perseverance_mastcamz_1737_metadata()` — open the label,
     find the file-area group, verify variable `data` is `NC_SHORT` with
     `ndims=3`, dims `[3,1200,1648]`, and `scaling_factor="5.0e-06"`.
   - `test_mission_perseverance_mastcamz_1737_data()` — read pixel `[0,0,0]`
     via `nc_get_vara_short()`, verify `NC_NOERR` and a value in
     `[-32768, 32767]` (zero allowed).
2. Wire both new functions into `main()` after the existing Sprint 5
   Perseverance calls.
3. Add `configure_file` copy rules to `test/CMakeLists.txt` for the 1737 `.xml`
   and `.IMG`.
4. Add `cp` rules to `test/Makefile.am` `check-local` and `EXTRA_DIST` entries
   for the 1737 files.

**Clarified decisions:**
- No reader changes; Sprint 5 (`#287`) is the dependency and is already merged.
- New identifiers are distinct from the Sprint 5 names so both data sets remain
  under test.
- Data verification is lightweight: `NC_NOERR` plus signed 16-bit range check.
- Build-system copy rules follow the explicit per-file pattern used for the
  1738 files.

**Acceptance Criteria:**
- `nc_open()` on the 1737 XML label returns `NC_NOERR`.
- File-area group named `ZLF_1737_0821123689_910RAD_N0830000ZCAM00091_1100LMJ01.IMG`
  exists.
- Variable `data` is `NC_SHORT`, `ndims=3`, dims `[3, 1200, 1648]`.
- `scaling_factor` string attribute equals `"5.0e-06"`.
- `nc_get_vara_short([0,0,0])` returns `NC_NOERR` and a value in `[-32768, 32767]`.
- Both `.xml` and `.IMG` are copied to the build directory under CMake and
  Autotools.
- All existing PDS4 tests continue to pass.

**Testing:** `tst_pds4_udf` built with `-DNEP_ENABLE_PDS4=ON`; existing
`ci-formats.yml` PDS4 job covers the new functions.

**Build System Integration:** `test/CMakeLists.txt`, `test/Makefile.am`,
`test/tst_pds4_udf.c`.

**Dependencies:** Sprint 5 (`#287`) merged.

**Definition of Done:** New `#define`, two test functions, `main()` wiring,
copy rules, and issue link in `docs/roadmap.md` are in place; all PDS4 tests
pass.

**GitHub Issue:** #289

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


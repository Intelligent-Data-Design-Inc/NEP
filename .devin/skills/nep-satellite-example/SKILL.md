# NEP Satellite Instrument Example Skill

## Purpose

Add a new standalone Python satellite-instrument example to NEP, following
the same pattern used for the NISAR Soil Moisture (`examples/nisar/`),
SWOT Sea Surface Height (`examples/swot/`), and GOES-R ABI CMIP
(`examples/abi/`) examples.  If the instrument's standard products are
distributed as GeoTIFF, follow the NEP GeoTIFF UDF-reader pattern used in
`examples/viz/GeoTIFF/` instead.

Each example demonstrates how to open a real Earth-science product from a
satellite mission and produce a publication-style figure.  NetCDF-4/HDF5
products are opened with `xarray`/`netCDF4`, `matplotlib`, and `cartopy`.
GeoTIFF products are opened transparently through NEP's GeoTIFF UDF reader
with the same `netCDF4.Dataset` API and plotted with `matplotlib`/`cartopy`.

**Important:** these examples must be based on real satellite data.  If a
real sample granule cannot be downloaded in this environment, stop after
producing a concise mission/spacecraft/instrument summary.  Do **not**
generate synthetic data, placeholder figures, or a full example package.

## When to Use

Use this skill when asked to add a new example for any Earth-observing
satellite mission.  Examples include ocean altimeters, atmospheric
sounders, hyperspectral imagers, scatterometers, SAR missions, optical
imagers, and lidars.

Choose the implementation path based on the mission's standard product
format:

* **NetCDF-4 / CF-1.x / HDF5** products → `examples/<MISSION_SHORT>/` pure
  Python package, no NEP UDF reader needed.
* **GeoTIFF** products → `examples/viz/GeoTIFF/` Python script that uses
  NEP's GeoTIFF UDF reader (enabled with `-DNEP_ENABLE_GEOTIFF=ON`).

## Required Inputs

Before starting, the user should provide the following (ask for any that are
missing):

| Input | Description | Example |
|---|---|---|
| `MISSION_NAME` | Full mission name | Surface Water and Ocean Topography |
| `MISSION_SHORT` | Short abbreviation for directory naming | `swot` |
| `INSTRUMENT_NAME` | Instrument name | Ka-band Radar Interferometer (KaRIn) |
| `PRODUCT_NAME` | Standard product family name | SWOT Level 2 KaRIn Low Rate Sea Surface Height |
| `PRODUCT_SHORT` | Product short name/collection | `SWOT_L2_LR_SSH_Basic_D` |
| `PRODUCT_FORMAT` | Standard product format | `NetCDF-4`, `HDF5`, `GeoTIFF` |
| `PRIMARY_VARIABLE` | Main variable/band to plot | `ssha` / `ssha_karin` / `VV` / `red`+`nir` |
| `COORDINATES` | Coordinate variables (1-D or 2-D) | 2-D `latitude`, `longitude` |
| `QUALITY_VARIABLE` | Quality/flag variable used for masking | `ssha_karin_qual` |
| `DATA_ACCESS` | How to obtain a sample file | Earthdata PO.DAAC, NOAA AWS Open Data, EUMETSAT, Copernicus, etc. |
| `FIGURE_COUNT` | Number and type of output figures | 2: map + footprint overview |

## Invocation Template

```
Add a NEP satellite example for <MISSION_NAME>.
Mission/instrument: <MISSION_NAME> / <INSTRUMENT_NAME>
Product: <PRODUCT_NAME> (<PRODUCT_SHORT>)
Product format: <PRODUCT_FORMAT>
Primary variable/band to plot: <PRIMARY_VARIABLE>
Coordinates: <COORDINATES>
Quality/flag variable: <QUALITY_VARIABLE>
Data access: <DATA_ACCESS>
Figures to produce: <FIGURE_COUNT>
```

## Steps

### 1. Study Existing Examples

Read these files to understand the established layout, CLI conventions,
plotting style, and documentation depth:

- `examples/nisar/README.md`
- `examples/nisar/nisar_example/cli.py`
- `examples/nisar/nisar_example/sme2.py`
- `examples/nisar/nisar_example/plots.py`
- `examples/nisar/nisar_example/fetch.py`
- `examples/swot/README.md`
- `examples/swot/swot_example/cli.py`
- `examples/swot/swot_example/l3_ssh.py`
- `examples/swot/swot_example/plots.py`
- `examples/swot/swot_example/fetch.py`
- `examples/abi/README.md`
- `examples/abi/abi_example/cli.py`
- `examples/abi/abi_example/reader.py`
- `examples/abi/abi_example/plots.py`
- `examples/abi/abi_example/fetch.py`
- `docs/netCDF_with_NISAR.md`
- `docs/netCDF_with_SWOT.md`
- `docs/netCDF_with_ABI_CMIP.md`
- `examples/viz/GeoTIFF/plot_geotiff_subset.py`
- `examples/viz/CMakeLists.txt`
- `examples/viz/README.md`
- `docs/geotiff.md`

Mirror the structure, naming, and argparse conventions of the most recent
example.  Do not introduce new conventions without a reason.

### 2. Plan the Sprint in the Roadmap

If the user has not already created a roadmap sprint:

1. Read `docs/roadmap.md`.
2. Add a new sprint under the current version, modeled on the NISAR/SWOT/
   ABI_CMIP sprint entries.  The sprint entry should state:
   - The standard product format (NetCDF-4, HDF5, or GeoTIFF).
   - The chosen implementation path (`examples/<MISSION_SHORT>/` or
     `examples/viz/GeoTIFF/`).
   - That a real sample file is required and that the sprint will be paused
     if one cannot be obtained; synthetic data is not allowed.
3. Create a detailed plan at
   `docs/plan/v<VERSION>-sprint<N>-<mission_short>-example.md` with goal,
   scope, tasks, acceptance criteria, testing requirements, dependencies
   (e.g. `libgeotiff`/`libtiff` and `-DNEP_ENABLE_GEOTIFF=ON` for GeoTIFF),
   and definition of done.
4. Update the version heading in `docs/roadmap.md` if necessary (e.g. add
   the new example name to the list).

### 3. Investigate Data Availability

Before writing any code or creating figures, determine the mission's
standard product format and whether a real sample granule can be obtained
from a public source in this environment.

| Native format | Example path | Required reader |
|---|---|---|
| NetCDF-4 / CF-1.x | `examples/<MISSION_SHORT>/` | None — plain netCDF-C |
| HDF5 with CF metadata | `examples/<MISSION_SHORT>/` | None — plain netCDF-C |
| GeoTIFF | `examples/viz/GeoTIFF/` | NEP GeoTIFF UDF reader (`-DNEP_ENABLE_GEOTIFF=ON`) |

Allowed sources include:

- Public NOAA AWS Open Data buckets (no credentials).
- NASA Earthdata / PO.DAAC / LP DAAC collections (requires a free login;
  attempt credential-free search/listing first, but do not block on it).
- EUMETSAT / Copernicus public catalogues (often requires authentication).
- Direct public HTTPS links to `.nc`, `.h5`, `.tif`, `.tiff`, or `.geotiff`
  files.

What counts as success:

- A real satellite granule is downloaded to a local path.
- For NetCDF/HDF5: `ncdump -h` or `xarray.open_dataset` confirms it contains
  the expected primary variable and coordinate data.
- For GeoTIFF: `gdalinfo` (or equivalent) confirms it contains the expected
  raster bands and georeferencing; and NEP is built with GeoTIFF support so
  that `nc_open()` or `netCDF4.Dataset` can read it.

What does **not** count:

- Synthetic or randomly generated data.
- Catalog entries or metadata records without an actual downloadable file.
- Zipped or packaged formats (e.g., ESA SAFE, zipped shapefiles) unless you
  can also obtain the raw GeoTIFF/NetCDF asset inside.

If you cannot obtain a real sample file, proceed to **Step 6** and stop
after producing the summary document.  Do not continue to code, figures, or
a full example package.

### 4. Create the Example (real data only)

If and only if a real sample file was obtained in Step 3, create the
example following the path chosen in the investigation table.

#### 4a. NetCDF-4 / HDF5 path

Create `examples/<MISSION_SHORT>/` with:

- `examples/<MISSION_SHORT>/<mission_short>_example/__init__.py`
- `examples/<MISSION_SHORT>/<mission_short>_example/__main__.py`
- `examples/<MISSION_SHORT>/<mission_short>_example/cli.py`
- `examples/<MISSION_SHORT>/<mission_short>_example/reader.py` (or a
  mission-specific module name such as `sme2.py`, `l3_ssh.py`)
- `examples/<MISSION_SHORT>/<mission_short>_example/plots.py`
- `examples/<MISSION_SHORT>/<mission_short>_example/fetch.py` (if remote
  data access is feasible)
- `examples/<MISSION_SHORT>/requirements.txt`
- `examples/<MISSION_SHORT>/README.md`
- `examples/<MISSION_SHORT>/.gitignore`
- `examples/<MISSION_SHORT>/figures/` (for committed example output PNGs)

Package conventions:

- The CLI accepts either a local file path or a fetch command with enough
  parameters to locate one granule.
- Use `argparse` with consistent flags: `--output-dir` (default `output/`),
  `--show` (interactive display), and fetch-specific flags such as
  `--bbox W S E N` or `--date YYYY-MM-DDTHH:MM`.
- Output PNGs are written to `--output-dir`; committed example figures live
  in `figures/`.
- `reader.py` opens the file with `xarray`/`netCDF4`, loads the primary
  variable, applies the quality/flag mask, and attaches coordinates.
- `plots.py` uses `cartopy`/`matplotlib`.  The first figure is the primary
  science map; the second is a footprint/bounds overview.
- `fetch.py` resolves credentials from `~/.netrc` or environment variables
  when Earthdata-style access is needed, or uses public HTTPS/S3 buckets
  when available.  Cache downloads in `data/` (untracked).
- `requirements.txt` lists only the Python packages the example needs.

The sample data file itself is **not** committed; `examples/<MISSION_SHORT>/
.gitignore` should ignore `data/`, `output/`, `.venv/`, `__pycache__/`, and
`*.pyc`.

#### 4b. GeoTIFF path (NEP GeoTIFF UDF reader)

Create a visualization script under `examples/viz/GeoTIFF/`, following the
existing pattern in `examples/viz/GeoTIFF/plot_geotiff_subset.py` and the
project diagram rules in `.devin/rules/diagram-rules.md`:

- `examples/viz/GeoTIFF/plot_<MISSION_SHORT>.py`
- Add the script to the `_viz_python_files` list in
  `examples/viz/CMakeLists.txt`.
- If the sample GeoTIFF is small enough to commit, add it to `test/data/`
  and configure it into the build tree under `if(HAVE_GEOTIFF)` in
  `examples/viz/CMakeLists.txt` (see the existing `MCDWD_L3_F1C_NRT...` entry
  for the exact pattern).
- If the sample is too large to commit, add a `--url` or environment-
  variable fetch path to the script and document it; do **not** commit the
  raw image.
- Add a `ctest` test under `if(HAVE_GEOTIFF)` in
  `examples/viz/CMakeLists.txt` that runs the script against the committed
  or build-copied sample.

Script conventions:

- Use `netCDF4.Dataset` to open the `.tif` file.  With NEP's GeoTIFF UDF
  reader enabled, this transparently exposes the raster as NetCDF variables
  such as `data` and 1-D coordinate variables `lon`/`lat` or `x`/`y`.
- Read the raster with a bounded subsample if the image is large.
- Plot with `matplotlib`/`cartopy`.  Keep the figure black-and-white, <= 8 in
  wide and <= 6.1 in high, no caption inside the PNG, and write a companion
  `_metadata.txt` file with `title`, `caption`, and `alt_text` using the
  shared `save_with_metadata` helper.
- The script should be runnable both from `ctest` (with the build-tree
  `.ncrc` and `LD_LIBRARY_PATH`) and manually from the configured build tree.

### 5. Generate Committed Example Figures (real data only)

For the NetCDF/HDF5 path:

1. Run the example CLI against the downloaded sample to produce PNGs.
2. Move/copy the resulting PNGs into `examples/<MISSION_SHORT>/figures/`.
3. Embed the figures in `examples/<MISSION_SHORT>/README.md`.

For the GeoTIFF path:

1. Build NEP with `-DNEP_ENABLE_GEOTIFF=ON` and `-DNEP_ENABLE_VIZ_EXAMPLES=ON`.
2. Run the new `ctest` test or execute the script manually with the proper
   `.ncrc` and `LD_LIBRARY_PATH` environment.
3. Move the produced PNG and its `_metadata.txt` into the source tree under
   `examples/viz/GeoTIFF/` only if it is meant to be a committed reference
   artifact; otherwise leave it as a build-tree test output.

### 6. Write the Summary or Companion Paper

If a real sample file was **not** obtained in Step 3:

Create a concise document at `docs/<MISSION_SHORT>_summary.md` (e.g.
`docs/landsat_oli2_summary.md`) containing only:

- Mission overview (agency, launch date, role).
- Spacecraft and orbit facts.
- Instrument description and key channels/bands.
- Target product family and why it is relevant to NEP.
- Data availability note: public sources checked, why a NetCDF sample could
  not be obtained, and what source/credential would be needed to proceed.
- References.

Do **not** create an `examples/<MISSION_SHORT>/` package, synthetic data,
figures, or a `docs/netCDF_with_...` companion paper.  Do **not** update
`README.md` or `examples/README.md` to list a non-existent example.

If a real sample file **was** obtained in Step 3:

Create `docs/netCDF_with_<MISSION>_<INSTRUMENT or PRODUCT>.md` (e.g.
`docs/netCDF_with_ABI_CMIP.md`).  It should mirror the depth of the
existing companion papers:

- Brief mission, spacecraft, orbit, and instrument overview.
- Product description and how it maps to the NetCDF-4 data model (or to the
  NEP GeoTIFF UDF mapping if using GeoTIFF).
- `ncdump -h` excerpt (NetCDF/HDF5) or `gdalinfo` / GeoTIFF metadata summary
  (GeoTIFF) showing the primary variable, coordinates, and key attributes.
- Explanation of any quality flags, fill values, or projections used.
- Python code snippet showing how the example reads the data.
- C code snippet showing the equivalent `nc_open()`/`nc_get_var()` calls.
- For GeoTIFF products, also reference `docs/geotiff.md` and explain how the
  NEP GeoTIFF reader exposes bands as NetCDF dimensions and variables.
- References section with authoritative links.

### 7. Update Cross-References (real data only)

If and only if a full example package or script was created in Step 4,
update the following files to mention the new example:

- For NetCDF/HDF5 examples: top-level `README.md` in the "Standalone Python
  examples" paragraph and `examples/README.md` in the "Python Examples"
  list.
- For GeoTIFF examples: `examples/viz/README.md` in the format/script list,
  and add a bullet to the top-level `README.md` section that describes NEP
  UDF readers or visualization examples (do not claim it is a standalone
  package in `examples/README.md`).
- `docs/roadmap.md` (if not already updated in step 2).

If only a summary document was created in Step 6, do **not** list the
example in `README.md`, `examples/README.md`, or `examples/viz/README.md`.
The summary document itself is the deliverable.

### 8. Verify End-to-End (real data only)

If a real sample file was obtained:

For NetCDF/HDF5 examples:

1. Create a virtual environment and install `requirements.txt`.
2. Run `python -m <mission_short>_example <local_file>` and confirm the
   PNGs are produced.
3. If a fetch path exists, run it with test credentials/parameters and
   confirm a sample file can be retrieved and plotted.
4. Run `python -m py_compile` on all package files.

For GeoTIFF examples:

1. Build NEP with `-DNEP_ENABLE_GEOTIFF=ON` and
   `-DNEP_ENABLE_VIZ_EXAMPLES=ON` in a `build/` directory (delete any
   existing `build/` first per project convention).
2. Run `ctest -R viz_<mission_short>` and confirm the test passes and the
   PNG artifact is produced.
3. If the script supports manual execution, run it from the build tree with
   `NCRCENV_RC`, `NETCDF_RC`, and `LD_LIBRARY_PATH` set, and confirm the
   figure is produced.

For both paths:

- Confirm that no files under `src/`, `include/`, or `.github/workflows/`
  were modified unless the sprint explicitly required library changes.
- For GeoTIFF examples, `test/data/` may gain a committed sample file and
  `examples/viz/CMakeLists.txt` will be updated; this is expected.

## Style and Conventions

- Match the most recently added satellite example (currently ABI_CMIP) for
  NetCDF/HDF5 code structure, docstring style, CLI behavior, and README
  format.
- For GeoTIFF examples, match the existing `examples/viz/GeoTIFF/` scripts,
  including the project diagram rules (black and white, <= 8 in × 6.1 in, no
  caption inside the PNG) and the `save_with_metadata` helper.
- NetCDF/HDF5 example code is self-contained; it does not depend on NEP's C
  library or UDF readers.
- GeoTIFF example code uses NEP's GeoTIFF UDF reader via the standard
  NetCDF-C/Python `netCDF4` API.
- Prefer public, credential-free data sources where possible; otherwise
  document credential setup clearly in the relevant README.
- Use descriptive variable names (`ssha`, `cmi`, `soil_moisture`, `vv`,
  `vh`, `ndvi`) rather than generic names.
- NetCDF/HDF5 examples: commit output PNGs but do not commit sample data
  files or virtual environments.
- GeoTIFF examples: small sample GeoTIFFs may be committed to `test/data/`
  and registered in `examples/viz/CMakeLists.txt`; commit the generated PNG
  and `_metadata.txt` only if they are intended as reference artifacts.
- NetCDF/HDF5 examples do not modify CMake, CTest, CI, or the NEP C/Fortran
  library unless the sprint plan explicitly requires it.
- GeoTIFF examples modify `examples/viz/CMakeLists.txt` and may add a sample
  file to `test/data/`; this is expected.
- **Do not create synthetic data or placeholder figures.**  If a real
  sample cannot be obtained, stop after producing the summary document.

## Output Checklist (NetCDF/HDF5 example, real data available)

- [ ] New `examples/<MISSION_SHORT>/` package created and functional.
- [ ] `examples/<MISSION_SHORT>/requirements.txt` installs successfully.
- [ ] `examples/<MISSION_SHORT>/README.md` documents setup, usage, fetch
      instructions, and embeds committed figures.
- [ ] `examples/<MISSION_SHORT>/figures/` contains committed example PNGs.
- [ ] `docs/netCDF_with_<MISSION>_<INSTRUMENT or PRODUCT>.md` companion
      paper created.
- [ ] Top-level `README.md` and `examples/README.md` updated.
- [ ] `docs/roadmap.md` and corresponding `docs/plan/...` file updated if
      applicable.
- [ ] Example runs end-to-end against a local or fetched sample file.
- [ ] No unintended changes to `src/`, `include/`, `test/`, or CI files.

## Output Checklist (GeoTIFF example, real data available)

- [ ] New `examples/viz/GeoTIFF/plot_<MISSION_SHORT>.py` script created.
- [ ] Script added to `_viz_python_files` in `examples/viz/CMakeLists.txt`.
- [ ] `ctest` test added under `if(HAVE_GEOTIFF)` in
      `examples/viz/CMakeLists.txt`.
- [ ] Real sample GeoTIFF committed to `test/data/` (if small enough) or
      documented fetch path provided.
- [ ] `docs/netCDF_with_<MISSION>_<INSTRUMENT or PRODUCT>.md` companion
      paper created, referencing NEP's GeoTIFF UDF reader.
- [ ] Top-level `README.md` and `examples/viz/README.md` updated.
- [ ] `docs/roadmap.md` and corresponding `docs/plan/...` file updated if
      applicable.
- [ ] `ctest -R viz_<MISSION_SHORT>` passes and produces the PNG artifact.
- [ ] No unintended changes to `src/`, `include/`, or `.github/workflows/`.

## Output Checklist (real data unavailable)

- [ ] Public data sources checked and documented.
- [ ] `docs/<MISSION_SHORT>_summary.md` created with mission, spacecraft,
      orbit, instrument, product, and data-availability notes.
- [ ] No synthetic data, placeholder figures, or example package created.
- [ ] No `README.md`, `examples/README.md`, or `examples/viz/README.md`
      entries for the non-existent example.

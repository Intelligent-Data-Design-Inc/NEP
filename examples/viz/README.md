# NEP Visualization Examples

The visualization examples use Python `netCDF4` to open NEP UDF files and
Matplotlib to write static PNG plots. They are optional and disabled by default.
The scripts cover FITS, CDF, GeoTIFF, GRIB2, PDS4 MESSENGER,
Perseverance, MAVEN, New Horizons, DICOM, legacy PDB, and PDBx/mmCIF products.
They use the generated build-tree `.ncrc` to autoload enabled NEP UDF libraries.

Format-specific scripts are organized in `CDF/`, `DICOM/`, `FITS/`,
`GeoTIFF/`, `GRIB2/`, `PDS4/`, `PDB/`, and `mmCIF/` directories. CMake mirrors this layout under
`build/examples/viz/`. Shared helpers, tests, the artifact verifier, this README,
and the CMake configuration remain at the visualization root.

## Requirements

- Python 3
- Project-local `.venv`
- Python modules `netCDF4`, `matplotlib`, and `numpy`
- At least one enabled NEP UDF reader
- Normal NEP examples enabled

Create the virtual environment in the NEP source root and install the pinned
minimum requirements:

```bash
python3 -m venv .venv
NETCDF4_DIR=/usr/local/netcdf-c \
HDF5_DIR=/usr/local/hdf5-2.1.1 \
CPPFLAGS="-I/usr/local/netcdf-c/include -I/usr/local/hdf5-2.1.1/include" \
LDFLAGS="-L/usr/local/netcdf-c/lib -L/usr/local/hdf5-2.1.1/lib" \
  .venv/bin/python -m pip install -r requirements.txt
```

The requirements file forces `netCDF4` to build from source. Python `netCDF4`
must link to the same NetCDF-C installation used to build NEP. PyPI wheels that
bundle another NetCDF-C library cannot load NEP's build-tree UDF handlers and
are not supported for these tests. CMake rejects visualization configuration
unless the source-root `.venv` exists and contains all required modules.

Plots follow the project diagram rules: black and white output, maximum width of
8 inches, maximum height of 6.1 inches, no caption inside the PNG, and a
companion `_metadata.txt` file containing exactly `title`, `caption`, and
`alt_text` fields.

## CMake

Configure with examples, visualization, and all supported UDF readers enabled:

```bash
cmake -S . -B build \
  -DNEP_BUILD_EXAMPLES=ON \
  -DNEP_ENABLE_VIZ_EXAMPLES=ON \
  -DNEP_ENABLE_FITS=ON \
  -DNEP_ENABLE_CDF=ON \
  -DNEP_ENABLE_GEOTIFF=ON \
  -DNEP_ENABLE_GRIB2=ON \
  -DNEP_ENABLE_PDS4=ON \
  -DNEP_ENABLE_DICOM=ON \
  -DNEP_ENABLE_PDB=ON \
  -DNEP_ENABLE_MMCIF=ON
cmake --build build
ctest --test-dir build -R viz --output-on-failure
```

CMake sets `NCRCENV_RC` to the generated build-tree `.ncrc`, sets `NETCDF_RC`
to the top-level build directory, and extends `LD_LIBRARY_PATH` for each
visualization test.

## Manual Runtime Environment

For direct execution from a configured build tree, set the runtime environment
to the directories containing the generated `.ncrc` and NEP shared libraries:

```bash
export NCRCENV_RC=/path/to/nep/build/.ncrc
export NETCDF_RC=/path/to/nep/build
export LD_LIBRARY_PATH=/path/to/nep/build/src:$LD_LIBRARY_PATH
python3 /path/to/nep/build/examples/viz/test_plot_common.py
python3 /path/to/nep/build/examples/viz/test_udf_open.py \
  /path/to/nep/build/test/data/WFPC2u5780205r_c0fx.fits
python3 /path/to/nep/build/examples/viz/FITS/plot_fits_image.py \
  /path/to/nep/build/test/data/WFPC2u5780205r_c0fx.fits
python3 /path/to/nep/build/examples/viz/CDF/plot_cdf_var.py \
  /path/to/nep/build/test/data/tst_cdf_simple.cdf
python3 /path/to/nep/build/examples/viz/GeoTIFF/plot_geotiff_subset.py \
  /path/to/nep/build/test/data/MCDWD_L3_F1C_NRT.A2025353.h00v03.061.tif
python3 /path/to/nep/build/examples/viz/GRIB2/plot_grib2_grid.py \
  /path/to/nep/build/test/data/gdaswave.t00z.wcoast.0p16.f000.grib2
python3 /path/to/nep/build/examples/viz/PDS4/plot_pds4_messenger.py \
  /path/to/nep/build/test/data/PDS4/messenger_tnmap/thermal_neutron_map.xml
python3 /path/to/nep/build/examples/viz/PDS4/plot_pds4_perseverance.py \
  /path/to/nep/build/test/data/PDS4/perseverance/ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.xml
python3 /path/to/nep/build/examples/viz/PDS4/plot_pds4_maven_l3.py \
  /path/to/nep/build/test/data/PDS4/maven/mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.xml
python3 /path/to/nep/build/examples/viz/PDS4/plot_pds4_new_horizons.py \
  /path/to/nep/build/test/data/PDS4/new_horizons/ali_0030420276_0x4b0_sci_1.lblx
python3 /path/to/nep/build/examples/viz/DICOM/plot_dicom_mrbrain.py \
  /path/to/nep/build/test/data/DICOM/MRBRAIN.DCM
python3 /path/to/nep/build/examples/viz/DICOM/plot_dicom_xa_montage.py \
  /path/to/nep/build/test/data/DICOM/0003.DCM
python3 /path/to/nep/build/examples/viz/DICOM/plot_dicom_ct_brain.py \
  /path/to/nep/build/test/data/DICOM/CT-MONO2-16-brain.dcm
python3 /path/to/nep/build/examples/viz/PDB/plot_pdb_xray_structure.py \
  /path/to/nep/build/test/data/PDB/4HHB.pdb
python3 /path/to/nep/build/examples/viz/PDB/plot_pdb_nmr_ensemble.py \
  /path/to/nep/build/test/data/PDB/1GAB.pdb
python3 /path/to/nep/build/examples/viz/mmCIF/plot_mmcif_1j7w.py \
  /path/to/nep/build/test/data/mmCIF/1J7W.cif
python3 /path/to/nep/build/examples/viz/mmCIF/plot_mmcif_2w6v.py \
  /path/to/nep/build/test/data/mmCIF/2W6V.cif
python3 /path/to/nep/build/examples/viz/mmCIF/plot_mmcif_4hhb.py \
  /path/to/nep/build/test/data/mmCIF/4HHB.cif
python3 /path/to/nep/build/examples/viz/verify_viz_artifacts.py \
  /path/to/nep/build/examples/viz \
  viz_plot_common_test fits_wfpc2_image cdf_temperature geotiff_modis_flood \
  grib2_wave pds4_messenger_tnmap pds4_perseverance_mastcamz \
  pds4_maven_ngims_l3 pds4_new_horizons_alice dicom_mrbrain_image dicom_xa_frame_montage \
  pdb_xray_structure pdb_nmr_ensemble mmcif_1j7w_structure mmcif_2w6v_structure mmcif_4hhb_structure
```

## Scripts

- **Shared root files**: `test_plot_common.py` validates `plot_common.py` and the
  `_metadata.txt` format; `test_udf_open.py` smoke-tests a UDF file through
  `netCDF4.Dataset`; `verify_viz_artifacts.py` validates expected PNG/metadata pairs,
  metadata schema, caption length, and publication dimensions.
- **`CDF/`**: `plot_cdf_var.py` opens `test/data/tst_cdf_simple.cdf`, reads the
  `temperature` zVariable, and writes `cdf_temperature.png` +
  `cdf_temperature_metadata.txt`.
- **`DICOM/`**: `_dicom_udf.py` provides DICOM NetCDF access; the four plot scripts
  render the MRBRAIN image, the `0003.DCM` frame montage, and CT and MR head images.
- **`FITS/`**: `plot_fits_image.py` opens `test/data/WFPC2u5780205r_c0fx.fits`, reads
  the first `image` plane, and writes `fits_wfpc2_image.png` +
  `fits_wfpc2_image_metadata.txt`.
- **`GeoTIFF/`**: `plot_geotiff_subset.py` plots the populated MODIS flood raster.
- **`GRIB2/`**: `plot_grib2_grid.py` plots valid cells from the GRIB2 `WIND` grid.
- **`PDS4/`**: the scripts plot the MESSENGER thermal neutron map, Perseverance
  Mastcam-Z band 0 radiance, MAVEN NGIMS L3 temperature, and a New Horizons Alice
  spectrum array.
- **`PDB/`**: `_pdb_udf.py` provides legacy PDB NetCDF access; `plot_pdb_xray_structure.py`
  renders a 3D scatter of the `4HHB.pdb` X-ray structure atoms, and
  `plot_pdb_nmr_ensemble.py` overlays all 20 models of the `1GAB.pdb` NMR ensemble.
- **`mmCIF/`**: `_mmcif_udf.py` provides PDBx/mmCIF NetCDF access; `plot_mmcif_1j7w.py`,
  `plot_mmcif_2w6v.py`, and `plot_mmcif_4hhb.py` each render a 3D scatter of ATOM and
  HETATM Cartesian coordinates for the `1J7W.cif`, `2W6V.cif`, and `4HHB.cif` X-ray
  structures.

Generated PNG and metadata files remain in the visualization build directory
for inspection. They are not installed and are not written to the source tree.
No caption appears inside the PNG, figures are limited to 8.0 by 6.1 inches at 150 DPI, and metadata fields are ordered `title`, `caption`, and `alt_text` with a 75-word caption limit.

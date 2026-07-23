# NEP Visualization Examples

The visualization examples use Python `netCDF4` to open NEP UDF files and
Matplotlib to write static PNG plots. They are optional and disabled by default.
Sprint 1 provides the common plotting helper and a FITS `.ncrc` autoload smoke
test. Sprint 2 adds a FITS image plot and a CDF line plot.

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
are not supported for these tests. Both build systems reject visualization
configuration unless the source-root `.venv` exists and contains all required
modules.

Plots follow the project diagram rules: black and white output, maximum width of
8 inches, maximum height of 6.1 inches, no caption inside the PNG, and a
companion `_metadata.txt` file containing exactly `title`, `caption`, and
`alt_text` fields.

## CMake

Configure with examples, visualization, FITS, and CDF enabled:

```bash
cmake -S . -B build \
  -DNEP_BUILD_EXAMPLES=ON \
  -DNEP_ENABLE_VIZ_EXAMPLES=ON \
  -DNEP_ENABLE_FITS=ON \
  -DENABLE_CDF=ON
cmake --build build
ctest --test-dir build -R viz --output-on-failure
```

CMake sets `NCRCENV_RC` to the generated build-tree `.ncrc`, sets `NETCDF_RC`
to the top-level build directory, and extends `LD_LIBRARY_PATH` for each
visualization test.

## Autotools

Configure with examples, visualization, FITS, and CDF enabled:

```bash
./configure --enable-examples --enable-viz-examples --enable-fits --enable-cdf
make
make check TESTS='test_plot_common.py test_udf_open.py plot_fits_image.py plot_cdf_var.py'
```

Automake sets `NCRCENV_RC`, `NETCDF_RC`, `LD_LIBRARY_PATH`, and the FITS
smoke-test path in the test environment.

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
python3 /path/to/nep/build/examples/viz/plot_fits_image.py \
  /path/to/nep/build/test/data/WFPC2u5780205r_c0fx.fits
python3 /path/to/nep/build/examples/viz/plot_cdf_var.py \
  /path/to/nep/build/test/data/tst_cdf_simple.cdf
```

## Scripts

- `test_plot_common.py` — validates `plot_common.py` and the `_metadata.txt` format.
- `test_udf_open.py` — smoke test that opens a UDF file through `netCDF4.Dataset`.
- `plot_fits_image.py` — opens `test/data/WFPC2u5780205r_c0fx.fits`, reads the first
  `image` plane, and writes `fits_wfpc2_image.png` + `fits_wfpc2_image_metadata.txt`.
- `plot_cdf_var.py` — opens `test/data/tst_cdf_simple.cdf`, reads the `temperature`
  zVariable, and writes `cdf_temperature.png` + `cdf_temperature_metadata.txt`.

Generated PNG and metadata files remain in the visualization build directory
for inspection. They are not installed and are not written to the source tree.

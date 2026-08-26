---
name: swot-l2-lr-ssh
description: Understanding the NASA PO.DAAC SWOT Level 2 KaRIn Low Rate Sea Surface Height (L2_LR_SSH) product — its NetCDF-4 file structure, key variables, quality masking, and how to access and plot it with Python.
metadata:
  author: NEP
  version: "1.0"
  date: "2026-08-26"
---

# SWOT L2_LR_SSH Skill

This skill covers the NASA PO.DAAC **SWOT Level 2 KaRIn Low Rate Sea Surface
Height** data product (`SWOT_L2_LR_SSH_*`).  It is the standard,
continually produced SWOT ocean topography product and is a good target for
a Python example that opens a gridded NetCDF-4 file and plots sea surface
height anomaly (SSHA) with cartopy.

## Product Overview

- **Mission:** Surface Water and Ocean Topography (SWOT), a joint NASA/CNES
  mission launched 2022-12-16.
- **Product:** Level 2 (L2) KaRIn Low Rate Sea Surface Height.
- **Format:** NetCDF-4, one granule per pass (half-orbit).
- **Grid:** Geographically fixed, swath-aligned grid.  The *Basic*,
  *Expert*, and *Windwave* sub-products are posted on a nominal 2 km x 2 km
  grid; the *Unsmoothed* sub-product is posted on a ~250 m native grid.
- **Coverage:** Global; each file covers one half-orbit (pass) with two
  KaRIn swaths separated by a nadir gap.
- **Distributor:** NASA PO.DAAC (requires a free NASA Earthdata Login).

## PO.DAAC Collection Short Names

| Collection | Short name | Notes |
|------------|------------|-------|
| Parent (all sub-products) | `SWOT_L2_LR_SSH_D` | Version D, current science-phase data. |
| Version C parent | `SWOT_L2_LR_SSH_2.0` | Version C science/calibration data. |
| Basic only | `SWOT_L2_LR_SSH_Basic_D` | Smallest file; recommended for examples. |
| Expert only | `SWOT_L2_LR_SSH_Expert_D` | Includes corrections and extra geophysical fields. |
| Windwave only | `SWOT_L2_LR_SSH_WindWave_D` | Adds wind/wave products. |
| Unsmoothed only | `SWOT_L2_LR_SSH_Unsmoothed_D` | 250 m native grid; much larger files. |

For a simple example, use the **Basic** sub-product (`SWOT_L2_LR_SSH_Basic_D`).

## File Structure (NetCDF-4)

### Dimensions

Typical dimension names in the Basic/Expert products:

| Dimension | Description |
|-----------|-------------|
| `num_lines` | Along-track direction (rows). |
| `num_pixels` | Cross-track direction (columns). |

Unsmoothed files may use `num_lines_250m` / `num_pixels_250m` or similar.

### Key Variables

Variables are 2-D arrays shaped `(num_lines, num_pixels)` unless noted.

| Variable | Description | Units | Notes |
|----------|-------------|-------|-------|
| `ssha` or `ssha_karin` | Sea surface height anomaly | m | The primary science variable to plot. |
| `ssh` or `ssh_karin` | Total sea surface height relative to the reference ellipsoid | m | Includes geoid, tides, etc. |
| `latitude` | Latitude of each pixel | degrees_north | Stored as a 2-D coordinate array. |
| `longitude` | Longitude of each pixel | degrees_east | Stored as a 2-D coordinate array. |
| `quality_flag` or `ssha_karin_qual` | Quality flags | -- | Non-zero values indicate invalid or degraded pixels. |
| `cross_track_distance` | Distance from nadir | km | Negative = left swath, positive = right swath. |
| `significant_wave_height` | Significant wave height | m | Wind/wave product variable. |
| `wind_speed` | Wind speed | m/s | Wind/wave product variable. |

The exact variable names differ slightly between Version C (`SWOT_L2_LR_SSH_2.0`)
and Version D (`SWOT_L2_LR_SSH_D`), and between Basic and Expert.  Reader code
should accept a list of candidate names.

### Quality Masking

Both `ssha_karin` and `ssha` are already corrected for geoid, tides, DAC,
etc.  Pixels with non-zero quality flags should be masked before plotting or
analysis.  Typical logic:

```python
import xarray as xr
import numpy as np

ds = xr.open_dataset(file_path, engine="netcdf4", decode_times=False)
ssha = ds["ssha_karin"]
quality = ds["ssha_karin_qual"]
ssha_masked = ssha.where(quality == 0)
```

## Relationship to NetCDF and NEP

SWOT L2_LR_SSH files are ordinary NetCDF-4 files.  They open directly with:

- `nc_open()` / `ncdump` (NetCDF-C)
- `h5dump` / `h5py` (HDF5)
- `xarray.open_dataset(..., engine="netcdf4")`
- `netCDF4.Dataset`

They do **not** require a NEP UDF reader.  An example can treat them like the
NISAR SME2 example: a standalone Python script that reads the file, applies a
quality mask, and plots the result.

## Access Patterns

### With `earthaccess` (Python)

```python
import earthaccess

earthaccess.login()   # reads ~/.netrc or environment variables
results = earthaccess.search_data(
    short_name="SWOT_L2_LR_SSH_Basic_D",
    bounding_box=(-45, 35, -37, 41),  # W, S, E, N
)
earthaccess.download(results, "./data")
```

### With `podaac-data-subscriber` (command line)

```bash
podaac-data-subscriber -c SWOT_L2_LR_SSH_Basic_D -d ./data \
  -b="-45,35,-37,41" -sd 2024-01-01T00:00:00Z
```

### With Earthdata Search (web GUI)

Search for collection `SWOT_L2_LR_SSH_Basic_D` or `SWOT_L2_LR_SSH_D`, draw a
spatial filter, and download individual granules.

## Plotting Guidance

1. Load `ssha` / `ssha_karin` and 2-D `latitude`/`longitude`.
2. Mask using the corresponding quality flag.
3. Use `matplotlib` + `cartopy` with `ccrs.PlateCarree()`:
   - `ax.pcolormesh(longitude, latitude, ssha, ...)`
   - Use a diverging colormap (e.g., `RdBu_r`) centered on zero for anomaly
     data.
   - Add coastlines and a colorbar labeled in meters.
4. The resulting image will show the two KaRIn swaths with a gap along the
   nadir track; masked/fill pixels are transparent.

## Example Python Snippet

```python
import xarray as xr
import matplotlib.pyplot as plt
import cartopy.crs as ccrs

with xr.open_dataset("SWOT_L2_LR_SSH_Basic_*.nc", decode_times=False) as ds:
    ssha = ds["ssha_karin"]
    quality = ds["ssha_karin_qual"]
    ssha = ssha.where(quality == 0)

    fig, ax = plt.subplots(figsize=(10, 8), subplot_kw={"projection": ccrs.PlateCarree()})
    ax.pcolormesh(
        ds["longitude"], ds["latitude"], ssha,
        transform=ccrs.PlateCarree(), cmap="RdBu_r", shading="auto"
    )
    ax.coastlines(resolution="110m")
    ax.set_title("SWOT Sea Surface Height Anomaly")
    plt.show()
```

## Common Pitfalls

- **Nadir gap:** The center columns are intentionally fill/masked because
  KaRIn does not observe directly at nadir.  Do not treat the gap as a data
  quality problem.
- **Variable name differences:** Version C uses `ssha`, Version D Basic uses
  `ssha_karin`.  Always check `ds.data_vars` and support candidate names.
- **Quality flag semantics:** A non-zero quality flag means the pixel is
  invalid or degraded.  Always mask before averaging or plotting.
- **Unsmoothed grid:** The *Unsmoothed* product uses a finer ~250 m grid
  with different dimension names and much larger file sizes.

## References

- PO.DAAC dataset: https://podaac.jpl.nasa.gov/dataset/SWOT_L2_LR_SSH_D
- PO.DAAC cookbook (local machine access): https://podaac.github.io/tutorials/notebooks/datasets/Localmachine_SWOT_Oceanography.html
- SWOT mission: https://swot.jpl.nasa.gov/

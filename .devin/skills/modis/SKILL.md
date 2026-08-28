# MODIS Terra/Aqua Science Products

## Purpose

Understand NASA's Moderate Resolution Imaging Spectroradiometer (MODIS) aboard
the Terra and Aqua spacecraft, its standard science product families, and how a
GeoTIFF-formatted MODIS product maps to the NetCDF data model through NEP's
GeoTIFF User Defined Format (UDF) reader.

MODIS products are most commonly distributed from the USGS/NASA Land Processes
Distributed Active Archive Center (LP DAAC) in HDF4. NEP does not provide an HDF4
UDF reader, so a MODIS example must use a GeoTIFF rendering of a MODIS product
(for example from NASA GIBS or AppEEARS). Once such a sample is obtained, NEP
exposes each GeoTIFF band as a 2-D NetCDF variable with CF-1.8 coordinate
variables and a `crs` grid-mapping variable.

## When to Use

Use this skill when adding, reviewing, or debugging any NEP example, handler, or
workflow that touches MODIS Terra/Aqua data. It covers the spacecraft, the MODIS
instrument, standard product families, file naming conventions, and the
GeoTIFF-to-NetCDF mapping.

## Required Context

When a user asks about a specific MODIS product, identify these items:

| Item | Typical Value | Notes |
|------|---------------|-------|
| Mission/spacecraft | Terra, Aqua | EOS AM-1 (Terra), EOS PM-1 (Aqua) |
| Instrument | MODIS | 36-band whisk-broom scanning radiometer |
| Common products | MOD09, MYD09, MOD13, MYD13, MOD11, MYD11, MCD43 | Surface reflectance, vegetation indices, LST, albedo |
| Standard format | HDF4 from LP DAAC | Not directly readable by NEP |
| NEP-compatible format | GeoTIFF from GIBS / AppEEARS | Required for NEP GeoTIFF UDF reader |
| NEP path | GeoTIFF UDF reader | `examples/viz/GeoTIFF/` or a custom GeoTIFF script |
| Primary variables | Band-dependent, e.g. `sur_refl_b01`, `250m_16_days_NDVI`, `LST_Day_1km` | Product-specific |
| Coordinates | Sinusoidal `x`/`y`, or geographic `lon`/`lat` after reprojection | Depends on GeoTIFF source |
| Quality/flag masks | `QA` bit-packed rasters, `pixel_reliability`, etc. | Product-specific |
| Data access | LP DAAC, NASA Earthdata, NASA GIBS, AppEEARS | Free; Earthdata login often required |

## Spacecraft and Orbit

* **Agency:** NASA
* **Spacecraft:** Terra (EOS AM-1), Aqua (EOS PM-1)
* **Launch:** 1999-12-18 (Terra), 2002-05-04 (Aqua)
* **Orbit:** Sun-synchronous, near-polar, ~705 km mean altitude
* **Equator crossing time:** ~10:30 local time descending (Terra), ~13:30 local
time ascending (Aqua)
* **Revisit:** Near-daily global coverage from each spacecraft; combined Terra+
Aqua coverage roughly twice daily for most locations.

## Instrument

MODIS is a whisk-broom scanning radiometer. It measures in 36 spectral bands at
resolutions from 250 m to 1 km.

| Band | Wavelength (µm) | Ground resolution | Primary use |
|------|-----------------|-------------------|-------------|
| 1 | 0.620 - 0.670 | 250 m | Red, NDVI |
| 2 | 0.841 - 0.876 | 250 m | NIR, NDVI |
| 3 | 0.459 - 0.479 | 500 m | Blue |
| 4 | 0.545 - 0.565 | 500 m | Green |
| 5 | 1.230 - 1.250 | 500 m | Snow/ice |
| 6 | 1.628 - 1.652 | 500 m | Land/cloud |
| 7 | 2.105 - 2.155 | 500 m | SWIR, burning |
| 8-15 | 0.405 - 0.877 | 1 km | Ocean color, aerosol |
| 16-19 | 0.866 - 0.936 | 1 km | Atmospheric water vapor, ocean |
| 20-25 | 3.660 - 4.549 | 1 km | Surface/cloud temperature |
| 26 | 1.360 - 1.390 | 1 km | Cirrus clouds |
| 27-36 | 6.535 - 14.385 | 1 km | Water vapor, ozone, temperature |

## Standard MODIS Product Families

| Family | Terra | Aqua | Description |
|--------|-------|------|-------------|
| Surface Reflectance | MOD09 | MYD09 | Daily atmospherically corrected surface reflectance |
| Vegetation Indices | MOD13 | MYD13 | 16-day NDVI / EVI composite |
| Land Surface Temperature | MOD11 | MYD11 | Daily land-surface temperature and emissivity |
| BRDF/Albedo | MCD43 | MCD43 | 16-day nadir BRDF-adjusted reflectance and albedo |
| Snow Cover | MOD10 | MYD10 | Daily global snow cover |
| Burned Area | MCD64 | MCD64 | Monthly burned-area maps |

For NEP, the key constraint is that the chosen product must be obtained as a
GeoTIFF (for example, via NASA GIBS or AppEEARS) because the standard LP DAAC
science products are in HDF4, which NEP does not read.

## File Naming

A typical MODIS tile file name has the form:

```text
PRODUCT.AYYYYDDD.h##v##.VVV.YYYYDDDHHMMSS.hdf
```

| Field | Meaning |
|-------|---------|
| PRODUCT | Product short name, e.g. `MOD09GA`, `MYD13Q1` |
| AYYYYDDD | Acquisition year and Julian day |
| h##v## | Horizontal and vertical tile index in the MODIS Sinusoidal grid |
| VVV | Collection version, e.g. `061` |
| YYYYDDDHHMMSS | Production date/time |
| hdf | HDF4-EOS format (standard LP DAAC distribution) |

When exported as GeoTIFF from AppEEARS or GIBS, the file will typically have a
`.tif` extension and may include the product, tile, date, and band information.

## NetCDF Mapping via NEP GeoTIFF Reader

When NEP is built with `-DNEP_ENABLE_GEOTIFF=ON`, a GeoTIFF MODIS product is
opened through the standard NetCDF API.

* **Dimensions:** `x`/`y` (or `lon`/`lat`) matching the raster columns and rows.
* **Data variable:** `data[y, x]` containing the band values. The variable name may
  be configurable through the NEP reader; default behavior exposes the raster as
  `data`.
* **Coordinate variables:** 1-D `x[x]` and `y[y]` in projected coordinates, or
  `lon[lon]` and `lat[lat]` in geographic coordinates.
* **Coordinate bounds:** For pixel-as-area rasters, `x_bnds`/`y_bnds` or
  `lon_bnds`/`lat_bnds` are created and referenced via `bounds` attributes.
* **Grid mapping:** A scalar `crs` variable stores the CRS attributes
  (`grid_mapping_name`, `semi_major_axis`, `inverse_flattening`, and projection
  parameters). The data variable carries `grid_mapping = "crs"`.
* **Multi-band files:** A GeoTIFF with multiple bands maps to a 3-D variable
  `data[band, y, x]`. Many MODIS GeoTIFF exports are single-band per file, so
  each band is opened as a separate 2-D dataset.

## Data Access

Public sources for MODIS data:

* **USGS/NASA LP DAAC:** <https://lpdaac.usgs.gov/> (standard HDF4 products)
* **NASA Earthdata Search:** <https://search.earthdata.nasa.gov/>
* **NASA GIBS:** <https://gibs.earthdata.nasa.gov/> (GeoTIFF browse and tile
  products)
* **NASA AppEEARS:** <https://appeears.earthdatacloud.nasa.gov/> (subset and
  export as GeoTIFF)

A free Earthdata login is usually required.

## Citation

Please cite MODIS datasets in the following manner:

MODIS/Terra+Aqua [Product Name] [Data Set]. NASA EOSDIS Land Processes
Distributed Active Archive Center (LP DAAC), USGS/EROS Center, Sioux Falls,
South Dakota. https://lpdaac.usgs.gov

A product-specific DOI (for example, `https://doi.org/10.5067/MODIS/MOD09GA.061`)
should be added once the exact MODIS product used in the example has been
selected.

## References

1. NASA MODIS Web Site. https://modis.gsfc.nasa.gov/
2. USGS/NASA Land Processes Distributed Active Archive Center (LP DAAC).
   https://lpdaac.usgs.gov/
3. NASA Earthdata Search. https://search.earthdata.nasa.gov/
4. NASA Global Imagery Browse Services (GIBS). https://gibs.earthdata.nasa.gov/
5. NASA AppEEARS. https://appeears.earthdatacloud.nasa.gov/

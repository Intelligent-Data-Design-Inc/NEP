# Landsat 8-9 OLI/TIRS Collection 2 Level-2 Science Products

## Purpose

Understand the USGS Landsat 8-9 Operational Land Imager / Thermal Infrared
Sensor (OLI/TIRS) Collection 2 Level-2 science product family and how to work
with it through the standard NetCDF API via NEP's GeoTIFF User Defined Format
(UDF) reader.

Landsat Collection 2 Level-2 products are delivered as individual per-band
GeoTIFF rasters. NEP exposes each GeoTIFF as a NetCDF dataset with 2-D
variables, CF-1.8 coordinate variables, and a `crs` grid-mapping variable.
This lets users read Landsat surface reflectance, surface temperature, and
quality-assessment bands with `nc_open()` or `netCDF4.Dataset` without converting
the files.

## When to Use

Use this skill when adding, reviewing, or debugging any NEP example, handler, or
workflow that touches Landsat 8-9 Collection 2 Level-2 data. It covers the
mission, instruments, product layout, and the NetCDF/GeoTIFF mapping.

## Required Context

When a user asks about a specific Landsat product, identify these items:

| Item | Typical Value | Notes |
|------|---------------|-------|
| Mission | Landsat 8 / Landsat 9 | NASA/USGS Earth-observing satellites |
| Sensors | OLI, TIRS | OLI = optical; TIRS = thermal |
| Product family | Collection 2 Level-2 Science Products | Surface reflectance + surface temperature |
| Native format | GeoTIFF (.TIF) per band inside a .tar.gz | Not NetCDF-4/HDF5 natively |
| NEP path | GeoTIFF UDF reader | `examples/viz/GeoTIFF/` or a custom GeoTIFF script |
| Primary variables | `SR_B1`..`SR_B7`, `SR_B9`, `ST_B10`, `ST_B11`, `QA_PIXEL`, `QA_RADSAT` | Band names are product-specific |
| Coordinates | 1-D `x`/`y` in projected CRS or `lon`/`lat` in geographic CRS | Determined by GeoTIFF CRS tags |
| Quality mask | `QA_PIXEL` bit-packed flags | Cloud, shadow, snow, water, aerosol, fill |
| Data access | USGS Earth Explorer, M2M API, Microsoft Planetary Computer | Free; credential may be required |

## Mission and Spacecraft

* **Agency:** NASA / U.S. Geological Survey
* **Launch:** 2013-02-11 (Landsat 8), 2021-09-27 (Landsat 9)
* **Orbit:** Sun-synchronous, near-polar, ~705 km mean altitude
* **Inclination:** ~98.2 degrees
* **Repeat cycle:** 16 days per spacecraft; Landsat 8 and 9 are phased so that
together they provide roughly an 8-day revisit for most locations.

## Instruments

### Operational Land Imager (OLI)

Push-broom optical imager built by Ball Aerospace.

| Band | Common name | Wavelength (µm) | Ground resolution |
|------|-------------|-----------------|-------------------|
| 1 | Coastal aerosol | 0.43 - 0.45 | 30 m |
| 2 | Blue | 0.45 - 0.51 | 30 m |
| 3 | Green | 0.53 - 0.59 | 30 m |
| 4 | Red | 0.64 - 0.67 | 30 m |
| 5 | Near Infrared (NIR) | 0.85 - 0.88 | 30 m |
| 6 | Short-Wave Infrared 1 (SWIR1) | 1.57 - 1.65 | 30 m |
| 7 | Short-Wave Infrared 2 (SWIR2) | 2.11 - 2.29 | 30 m |
| 8 | Panchromatic | 0.50 - 0.68 | 15 m |
| 9 | Cirrus | 1.36 - 1.38 | 30 m |

### Thermal Infrared Sensor (TIRS)

Measures emitted thermal radiation for land-surface temperature retrieval.

| Band | Common name | Wavelength (µm) | Native resolution |
|------|-------------|-----------------|-------------------|
| 10 | TIRS1 | 10.6 - 11.19 | 100 m |
| 11 | TIRS2 | 11.50 - 12.51 | 100 m |

In Collection 2 Level-2, the thermal bands are resampled to 30 m to match the
multispectral bands.

## Collection 2 Level-2 Products

Collection 2 Level-2 science products are produced by the Landsat Level-2
Product Generation System (LPGS):

* **Surface Reflectance (SR):** Atmospherically corrected top-of-canopy
  reflectance for OLI bands 1-7 and 9 at 30 m.
* **Surface Temperature (ST):** Atmospherically compensated land-surface
  temperature from TIRS bands 10 and 11, delivered at 30 m.
* **QA_PIXEL:** Bit-packed per-pixel quality flags covering cloud, cloud
  shadow, snow/ice, water, aerosol level, and fill.
* **QA_RADSAT:** Radiometric saturation and terrain-occlusion flags.
* **Angle Coefficient Files:** Auxiliary geometry coefficients.

## Product Packaging and Naming

A granule is distributed as a gzip-compressed tar archive containing one
GeoTIFF per band plus metadata XML. The product identifier has the form:

```text
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2.tar.gz
```

| Field | Meaning |
|-------|---------|
| L | Landsat |
| X | Spacecraft number (8 or 9) |
| SS | Sensor (`C` = combined OLI/TIRS) |
| L2SP | Level-2 Surface Reflectance and Surface Temperature |
| PPPP | WRS-2 path |
| RRRRR | WRS-2 row |
| YYYYMMDD | Acquisition date |
| yyyymmdd | Processing date |
| CC | Collection number, `02` |
| TX | Tier (`T1` or `T2`) |
| TX2 | Secondary tier (`RT` or `T2`) |

Inside the archive, individual band GeoTIFFs are named like:

```text
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2_SR_B3.TIF
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2_SR_B5.TIF
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2_ST_B10.TIF
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2_QA_PIXEL.TIF
```

## NetCDF Mapping via NEP GeoTIFF Reader

When NEP is built with `-DNEP_ENABLE_GEOTIFF=ON`, a single Landsat band GeoTIFF
is opened through the standard NetCDF API.

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
  `data[band, y, x]`. Landsat standard products are single-band per file, so each
  band is opened as a separate 2-D dataset.

## Data Access

Public sources for obtaining sample granules:

* **USGS Landsat Missions:** <https://www.usgs.gov/landsat-missions>
* **USGS Earth Explorer:** <https://earthexplorer.usgs.gov/>
* **USGS M2M API:** <https://m2m.cr.usgs.gov/>
* **Microsoft Planetary Computer:** Landsat Collection 2 Level-2 catalog
  (cloud-optimized GeoTIFFs)

A free account may be required for USGS sources.

## Citation

Please cite this dataset in the following manner:

Earth Resources Observation and Science (EROS) Center. (2020). Landsat 8-9
Operational Land Imager / Thermal Infrared Sensor Level-2, Collection 2
[dataset]. U.S. Geological Survey. https://doi.org/10.5066/P9OGBGM6

## References

1. USGS EROS Archive - Landsat Archives - Landsat 8-9 OLI/TIRS Collection 2
   Level-2.
   <https://www.usgs.gov/centers/eros/science/usgs-eros-archive-landsat-archives-landsat-8-9-olitirs-collection-2-level-2>
2. Landsat 8 Data Users Handbook.
   <https://www.usgs.gov/landsat-missions/landsat-8-data-users-handbook>
3. Landsat 9 Data Users Handbook.
   <https://www.usgs.gov/landsat-missions/landsat-9-data-users-handbook>
4. Landsat Collection 2 Level-2 Science Product Guide.
   <https://www.usgs.gov/media/images/landsat-collection-2-level-2-science-product-guide>

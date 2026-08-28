# NetCDF with Landsat 8-9 OLI/TIRS Collection 2 Level-2 Science Products

The U.S. Geological Survey (USGS) Landsat 8 and Landsat 9 satellites carry the
Operational Land Imager (OLI) and the Thermal Infrared Sensor (TIRS). Their
Collection 2 Level-2 science products are delivered as surface reflectance (SR)
and surface temperature (ST) estimates packaged in a per-granule archive of
GeoTIFF rasters plus metadata. Because the data are already GeoTIFF, they can be
read directly through NEP's GeoTIFF User Defined Format (UDF) handler using the
standard `nc_open()` / `netCDF4.Dataset` API.

This document will be expanded once a real sample granule has been obtained.
Sample code and the full NetCDF data-model mapping will be added after the file
is available.

## Mission Overview

Landsat 8 was launched on 11 February 2013, followed by Landsat 9 on 27
September 2021. Both missions are operated by NASA and the USGS as part of the
long-running Landsat programme, providing a continuous archive of medium-resolution
Earth-surface observations dating back to 1972.

* **Agency:** NASA / U.S. Geological Survey  
* **Launch:** 2013-02-11 (Landsat 8), 2021-09-27 (Landsat 9)  
* **Orbit:** Sun-synchronous, near-polar, ~705 km mean altitude  
* **Inclination:** ~98.2 degrees  
* **Repeat cycle:** 16 days per spacecraft; Landsat 8 and 9 operate in a
  phased orbit that together yield an 8-day global revisit for most targets.

## Spacecraft and Instruments

### Operational Land Imager (OLI)

OLI is a push-broom optical sensor built by Ball Aerospace. It collects reflected
solar radiation in nine spectral bands at a range of spatial resolutions.

| Band | Common name | Wavelength (µm) | Resolution (m) |
|------|-------------|-----------------|----------------|
| 1    | Coastal aerosol | 0.43 - 0.45 | 30 |
| 2    | Blue | 0.45 - 0.51 | 30 |
| 3    | Green | 0.53 - 0.59 | 30 |
| 4    | Red | 0.64 - 0.67 | 30 |
| 5    | Near Infrared (NIR) | 0.85 - 0.88 | 30 |
| 6    | Short-Wave Infrared 1 (SWIR1) | 1.57 - 1.65 | 30 |
| 7    | Short-Wave Infrared 2 (SWIR2) | 2.11 - 2.29 | 30 |
| 8    | Panchromatic | 0.50 - 0.68 | 15 |
| 9    | Cirrus | 1.36 - 1.38 | 30 |

### Thermal Infrared Sensor (TIRS)

TIRS measures emitted thermal radiation in two bands for land-surface
temperature retrieval. Landsat 8 carries the first TIRS unit; Landsat 9
carries an upgraded second unit with improved stray-light mitigation.

| Band | Common name | Wavelength (µm) | Resolution (m) |
|------|-------------|-----------------|----------------|
| 10   | Thermal Infrared 1 (TIRS1) | 10.6 - 11.19 | 100 (resampled to 30 in C2 L2) |
| 11   | Thermal Infrared 2 (TIRS2) | 11.50 - 12.51 | 100 (resampled to 30 in C2 L2) |

## Collection 2 Level-2 Science Products

The Collection 2 Level-2 product family includes atmospherically corrected
surface reflectance and land-surface temperature, derived from Level-1 data by
the Landsat Level-2 Product Generation System (LPGS).

* **Surface Reflectance (SR):** Bands 1-7 and 9 at 30 m. Band 8 (panchromatic) is
  not included in the standard SR product.
* **Surface Temperature (ST):** Bands 10 and 11, delivered resampled to 30 m.
* **Quality Assessment (QA):** A per-pixel raster band containing bit-packed
  flags for cloud, cloud shadow, snow/ice, water, aerosol levels, and fill.
* **Pixel Quality Assessment Band (PIXELQA / QA_PIXEL):** Standard quality band.
* **Radiometric Saturation and Terrain Occlusion QA (QA_RADSAT):** Indicates
  saturation and terrain-occlusion conditions.
* **Angle Coefficient Files:** Separate auxiliary files for solar and viewing
  geometry coefficients.

Products are distributed as individual GeoTIFF files inside a tar/gzipped
package, one raster per band, plus an XML metadata file. Product identifiers
follow the pattern:

```
LXSS_L2SP_PPPPRRRRR_YYYYMMDD_yyyymmdd_CC_TX_TX2.tar.gz
```

where `L` denotes Landsat, `X` the spacecraft number (8 or 9), `SS` the sensor
(`C` for combined OLI/TIRS), `PPPP` the path, `RRRRR` the row, `CC` the
collection number (02), and `TX` / `TX2` the tier and processing level
(`T1` = Tier 1, `T2` = Tier 2, `RT` = Real-Time).

## Relevance to NEP

Landsat Collection 2 Level-2 products are a canonical example of widely used
geospatial raster data delivered in GeoTIFF. Using NEP's GeoTIFF UDF reader,
the individual band GeoTIFFs can be opened with `nc_open()` and are exposed as
2-D NetCDF variables with CF-1.8 coordinate variables and a grid-mapping variable
for the CRS. This lets users combine Landsat bands, masks, and derived indices
within the same NetCDF workflow used for model output or satellite swath data.

## Data Access

Sample data must be obtained before example code and the full data-model mapping
are added. Public sources include:

* **USGS Landsat data archive:** <https://www.usgs.gov/landsat-missions>
* **USGS Earth Explorer:** <https://earthexplorer.usgs.gov/>
* **USGS Machine-to-Machine (M2M) API:** <https://m2m.cr.usgs.gov/>
* **Microsoft Planetary Computer:** Landsat Collection 2 Level-2 catalog
  (cloud-optimized GeoTIFFs, API access may be required)

A Tier 1 (`T1`) Landsat 8 or 9 `L2SP` surface-reflectance-and-temperature
granule is the preferred sample. Only a real downloaded granule will be used
for code and figures.

## Citation

Please cite this dataset in the following manner:

Earth Resources Observation and Science (EROS) Center. (2020). Landsat 8-9
Operational Land Imager / Thermal Infrared Sensor Level-2, Collection 2
[dataset]. U.S. Geological Survey. https://doi.org/10.5066/P9OGBGM6

## References

1. USGS EROS Archive - Landsat Archives - Landsat 8-9 OLI/TIRS Collection 2
   Level-2. https://www.usgs.gov/centers/eros/science/usgs-eros-archive-landsat-archives-landsat-8-9-olitirs-collection-2-level-2
2. Landsat 8 (L8) Data Users Handbook.
   https://www.usgs.gov/landsat-missions/landsat-8-data-users-handbook
3. Landsat 9 (L9) Data Users Handbook.
   https://www.usgs.gov/landsat-missions/landsat-9-data-users-handbook
4. Landsat Collection 2 Level-2 Science Product Guide.
   https://www.usgs.gov/media/images/landsat-collection-2-level-2-science-product-guide

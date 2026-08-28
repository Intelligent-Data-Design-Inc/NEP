# NetCDF with MODIS Terra/Aqua Science Products

The Moderate Resolution Imaging Spectroradiometer (MODIS) is a key instrument
aboard NASA's Terra (launched 1999) and Aqua (launched 2002) satellites. It
measures reflected solar and emitted thermal radiation in 36 spectral bands,
producing daily global data used for land, ocean, and atmosphere science.

This document will focus on a MODIS Level-2 or Level-3 science product that is
available as a GeoTIFF, so it can be read through NEP's GeoTIFF User Defined
Format (UDF) reader using the standard NetCDF API. The exact product, citation,
file naming, and NetCDF mapping will be added once a real sample granule has been
obtained. Standard MODIS science products from the USGS/NASA LP DAAC are
distributed in HDF4; GeoTIFF versions are available from services such as NASA
GIBS and AppEEARS.

## Mission Overview

MODIS is part of NASA's Earth Observing System (EOS). Terra's morning crossing
and Aqua's afternoon crossing together provide frequent global coverage.

* **Agency:** NASA  
* **Spacecraft:** Terra (EOS AM-1), Aqua (EOS PM-1)  
* **Launch:** 1999-12-18 (Terra), 2002-05-04 (Aqua)  
* **Orbit:** Sun-synchronous, near-polar, ~705 km mean altitude  
* **Equator crossing time:** ~10:30 local time descending (Terra), ~13:30 local
time ascending (Aqua)  
* **Revisit:** Near-daily global coverage from each spacecraft; combined Terra+
Aqua coverage roughly twice daily for most locations.

## Instrument

MODIS is a whisk-broom scanning radiometer built by Raytheon / Santa Barbara
Remote Sensing. It collects data in 36 spectral bands spanning visible, near-
infrared, and thermal infrared wavelengths at spatial resolutions from 250 m
to 1 km.

| Band range | Primary use | Representative bands | Ground resolution |
|------------|-------------|----------------------|-------------------|
| 1-2 | Land/cloud/aerosols properties | 1 (red), 2 (NIR) | 250 m |
| 3-7 | Land/ocean/cloud properties | 3-7 (blue, green, NIR, SWIR) | 500 m |
| 8-36 | Ocean color, cloud, water vapor, surface/atmospheric temperature | 8-36 | 1 km |

Key MODIS bands for land science:

| Band | Wavelength (µm) | Common use |
|------|-----------------|------------|
| 1 | 0.620 - 0.670 | Red reflectance, NDVI |
| 2 | 0.841 - 0.876 | NIR reflectance, NDVI |
| 3 | 0.459 - 0.479 | Blue reflectance |
| 4 | 0.545 - 0.565 | Green reflectance |
| 5 | 1.230 - 1.250 | Snow/ice discrimination |
| 6 | 1.628 - 1.652 | Land/cloud properties |
| 7 | 2.105 - 2.155 | SWIR, biomass burning |
| 31 | 10.780 - 11.280 | Land surface temperature |
| 32 | 11.770 - 12.270 | Land surface temperature |

## MODIS Product Families

Common MODIS land product families relevant to NEP examples include:

* **Surface Reflectance (MOD09 / MYD09):** Daily Level-2G atmospherically
  corrected surface reflectance.
* **Vegetation Indices (MOD13 / MYD13):** 16-day composite NDVI/EVI at 250 m or
  1 km.
* **Land Surface Temperature/Emissivity (MOD11 / MYD11):** Daily LST at 1 km.
* **BRDF/Albedo (MCD43):** 16-day nadir BRDF-adjusted reflectance and albedo.
* **Snow Cover (MOD10 / MYD10):** Daily global snow cover.
* **Burned Area (MCD64):** Monthly burned-area maps.

Standard LP DAAC distributions of these products are in HDF4. To read them
through NEP without conversion, a GeoTIFF rendering must be obtained (for
example, through NASA GIBS, AppEEARS, or another NASA service that exports MODIS
products as GeoTIFF).

## Relevance to NEP

MODIS provides decades of globally consistent moderate-resolution Earth imagery
and derived science products. A GeoTIFF-formatted MODIS product can be opened
directly with `nc_open()` or `netCDF4.Dataset` via NEP's GeoTIFF UDF reader,
exposing bands as 2-D NetCDF variables with CF-1.8 coordinate variables and a
`crs` grid-mapping variable. This enables MODIS data to be combined with other
NEP-readable formats in a single NetCDF workflow.

## Data Access

Public sources for MODIS data include:

* **USGS/NASA LP DAAC:** <https://lpdaac.usgs.gov/> (standard HDF4 products)
* **NASA Earthdata Search:** <https://search.earthdata.nasa.gov/>
* **NASA GIBS:** <https://gibs.earthdata.nasa.gov/> (visualization GeoTIFFs)
* **NASA AppEEARS:** <https://appeears.earthdatacloud.nasa.gov/> (subset and
  export as GeoTIFF)

A free Earthdata login is usually required. A product-specific GeoTIFF sample
must be obtained before example code and the full NetCDF mapping are added.

## Citation

Please cite MODIS datasets in the following manner:

MODIS/Terra+Aqua [Product Name] [Data Set]. NASA EOSDIS Land Processes
Distributed Active Archive Center (LP DAAC), USGS/EROS Center, Sioux Falls,
South Dakota. https://lpdaac.usgs.gov

A product-specific DOI (for example, `https://doi.org/10.5067/MODIS/MOD09GA.061`)
will be added once the exact MODIS product used in the example has been
selected.

## References

1. NASA MODIS Web Site. https://modis.gsfc.nasa.gov/
2. USGS/NASA Land Processes Distributed Active Archive Center (LP DAAC).
   https://lpdaac.usgs.gov/
3. NASA Earthdata Search. https://search.earthdata.nasa.gov/
4. NASA Global Imagery Browse Services (GIBS). https://gibs.earthdata.nasa.gov/

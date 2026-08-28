# Sentinel-3 OLCI Ocean and Land Colour Instrument Products

## Purpose

Understand the European Space Agency (ESA) Sentinel-3 Ocean and Land Colour
Instrument (OLCI), its standard product families, and how to work with them
through the standard NetCDF API in NEP.

Sentinel-3 OLCI products are distributed as NetCDF-4 files inside ESA SAFE
(Standard Archive Format for Europe) packages. They can therefore be opened
directly with `nc_open()` or `netCDF4.Dataset` without requiring a NEP UDF
reader. A standalone Python example under `examples/<mission_short>/` is the
appropriate implementation path.

A real `OL_2_LFR___` granule has been inspected; the NetCDF file layout,
variable names, dimensions, scale factors, and quality-flag meanings below are
derived from that sample.

## When to Use

Use this skill when adding, reviewing, or debugging any NEP example, handler,
or workflow that touches Sentinel-3 OLCI data. It covers the spacecraft, the
OLCI instrument, standard product families, file packaging conventions, and the
NetCDF data-model mapping.

## Required Context

When a user asks about a specific Sentinel-3 OLCI product, identify these items:

| Item | Typical Value | Notes |
|------|---------------|-------|
| Mission | Sentinel-3 | Copernicus Earth observation programme |
| Spacecraft | Sentinel-3A, Sentinel-3B | Tandem constellation |
| Instrument | OLCI | Ocean and Land Colour Instrument |
| Standard products | OL_1_EFR___, OL_2_WFR___, OL_2_LFR___ | Level-1B, Level-2 water, Level-2 land |
| Native format | NetCDF-4 inside SAFE package | No UDF reader required |
| NEP path | `examples/<MISSION_SHORT>/` pure Python | NetCDF-4 / CF-1.x example |
| Primary variables | `GIFAPAR`, `OTCI`, `IWV`, `RC681`, `RC865` (land products); water products use variables such as `Oa01_reflectance`, `chlor_a`, etc. | Product-specific |
| Coordinates | 2-D `latitude[rows, columns]` and `longitude[rows, columns]` in `geo_coordinates.nc`; 1-D `time_stamp[rows]` in `time_coordinates.nc` | Scaled integers for lat/lon |
| Quality/flag masks | `LQSF` (land), `WQSF` (water), `OTCI_quality_flags` | Bit-packed quality flags, product-specific |
| Data access | Copernicus Data Space, EUMETSAT Data Store, CREODIAS, WEkEO | Free; registration required |

## Spacecraft and Orbit

* **Agency:** European Space Agency (ESA) / EUMETSAT
* **Launch:** 2016-02-16 (Sentinel-3A), 2018-04-25 (Sentinel-3B)
* **Orbit:** Sun-synchronous, ~814.5 km mean altitude
* **Inclination:** ~98.65 degrees
* **Repeat cycle:** 27 days per spacecraft
* **Tandem operation:** Sentinel-3A and 3B are phased to improve spatial and
temporal coverage.

## Instrument

OLCI is a five-camera push-broom imaging spectrometer. It is a follow-on to the
MERIS instrument on Envisat.

* **Swath:** ~1270 km
* **Spatial resolution:** 300 m at sub-satellite point (full-resolution products)
* **Spectral range:** ~400 nm to 1020 nm
* **Number of spectral bands:** 21

### OLCI spectral bands

| Band | Centre wavelength (nm) | Bandwidth (nm) | Primary use |
|------|------------------------|----------------|-------------|
| Oa01 | 400.0 | 15.0 | Aerosol correction, water constituents |
| Oa02 | 412.5 | 10.0 | Yellow substance, chlorophyll |
| Oa03 | 442.5 | 10.0 | Chlorophyll absorption maximum |
| Oa04 | 490.0 | 10.0 | Chlorophyll |
| Oa05 | 510.0 | 10.0 | Suspended sediment, chlorophyll |
| Oa06 | 560.0 | 10.0 | Chlorophyll reference |
| Oa07 | 620.0 | 10.0 | Sediment loading |
| Oa08 | 665.0 | 10.0 | Chlorophyll absorption |
| Oa09 | 673.75 | 7.5 | Fluorescence baseline |
| Oa10 | 681.25 | 7.5 | Chlorophyll fluorescence peak |
| Oa11 | 708.75 | 10.0 | Oxygen absorption, red-edge |
| Oa12 | 753.75 | 7.5 | Red-edge, vegetation stress |
| Oa13 | 761.875 | 3.75 | Oxygen A absorption, cloud-top pressure |
| Oa14 | 764.375 | 3.75 | Oxygen A reference |
| Oa15 | 767.5 | 2.5 | Atmospheric correction support |
| Oa16 | 778.75 | 15.0 | Aerosol correction |
| Oa17 | 865.0 | 20.0 | Atmospheric correction, vegetation |
| Oa18 | 885.0 | 10.0 | Water vapour absorption |
| Oa19 | 900.0 | 10.0 | Water vapour, atmospheric correction |
| Oa20 | 940.0 | 20.0 | Water vapour absorption |
| Oa21 | 1020.0 | 40.0 | Atmospheric correction |

## Product Families

Sentinel-3 OLCI products use the following short names (selected list):

| Short name | Level | Description |
|------------|-------|-------------|
| OL_1_EFR___ | 1B | Earth observation full-resolution radiance/reflectance |
| OL_2_WFR___ | 2 | Water full-resolution geophysical products |
| OL_2_LFR___ | 2 | Land full-resolution geophysical products |
| OL_2_WRR___ | 2 | Water reduced-resolution geophysical products |
| OL_2_LRR___ | 2 | Land reduced-resolution geophysical products |

A full-resolution Level-2 granule (`OL_2_WFR___` or `OL_2_LFR___`) is a good
candidate for a NEP Python example because it contains CF-style NetCDF-4
variables that can be plotted directly.

## Inspected Sample Granule

The following Sentinel-3A OLCI Level-2 Land Full Resolution granule has been
examined:

```text
S3A_OL_2_LFR____20260801T112126_20260801T112426_20260801T133209_0179_142_194_1980_PS1_O_NR_003.SEN3
```

* **Spacecraft:** Sentinel-3A
* **Product type:** `OL_2_LFR___`
* **Sensing start / stop:** 2026-08-01 11:21:26Z / 11:24:26Z
* **Processing baseline:** `OL__L2L.003.00.01`
* **Common grid dimensions:** `rows = 4091`, `columns = 4865`

### NetCDF files in the SAFE package

| File | Purpose | Key variables |
|------|---------|---------------|
| `geo_coordinates.nc` | Per-pixel geolocation | `latitude[rows, columns]`, `longitude[rows, columns]`, `altitude[rows, columns]` |
| `time_coordinates.nc` | Per-row timestamps | `time_stamp[rows]` |
| `instrument_data.nc` | Instrument characteristics | `FWHM[bands, detectors]`, `lambda0[bands, detectors]`, `solar_flux[bands, detectors]`, `detector_index[rows, columns]`, `frame_offset[rows, columns]`, `relative_spectral_covariance[bands, bands]` |
| `gifapar.nc` | Green instantaneous fAPAR | `GIFAPAR[rows, columns]`, `GIFAPAR_unc[rows, columns]` |
| `otci.nc` | Terrestrial chlorophyll index | `OTCI[rows, columns]`, `OTCI_unc[rows, columns]`, `OTCI_quality_flags[rows, columns]` |
| `iwv.nc` | Integrated water vapour | `IWV[rows, columns]`, `IWV_unc[rows, columns]` |
| `rc_gifapar.nc` | Rectified reflectances | `RC681[rows, columns]`, `RC865[rows, columns]`, uncertainties |
| `lqsf.nc` | Land quality/science flags | `LQSF[rows, columns]` |
| `tie_geo_coordinates.nc` | Tie-point geolocation | — |
| `tie_geometries.nc` | Tie-point geometry | — |
| `tie_meteo.nc` | Tie-point meteorology | — |

### Scale factors and fill values

Most land geophysical variables are stored as unsigned bytes with a
`scale_factor` and `_FillValue = 255`:

| Variable | File | Type | Scale factor | Units |
|----------|------|------|--------------|-------|
| `GIFAPAR` | `gifapar.nc` | `ubyte` | `0.003937008` | — |
| `GIFAPAR_unc` | `gifapar.nc` | `ubyte` | `0.003937008` | — |
| `OTCI` | `otci.nc` | `ubyte` | `0.02559055` | — |
| `OTCI_unc` | `otci.nc` | `ubyte` | `0.02559055` | — |
| `IWV` | `iwv.nc` | `ubyte` | `0.3` | `kg.m-2` |
| `IWV_unc` | `iwv.nc` | `ubyte` | `0.3` | `kg.m-2` |

The rectified reflectances are stored as unsigned 16-bit integers:

| Variable | File | Type | Scale factor | Units |
|----------|------|------|--------------|-------|
| `RC681` | `rc_gifapar.nc` | `ushort` | `1.525925e-05` | `mW.m-2.sr-1.nm-1` |
| `RC865` | `rc_gifapar.nc` | `ushort` | `1.525925e-05` | `mW.m-2.sr-1.nm-1` |

### Quality flags

`LQSF` in `lqsf.nc` is a 32-bit unsigned integer bit field with meanings
(extracted from the sample metadata):

```text
INVALID WATER LAND COASTLINE CLOUD CLOUD_AMBIGUOUS CLOUD_MARGIN SNOW_ICE
INLAND_WATER TIDAL COSMETIC SUSPECT HISOLZEN SATURATED WV_FAIL GIFAPAR_FAIL
OTCI_FAIL LRAYFAIL GIFAPAR_CLASS_BAD GIFAPAR_CLASS_WS GIFAPAR_CLASS_CSI
GIFAPAR_CLASS_BRIGHT GIFAPAR_CLASS_INVAL_REC OTCI_BAD_IN OTCI_CLASS_CLSN
```

`OTCI_quality_flags` provides additional OTCI-specific masks for soil, angle,
radiometry, and TCI conditions.

## File Packaging and Naming

Sentinel-3 products are delivered in SAFE format. A typical product is a
directory ending in `.SEN3` (or a `.zip`/`.tar` archive) that contains multiple
NetCDF-4 files:

```text
S3A_OL_2_WFR____20240115T120000_20240115T120300_20240116T142030_0180_108_234______LR1_R_NT_003.SEN3/
├── xfdumanifest.xml
├── geo_coordinates.nc
├── tie_geometries.nc
├── tie_meteo.nc
├── WFR/*.nc          # per-variable NetCDF files (variable naming is product-specific)
└── ...
```

The product identifier encodes satellite, sensor, product type, start/stop times,
processing baseline, and orbit/cycle information.

## NetCDF Data Model

OLCI NetCDF-4 files generally follow CF conventions. In the inspected
`OL_2_LFR___` sample:

* **Dimensions:** `rows = 4091`, `columns = 4865` for the full-resolution grid;
  `bands = 21` and `detectors = 3700` for instrument data.
* **Coordinate variables:** `latitude[rows, columns]` and
  `longitude[rows, columns]` are stored as 32-bit scaled integers
  (`scale_factor = 1.e-06`, `_FillValue = -2147483648`) in `geo_coordinates.nc`;
  `time_stamp[rows]` is stored as 64-bit integers in microseconds since
  2000-01-01 in `time_coordinates.nc`.
* **Data variables:** Geophysical variables such as `GIFAPAR`, `OTCI`, `IWV`,
  `RC681`, and `RC865` are stored as unsigned bytes or unsigned 16-bit integers
  with `scale_factor` and `_FillValue` attributes.
* **Quality variables:** `LQSF` in `lqsf.nc` is a 32-bit unsigned integer bit field
  carrying land/cloud/snow/quality/science-class flags. Water products use a
  comparable `WQSF` variable.
* **Global attributes:** Each NetCDF file carries `title`, `institution`,
  `source`, `history`, `product_name`, `start_time`, `stop_time`,
  `processing_baseline`, and `references`.

An example reader opens the `xfdumanifest.xml` to find the relevant NetCDF files,
loads the desired data variable together with its 2-D latitude/longitude arrays,
applies the `scale_factor`, and masks with the bit-packed quality flags.

## Data Access

Public sources for Sentinel-3 OLCI data:

* **Copernicus Data Space Ecosystem:** <https://dataspace.copernicus.eu/>
* **EUMETSAT Data Store:** <https://data.eumetsat.int/>
* **CREODIAS:** <https://creodias.eu/>
* **WEkEO:** <https://www.wekeo.eu/>

A free Copernicus / EUMETSAT account is usually required.

## Citation

Please cite Sentinel-3 data in the following manner:

Copernicus Sentinel data [Year]. Sentinel-3 OLCI Ocean and Land Colour
Instrument [Data set]. European Space Agency.
https://dataspace.copernicus.eu/

A product-specific DOI will be added once the exact OLCI product used in the
example has been selected.

## References

1. ESA Sentinel-3 mission page.
   https://www.esa.int/Applications/Observing_the_Earth/Copernicus/Sentinel-3
2. EUMETSAT Sentinel-3 OLCI page. https://www.eumetsat.int/sentinel-3-olci
3. Copernicus Data Space Ecosystem. https://dataspace.copernicus.eu/
4. Sentinel-3 OLCI User Guide.
   https://sentinels.copernicus.eu/web/sentinel/user-guides/sentinel-3-olci

# NetCDF with Sentinel-3 OLCI Ocean and Land Colour Products

The Ocean and Land Colour Instrument (OLCI) is a push-broom imaging spectrometer
flown on the European Space Agency (ESA) Sentinel-3A and Sentinel-3B satellites.
OLCI provides medium-resolution measurements of ocean, land, and atmospheric
properties in 21 visible, near-infrared, and short-wave infrared spectral bands.

Sentinel-3 OLCI products are distributed in ESA's SAFE (Standard Archive Format
for Europe) packages as NetCDF-4 files. They can therefore be opened directly
with the standard NetCDF-C / `netCDF4.Dataset` API without requiring a NEP UDF
reader.

A real sample granule has been inspected for this document. The exact NetCDF
file layout, variable names, dimensions, scale factors, and quality-flag
meanings below come from that granule. Sample code and figures will be added in
a follow-up step.

## Mission Overview

Sentinel-3 is part of the European Copernicus programme. The two spacecraft,
Sentinel-3A and Sentinel-3B, fly in a tandem configuration to improve coverage
and revisit.

* **Agency:** European Space Agency (ESA) / EUMETSAT  
* **Launch:** 2016-02-16 (Sentinel-3A), 2018-04-25 (Sentinel-3B)  
* **Orbit:** Sun-synchronous, ~814.5 km mean altitude  
* **Inclination:** ~98.65 degrees  
* **Repeat cycle:** 27 days per spacecraft; combined Sentinel-3A/3B tandem
  configuration improves revisit for most latitudes.

## Instrument

OLCI is a five-camera push-broom imaging spectrometer derived from Envisat's
MERIS instrument.

* **Swath:** ~1270 km (with a small gap between cameras that is corrected in
  ground processing)  
* **Spatial resolution:** 300 m at sub-satellite point for full-resolution products  
* **Spectral range:** ~400 nm to 1020 nm  
* **Number of bands:** 21

### OLCI spectral bands

| Band | Centre wavelength (nm) | Bandwidth (nm) | Common use |
|------|------------------------|----------------|------------|
| Oa01 | 400.0 | 15.0 | Aerosol correction, water constituents |
| Oa02 | 412.5 | 10.0 | Yellow substance / chlorophyll |
| Oa03 | 442.5 | 10.0 | Chlorophyll absorption maximum |
| Oa04 | 490.0 | 10.0 | Chlorophyll |
| Oa05 | 510.0 | 10.0 | Suspended sediment, chlorophyll |
| Oa06 | 560.0 | 10.0 | Chlorophyll reference |
| Oa07 | 620.0 | 10.0 | Sediment loading |
| Oa08 | 665.0 | 10.0 | Chlorophyll absorption |
| Oa09 | 673.75 | 7.5 | Fluorescence baseline |
| Oa10 | 681.25 | 7.5 | Chlorophyll fluorescence peak |
| Oa11 | 708.75 | 10.0 | Oxygen absorption / red-edge |
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

Common Sentinel-3 OLCI product families available from EUMETSAT / Copernicus:

* **OL_1_EFR___** — Level-1B Earth Observation Full Resolution (top-of-atmosphere
  radiance/reflectance).
* **OL_2_WFR___** — Level-2 Water Full Resolution (water-leaving reflectance,
  chlorophyll concentration, total suspended matter, etc.).
* **OL_2_LFR___** — Level-2 Land Full Resolution (vegetation indices, terrestrial
  chlorophyll, etc.).
* **OL_2_WRR___ / OL_2_LRR___** — Reduced-resolution versions of the water and
  land products.

Products are packaged in SAFE format, typically as a directory (`.SEN3`) or a
`.zip`/`.tar` archive containing multiple NetCDF-4 files: one per measurement,
plus geo-coordinates, instrument data, and quality information.

## Relevance to NEP

Because Sentinel-3 OLCI products are native NetCDF-4, they are a natural fit for
NEP's standalone Python example path. An example can open an OLCI product with
`xarray` or `netCDF4`, apply quality flags, select the relevant spectral band or
derived geophysical variable, and produce a publication-style map with
`matplotlib` and `cartopy`. No UDF reader or format conversion is required.

## Data Access

Public sources for Sentinel-3 OLCI data include:

* **Copernicus Data Space Ecosystem:** <https://dataspace.copernicus.eu/>
* **EUMETSAT Data Store:** <https://data.eumetsat.int/>
* **CREODIAS:** <https://creodias.eu/>
* **WEkEO:** <https://www.wekeo.eu/>

A free Copernicus / EUMETSAT account is usually required. A Level-2 water or
land full-resolution granule (`OL_2_WFR___` or `OL_2_LFR___`) is a suitable
sample for an NEP example. Only a real downloaded granule will be used for code
and figures.

## Inspected Sample Granule

The following Sentinel-3A OLCI Level-2 Land Full Resolution granule was examined:

```text
S3A_OL_2_LFR____20260801T112126_20260801T112426_20260801T133209_0179_142_194_1980_PS1_O_NR_003.SEN3
```

* **Spacecraft:** Sentinel-3A
* **Product type:** `OL_2_LFR___` — Level-2 Land Full Resolution
* **Sensing start / stop:** 2026-08-01 11:21:26Z / 11:24:26Z
* **Processing baseline:** `OL__L2L.003.00.01`
* **Source processor:** `IPF-OL-2 06.21`
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
| `rc_gifapar.nc` | Rectified reflectances | `RC681[rows, columns]`, `RC865[rows, columns]`, uncertainty counterparts |
| `lqsf.nc` | Land quality/science flags | `LQSF[rows, columns]` |
| `tie_geo_coordinates.nc` | Tie-point geolocation | — |
| `tie_geometries.nc` | Tie-point geometry | — |
| `tie_meteo.nc` | Tie-point meteorology | — |

### Coordinates

`geo_coordinates.nc` stores 2-D latitude and longitude as scaled 32-bit integers:

```text
int latitude(rows, columns) ;
    latitude:_FillValue = -2147483648 ;
    latitude:scale_factor = 1.e-06 ;
    latitude:units = "degrees_north" ;
    latitude:valid_min = -90000000 ;
    latitude:valid_max = 90000000 ;
int longitude(rows, columns) ;
    longitude:_FillValue = -2147483648 ;
    longitude:scale_factor = 1.e-06 ;
    longitude:units = "degrees_east" ;
    longitude:valid_min = -180000000 ;
    longitude:valid_max = 180000000 ;
```

The `time_coordinates.nc` file stores one timestamp per row:

```text
int64 time_stamp(rows) ;
    time_stamp:units = "microseconds since 2000-01-01 00:00:00" ;
```

### Data variables and scale factors

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

The land quality and science flag file (`lqsf.nc`) contains a single 32-bit
unsigned integer variable `LQSF` with the following bit-packed flag meanings
(extracted from the sample metadata):

| Bit | Flag meaning |
|-----|--------------|
| 1 | `INVALID` |
| 2 | `WATER` |
| 4 | `LAND` |
| 8 | `COASTLINE` |
| 16 | `CLOUD` |
| 32 | `CLOUD_AMBIGUOUS` |
| 64 | `CLOUD_MARGIN` |
| 128 | `SNOW_ICE` |
| 256 | `INLAND_WATER` |
| 512 | `TIDAL` |
| 1024 | `COSMETIC` |
| 2048 | `SUSPECT` |
| 4096 | `HISOLZEN` |
| 8192 | `SATURATED` |
| 16384 | `WV_FAIL` |
| 32768 | `GIFAPAR_FAIL` |
| 65536 | `OTCI_FAIL` |
| 131072 | `LRAYFAIL` |
| 262144 | `GIFAPAR_CLASS_BAD` |
| 524288 | `GIFAPAR_CLASS_WS` |
| 1048576 | `GIFAPAR_CLASS_CSI` |
| 2097152 | `GIFAPAR_CLASS_BRIGHT` |
| 4194304 | `GIFAPAR_CLASS_INVAL_REC` |
| 8388608 | `OTCI_BAD_IN` |
| 16777216 | `OTCI_CLASS_CLSN` |

`OTCI_quality_flags` in `otci.nc` provides additional per-pixel OTCI-specific
flags with masks `1`, `2`, `16`, `32`, `64`, `128` for soil, angle, radiometry,
and TCI conditions.

### Instrument data

`instrument_data.nc` stores OLCI calibration information shared by all pixels:

```text
dimensions:
    bands = 21 ;
    detectors = 3700 ;
    columns = 4865 ;
    rows = 4091 ;
variables:
    float FWHM(bands, detectors) ;
    float lambda0(bands, detectors) ;
    float solar_flux(bands, detectors) ;
    float relative_spectral_covariance(bands, bands) ;
    short detector_index(rows, columns) ;
    byte frame_offset(rows, columns) ;
```

## Citation

Please cite Sentinel-3 data in the following manner:

Copernicus Sentinel data [Year]. Sentinel-3 OLCI Ocean and Land Colour
Instrument [Data set]. European Space Agency.
https://dataspace.copernicus.eu/

A product-specific DOI (for example, for `OL_2_WFR___`) will be added once the
exact OLCI product used in the example has been selected.

## References

1. ESA Sentinel-3 mission page. https://www.esa.int/Applications/Observing_the_Earth/Copernicus/Sentinel-3
2. EUMETSAT Sentinel-3 OLCI page. https://www.eumetsat.int/sentinel-3-olci
3. Copernicus Data Space Ecosystem. https://dataspace.copernicus.eu/
4. Sentinel-3 OLCI User Guide. https://sentinels.copernicus.eu/web/sentinel/user-guides/sentinel-3-olci

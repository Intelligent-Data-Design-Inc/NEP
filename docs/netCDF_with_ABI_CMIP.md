# netCDF with ABI: Finding Cloud and Moisture Imagery

*By Edward Hartnett, Intelligent Data Design, Inc.*  
*27 August 2026*

ABI (the Advanced Baseline Imager) is the geostationary imager on the
NOAA GOES-R series of satellites.  Its standard Level 2 Cloud and
Moisture Imagery Product (CMIP) is a Network Common Data Form (netCDF)
version 4 file that is also Climate and Forecast (CF) 1.7 compliant.  You
can open one with `nc_open()`, `ncdump`, `netCDF4`, or `xarray`, exactly
as you would any other netCDF-4 dataset.  You can also open the same
file with plain HDF5 tools, because netCDF-4 is HDF5 underneath.

This document walks through a real GOES-16 ABI CMIP CONUS granule two
ways: through the netCDF-4 view (`ncdump`) and through a short Python
example.  The goal is to show exactly where the cloud and moisture
imagery lives and how to read it without a special reader.  The file
used was downloaded from the NOAA GOES AWS Open Data registry:

```
OR_ABI-L2-CMIPC-M6C01_G16_s20241830001180_e20241830003553_c20241830004036.nc
```

NEP (the NetCDF Expansion Pack) is a superset of netCDF-C that adds
compression filters and read layers for scientific data formats netCDF
does not natively understand.  ABI CMIP files need none of that: they
are already CF-1.7 netCDF-4 files, so a plain `nc_open()` or `ncdump`
opens them directly.  But NEP ships a working, standalone Python
example that opens an ABI CMIP file and plots the `CMI` variable on the
native geostationary projection, in `examples/abi/`; see
`examples/abi/README.md`.  Code from that example is used throughout this
document.

---

## The GOES-R Program

GOES-R is the National Oceanic and Atmospheric Administration (NOAA)
next-generation geostationary weather satellite program, a collaboration
with NASA for spacecraft and instrument development.  The series replaces
the legacy GOES-N/O/P satellites with far more capable imagers,
lightning mappers, solar irradiance monitors, and space-weather sensors.
The operational constellation normally consists of two spacecraft: one
over the Atlantic Ocean at approximately 75.2° W (GOES-East) and one over
the Pacific Ocean at approximately 137.0° W (GOES-West).  Together they
provide continuous, full-disk weather surveillance for North and South
America, the Atlantic, and the Pacific.

| Satellite | Launch | Operational role | Nominal longitude |
|---|---|---|---|
| GOES-16 | 19 November 2016 | GOES-East | ~75.2° W |
| GOES-17 | 1 March 2018 | GOES-West (February 2019 – January 2023) | ~137.0° W |
| GOES-18 | 1 March 2022 | GOES-West (January 2023–present) | ~136.8° W |
| GOES-19 | 25 June 2024 | On-orbit spare / future GOES-West | ~136.8° W |

GOES-17 served as GOES-West from early 2019 through early 2023.  During
that period an ABI loop-heat-pipe anomaly reduced the cooling efficiency
for some infrared channels at certain local times, so NOAA kept GOES-17 in
operations while preparing GOES-18 as its replacement.  GOES-18 assumed
the GOES-West role in January 2023; GOES-17 was subsequently moved to a
backup/spare orbital slot.

### Orbit and Spacecraft

All GOES-R spacecraft fly in geostationary orbit at approximately
35,786 km above the equator, matching Earth's rotation so that each
satellite remains fixed over one longitude.  Key orbital and spacecraft
facts:

* **Altitude:** ~35,786 km (geostationary).
* **Orbital period:** one sidereal day (~23 hours 56 minutes).
* **Inclination:** near-zero (small inclination is maintained by
  station-keeping maneuvers).
* **Spacecraft bus:** based on Lockheed Martin's A2100 bus design.
* **Launch mass:** roughly 5,200 kg.
* **Power:** two solar arrays producing about 4 kW of electrical power at
  end of life, with batteries for eclipse periods.
* **Design life:** at least 15 years (5 years on-orbit storage plus 10
  years operational).
* **Three-axis stabilized:** the spacecraft points its instruments toward
  Earth with high precision; ABI is pointed and scanned by an
  instrument-mounted two-axis scan mirror.
* **Data downlink:** science data are downlinked in Ka-band to NOAA
  ground stations at Wallops, Virginia, and Fairmont, West Virginia.  A
  second Wallops receive site and a retransmission via NOAA's GOES
  Rebroadcast (GRB) provide real-time relay to users worldwide.

### Instruments on the GOES-R Series

ABI is the best-known instrument, but GOES-R carries a suite of sensors
for weather, space weather, and solar monitoring:

1. **Advanced Baseline Imager (ABI)** — the primary Earth imager, a
   16-channel scanning radiometer that produces visible and infrared
   imagery used for severe-weather warnings, hurricane tracking, aviation,
   fog and low-cloud detection, volcanic ash monitoring, and many other
   applications.  ABI channels 1–6 are reflected solar bands; channels 7–16
   are emitted infrared bands.

2. **Geostationary Lightning Mapper (GLM)** — a near-infrared optical
   transient detector that maps total lightning (in-cloud and cloud-to-
   ground) over the Americas and adjacent oceans at 8 km nadir resolution.
   GLM data feed short-term severe-storm and tornado warning systems
   because rapid increases in total lightning often precede severe
   weather at the ground.

3. **Solar Ultraviolet Imager (SUVI)** — a telescope that observes the Sun
   in extreme ultraviolet (EUV) wavelengths.  SUVI images the solar
   corona, monitors solar flares, and detects coronal mass ejections
   (CMEs) that can cause geomagnetic storms at Earth.

4. **Extreme Ultraviolet and X-ray Irradiance Sensors (EXIS)** — measures
   solar irradiance in the EUV and X-ray bands.  These measurements are
   inputs to space-weather models that predict ionospheric disturbances
   and radio blackouts.

5. **Space Environment In-Situ Suite (SEISS)** — a set of particle
   detectors that measures electrons, protons, and heavy ions in the
   satellite's local space environment.  SEISS data warn of radiation
   hazards to astronauts and spacecraft electronics.

6. **Magnetometer (MAG)** — measures the strength and orientation of
   Earth's magnetic field near geostationary orbit.  Sudden changes in the
   field are signatures of geomagnetic storms and substorms.

7. **Compact Coronagraph (CCOR)** — carried on GOES-19 and later
   spacecraft, CCOR images the solar corona in visible light to detect
   CMEs heading toward Earth, complementing SUVI and ground-based
   coronagraph data.

### ABI Capabilities in More Detail

ABI observes Earth in 16 spectral bands:

| Channel | Wavelength | Typical use | Nominal resolution |
|---|---|---|---|
| 1 | 0.47 µm | Blue visible, aerosol/snow | 0.5 km |
| 2 | 0.64 µm | Red visible, clouds/fog | 0.5 km |
| 3 | 0.86 µm | Vegetation, low cloud/fog | 1 km |
| 4 | 1.37 µm | Cirrus detection | 1 km |
| 5 | 1.6 µm | Snow/ice, cloud phase | 1 km |
| 6 | 2.2 µm | Cloud particle size | 1 km |
| 7 | 3.9 µm | Shortwave IR, fog/fire | 2 km |
| 8 | 6.2 µm | Upper-level water vapor | 2 km |
| 9 | 6.9 µm | Mid-level water vapor | 2 km |
| 10 | 7.3 µm | Lower-level water vapor | 2 km |
| 11 | 8.4 µm | Cloud-top phase | 2 km |
| 12 | 9.6 µm | Ozone absorption | 2 km |
| 13 | 10.3 µm | Clean longwave IR | 2 km |
| 14 | 11.2 µm | Longwave IR | 2 km |
| 15 | 12.3 µm | Longwave IR, SO₂ | 2 km |
| 16 | 13.3 µm | CO₂ absorption, cloud heights | 2 km |

ABI scans the full Earth disk about every 5–15 minutes, the CONUS sector
about every 5 minutes, and two mesoscale sectors about every 1 minute.
Scan schedules are flexible and can be adjusted by NOAA for rapidly
evolving weather events such as hurricanes or severe thunderstorm
outbreaks.

### ABI CMIP Products

The Level 2 Cloud and Moisture Imagery Product family (CMIP) contains
calibrated, single-channel imagery on the ABI fixed grid.  The three
sector products are:

* `ABI-L2-CMIPF` — Full Disk
* `ABI-L2-CMIPC` — CONUS
* `ABI-L2-CMIPM1` / `ABI-L2-CMIPM2` — Mesoscale sectors 1 and 2

Each file contains at least the variables:

* `CMI` — the Cloud and Moisture Imagery variable (reflectance factor
  for visible/near-infrared channels, brightness temperature for
  infrared channels).
* `DQF` — data quality flags.
* `x`, `y` — 1-D fixed-grid coordinates in radians.
* `goes_imager_projection` — scalar grid-mapping variable holding the
  geostationary projection parameters.

## Ground Data Processing and Other GOES-R Products

GOES-R raw instrument telemetry is received at NOAA ground stations and
routed to the NOAA/National Environmental Satellite, Data, and
Information Service (NESDIS) Product Generation Centers.  There the data
are decoded, calibrated, navigated, and packaged into standard product
families.  Processing is organized into the familiar NASA/NESDIS levels:

* **Level 0** — raw, time-ordered telemetry from the spacecraft.
* **Level 1b** — calibrated, navigated radiance data for each instrument.
  For ABI, this is the `ABI-L1b-Rad` product family, containing
  geolocated radiances for each of the 16 channels.
* **Level 2+** — derived geophysical products such as cloud and moisture
  imagery (CMIP), cloud-top properties, aerosol detection, precipitation,
  fire, land surface temperature, and derived motion winds.

Important ABI-derived product families (all NetCDF-4/CF) include:

* **Aerosol Detection (`ABI-L2-ADP`)** — identifies smoke, dust, and
  volcanic ash.
* **Aerosol Optical Depth (`ABI-L2-AOD`)** — quantifies aerosol loading
  over land and ocean.
* **Cloud Top Height / Temperature / Pressure (`ABI-L2-ACHA`)** —
  estimates the altitude and thermodynamic state of cloud tops.
* **Clear Sky Mask (`ABI-L2-ACMC/F/M`)** — classifies each pixel as
  clear or cloudy, used as input to many downstream retrievals.
* **Derived Motion Winds (`ABI-L2-DMW`)** — atmospheric motion vectors
  computed by tracking cloud and water-vapor features over time.
* **Fire / Hot Spot Characterization (`ABI-L2-FDC`)** — detects and
  characterizes active wildfires.
* **Land Surface Temperature (`ABI-L2-LST`)** — estimates skin
  temperature over clear land.
* **Rainfall Rate / QPE (`ABI-L2-RRQPE`)** — estimates precipitation from
  infrared-window brightness temperatures.
* **Total Precipitable Water (`ABI-L2-TPW`)** — column-integrated water
  vapor.
* **Volcanic Ash (`ABI-L2-VAA/Vol`)** — detects and quantifies volcanic
  ash in the atmosphere.

These products feed operational weather forecast models, aviation
hazard warnings, wildfire response, air-quality forecasts, and climate
monitoring.  The NEP example focuses on `ABI-L2-CMIP` because it is the
simplest, most widely used single-channel imagery product and is ideal
for demonstrating how a geostationary gridded image maps onto the NetCDF-4
data model.

---

## An ABI CMIP File in netCDF-4

The file we use here is a GOES-16 CONUS channel 01 (blue visible)
granule from 2024-07-01 00:01 UTC.  It is a plain NetCDF-4 file.

```bash
$ ncdump -h OR_ABI-L2-CMIPC-M6C01_G16_s20241830001180_e20241830003553_c20241830004036.nc
netcdf OR_ABI-L2-CMIPC-M6C01_G16_s20241830001180_e20241830003553_c20241830004036 {
dimensions:
	y = 3000 ;
	x = 5000 ;
	...
variables:
	short CMI(y, x) ;
		CMI:_FillValue = -1s ;
		CMI:long_name = "ABI L2+ Cloud and Moisture Imagery reflectance factor" ;
		CMI:standard_name = "toa_lambertian_equivalent_albedo_multiplied_by_cosine_solar_zenith_angle" ;
		CMI:_Unsigned = "true" ;
		CMI:valid_range = 0s, 4095s ;
		CMI:scale_factor = 0.00031746f ;
		CMI:add_offset = 0.f ;
		CMI:units = "1" ;
		CMI:coordinates = "band_id band_wavelength t y x" ;
		CMI:grid_mapping = "goes_imager_projection" ;
		CMI:ancillary_variables = "DQF" ;
	byte DQF(y, x) ;
		...
	double t ;
		...
	short y(y) ;
		y:scale_factor = -2.8e-05f ;
		y:add_offset = 0.128226f ;
		y:units = "rad" ;
		y:axis = "Y" ;
		y:long_name = "GOES fixed grid projection y-coordinate" ;
		y:standard_name = "projection_y_coordinate" ;
	short x(x) ;
		x:scale_factor = 2.8e-05f ;
		x:add_offset = -0.101346f ;
		x:units = "rad" ;
		x:axis = "X" ;
		x:long_name = "GOES fixed grid projection x-coordinate" ;
		x:standard_name = "projection_x_coordinate" ;
	int goes_imager_projection ;
		goes_imager_projection:long_name = "GOES-R ABI fixed grid projection" ;
		goes_imager_projection:grid_mapping_name = "geostationary" ;
		goes_imager_projection:perspective_point_height = 35786023. ;
		goes_imager_projection:semi_major_axis = 6378137. ;
		goes_imager_projection:semi_minor_axis = 6356752.31414 ;
		goes_imager_projection:inverse_flattening = 298.2572221 ;
		goes_imager_projection:latitude_of_projection_origin = 0. ;
		goes_imager_projection:longitude_of_projection_origin = -75. ;
		goes_imager_projection:sweep_angle_axis = "x" ;
	...
```

The dimensions `y = 3000` and `x = 5000` are the CONUS grid at the
channel 01 spatial sampling.  The `CMI` variable is stored as unsigned
short integers with a scale factor of `0.00031746`, while `x` and `y`
are stored as scaled short integers whose decoded values are radians from
the geostationary sub-point.  The `goes_imager_projection` variable is a
scalar integer whose attributes carry all the parameters needed to map
those radian angles to geodetic latitude and longitude.

### Mapping the ABI Fixed Grid to Earth

The `goes_imager_projection` attributes define a geostationary
projection centered on `longitude_of_projection_origin = -75.0` degrees
(GOES-16's nominal sub-point).  The `x` and `y` values are scan angles in
radians from that sub-point.  To plot in a geographic context, multiply
the radian values by `perspective_point_height` to obtain map
projection coordinates in meters, then use a geostationary projection
with the same satellite height, central longitude, ellipsoid axes, and
sweep axis (`x` for ABI).  This is exactly what the NEP Python example
does with `cartopy`.

The `DQF` variable provides per-pixel quality information.  For most
applications the `good_pixel_qf` value (`0`) should be used; higher
values mark conditionally usable, out-of-range, or missing pixels.

---

## Reading the Data with Python

The NEP example (`examples/abi/abi_example/reader.py`) opens the file
with `xarray`, applies the `DQF` mask, and attaches the 1-D `x` and `y`
coordinates:

```python
import xarray as xr

with xr.open_dataset(path, engine="netcdf4", decode_times=False) as ds:
    cmi = ds["CMI"].copy()
    x = ds["x"]
    y = ds["y"]
    cmi = cmi.assign_coords(x=x, y=y)
    dqf = ds["DQF"].values
    cmi = cmi.where(dqf == 0)
```

Plotting uses `cartopy`'s `Geostationary` projection with the parameters
from `goes_imager_projection`:

```python
import cartopy.crs as ccrs
import matplotlib.pyplot as plt

sat_h = 35786023.0
sat_lon = -75.0
a = 6378137.0
b = 6356752.31414
globe = ccrs.Globe(semimajor_axis=a, semiminor_axis=b, flattening=None)
proj = ccrs.Geostationary(
    central_longitude=sat_lon,
    satellite_height=sat_h,
    sweep_axis="x",
    globe=globe,
)

x_m = cmi["x"].values * sat_h
y_m = cmi["y"].values * sat_h
extent = [x_m.min(), x_m.max(), y_m.min(), y_m.max()]

fig, ax = plt.subplots(figsize=(10, 10), subplot_kw={"projection": proj})
ax.imshow(cmi.values, origin="upper", extent=extent, transform=proj)
ax.coastlines(resolution="50m")
plt.show()
```

The same code is wrapped into the `python -m abi_example` CLI documented
in `examples/abi/README.md`.

## Reading the Data with C

Because the file is a normal NetCDF-4 file, it opens directly with the
NetCDF-C API:

```c
#include <netcdf.h>

int ncid, varid, status;
float *cmi;
size_t nx, ny;

status = nc_open("OR_ABI-L2-CMIPC-M6C01_G16_....nc", NC_NOWRITE, &ncid);

nc_inq_dimid(ncid, "y", &ydimid);
nc_inq_dimid(ncid, "x", &xdimid);
nc_inq_dimlen(ncid, ydimid, &ny);
nc_inq_dimlen(ncid, xdimid, &nx);

cmi = (float *) malloc(nx * ny * sizeof(float));
nc_inq_varid(ncid, "CMI", &varid);
nc_get_var_float(ncid, varid, cmi);

nc_close(ncid);
```

The `CMI` variable's `_FillValue` and `valid_range` attributes (and the
`DQF` flag) should be consulted to identify valid pixels.

---

## ABI and Other Geostationary Imagers

The same NetCDF-4/CF structure is used by several geostationary imager
products.  While this example targets NOAA GOES-R ABI CMIP, the general
pattern — a 2-D geophysical array on a geostationary fixed grid with `x`
and `y` radian coordinates and a `grid_mapping` scalar variable — also
appears in:

* EUMETSAT MTG Flexible Combined Imager (FCI) products
* KMA GEO-KOMPSAT-2A Advanced Meteorological Imager (AMI) products
* JAXA Himawari-8/9 Advanced Himawari Imager (AHI) products

Each mission uses its own central longitude, sector naming, and channel
definitions, but the underlying NetCDF data model is the same.

---

## References

* NOAA GOES-R Program:
  <https://www.goes-r.gov/>
* GOES-R Spacecraft and Instruments:
  <https://www.goes-r.gov/spacesegment/spacesegment.html>
* GOES-R ABI (Advanced Baseline Imager):
  <https://www.goes-r.gov/mission/ABI.html>
* GOES-R Cloud and Moisture Imagery Product (CMIP) PUG
  (Product Definition and User Guide):
  <https://www.goes-r.gov/products/abi.html>
* NOAA GOES AWS Open Data Registry:
  <https://registry.opendata.aws/noaa-goes/>
* NOAA GOES-R Data Users' Guides and Algorithm Theoretical Basis
  Documents (ATBDs):
  <https://www.goes-r.gov/products/data-products.html>
* NetCDF Climate and Forecast (CF) Metadata Conventions:
  <https://cfconventions.org/>
* CF Geostationary Projection:
  <https://cfconventions.org/Data/cf-conventions/cf-conventions-1.10/cf-conventions.html#_geostationary_projection>

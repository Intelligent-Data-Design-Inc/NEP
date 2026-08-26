# netCDF with SWOT: Finding Sea Surface Height Data

*By Edward Hartnett, Intelligent Data Design, Inc.*  
*26 August 2026*

SWOT (the Surface Water and Ocean Topography mission) is a joint
NASA/CNES mission that uses a Ka-band radar interferometer (KaRIn) to
measure sea surface height and its anomaly. The standard Level 2 Low
Rate Sea Surface Height (L2_LR_SSH) products are NetCDF-4 files. They
are also Climate and Forecast (CF) 1.7 compliant.

This document walks through a real SWOT L2_LR_SSH Basic granule through
the netCDF-4 view (`ncdump`) and through a short Python example. The
goal is to show where the sea surface height anomaly lives and how to
read it without a special reader. The file used was downloaded from NASA Earth Data site (earthdata.nasa.gov):

```
SWOT_L2_LR_SSH_Basic_032_140_20250503T000222_20250503T005350_PIC2_01.nc
```

NEP (the NetCDF Expansion Pack) is a superset of netCDF-C that adds
compression filters and read layers for scientific data formats netCDF
does not natively understand. NEP also includes a working, standalone Python
example that opens a SWOT SSH file and plots SSHA, in `examples/swot/`;
see `examples/swot/README.md`. Code from that example is used below.

---

## The SWOT Mission, Spacecraft, and Instrument

SWOT (Surface Water and Ocean Topography) is a joint mission of NASA and
the French space agency CNES, with contributions from the Canadian Space
Agency (CSA) and the UK Space Agency (UKSA). Its goal is the first
near-global survey of Earth's surface water, including both ocean
mesoscale and sub-mesoscale circulation and continental surface water.
The satellite was launched on 16 December 2022 from Vandenberg Space
Force Base and entered its final science orbit in July 2023.

### Spacecraft and Orbit

The SWOT spacecraft is built around a payload that must simultaneously
support a 10-meter radar interferometer boom and a suite of conventional
altimetry instruments. In its science orbit, SWOT flies at an altitude
of roughly 890 km in a non-sun-synchronous, near-polar orbit with an
inclination of about 77.6°. The orbit repeats every 21 days, which means
a given location is revisited on the same pass every three weeks. That
repeat period is a compromise: it is short enough to track evolving
ocean eddies and long enough to cover the entire globe between
successive tracks. Because the orbit is not sun-synchronous, the local
time of observation changes slowly, which helps reduce aliasing of tidal
signals into the height measurements.

### The KaRIn Instrument

The mission's key science instrument is the Ka-band Radar
Interferometer, or KaRIn. It operates at a center frequency of 35.75 GHz
(wavelength ~8.4 mm). KaRIn has two synthetic-aperture radar antennas
mounted at opposite ends of a 10-meter boom, giving the two antennas a
known, fixed physical separation called the *baseline*. Each antenna is
about 5 m long and 0.25 m wide. The boom is pointed so that the two
antennas look slightly off to either side of the satellite's ground
track.

A radar pulse is transmitted and the return signal is received by both
antennas. Because the two antennas are separated by the baseline, the
same patch of ocean is viewed from two slightly different angles. The
phase difference between the two received echoes is measured, and from
that phase difference the instrument can determine the angle of arrival
and therefore the distance to the surface with high precision. This is
the same interferometric principle used by some airborne and spaceborne
SAR systems, but tuned for fine-scale ocean height.

### Why Two Swaths?

Because the antennas look off to the sides of the spacecraft rather than
straight down, KaRIn observes two separate strips, or *swaths*, on
either side of the nadir track. The right swath is observed with VV
polarization, the left with HH. Each swath covers cross-track distances
from about 10 km to 60 km from nadir, so the total KaRIn swath is about
120 km wide. The region directly beneath the satellite, within a few
kilometers of the nadir track, is not observed by the interferometer;
this is the *nadir gap*. It is filled instead by the conventional nadir
altimeter also carried on SWOT, but that coverage is not what you see in
the KaRIn-only `L2_LR_SSH` Basic product.

In the NetCDF file, the `num_sides = 2` dimension encodes this
left/right geometry, and the `num_pixels` dimension spans the full
cross-track grid: left swath, nadir gap (fill or masked values), and
right swath. The `num_lines` dimension is the along-track direction. At
the Basic data rate, the product is averaged to a nominal 2 km x 2 km
grid. When the data are plotted, the two swaths appear as two parallel
ribbons of SSHA color with a central blank stripe where the nadir gap
lies.

### The Full SWOT Instrument Suite

SWOT is not a single-instrument mission. The payload has six science
instruments, grouped into the KaRIn module and the nadir payload module.
Together they produce the measurements needed to turn raw radar echoes
into accurate sea surface height.

1. **KaRIn** — the wide-swath Ka-band SAR interferometer described
   above. It provides the two off-nadir swaths and is the mission's
   headline innovation.

2. **Poseidon-3C nadir altimeter** — a conventional Jason-class
   dual-frequency radar altimeter that points straight down. It operates
   in C-band (5.3 GHz) and Ku-band (13.575 GHz) and measures the nadir
   gap that KaRIn cannot see, giving the mission a continuous height
   reference along the satellite track.

3. **AMR-S (Advanced Microwave Radiometer)** — a three-frequency passive
   microwave radiometer (18.7, 23.8, and 34 GHz). It measures the amount
   of water vapor in the atmosphere so the radar range can be corrected
   for wet-troposphere delay.

4. **DORIS (Doppler Orbitography and Radiopositioning Integrated by
   Satellite)** — a French radio receiver that listens to ground beacons
   around the world to determine the spacecraft's orbit with
   centimeter-level accuracy.

5. **GPSP (GPS Payload)** — GPS receivers that use the NAVSTAR GPS
   constellation for the same purpose: precise orbit determination.

6. **LRA (Laser Retroreflector Array)** — a set of mirrors that reflect
   laser pulses from ground stations back to the station, providing an
   independent, highly precise measurement of the satellite's position.

The nadir altimeter, radiometer, DORIS, GPS, and LRA are the classic
supporting suite that has flown on every high-precision ocean altimetry
mission since TOPEX/Poseidon. KaRIn is the new addition that turns one
narrow altimeter track into a 120-km-wide swath.

---

## SWOT Data Processing Levels

Like most NASA/CNES Earth-observing missions, SWOT data are organized
into processing levels. Each level is more refined than the last, moving
from raw instrument voltages to calibrated geophysical quantities.

- **Level 0** — raw telemetry as received from the spacecraft. These are
  not geolocated or calibrated and are generally not useful to end
  users.

- **Level 1** — reformatted, time-ordered, and partially calibrated
  instrument data. Level-1 KaRIn products contain the raw SAR
  interferograms on the native ~250 m grid. They are large and still
  close to the instrument.

- **Level 2** — geophysical products derived from Level-1 data. The
  example in this document uses the **L2_LR_SSH** product family:
  geolocated, corrected sea surface height and sea surface height
  anomaly from KaRIn. L2_LR_SSH has four sub-products:

  - **Basic** — the smallest file, with SSH, SSHA, quality flags, and
    key corrections on a 2 km grid.
  - **Expert** — Basic plus additional instrument and environmental
    corrections, radiometer data, and geophysical models.
  - **WindWave** — significant wave height, sigma0, wind speed, and wave
    model information.
  - **Unsmoothed** — SSH and sigma0 on the native ~250 m grid, split
    into left- and right-swath groups.

  SWOT's nadir altimeter produces its own Level-2 products (GDRs) that
  are analogous to the Jason-class altimetry record and cover the nadir
  track.

- **Level 3** — gridded, multi-mission-calibrated products. These take
  the L2 measurements and reprocess them onto regular grids, often
  merging SWOT data with data from other altimeters such as Jason-3 or
  Sentinel-6 to fill gaps.

- **Level 4** — research and analysis products. These may include
  merged, gridded, or derived data sets such as experimental blends of
  nadir and KaRIn data, or downstream ocean-circulation models that
  assimilate SWOT heights.

This example focuses on the L2_LR_SSH **Basic** product because it is a
complete, self-contained NetCDF-4 file that is small enough to plot on a
laptop and contains all of the SSHA information a typical user needs.

---

## How SWOT L2_LR_SSH Maps to netCDF-4

A SWOT Basic file is a flat netCDF-4 file: all the data variables live
at the root group. It has three small dimensions — `num_lines` (along
track), `num_pixels` (across track), and `num_sides` — and a handful of
2-D variables sized `(num_lines, num_pixels)`. The primary science
variables are `ssha_karin` (sea surface height anomaly) and `ssh_karin`
(total sea surface height). Each has a companion quality flag
(`ssha_karin_qual`, `ssh_karin_qual`) whose non-zero bits mark suspect
or bad pixels.

Because the file follows CF-1.7, the global `Conventions` attribute is
`"CF-1.7"`. There are no groups to
navigate; the root group is the entire file.

---

## The netCDF-4 View: `ncdump -h`

`ncdump -h` prints header metadata only (dimensions, variables,
attributes) without reading any data. Run against the sample granule:

```bash
ncdump -h /home/ed/Downloads/SWOT_L2_LR_SSH_Basic_032_140_20250503T000222_20250503T005350_PIC2_01.nc
```

The file opens as one netCDF-4 dataset. The dimensions are small; each
Basic granule is one half-orbit pass:

```
netcdf SWOT_L2_LR_SSH_Basic_032_140_20250503T000222_20250503T005350_PIC2_01 {
dimensions:
    num_lines = 9866 ;
    num_pixels = 69 ;
    num_sides = 2 ;
```

`num_lines` is the along-track dimension, `num_pixels` is the
cross-track dimension, and `num_sides` distinguishes the two KaRIn
swaths. The full file contains many ancillary variables; the ones the
example needs are `ssha_karin`, `ssha_karin_qual`, `latitude`, and
`longitude`:

```
    int latitude(num_lines, num_pixels) ;
        latitude:_FillValue = 2147483647 ;
        latitude:long_name = "latitude (positive N, negative S)" ;
        latitude:standard_name = "latitude" ;
        latitude:units = "degrees_north" ;
        latitude:scale_factor = 1.e-06 ;
        latitude:valid_min = -80000000 ;
        latitude:valid_max = 80000000 ;
        latitude:comment = "Latitude of measurement [-80,80]. Positive latitude is North latitude, negative latitude is South latitude." ;
    int longitude(num_lines, num_pixels) ;
        longitude:_FillValue = 2147483647 ;
        longitude:long_name = "longitude (degrees East)" ;
        longitude:standard_name = "longitude" ;
        longitude:units = "degrees_east" ;
        longitude:scale_factor = 1.e-06 ;
        longitude:valid_min = 0 ;
        longitude:valid_max = 359999999 ;
        longitude:comment = "Longitude of measurement. East longitude relative to Greenwich meridian." ;
    int ssha_karin(num_lines, num_pixels) ;
        ssha_karin:_FillValue = 2147483647 ;
        ssha_karin:long_name = "sea surface height anomaly" ;
        ssha_karin:units = "m" ;
        ssha_karin:scale_factor = 0.0001 ;
        ssha_karin:quality_flag = "ssha_karin_qual" ;
        ssha_karin:valid_min = -1000000 ;
        ssha_karin:valid_max = 1000000 ;
        ssha_karin:coordinates = "longitude latitude" ;
    uint ssha_karin_qual(num_lines, num_pixels) ;
        ssha_karin_qual:_FillValue = 4294967295U ;
        ssha_karin_qual:long_name = "sea surface height anomaly quality flag" ;
        ssha_karin_qual:standard_name = "status_flag" ;
        ssha_karin_qual:flag_meanings = "suspect_large_ssh_delta ..." ;
        ssha_karin_qual:flag_masks = 1U, 2U, 4U, ... ;
        ssha_karin_qual:valid_min = 0U ;
        ssha_karin_qual:valid_max = 4279222239U ;
        ssha_karin_qual:coordinates = "longitude latitude" ;
```

`ssha_karin` is a 32-bit signed integer array. Its `scale_factor` of
`0.0001` converts the stored integer values to meters; `xarray` (and
`netCDF4` with `set_auto_scale(True)`) applies this automatically. Its
`_FillValue` marks missing pixels, and its `quality_flag` attribute
points at `ssha_karin_qual`. Any non-zero quality flag means the pixel
is suspect or bad and should be masked before plotting.

The global attributes identify the product:

```
// global attributes:
    :Conventions = "CF-1.7" ;
    :title = "Level 2 Low Rate Sea Surface Height Data Product - Basic SSH" ;
    :institution = "CNES" ;
    :source = "Ka-band radar interferometer" ;
    :platform = "SWOT" ;
    :reference_document = "D-56407_SWOT_Product_Description_L2_LR_SSH" ;
    :cycle_number = 32s ;
    :pass_number = 140s ;
    :equator_time = "2025-05-03T00:28:04.481000Z" ;
    :short_name = "L2_LR_SSH" ;
    :product_file_id = "Basic" ;
    :geospatial_lon_min = 97.082307 ;
    :geospatial_lon_max = 264.52684 ;
    :geospatial_lat_min = -78.271942 ;
    :geospatial_lat_max = 78.272068 ;
    :good_ocean_data_percent = 67.8560967260382 ;
```

`Conventions = "CF-1.7"` says the file follows CF metadata conventions,
`title` names the product, and `geospatial_*` attributes give the
global lat/lon bounds of the half-orbit pass.

---

## Finding SSHA: Two Equivalent Paths

**Through netCDF-C:**

```c
int ncid, varid;
nc_open(path, NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "ssha_karin", &varid);
nc_get_var_int(ncid, varid, ssha_raw);
```

The returned integers must be multiplied by `0.0001` to get meters.

**Through NEP's `examples/swot` example, in Python:**

NEP's `examples/swot/swot_example/l3_ssh.py` opens the file with
`xarray`, applies the `scale_factor` automatically, attaches the
`latitude` and `longitude` coordinate arrays, and masks any pixel whose
`ssha_karin_qual` is non-zero:

```python
import xarray as xr
import numpy as np

SSH_VARIABLES = ("ssha", "ssha_karin", "sea_surface_height_anomaly")
QUALITY_VARIABLES = ("quality_flag", "ssha_karin_qual", "ssha_qual", "ssh_karin_qual")

with xr.open_dataset(path, engine="netcdf4", decode_times=False) as ds:
    ssha = ds["ssha_karin"]
    lat = ds["latitude"]
    lon = ds["longitude"]
    ssha = ssha.assign_coords(latitude=lat, longitude=lon)

    flag = ds["ssha_karin_qual"].values
    invalid = np.zeros(flag.shape, dtype=bool)
    valid_flag = np.isfinite(flag)
    invalid[valid_flag] = flag[valid_flag].astype(np.int64) != 0
    ssha = ssha.where(~invalid)
```

`open_dataset(..., decode_times=False)` is enough to read the file. The
netCDF4 engine applies `scale_factor` and fill-value masking, and the
extra `ssha_karin_qual` mask removes degraded or suspect pixels. The
`coordinates = "longitude latitude"` attribute on `ssha_karin` tells
humans (and some tools) which 2-D coordinate arrays describe the grid.

Run it from the command line:

```bash
cd examples/swot
python -m swot_example /home/ed/Downloads/SWOT_L2_LR_SSH_Basic_032_140_20250503T000222_20250503T005350_PIC2_01.nc --output-dir figures
```

which produces `figures/ssha_map.png` — a quality-masked SSHA map
plotted with cartopy — and `figures/swath_footprint.png`, the granule's
lat/lon bounding box. See `examples/swot/README.md` for the full CLI,
including fetching a granule by lat/lon box from NASA Earthdata with
`--bbox`.

---

## Command Reference

```bash
# Full header, netCDF view
ncdump -h SWOT_L2_LR_SSH_Basic_*.nc

# Show header only for the SSHA and quality variables
ncdump -h -v ssha_karin,ssha_karin_qual,latitude,longitude SWOT_L2_LR_SSH_Basic_*.nc

# HDF5 view of all dataset paths
h5dump -n SWOT_L2_LR_SSH_Basic_*.nc
```

`ncdump -h` skips data by design; a full `ncdump` of `ssha_karin` is
tens of megabytes of text for a value you almost certainly want in a
NumPy array instead, not a terminal.

---

## References

- **netCDF-C**: <https://www.unidata.ucar.edu/software/netcdf/>
- **NEP (NetCDF Expansion Pack)**: <https://github.com/Intelligent-Data-Design-Inc/NEP>
- **SWOT Mission (NASA JPL)**: <https://swot.jpl.nasa.gov/>
- **SWOT L2_LR_SSH (PO.DAAC)**: <https://podaac.jpl.nasa.gov/dataset/SWOT_L2_LR_SSH_D>

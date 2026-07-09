# NEP Format Readers

NEP implements five NetCDF User Defined Format (UDF) handlers that allow external scientific data formats to be opened with the standard `nc_open()` API. All readers are **disabled by default** and must be enabled at build time.

## UDF Slot Assignments

| Slot | Format | Magic / Detection | Introduced |
|------|--------|-------------------|------------|
| UDF0 | GeoTIFF BigTIFF | `II+` | v1.5.0 |
| UDF1 | GeoTIFF standard TIFF | `II*` or `MM\x00*` | v1.5.0 |
| UDF2 | GRIB2 | `GRIB` | v1.7.0 |
| UDF3 | FITS | `SIMPLE` | v2.0.0 |
| UDF4 | NASA CDF | `\xCD\xF3\x00\x01` | v1.3.0 |
| UDF5 | NASA/ESA PDS4 | XML root `Product_Observational` | v2.2.0 |

All five readers can be enabled simultaneously — there are no mutual-exclusivity restrictions as of v2.2.0.

---

## GeoTIFF (UDF0 / UDF1)

GeoTIFF is a TIFF-based format for geospatial raster data, used widely in remote sensing, GIS, and earth observation.

**Transparent Access**: Read GeoTIFF files with `nc_open()` and standard NetCDF functions.

**Dimension Mapping**:
- Bands → NetCDF dimension (multi-band files)
- Rows (height) → NetCDF `y` dimension
- Columns (width) → NetCDF `x` dimension

**CF-1.8 CRS Metadata**: A `crs` grid-mapping variable is emitted with coordinate variables (`lon`/`lat` or `x`/`y`) and optional bounds. See `docs/cf-compliance.md` for the full attribute specification.

**Use Cases**: Satellite imagery, land cover maps, digital elevation models, MODIS flood products.

**Enabling:**
```bash
cmake -B build -DENABLE_GEOTIFF=ON   # CMake
./configure --enable-geotiff          # Autotools
```
**Dependencies**: libgeotiff, libtiff.

**Example:**
```c
nc_open("satellite_image.tif", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "data", &varid);
nc_get_vara_uchar(ncid, varid, start, count, buf);
nc_close(ncid);
```

---

## GRIB2 (UDF2)

GRIB2 is the standard binary format used by NOAA, ECMWF, and other meteorological agencies for gridded NWP model output and wave forecasts.

**Transparent Access**: Read GRIB2 files with `nc_open()`, `nc_get_var()`, and `ncdump`.

**Product-to-Variable Mapping**: Each GRIB2 product becomes a named `NC_FLOAT` variable with shared `[y, x]` dimensions. Variable names come from `g2c_param_abbrev()`; duplicates are uniquified with `_2`, `_3` suffixes.

**Full Grid Expansion**: `NC_GRIB2_get_vara()` expands the full `[ny, nx]` grid, substituting `_FillValue = 9.999e20f` for bitmap-masked (land) points.

**Metadata**: Per-variable attributes (`long_name`, `_FillValue`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`) and global attributes (`Conventions = "GRIB2"`, `GRIB2_edition = 2`).

**`.ncrc` Autoload**: Append the installed `.ncrc` to `~/.ncrc` and `nc_open()` / `ncdump` work on `.grib2` files with no code changes.

**Use Cases**: NOAA GDAS, GFS, NAM, HRRR model output; ocean wave forecasts.

**Enabling:**
```bash
cmake -B build -DENABLE_GRIB2=ON   # CMake
./configure --enable-grib2          # Autotools
```
**Dependencies**: NOAA NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0.

**Example:**
```c
nc_open("gdaswave.t00z.wcoast.0p16.f000.grib2", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "WIND", &varid);
nc_get_var_float(ncid, varid, data);  /* land points = 9.999e20 */
nc_close(ncid);
```

Or with `ncdump`:
```bash
ncdump -h gdaswave.t00z.wcoast.0p16.f000.grib2
ncdump -v WIND gdaswave.t00z.wcoast.0p16.f000.grib2
```

---

## FITS (UDF3)

FITS (Flexible Image Transport System) is the standard format for astronomical data from telescopes and instruments such as HST, Chandra, and JWST.

**Transparent Access**: Read FITS files with `nc_open()` after calling `NC_FITS_initialize()`.

**HDU-to-Group Mapping**:
- Primary HDU → root group; variable named `image`
- Each extension HDU → child group named from `EXTNAME` or `hdu_N`
- Binary/ASCII table HDUs → one netCDF variable per column

**Metadata Mapping**: `BUNIT`→`units`, `BZERO`→`add_offset`, `BSCALE`→`scale_factor`, `BLANK`→`_FillValue`; all other header keywords become string attributes.

**Use Cases**: HST imaging, Chandra X-ray data, JWST observations, any FITS-format observatory data.

**Enabling:**
```bash
cmake -B build -DENABLE_FITS=ON   # CMake
./configure --enable-fits          # Autotools
```
**Dependencies**: CFITSIO ≥ 3.0.

**Example:**
```c
#include "fitsdispatch.h"
NC_FITS_initialize();   /* register UDF3; safe to call even if already registered */
nc_open("image.fits", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "image", &varid);
nc_get_vara_float(ncid, varid, start, count, pixels);
nc_close(ncid);
```

---

## NASA CDF (UDF4)

The Common Data Format (CDF) is developed by NASA's Space Physics Data Facility (SPDF) for time-series and multi-dimensional scientific data from space missions.

**Transparent Access**: Read CDF files with `nc_open()` and standard NetCDF functions — no explicit initialization call needed when using `.ncrc` autoload.

**Type Mapping**:
- `CDF_INT4` → `NC_INT`
- `CDF_REAL8` → `NC_DOUBLE`
- `CDF_TIME_TT2000` → `NC_INT64`

**Attribute Conventions**: CDF `FILLVAL` attributes are automatically renamed to `_FillValue`.

**Note**: CDF moved from UDF2 to UDF4 in v2.2.0, eliminating the former mutual-exclusivity with GRIB2.

**Use Cases**: NASA IMAP, MMS, Van Allen Probes, and other space physics mission data.

**Enabling:**
```bash
cmake -B build -DENABLE_CDF=ON   # CMake
./configure --enable-cdf          # Autotools
```
**Dependencies**: NASA CDF library v3.9.x — download from https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/ or `spack install cdf`.

**Resources**: [NASA CDF Homepage](https://cdf.gsfc.nasa.gov/) · [CDF C Reference Manual](https://spdf.gsfc.nasa.gov/pub/software/cdf/doc/cdf_C_RefManual.pdf)

**Example:**
```c
nc_open("data.cdf", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "Epoch", &varid);
nc_get_var_longlong(ncid, varid, epoch_data);
nc_close(ncid);
```

---

## NASA/ESA PDS4 (UDF5)

PDS4 (Planetary Data System version 4) is the current archival standard used by NASA and ESA for planetary science data. A PDS4 dataset is an XML label file (`.xml`) paired with one or more binary or text data files.

**Transparent Access**: Read PDS4 products with `nc_open()` after calling `NC_PDS4_initialize()`.

**netCDF Model Mapping**:
- `Identification_Area` and `Observation_Area` → global string attributes on root group
- Each `File_Area_Observational` → child group named from `File/file_name`
- `Array` / `Array_2D_Image` → dimensions from `Axis_Array` entries + one variable
- `Table_Binary` / `Table_Character` / `Table_Delimited` → `record` dimension + one variable per field

**Data Reading**: Full hyperslab (`start`/`count`) support; automatic MSB/LSB byte-order conversion.

**Use Cases**: Mars Reconnaissance Orbiter, Curiosity, Perseverance, Cassini, OSIRIS-REx, and other planetary mission archives.

**Enabling:**
```bash
cmake -B build -DENABLE_PDS4=ON   # CMake
./configure --enable-pds4          # Autotools
```
**Dependencies**: libxml2 ≥ 2.9 (`libxml2-dev` on Ubuntu/Debian; `libxml2-devel` on RHEL/Fedora).

**Example:**
```c
#include "pds4dispatch.h"
NC_PDS4_initialize();   /* register UDF5 */
nc_open("product.xml", NC_NOWRITE, &ncid);
nc_inq_grp_ncid(ncid, "datafile.img", &grpid);
nc_inq_varid(grpid, "Array_2D_Image", &varid);
nc_get_vara_float(grpid, varid, start, count, image_data);
nc_close(ncid);
```

---

## UDF Autoloading via `.ncrc`

NEP installs a `.ncrc` configuration file that enables NetCDF-C's UDF self-loading mechanism. Once configured, `nc_open()` and `ncdump` automatically select the correct format handler with no application code changes.

**Setup** — merge the installed file into `~/.ncrc`:

```bash
cat /usr/local/share/nep/.ncrc >> ~/.ncrc
```

Or point `NETCDF_RC` to the directory for a per-session override:

```bash
export NETCDF_RC=/usr/local/share/nep
```

Then open any supported format transparently:

```c
nc_open("satellite_image.tif",                  NC_NOWRITE, &ncid);  /* GeoTIFF */
nc_open("data.cdf",                             NC_NOWRITE, &ncid);  /* CDF */
nc_open("gdaswave.t00z.wcoast.0p16.f000.grib2", NC_NOWRITE, &ncid);  /* GRIB2 */
nc_open("image.fits",                           NC_NOWRITE, &ncid);  /* FITS */
```

**Install path**:

| Build system | Default | Override |
|---|---|---|
| CMake | `${prefix}/share/nep/.ncrc` | `-DNEP_NCRC_INSTALL_DIR=<path>` |
| Autotools | `${datarootdir}/nep/.ncrc` | `--with-ncrc-dir=<path>` |

**Note**: `.ncrc` autoload requires NetCDF-C built from the main branch. With NetCDF-C 4.10.0, call `NC_*_initialize()` explicitly before `nc_open()`.

See the [NetCDF UDF documentation](https://docs.unidata.ucar.edu/netcdf/NUG/user_defined_formats.html) for the full RC file format reference.

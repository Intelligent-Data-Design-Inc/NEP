# NASA CDF Format Reader

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
cmake -B build -DNEP_ENABLE_CDF=ON   # CMake
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

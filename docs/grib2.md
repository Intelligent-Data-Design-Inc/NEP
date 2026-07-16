# GRIB2 Format Reader

GRIB2 is the standard binary format used by NOAA, ECMWF, and other meteorological agencies for gridded NWP model output and wave forecasts.

**Transparent Access**: Read GRIB2 files with `nc_open()`, `nc_get_var()`, and `ncdump`.

**Product-to-Variable Mapping**: Each GRIB2 product becomes a named `NC_FLOAT` variable with shared `[y, x]` dimensions. Variable names come from `g2c_param_abbrev()`; duplicates are uniquified with `_2`, `_3` suffixes.

**Full Grid Expansion**: `NC_GRIB2_get_vara()` expands the full `[ny, nx]` grid, substituting `_FillValue = 9.999e20f` for bitmap-masked (land) points.

**Metadata**: Per-variable attributes (`long_name`, `_FillValue`, `GRIB2_discipline`, `GRIB2_category`, `GRIB2_param_number`) and global attributes (`Conventions = "GRIB2"`, `GRIB2_edition = 2`).

**`.ncrc` Autoload**: Append the installed `.ncrc` to `~/.ncrc` and `nc_open()` / `ncdump` work on `.grib2` files with no code changes.

**Use Cases**: NOAA GDAS, GFS, NAM, HRRR model output; ocean wave forecasts.

**Enabling:**
```bash
cmake -B build -DNEP_ENABLE_GRIB2=ON   # CMake
./configure --enable-grib2          # Autotools
```
**Dependencies**: NOAA NCEPLIBS-g2c ≥ 2.1.0, libjasper ≥ 3.0.0.

**Resources**: [NCEP GRIB2 Documentation](https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/) · [WMO GRIB2 Specification](https://library.wmo.int/records/item/35713-manual-on-the-grib2-code-form) · [NCEPLIBS-g2c](https://github.com/NOAA-EMC/NCEPLIBS-g2c)

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

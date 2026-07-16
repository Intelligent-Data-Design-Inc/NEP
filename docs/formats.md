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

## Format Reference Pages

Each NEP format reader has a dedicated page with the full mapping, build options,
dependencies, resources, and examples:

| Format | UDF Slot | Reference Page |
|--------|----------|----------------|
| GeoTIFF | UDF0 / UDF1 | [GeoTIFF](geotiff.md) |
| GRIB2 | UDF2 | [GRIB2](grib2.md) |
| FITS | UDF3 | [FITS](fits.md) |
| NASA CDF | UDF4 | [NASA CDF](cdf.md) |
| NASA/ESA PDS4 | UDF5 | [PDS4-to-NetCDF Mapping](pds4.md) |

See also the native NetCDF-4 compression documentation:
[LZ4/BZIP2 HDF5 filters](compression.md).


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

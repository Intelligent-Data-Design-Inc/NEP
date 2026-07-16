# GeoTIFF Format Reader

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
cmake -B build -DNEP_ENABLE_GEOTIFF=ON   # CMake
./configure --enable-geotiff          # Autotools
```
**Dependencies**: libgeotiff, libtiff.

**Resources**: [GeoTIFF Home](https://geotiff.io/) · [OGC GeoTIFF Standard](https://docs.ogc.org/is/19-008r4/19-008r4.html) · [GDAL GeoTIFF Driver](https://gdal.org/drivers/raster/gtiff.html)

**Example:**
```c
nc_open("satellite_image.tif", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "data", &varid);
nc_get_vara_uchar(ncid, varid, start, count, buf);
nc_close(ncid);
```

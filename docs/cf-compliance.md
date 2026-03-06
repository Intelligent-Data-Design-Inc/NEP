# GeoTIFF CF-1.8 Compliance Guide

This document describes the CF-1.8 compliant metadata that NEP emits when opening
a GeoTIFF file through the standard NetCDF API.  The authoritative CF specification
for grid mappings is Appendix F of the CF conventions document:
https://cfconventions.org/Data/cf-conventions/cf-conventions-1.8/cf-conventions.html#appendix-grid-mappings

---

## Variables Created on Open

For every GeoTIFF file that contains valid CRS tags, NEP creates the following
variables in addition to the raster `data` variable:

| Variable | Shape | Type | Description |
|----------|-------|------|-------------|
| `crs` | scalar | `NC_INT` | Grid-mapping variable; value is always 0 |
| `lon` or `x` | `(x,)` | `NC_DOUBLE` | Longitude (degrees_east) or easting (m) coordinate |
| `lat` or `y` | `(y,)` | `NC_DOUBLE` | Latitude (degrees_north) or northing (m) coordinate |
| `lon_bnds` / `x_bnds` | `(x, 2)` | `NC_DOUBLE` | Coordinate bounds (pixel-as-area only) |
| `lat_bnds` / `y_bnds` | `(y, 2)` | `NC_DOUBLE` | Coordinate bounds (pixel-as-area only) |

If the GeoTIFF file has no CRS tags, or if CRS extraction fails, the `crs` and
coordinate variables are omitted and the `data` variable is still readable.

---

## `crs` Variable Attributes

### Common attributes (all CRS types)

| Attribute | Type | Example value |
|-----------|------|---------------|
| `grid_mapping_name` | `NC_CHAR` | `"latitude_longitude"` |
| `semi_major_axis` | `NC_DOUBLE` | `6378206.4` |
| `inverse_flattening` | `NC_DOUBLE` | `294.978698` |

A sphere (equal semi-major and semi-minor axes) omits `inverse_flattening` and
sets `semi_major_axis` equal to the sphere radius.

### `latitude_longitude` (geographic CRS)

No additional projection-specific attributes beyond the common set above.

### `sinusoidal`

| Attribute | Type | Notes |
|-----------|------|-------|
| `longitude_of_projection_origin` | `NC_DOUBLE` | Central meridian |
| `false_easting` | `NC_DOUBLE` | |
| `false_northing` | `NC_DOUBLE` | |

### `transverse_mercator`

| Attribute | Type | Notes |
|-----------|------|-------|
| `longitude_of_central_meridian` | `NC_DOUBLE` | |
| `latitude_of_projection_origin` | `NC_DOUBLE` | |
| `scale_factor_at_central_meridian` | `NC_DOUBLE` | |
| `false_easting` | `NC_DOUBLE` | |
| `false_northing` | `NC_DOUBLE` | |

### `lambert_conformal_conic`

| Attribute | Type | Notes |
|-----------|------|-------|
| `longitude_of_central_meridian` | `NC_DOUBLE` | |
| `latitude_of_projection_origin` | `NC_DOUBLE` | |
| `standard_parallel` | `NC_DOUBLE` or `NC_DOUBLE[2]` | Scalar for 1-SP, 2-element array for 2-SP |
| `false_easting` | `NC_DOUBLE` | |
| `false_northing` | `NC_DOUBLE` | |

### `albers_conical_equal_area`

| Attribute | Type | Notes |
|-----------|------|-------|
| `longitude_of_central_meridian` | `NC_DOUBLE` | |
| `latitude_of_projection_origin` | `NC_DOUBLE` | |
| `standard_parallel` | `NC_DOUBLE[2]` | Always 2-element array |
| `false_easting` | `NC_DOUBLE` | |
| `false_northing` | `NC_DOUBLE` | |

---

## Coordinate Variable Attributes

### Geographic CRS (`lon`, `lat`)

| Attribute | `lon` value | `lat` value |
|-----------|-------------|-------------|
| `standard_name` | `"longitude"` | `"latitude"` |
| `long_name` | `"longitude"` | `"latitude"` |
| `units` | `"degrees_east"` | `"degrees_north"` |
| `axis` | `"X"` | `"Y"` |
| `bounds` | `"lon_bnds"` (pixel-as-area) | `"lat_bnds"` (pixel-as-area) |

### Projected CRS (`x`, `y`)

| Attribute | `x` value | `y` value |
|-----------|-----------|-----------|
| `standard_name` | `"projection_x_coordinate"` | `"projection_y_coordinate"` |
| `long_name` | `"x coordinate of projection"` | `"y coordinate of projection"` |
| `units` | `"m"` | `"m"` |
| `axis` | `"X"` | `"Y"` |
| `bounds` | `"x_bnds"` (pixel-as-area) | `"y_bnds"` (pixel-as-area) |

---

## Data Variable Attributes

| Attribute | Value |
|-----------|-------|
| `grid_mapping` | `"crs"` |
| `coordinates` | `"lon lat"` (geographic) or `"x y"` (projected) |

---

## Pixel Raster Type and Coordinate Values

The GeoTIFF `GTRasterTypeGeoKey` controls how coordinate values are computed from
the GeoTransform parameters `(origin_x, pixel_x, origin_y, pixel_y)`:

### Pixel-as-area (default, `RasterPixelIsArea`)

Coordinates are at pixel centres:

```
lon[i] = origin_x + (i + 0.5) * pixel_x
lat[j] = origin_y + (j + 0.5) * pixel_y
```

Bounds variables are created covering the full pixel extent:

```
lon_bnds[i, 0] = origin_x + i * pixel_x        (left edge)
lon_bnds[i, 1] = origin_x + (i + 1) * pixel_x  (right edge)
```

### Pixel-as-point (`RasterPixelIsPoint`)

Coordinates are at pixel corners (no bounds variables created):

```
lon[i] = origin_x + i * pixel_x
lat[j] = origin_y + j * pixel_y
```

---

## CRS Validation

`validate_crs_completeness()` enforces the following rules before attempting to
create the `crs` variable:

| CRS type | Rule |
|----------|------|
| `NC_GEOTIFF_CRS_UNKNOWN` | Always accepted; no `crs` variable created |
| `NC_GEOTIFF_CRS_GEOGRAPHIC` | Rejected if `semi_major_axis < 0` |
| `NC_GEOTIFF_CRS_PROJECTED` | Rejected if `semi_major_axis < 0` or `ct_projection == 0` |

A zero `semi_major_axis` (GeoKey absent) is accepted for all known CRS types
because the ellipsoid parameters may be implicit in the EPSG code.

---

## Supported Projections

The following GeoTIFF projection codes (`CTProjection`) map to CF grid mapping names:

| GeoTIFF code | CF `grid_mapping_name` |
|-------------|----------------------|
| `CT_TransverseMercator` | `transverse_mercator` |
| `CT_TransvMercator_SouthOriented` | `transverse_mercator` |
| `CT_LambertConfConic_2SP` | `lambert_conformal_conic` |
| `CT_LambertConfConic_1SP` | `lambert_conformal_conic` |
| `CT_AlbersEqualArea` | `albers_conical_equal_area` |
| `CT_Sinusoidal` | `sinusoidal` |
| Geographic model | `latitude_longitude` |

Unrecognised projection codes produce `grid_mapping_name = "unknown"` and the
file still opens successfully (graceful degradation).

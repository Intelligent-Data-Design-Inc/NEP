## CF Conventions Quick Reference

Quick lookup guide for Climate and Forecast (CF) Convention attributes.

## Common CF Attributes by Variable Type

| Variable Type | Required Attributes | Recommended Attributes | Optional Attributes |
|--------------|---------------------|------------------------|---------------------|
| **Data Variable** | `units`, `_FillValue` | `long_name`, `standard_name`, `valid_min`, `valid_max` | `cell_methods`, `coordinates`, `comment` |
| **Time Coordinate** | `units` (with "since"), `calendar` | `long_name`, `standard_name` = "time", `axis` = "T" | `bounds`, `climatology` |
| **Latitude** | `units` = "degrees_north" | `long_name`, `standard_name` = "latitude", `axis` = "Y" | `bounds` |
| **Longitude** | `units` = "degrees_east" | `long_name`, `standard_name` = "longitude", `axis` = "X" | `bounds` |
| **Vertical (Z)** | `units`, `positive` (up/down) | `long_name`, `standard_name`, `axis` = "Z" | `formula_terms` |
| **Global** | `Conventions` | `title`, `institution`, `source`, `history` | `references`, `comment` |

## CF Standard Names by Discipline

| Discipline | Common Standard Names |
|------------|----------------------|
| **Atmosphere** | `air_temperature`, `specific_humidity`, `eastward_wind`, `northward_wind`, `air_pressure`, `relative_humidity` |
| **Ocean** | `sea_surface_temperature`, `sea_water_salinity`, `sea_water_x_velocity`, `sea_water_potential_temperature` |
| **Land** | `surface_temperature`, `soil_moisture_content`, `leaf_area_index`, `surface_upward_latent_heat_flux` |
| **Radiation** | `surface_downwelling_shortwave_flux_in_air`, `toa_outgoing_longwave_flux` |
| **Precipitation** | `precipitation_flux`, `rainfall_flux`, `snowfall_flux` |

Find all standard names at: http://cfconventions.org/standard-names.html

## Choosing the Right Convention

Not every dataset needs all conventions. Use this decision tree:

### For Internal/Research Data (Not Shared)
- Follow NUG conventions (`units`, `_FillValue`, coordinate variables)
- This is the minimum for self-describing data

### For Data Shared Within a Research Group
- Use COARDS for time handling rules
- Ensures time coordinates are interpreted consistently

### For Data Published or Shared with the Broader Community
- Use **CF Conventions** (current version is CF-1.11)
- Required for: climate model output, observational datasets, satellite data
- Enables tools like Panoply, ncview, and scientific Python libraries to automatically understand your data

### For Data Going into a Data Center or Catalog (NASA, NOAA, ESGF)
- Use **CF + ACDD together**
- ACDD attributes enable automated discovery in search portals
- Required attributes: `title`, `summary`, `keywords`, `creator_name`, `institution`

## Convention Hierarchy

ACDD extends CF → CF extends COARDS → COARDS extends NUG

## Common CF Compliance Errors and Fixes

| Error Message | Meaning | Fix |
|--------------|---------|-----|
| `units are not consistent with standard name` | Units don't match the CF standard name definition | Check CF standard names table for correct units (e.g., `air_temperature` requires "K" not "Celsius") |
| `coordinates attribute references non-existent variable` | The `coordinates` attribute points to a variable that doesn't exist | Ensure all variables listed in `coordinates` are defined in the file |
| `axis attribute is not consistent with coordinate type` | `axis="X"` on a variable with `units="degrees_north"` | Latitude should have `axis="Y"`, longitude `axis="X"` |
| `_FillValue must be same type as variable` | Float variable has integer fill value | Use matching types: `nc_put_att_float()` for float variables |
| `calendar attribute is not recognized` | Calendar value is not CF-compliant | Use: "gregorian", "standard", "proleptic_gregorian", "noleap", "365_day", "360_day" |
| `missing required attribute 'units'` | Variable lacks units attribute | Add units to every numeric variable, use "1" for dimensionless quantities |
| `time units must be parsable by udunits` | Time units string is malformed | Format must be: "units since YYYY-MM-DD HH:MM:SS" |

## Validating Without cf-checker

If you cannot install cf-checker, use these manual checks:

```bash
# 1. Check all variables have units
ncdump -h file.nc | grep -A2 "variables:" | grep -v ".*:.*="

# 2. Verify time units format
ncdump -h file.nc | grep "time:units"

# 3. Check for _FillValue on all data variables
ncdump -h file.nc | grep "_FillValue"

# 4. Verify coordinate variables have proper axis attributes
ncdump -h file.nc | grep "axis ="
```

## Time Units Format

CF-compliant time units must follow this pattern:

```
<unit> since <YYYY-MM-DD> <HH:MM:SS>
```

Examples:
- `seconds since 1900-01-01 00:00:00`
- `days since 1970-01-01 00:00:00`
- `hours since 2000-01-01`

## Calendar Systems

| Calendar | Use Case |
|----------|----------|
| `gregorian` or `standard` | Mixed Gregorian/Julian calendar (default) |
| `proleptic_gregorian` | Gregorian calendar extended backwards |
| `noleap` or `365_day` | All years have 365 days (climate models) |
| `all_leap` or `366_day` | All years have 366 days |
| `360_day` | All months have 30 days (some climate models) |
| `julian` | Julian calendar |

## Resources

- **CF Conventions Website**: http://cfconventions.org/
- **Standard Names Table**: http://cfconventions.org/standard-names.html
- **CF Checker**: `pip install cfchecker`
- **UDUNITS Database**: https://www.unidata.ucar.edu/software/udunits/

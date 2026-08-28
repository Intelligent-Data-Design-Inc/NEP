"""Reader utilities for GOES-R ABI Level 2 Cloud and Moisture Imagery (CMIP)
products.

ABI CMIP products are CF/netCDF-4 files.  The variables used by the example
are:

* ``CMI`` (Cloud and Moisture Imagery) — the variable to plot (reflectance
  or brightness temperature, channel-dependent).
* ``DQF`` (Data Quality Flags) — optional flag used to mask invalid pixels.
* ``x`` / ``y`` — 1-D coordinate arrays in radians from the geostationary
  sub-satellite point.
* ``goes_imager_projection`` — grid-mapping scalar variable containing the
  geostationary projection parameters.

The helper below opens a file with ``xarray`` and returns a quality-masked
``CMI`` ``DataArray`` with the native geostationary projection metadata
attached.
"""

from pathlib import Path

import numpy as np
import xarray as xr

#: Default directory used to cache granules downloaded via ``--date``.
DATA_DIR = Path("data")

#: Candidate names for the imagery variable.
CMI_VARIABLES = ("CMI", "cmi")

#: Candidate names for the data quality flag variable.
DQF_VARIABLES = ("DQF", "dqf", "data_quality_flags")

#: Candidate names for the x coordinate variable (radians from subpoint).
X_VARIABLES = ("x",)

#: Candidate names for the y coordinate variable (radians from subpoint).
Y_VARIABLES = ("y",)

#: Candidate names for the geostationary projection grid-mapping variable.
PROJ_VARIABLES = ("goes_imager_projection", "projection")


def _find_variable(ds: xr.Dataset, candidates: tuple[str, ...]) -> xr.DataArray:
    """Return the first candidate variable present in *ds*.

    Parameters
    ----------
    ds : xarray.Dataset
        Dataset to search.
    candidates : tuple of str
        Candidate variable names, in order of preference.

    Returns
    -------
    xarray.DataArray
        The first matching variable.

    Raises
    ------
    KeyError
        If none of the candidates exist in *ds*.
    """
    for name in candidates:
        if name in ds:
            return ds[name]
    raise KeyError(
        f"none of the expected variables found: {candidates}; "
        f"dataset variables: {list(ds.data_vars)}"
    )


def open_abi(path: Path) -> xr.Dataset:
    """Open an ABI CMIP NetCDF file.

    Parameters
    ----------
    path : Path
        Path to an ABI CMIP ``.nc`` product file.

    Returns
    -------
    xarray.Dataset
        The opened dataset.
    """
    return xr.open_dataset(path, engine="netcdf4", decode_times=False)


def load_cmi(
    path: Path, mask_quality: bool = True
) -> xr.DataArray:
    """Load the quality-masked CMI grid with projection metadata.

    Parameters
    ----------
    path : Path
        Path to an ABI CMIP ``.nc`` product file.
    mask_quality : bool, optional
        If true (default), pixels whose ``DQF`` is non-zero are masked
        to NaN in addition to the variable's fill value.

    Returns
    -------
    xarray.DataArray
        ``CMI`` with ``x`` and ``y`` coordinates and a
        ``goes_imager_projection`` attribute attached.
    """
    with open_abi(path) as ds:
        cmi = _find_variable(ds, CMI_VARIABLES).copy()
        x = _find_variable(ds, X_VARIABLES)
        y = _find_variable(ds, Y_VARIABLES)
        proj = _find_variable(ds, PROJ_VARIABLES)

        cmi = cmi.assign_coords(x=x, y=y)
        cmi.attrs["grid_mapping"] = proj.name
        # Copy the projection variable attributes into the DataArray so
        # plotting code can access them without reopening the file.
        for key, value in proj.attrs.items():
            cmi.attrs[f"{proj.name}_{key}"] = value

        if mask_quality:
            try:
                dqf = _find_variable(ds, DQF_VARIABLES).values
            except KeyError:
                dqf = None
            if dqf is not None:
                invalid = np.zeros(dqf.shape, dtype=bool)
                valid_flag = np.isfinite(dqf)
                invalid[valid_flag] = dqf[valid_flag].astype(np.int64) != 0
                cmi = cmi.where(~invalid)

    return cmi


def projection_params(cmi: xr.DataArray) -> dict:
    """Return the geostationary projection parameters for *cmi*.

    Parameters
    ----------
    cmi : xarray.DataArray
        ``CMI`` DataArray returned by :func:`load_cmi`.

    Returns
    -------
    dict
        Dictionary with keys ``semi_major_axis``, ``semi_minor_axis``,
        ``inverse_flattening``, ``perspective_point_height``,
        ``longitude_of_projection_origin``, and ``sweep_angle_axis``.
    """
    prefix = "goes_imager_projection_"
    return {
        "semi_major_axis": cmi.attrs.get(f"{prefix}semi_major_axis"),
        "semi_minor_axis": cmi.attrs.get(f"{prefix}semi_minor_axis"),
        "inverse_flattening": cmi.attrs.get(f"{prefix}inverse_flattening"),
        "perspective_point_height": cmi.attrs.get(
            f"{prefix}perspective_point_height"
        ),
        "longitude_of_projection_origin": cmi.attrs.get(
            f"{prefix}longitude_of_projection_origin"
        ),
        "sweep_angle_axis": cmi.attrs.get(f"{prefix}sweep_angle_axis", "x"),
    }


def lonlat_bounds(path: Path) -> tuple[float, float, float, float]:
    """Return the approximate geographic bounds of the granule.

    The bounds are computed by converting the corner ``x``/``y`` radian
    coordinates to geodetic latitude/longitude using the projection
    metadata.  This is a coarse bounding box, not the exact ABI sector
    footprint.

    Parameters
    ----------
    path : Path
        Path to an ABI CMIP ``.nc`` product file.

    Returns
    -------
    tuple of float
        Bounds as ``(lon_min, lon_max, lat_min, lat_max)`` in degrees.
    """
    import cartopy.crs as ccrs

    with open_abi(path) as ds:
        x = _find_variable(ds, X_VARIABLES).values
        y = _find_variable(ds, Y_VARIABLES).values
        proj = _find_variable(ds, PROJ_VARIABLES)
        sat_h = float(proj.attrs["perspective_point_height"])
        sat_lon = float(proj.attrs["longitude_of_projection_origin"])
        sweep = proj.attrs.get("sweep_angle_axis", "x")
        a = float(proj.attrs["semi_major_axis"])
        b = float(proj.attrs["semi_minor_axis"])

    # ABI x/y are in radians; multiply by the perspective point height to
    # obtain the map-projection coordinates used by cartopy's Geostationary
    # projection (matching the standard GOES-R plotting convention).
    x_m = x * sat_h
    y_m = y * sat_h
    globe = ccrs.Globe(semimajor_axis=a, semiminor_axis=b, flattening=None)
    geos = ccrs.Geostationary(
        central_longitude=sat_lon,
        satellite_height=sat_h,
        sweep_axis=sweep,
        globe=globe,
    )
    plate = ccrs.PlateCarree()

    corners_x = np.array([x_m.min(), x_m.max(), x_m.max(), x_m.min()])
    corners_y = np.array([y_m.min(), y_m.min(), y_m.max(), y_m.max()])
    lonlat = plate.transform_points(geos, corners_x, corners_y)
    lons = lonlat[:, 0]
    lats = lonlat[:, 1]
    valid = np.isfinite(lons) & np.isfinite(lats)
    if not np.any(valid):
        return (-180.0, 180.0, -90.0, 90.0)

    return (
        float(lons[valid].min()),
        float(lons[valid].max()),
        float(lats[valid].min()),
        float(lats[valid].max()),
    )

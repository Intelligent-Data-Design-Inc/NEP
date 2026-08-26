"""Reader utilities for SWOT L2/L3 gridded sea-surface-height products.

SWOT L2/L3 SSH products are CF/netCDF-4 files.  The variables used by the
example are:

* ``ssha`` (sea surface height anomaly) — the variable to plot.
* ``latitude`` / ``longitude`` — either 2-D coordinate arrays or 1-D
  dimension coordinates describing the grid.
* ``quality_flag`` (or ``ssha_qual``) — optional flag used to mask invalid
  pixels.

The helper below opens a file with ``xarray`` and returns a quality-masked
``ssha`` ``DataArray`` with ``latitude`` and ``longitude`` coordinates
attached.
"""

from pathlib import Path

import numpy as np
import xarray as xr

#: Default directory used to cache granules downloaded via ``--bbox``.
DATA_DIR = Path("data")

#: Candidate names for the sea-surface-height-anomaly variable.
#: PO.DAAC Version D Basic/Expert use ``ssha_karin``; Version C and AVISO+
#: L3 products often use ``ssha``.
SSH_VARIABLES = ("ssha", "ssha_karin", "sea_surface_height_anomaly")

#: Candidate names for the latitude coordinate variable.
LAT_VARIABLES = ("latitude", "lat")

#: Candidate names for the longitude coordinate variable.
LON_VARIABLES = ("longitude", "lon")

#: Candidate names for the quality / flag variable.
#: PO.DAAC Version D uses ``ssha_karin_qual``; AVISO+ L3 uses
#: ``quality_flag``.
QUALITY_VARIABLES = (
    "quality_flag",
    "ssha_karin_qual",
    "ssha_qual",
    "ssh_karin_qual",
)


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


def open_ssh(path: Path, group: str | None = None) -> xr.Dataset:
    """Open a SWOT SSH product file.

    Parameters
    ----------
    path : Path
        Path to a SWOT L2/L3 SSH NetCDF product file.
    group : str or None, optional
        NetCDF-4 group to open.  SWOT L2/L3 SSH data are normally at the
        root group, so the default is ``None``.

    Returns
    -------
    xarray.Dataset
        The opened group.
    """
    return xr.open_dataset(
        path, engine="netcdf4", group=group, decode_times=False
    )


def load_ssha(
    path: Path, group: str | None = None, mask_quality: bool = True
) -> xr.DataArray:
    """Load the quality-masked sea surface height anomaly grid.

    Parameters
    ----------
    path : Path
        Path to a SWOT L2/L3 SSH NetCDF product file.
    group : str or None, optional
        NetCDF-4 group to open (default: root group).
    mask_quality : bool, optional
        If true (default), pixels whose quality flag is non-zero are masked
        to NaN in addition to the variable's fill value.

    Returns
    -------
    xarray.DataArray
        ``ssha`` with ``latitude`` and ``longitude`` coordinates attached.
    """
    with open_ssh(path, group=group) as ds:
        ssha = _find_variable(ds, SSH_VARIABLES).copy()
        lat = _find_variable(ds, LAT_VARIABLES)
        lon = _find_variable(ds, LON_VARIABLES)
        ssha = ssha.assign_coords(latitude=lat, longitude=lon)

        if mask_quality:
            try:
                flag = _find_variable(ds, QUALITY_VARIABLES).values
            except KeyError:
                flag = None
            if flag is not None:
                invalid = np.zeros(flag.shape, dtype=bool)
                valid_flag = np.isfinite(flag)
                invalid[valid_flag] = flag[valid_flag].astype(np.int64) != 0
                ssha = ssha.where(~invalid)

    return ssha


def lonlat_bounds(
    path: Path, group: str | None = None
) -> tuple[float, float, float, float]:
    """Return the geographic bounds covered by the granule.

    Parameters
    ----------
    path : Path
        Path to a SWOT L2/L3 SSH NetCDF product file.
    group : str or None, optional
        NetCDF-4 group to open (default: root group).

    Returns
    -------
    tuple of float
        Bounds as ``(lon_min, lon_max, lat_min, lat_max)`` in degrees.
    """
    with open_ssh(path, group=group) as ds:
        lat = _find_variable(ds, LAT_VARIABLES).values
        lon = _find_variable(ds, LON_VARIABLES).values

    valid = np.isfinite(lat) & np.isfinite(lon)
    if not np.any(valid):
        return (-180.0, 180.0, -90.0, 90.0)

    return (
        float(lon[valid].min()),
        float(lon[valid].max()),
        float(lat[valid].min()),
        float(lat[valid].max()),
    )

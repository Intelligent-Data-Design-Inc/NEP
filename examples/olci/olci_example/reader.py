"""Reader utilities for Sentinel-3 OLCI Level-2 Land Full Resolution products.

A Sentinel-3 ``OL_2_LFR___`` product is a SAFE package (``.SEN3`` directory)
containing several NetCDF-4 files.  The example uses:

* ``geo_coordinates.nc`` — ``latitude[rows, columns]`` and
  ``longitude[rows, columns]`` (scaled integers).
* ``otci.nc`` — ``OTCI[rows, columns]`` (OLCI Terrestrial Chlorophyll Index)
  stored as unsigned bytes with a scale factor.
* ``lqsf.nc`` — ``LQSF[rows, columns]`` (Land Quality and Science Flags), a
  bit-packed 32-bit unsigned integer.
"""

from pathlib import Path

import numpy as np
import xarray as xr

#: Default directory used to cache granules downloaded via ``--date``.
DATA_DIR = Path("data")

#: Scale factor applied to the ``OTCI`` byte values.
OTCI_SCALE = 0.02559055

#: Fill value for the ``OTCI`` variable.
OTCI_FILL = 255

#: Bit masks for ``LQSF`` flags that should be excluded from the OTCI map.
#: These are the flags most likely to indicate invalid or non-land pixels.
LQSF_BAD_FLAGS = (
    0x00000001,  # INVALID
    0x00000002,  # WATER
    0x00000010,  # CLOUD
    0x00000020,  # CLOUD_AMBIGUOUS
    0x00000040,  # CLOUD_MARGIN
    0x00000080,  # SNOW_ICE
    0x00010000,  # OTCI_FAIL
    0x00800000,  # OTCI_BAD_IN
    0x01000000,  # OTCI_CLASS_CLSN
)

#: Candidate names for the latitude coordinate variable.
LAT_VARIABLES = ("latitude", "lat")

#: Candidate names for the longitude coordinate variable.
LON_VARIABLES = ("longitude", "lon")


def _find_variable(ds: xr.Dataset, candidates: tuple[str, ...]) -> xr.DataArray:
    """Return the first candidate variable present in *ds*."""
    for name in candidates:
        if name in ds:
            return ds[name]
    raise KeyError(
        f"none of the expected variables found: {candidates}; "
        f"dataset variables: {list(ds.data_vars)}"
    )


def open_nc(path: Path) -> xr.Dataset:
    """Open a NetCDF file from a SAFE package.

    Parameters
    ----------
    path : Path
        Path to a NetCDF-4 file inside a ``.SEN3`` package.

    Returns
    -------
    xarray.Dataset
        The opened dataset, with time decoding disabled (OLCI uses
        non-standard time units for some variables).
    """
    return xr.open_dataset(path, engine="netcdf4", decode_times=False)


def load_otci(
    sen3_dir: Path,
    mask_quality: bool = True,
) -> xr.DataArray:
    """Load the quality-masked OTCI grid with 2-D lat/lon coordinates.

    Parameters
    ----------
    sen3_dir : Path
        Path to a Sentinel-3 ``OL_2_LFR___`` ``.SEN3`` directory.
    mask_quality : bool, optional
        If true (default), pixels flagged as invalid, water, cloud,
        cloud-ambiguous, cloud-margin, snow/ice, or OTCI failure in
        ``LQSF`` are masked to NaN, in addition to the OTCI fill value.

    Returns
    -------
    xarray.DataArray
        ``OTCI`` with ``latitude`` and ``longitude`` 2-D coordinates attached.
    """
    geo_path = sen3_dir / "geo_coordinates.nc"
    otci_path = sen3_dir / "otci.nc"
    lqsf_path = sen3_dir / "lqsf.nc"

    for p in (geo_path, otci_path):
        if not p.exists():
            raise FileNotFoundError(f"required OLCI LFR file not found: {p}")

    with open_nc(geo_path) as ds_geo, open_nc(otci_path) as ds_otci:
        otci = ds_otci["OTCI"].copy()
        lat = _find_variable(ds_geo, LAT_VARIABLES)
        lon = _find_variable(ds_geo, LON_VARIABLES)

        # Apply scale factor and fill value.
        otci = otci.where(otci != OTCI_FILL) * OTCI_SCALE

        # Attach 2-D lat/lon coordinates.  The geo file stores them as scaled
        # 32-bit integers; xarray should apply the scale_factor automatically
        # when the attribute is present, but apply it explicitly to be safe.
        lat_scale = float(lat.attrs.get("scale_factor", 1.0))
        lon_scale = float(lon.attrs.get("scale_factor", 1.0))
        otci = otci.assign_coords(
            latitude=lat * lat_scale,
            longitude=lon * lon_scale,
        )

        if mask_quality:
            if not lqsf_path.exists():
                raise FileNotFoundError(
                    f"quality mask requested but file not found: {lqsf_path}"
                )
            with open_nc(lqsf_path) as ds_lqsf:
                lqsf = ds_lqsf["LQSF"].values
                bad = np.zeros(lqsf.shape, dtype=bool)
                for flag in LQSF_BAD_FLAGS:
                    bad |= (lqsf & flag) != 0
                otci = otci.where(~bad)

    return otci


def lonlat_bounds(otci: xr.DataArray) -> tuple[float, float, float, float]:
    """Return the geographic bounds of the valid OTCI pixels.

    Parameters
    ----------
    otci : xarray.DataArray
        OTCI DataArray with 2-D ``latitude`` and ``longitude`` coordinates
        (as returned by :func:`load_otci`).

    Returns
    -------
    tuple of float
        Bounds as ``(lon_min, lon_max, lat_min, lat_max)`` in degrees.
    """
    lat = otci.coords["latitude"].values
    lon = otci.coords["longitude"].values
    valid = np.isfinite(otci.values) & np.isfinite(lat) & np.isfinite(lon)
    if not np.any(valid):
        return (-180.0, 180.0, -90.0, 90.0)

    return (
        float(lon[valid].min()),
        float(lon[valid].max()),
        float(lat[valid].min()),
        float(lat[valid].max()),
    )

"""Plotting utilities for GOES-R ABI CMIP data.

Produces projection-aware (cartopy) figures: a Cloud and Moisture Imagery
(CMI) map on the native geostationary projection and a footprint overview
map showing the granule's lat/lon bounding box.
"""

from pathlib import Path

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import numpy as np
import xarray as xr

from .reader import projection_params

Bounds = tuple[float, float, float, float]  # lon_min, lon_max, lat_min, lat_max


def _finish(fig, out_path: Path | None, show: bool) -> Path | None:
    """Save a figure, optionally show it, then close it.

    Parameters
    ----------
    fig : matplotlib.figure.Figure
        Figure to finalize.
    out_path : Path or None
        PNG destination; parent directories are created.  If None, the
        figure is not saved.  ``bbox_inches="tight"`` is intentionally not
        used — it collapses cartopy GeoAxes.
    show : bool
        If true, display the figure interactively before closing.

    Returns
    -------
    Path or None
        The saved path, or None if not saved.
    """
    if out_path is not None:
        out_path = Path(out_path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out_path, dpi=150)
    if show:
        plt.show()
    plt.close(fig)
    return out_path


def _build_geostationary_projection(cmi: xr.DataArray) -> ccrs.Geostationary:
    """Build a cartopy Geostationary projection from *cmi* metadata."""
    params = projection_params(cmi)
    globe = ccrs.Globe(
        semimajor_axis=params["semi_major_axis"],
        semiminor_axis=params["semi_minor_axis"],
        flattening=None,
    )
    return ccrs.Geostationary(
        central_longitude=params["longitude_of_projection_origin"],
        satellite_height=params["perspective_point_height"],
        sweep_axis=params["sweep_angle_axis"],
        globe=globe,
    )


def plot_cmi_map(
    cmi: xr.DataArray,
    out_path: Path | None = None,
    show: bool = False,
    title: str | None = None,
) -> Path | None:
    """Plot a quality-masked CMI grid on its native geostationary projection.

    Parameters
    ----------
    cmi : xarray.DataArray
        ``CMI`` DataArray carrying 1-D ``x`` and ``y`` coordinates (radians
        from the sub-satellite point) and the ``goes_imager_projection``
        metadata (as returned by :func:`abi_example.reader.load_cmi`).
    out_path : Path or None, optional
        PNG destination; not saved if None.
    show : bool, optional
        If true, also display the figure interactively.
    title : str or None, optional
        Plot title.  Defaults to ``CMI`` plus the variable units.

    Returns
    -------
    Path or None
        The saved PNG path, or None if not saved.
    """
    proj = _build_geostationary_projection(cmi)
    sat_h = projection_params(cmi)["perspective_point_height"]

    # Convert radian x/y coordinates to map-projection coordinates (m).
    x_m = cmi["x"].values * sat_h
    y_m = cmi["y"].values * sat_h
    extent = [x_m.min(), x_m.max(), y_m.min(), y_m.max()]

    fig, ax = plt.subplots(figsize=(10, 10), subplot_kw={"projection": proj})
    data = cmi.values
    valid = np.isfinite(data)
    vmin = float(np.nanpercentile(data[valid], 1)) if np.any(valid) else 0.0
    vmax = float(np.nanpercentile(data[valid], 99)) if np.any(valid) else 1.0

    im = ax.imshow(
        data,
        origin="upper",
        extent=extent,
        transform=proj,
        cmap="viridis",
        vmin=vmin,
        vmax=vmax,
    )
    ax.coastlines(resolution="50m")
    ax.add_feature(cfeature.LAND, facecolor="0.9")
    gl = ax.gridlines(draw_labels=True, linewidth=0.3)
    gl.top_labels = gl.right_labels = False

    units = cmi.attrs.get("units", "")
    fig.colorbar(im, ax=ax, shrink=0.5, label=f"CMI ({units})".strip())

    if title is None:
        title = f"ABI CMI ({cmi.attrs.get('units', '')})".strip()
    ax.set_title(title)

    return _finish(fig, out_path, show)


def plot_footprint(
    bounds: Bounds,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "ABI CMIP Granule Footprint",
    pad_factor: float = 1.5,
) -> Path | None:
    """Plot a granule's lat/lon bounding box on a regional overview map.

    Parameters
    ----------
    bounds : tuple of float
        Bounding box as ``(lon_min, lon_max, lat_min, lat_max)`` in
        degrees (as returned by :func:`abi_example.reader.lonlat_bounds`).
    out_path : Path or None, optional
        PNG destination; not saved if None.
    show : bool, optional
        If true, also display the figure interactively.
    title : str, optional
        Plot title; the bounds are appended on a second line.
    pad_factor : float, optional
        Map extent padding on each side, as a multiple of the bounding
        box size.

    Returns
    -------
    Path or None
        The saved PNG path, or None if not saved.
    """
    lon_min, lon_max, lat_min, lat_max = bounds
    pad_lon = max((lon_max - lon_min) * pad_factor, 5.0)
    pad_lat = max((lat_max - lat_min) * pad_factor, 5.0)
    proj = ccrs.PlateCarree()
    fig, ax = plt.subplots(figsize=(8, 8), subplot_kw={"projection": proj})
    ax.set_extent(
        [
            max(lon_min - pad_lon, -180),
            min(lon_max + pad_lon, 180),
            max(lat_min - pad_lat, -90),
            min(lat_max + pad_lat, 90),
        ],
        crs=ccrs.PlateCarree(),
    )
    ax.coastlines(resolution="50m")
    ax.add_feature(cfeature.LAND, facecolor="0.9")
    ax.add_feature(cfeature.STATES, linewidth=0.5)
    ax.plot(
        [lon_min, lon_max, lon_max, lon_min, lon_min],
        [lat_min, lat_min, lat_max, lat_max, lat_min],
        color="red",
        linewidth=2,
        transform=ccrs.PlateCarree(),
        label="Data bounds",
    )
    gl = ax.gridlines(draw_labels=True, linewidth=0.3)
    gl.top_labels = gl.right_labels = False
    ax.legend(loc="upper right")
    ax.set_title(
        f"{title}\n"
        f"lon [{lon_min:.3f}, {lon_max:.3f}], lat [{lat_min:.3f}, {lat_max:.3f}]"
    )
    return _finish(fig, out_path, show)

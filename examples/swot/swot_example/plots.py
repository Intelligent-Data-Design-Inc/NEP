"""Plotting utilities for SWOT L2/L3 gridded sea-surface-height data.

Produces projection-aware (cartopy) figures: a sea surface height anomaly
(SSHA) map and a swath footprint overview map showing the granule's
lat/lon bounding box.
"""

from pathlib import Path

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import numpy as np
import xarray as xr

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


def plot_ssha_map(
    ssha: xr.DataArray,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "SWOT Sea Surface Height Anomaly",
) -> Path | None:
    """Plot a SWOT SSHA grid on a cartopy map.

    Parameters
    ----------
    ssha : xarray.DataArray
        ``ssha`` variable carrying ``latitude`` and ``longitude``
        coordinates (as returned by
        :func:`swot_example.l3_ssh.load_ssha`).  Masked/invalid pixels
        (NaN) are left transparent.
    out_path : Path or None, optional
        PNG destination; not saved if None.
    show : bool, optional
        If true, also display the figure interactively.
    title : str, optional
        Plot title.

    Returns
    -------
    Path or None
        The saved PNG path, or None if not saved.
    """
    proj = ccrs.PlateCarree()
    fig, ax = plt.subplots(figsize=(10, 8), subplot_kw={"projection": proj})

    lon = ssha["longitude"].values
    lat = ssha["latitude"].values
    data = ssha.values

    # Center the color scale on zero for anomaly data.
    vmax = float(np.nanmax(np.abs(data)))
    vmin = -vmax if not np.isnan(vmax) else 0.0

    mesh = ax.pcolormesh(
        lon,
        lat,
        data,
        transform=ccrs.PlateCarree(),
        cmap="RdBu_r",
        shading="auto",
        vmin=vmin,
        vmax=vmax,
    )
    ax.coastlines(resolution="110m")
    ax.add_feature(cfeature.LAND, facecolor="0.9")
    gl = ax.gridlines(draw_labels=True, linewidth=0.3)
    gl.top_labels = gl.right_labels = False
    fig.colorbar(mesh, ax=ax, shrink=0.7, label="SSHA (m)")
    ax.set_title(title)
    return _finish(fig, out_path, show)


def plot_footprint(
    bounds: Bounds,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "SWOT Granule Footprint",
    pad_factor: float = 1.5,
) -> Path | None:
    """Plot a granule's lat/lon bounding box on a regional overview map.

    Parameters
    ----------
    bounds : tuple of float
        Bounding box as ``(lon_min, lon_max, lat_min, lat_max)`` in
        degrees (as returned by :func:`swot_example.l3_ssh.lonlat_bounds`).
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
    pad_lon = max((lon_max - lon_min) * pad_factor, 0.5)
    pad_lat = max((lat_max - lat_min) * pad_factor, 0.5)
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
    ax.coastlines(resolution="110m")
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

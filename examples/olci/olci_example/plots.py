"""Plotting utilities for Sentinel-3 OLCI Level-2 Land products.

Produces two figures from a quality-masked OTCI DataArray:

1. A geographic map of the OLCI Terrestrial Chlorophyll Index using
   ``cartopy``/``matplotlib``.
2. A footprint overview showing the granule's lat/lon bounding box on a
   regional map.
"""

from pathlib import Path

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import numpy as np
import xarray as xr

Bounds = tuple[float, float, float, float]  # lon_min, lon_max, lat_min, lat_max


def _finish(fig, out_path: Path | None, show: bool) -> Path | None:
    """Save a figure, optionally show it, then close it."""
    if out_path is not None:
        out_path = Path(out_path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out_path, dpi=150)
    if show:
        plt.show()
    plt.close(fig)
    return out_path


def plot_otci_map(
    otci: xr.DataArray,
    out_path: Path | None = None,
    show: bool = False,
    title: str | None = None,
    stride: int = 10,
) -> Path | None:
    """Plot the quality-masked OTCI grid on a lat/lon map.

    Parameters
    ----------
    otci : xarray.DataArray
        ``OTCI`` DataArray carrying 2-D ``latitude`` and ``longitude``
        coordinates (as returned by :func:`olci_example.reader.load_otci`).
    out_path : Path or None, optional
        PNG destination; not saved if None.
    show : bool, optional
        If true, also display the figure interactively.
    title : str or None, optional
        Plot title.  Defaults to a Sentinel-3 OLCI OTCI title.
    stride : int, optional
        Plot every *stride*-th pixel in each dimension to reduce rendering
        time (default: 10).

    Returns
    -------
    Path or None
        The saved PNG path, or None if not saved.
    """
    lat = otci.coords["latitude"].values[::stride, ::stride]
    lon = otci.coords["longitude"].values[::stride, ::stride]
    data = otci.values[::stride, ::stride]

    valid = np.isfinite(data)
    vmin = float(np.nanpercentile(data[valid], 1)) if np.any(valid) else 0.0
    vmax = float(np.nanpercentile(data[valid], 99)) if np.any(valid) else 1.0

    proj = ccrs.PlateCarree()
    fig, ax = plt.subplots(figsize=(10, 10), subplot_kw={"projection": proj})

    im = ax.pcolormesh(
        lon,
        lat,
        data,
        transform=proj,
        cmap="viridis",
        vmin=vmin,
        vmax=vmax,
        shading="nearest",
    )

    ax.coastlines(resolution="50m")
    ax.add_feature(cfeature.LAND, facecolor="0.9")
    ax.add_feature(cfeature.OCEAN, facecolor="0.95")
    gl = ax.gridlines(draw_labels=True, linewidth=0.3)
    gl.top_labels = gl.right_labels = False

    fig.colorbar(im, ax=ax, shrink=0.5, label="OTCI (dimensionless)")

    if title is None:
        title = "Sentinel-3 OLCI Terrestrial Chlorophyll Index"
    ax.set_title(title)

    return _finish(fig, out_path, show)


def plot_footprint(
    bounds: Bounds,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "Sentinel-3 OLCI Granule Footprint",
    pad_factor: float = 0.5,
) -> Path | None:
    """Plot a granule's lat/lon bounding box on a regional overview map.

    Parameters
    ----------
    bounds : tuple of float
        Bounding box as ``(lon_min, lon_max, lat_min, lat_max)`` in
        degrees (as returned by :func:`olci_example.reader.lonlat_bounds`).
    out_path : Path or None, optional
        PNG destination; not saved if None.
    show : bool, optional
        If true, also display the figure interactively.
    title : str, optional
        Plot title; the bounds are appended on a second line.
    pad_factor : float, optional
        Map extent padding on each side, as a multiple of the bounding
        box size (default: 0.5).

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
    ax.add_feature(cfeature.OCEAN, facecolor="0.95")
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

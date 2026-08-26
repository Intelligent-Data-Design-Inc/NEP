"""Plotting utilities for NISAR SME2 soil moisture data.

Produces projection-aware (cartopy) figures: a soil moisture map and a
footprint overview map showing the granule's lat/lon bounding box. SME2
grids are posted on EASE-Grid 2.0 but ship 1-D latitude/longitude
coordinate arrays, so data is drawn in geographic coordinates via
``transform=ccrs.PlateCarree()``.
"""

from pathlib import Path

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import xarray as xr

Bounds = tuple[float, float, float, float]  # lon_min, lon_max, lat_min, lat_max


def _finish(fig, out_path: Path | None, show: bool) -> Path | None:
    """Save a figure, optionally show it, then close it.

    Parameters
    ----------
    fig : matplotlib.figure.Figure
        Figure to finalize.
    out_path : Path or None
        PNG destination; parent directories are created. If None, the
        figure is not saved. ``bbox_inches="tight"`` is intentionally not
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


def plot_soil_moisture(
    sm: xr.DataArray,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "NISAR SME2 Soil Moisture",
) -> Path | None:
    """Plot a soil moisture grid on a cartopy map.

    Parameters
    ----------
    sm : xarray.DataArray
        2-D soil moisture grid carrying 1-D ``latitude`` and ``longitude``
        coordinates (as returned by
        :func:`nisar_example.sme2.load_soil_moisture`). Masked/invalid
        pixels (NaN) are left blank.
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
    mesh = ax.pcolormesh(
        sm["longitude"].values,
        sm["latitude"].values,
        sm.values,
        transform=ccrs.PlateCarree(),
        cmap="viridis",
        shading="auto",
    )
    ax.coastlines(resolution="10m")
    ax.add_feature(cfeature.STATES, linewidth=0.5)
    gl = ax.gridlines(draw_labels=True, linewidth=0.3)
    gl.top_labels = gl.right_labels = False
    units = sm.attrs.get("units", "m^3/m^3")
    fig.colorbar(mesh, ax=ax, shrink=0.7, label=f"Soil moisture ({units})")
    ax.set_title(title)
    return _finish(fig, out_path, show)


def plot_footprint(
    bounds: Bounds,
    out_path: Path | None = None,
    show: bool = False,
    title: str = "NISAR SME2 Granule Footprint",
    pad_factor: float = 1.5,
) -> Path | None:
    """Plot a granule's lat/lon bounding box on a regional overview map.

    Parameters
    ----------
    bounds : tuple of float
        Bounding box as ``(lon_min, lon_max, lat_min, lat_max)`` in
        degrees (as returned by :func:`nisar_example.sme2.lonlat_bounds`).
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
    pad_lon = (lon_max - lon_min) * pad_factor
    pad_lat = (lat_max - lat_min) * pad_factor
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
    ax.coastlines(resolution="10m")
    ax.add_feature(cfeature.STATES, linewidth=0.5)
    ax.add_feature(cfeature.LAND, facecolor="0.9")
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

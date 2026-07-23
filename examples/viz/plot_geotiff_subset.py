"""Plot a GeoTIFF raster through the NetCDF UDF interface.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import os
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from netCDF4 import Dataset

from plot_common import save_with_metadata


_MAX_SAMPLES_PER_AXIS = 1000


def _sample_step(length):
    return max(1, (length + _MAX_SAMPLES_PER_AXIS - 1) // _MAX_SAMPLES_PER_AXIS)


def _geographic_extent(ds, data_var, row_step, column_step):
    if "lon" not in ds.variables or "lat" not in ds.variables:
        return None

    lon_var = ds.variables["lon"]
    lat_var = ds.variables["lat"]
    if lon_var.ndim != 1 or lat_var.ndim != 1:
        return None
    if lon_var.dimensions != (data_var.dimensions[1],):
        return None
    if lat_var.dimensions != (data_var.dimensions[0],):
        return None
    if lon_var.shape[0] != data_var.shape[1] or lat_var.shape[0] != data_var.shape[0]:
        return None

    lon = np.asarray(lon_var[::column_step])
    lat = np.asarray(lat_var[::row_step])
    if lon.size == 0 or lat.size == 0:
        return None

    endpoints = np.asarray(
        [lon_var[0], lon_var[-1], lat_var[0], lat_var[-1]], dtype=float
    )
    if not np.all(np.isfinite(endpoints)):
        return None

    return (endpoints[0], endpoints[1], endpoints[3], endpoints[2])


def main():
    parser = argparse.ArgumentParser(
        description="Plot a GeoTIFF raster through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_GEOTIFF_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a GeoTIFF file path is required (or set NEP_VIZ_GEOTIFF_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as ds:
        if "data" not in ds.variables:
            raise ValueError("GeoTIFF dataset does not contain a root data variable")
        data_var = ds.variables["data"]
        if data_var.ndim != 2:
            raise ValueError(f"GeoTIFF data must be two-dimensional, got {data_var.ndim}")

        row_step = _sample_step(data_var.shape[0])
        column_step = _sample_step(data_var.shape[1])
        data = np.ma.masked_invalid(
            np.ma.asarray(data_var[::row_step, ::column_step])
        )
        extent = _geographic_extent(ds, data_var, row_step, column_step)

    if data.size == 0 or np.ma.count(data) == 0:
        raise ValueError("GeoTIFF data contains no valid raster cells")

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    image = ax.imshow(data, origin="upper", extent=extent)
    ax.set_title("MODIS Flood GeoTIFF")
    if extent is None:
        ax.set_xlabel("x pixel")
        ax.set_ylabel("y pixel")
    else:
        ax.set_xlabel("Longitude")
        ax.set_ylabel("Latitude")
    fig.colorbar(image, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "geotiff_modis_flood",
        "MODIS Flood GeoTIFF",
        "Grayscale view of the complete MODIS flood raster opened through the NetCDF GeoTIFF UDF interface and downsampled for publication.",
        "Grayscale map of a MODIS flood raster, with longitude and latitude axes when georeferencing is available.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

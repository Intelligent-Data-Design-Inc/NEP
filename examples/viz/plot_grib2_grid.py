"""Plot a GRIB2 wind grid through the NetCDF UDF interface.

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


def main():
    parser = argparse.ArgumentParser(
        description="Plot a GRIB2 wind grid through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_GRIB2_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a GRIB2 file path is required (or set NEP_VIZ_GRIB2_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as ds:
        if "WIND" not in ds.variables:
            raise ValueError("GRIB2 dataset does not contain the WIND variable")
        wind_var = ds.variables["WIND"]
        if wind_var.ndim != 2:
            raise ValueError(f"GRIB2 WIND must be two-dimensional, got {wind_var.ndim}")

        wind = np.ma.masked_invalid(np.ma.asarray(wind_var[:, :]))
        fill_value = getattr(wind_var, "_FillValue", None)
        if fill_value is not None:
            wind = np.ma.masked_where(
                np.equal(np.ma.getdata(wind), fill_value), wind
            )

    if wind.size == 0 or np.ma.count(wind) == 0:
        raise ValueError("GRIB2 WIND contains no valid grid cells")

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    image = ax.imshow(wind, origin="upper")
    ax.set_title("GRIB2 Wave Wind Speed")
    ax.set_xlabel("x grid index")
    ax.set_ylabel("y grid index")
    fig.colorbar(image, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "grib2_wave",
        "GRIB2 Wave Wind Speed",
        "Grayscale wind-speed grid from the GDAS wave GRIB2 test file opened through the NetCDF UDF interface, with land fill values masked.",
        "Grayscale map of valid wave wind-speed cells; masked land cells are blank.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

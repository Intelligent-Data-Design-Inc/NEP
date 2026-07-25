"""Plot a New Horizons Alice 2-D spectrum through the NetCDF UDF interface.

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


def main():
    parser = argparse.ArgumentParser(
        description="Plot a New Horizons Alice 2-D spectrum through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_PDS4_NEW_HORIZONS_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a PDS4 New Horizons file path is required (or set NEP_VIZ_PDS4_NEW_HORIZONS_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    group_name = "ali_0030420276_0x4b0_sci_1.fit"

    with Dataset(str(path), mode="r") as ds:
        if group_name not in ds.groups:
            raise ValueError(f"New Horizons dataset does not contain the {group_name} group")
        grp = ds.groups[group_name]

        if "Observational Data" not in grp.variables:
            raise ValueError("New Horizons group does not contain the Observational Data variable")
        data_var = grp.variables["Observational Data"]
        if data_var.ndim != 2:
            raise ValueError(f"New Horizons Observational Data must be two-dimensional, got {data_var.ndim}")

        row_step = _sample_step(data_var.shape[0])
        column_step = _sample_step(data_var.shape[1])
        data = np.ma.masked_invalid(np.ma.asarray(data_var[::row_step, ::column_step]))

    if data.size == 0 or np.ma.count(data) == 0:
        raise ValueError("New Horizons Alice spectrum contains no valid data")

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    image = ax.imshow(data, origin="upper")
    ax.set_title("New Horizons Alice Spectrum")
    ax.set_xlabel("Sample (downsampled)")
    ax.set_ylabel("Line (downsampled)")
    fig.colorbar(image, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "pds4_new_horizons_alice",
        "New Horizons Alice Spectrum",
        "Grayscale image of a New Horizons Alice 2-D spectrum opened through the NetCDF PDS4 UDF interface and downsampled for publication.",
        "Grayscale plot of a New Horizons Alice ultraviolet spectrum array, downsampled to approximately 1000 samples per axis.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

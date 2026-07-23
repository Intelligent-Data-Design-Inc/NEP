"""Plot a Perseverance Mastcam-Z radiance image through the NetCDF UDF interface.

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
        description="Plot a Perseverance Mastcam-Z radiance image through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_PDS4_PERSEVERANCE_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a PDS4 Perseverance file path is required (or set NEP_VIZ_PDS4_PERSEVERANCE_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    group_name = "ZLF_1738_0821212185_707RAD_N0830000ZCAM00091_1100LMJ01.IMG"

    with Dataset(str(path), mode="r") as ds:
        if group_name not in ds.groups:
            raise ValueError(f"Perseverance dataset does not contain the {group_name} group")
        grp = ds.groups[group_name]

        if "data" not in grp.variables:
            raise ValueError("Perseverance group does not contain the data variable")
        data_var = grp.variables["data"]
        if data_var.ndim != 3:
            raise ValueError(f"Perseverance data must be three-dimensional, got {data_var.ndim}")

        scaling_factor = getattr(data_var, "scaling_factor", None)
        if scaling_factor is None:
            raise ValueError("Perseverance data variable is missing the scaling_factor attribute")
        try:
            scale = float(scaling_factor)
        except ValueError as exc:
            raise ValueError(f"Perseverance scaling_factor cannot be parsed: {scaling_factor}") from exc

        row_step = _sample_step(data_var.shape[1])
        column_step = _sample_step(data_var.shape[2])
        band0 = np.ma.asarray(data_var[0, ::row_step, ::column_step])
        radiance = np.ma.masked_invalid(band0 * scale)
        radiance = np.ma.masked_where(np.ma.getdata(radiance) == 0.0, radiance)

    if radiance.size == 0 or np.ma.count(radiance) == 0:
        raise ValueError("Perseverance Mastcam-Z radiance image contains no valid data")

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    image = ax.imshow(radiance, origin="upper")
    ax.set_title("Perseverance Mastcam-Z Radiance (Band 0)")
    ax.set_xlabel("Sample (downsampled)")
    ax.set_ylabel("Line (downsampled)")
    fig.colorbar(image, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "pds4_perseverance_mastcamz",
        "Perseverance Mastcam-Z Radiance",
        "Grayscale radiance image of Perseverance Mastcam-Z band 0 opened through the NetCDF PDS4 UDF interface, scaled by the label's scaling_factor and downsampled for publication.",
        "Grayscale image of Perseverance Mastcam-Z radiance with masked zero values, downsampled to approximately 1000 pixels per axis.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

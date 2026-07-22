"""Plot the first plane of a FITS image through the NetCDF UDF interface.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import os
from pathlib import Path

import matplotlib.pyplot as plt
from netCDF4 import Dataset

from plot_common import save_with_metadata


def main():
    parser = argparse.ArgumentParser(
        description="Plot a FITS image through the NetCDF UDF interface."
    )
    parser.add_argument("path", nargs="?", default=os.environ.get("NEP_VIZ_FITS_FILE"))
    args = parser.parse_args()
    if not args.path:
        parser.error("a FITS file path is required (or set NEP_VIZ_FITS_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as ds:
        data = ds.variables["image"][0, :, :]

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    im = ax.imshow(data, origin="lower")
    ax.set_title("WFPC2 FITS Sample Image")
    ax.set_xlabel("x pixel")
    ax.set_ylabel("y pixel")
    fig.colorbar(im, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "fits_wfpc2_image",
        "WFPC2 FITS Sample Image",
        "Grayscale image of the first data plane from the WFPC2u5780205r_c0fx.fits test file opened through the NetCDF UDF interface.",
        "Grayscale plot of a 200 by 200 pixel WFPC2 FITS image.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

"""Plot the single-frame MR-MONO2-16-head DICOM image through the NetCDF UDF interface.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import os
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from _dicom_udf import read_pixel_data
from plot_common import save_with_metadata


def _contrast_window(data):
    """Return a vmin/vmax window focused on the nonzero tissue values."""
    data = np.asarray(data)
    nonzero = data[data > 0]
    if nonzero.size == 0:
        return float(data.min()), float(data.max())
    low = np.percentile(nonzero, 1)
    high = np.percentile(nonzero, 99.5)
    if low == high:
        low, high = float(data.min()), float(data.max())
    return low, high


def main():
    parser = argparse.ArgumentParser(
        description="Plot the MR-MONO2-16-head DICOM image through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_DICOM_MR_HEAD_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "a DICOM MR head file path is required (or set NEP_VIZ_DICOM_MR_HEAD_FILE)"
        )

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    data = read_pixel_data(path)[0, :, :]

    vmin, vmax = _contrast_window(data)

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    im = ax.imshow(data, origin="lower", cmap="gray", vmin=vmin, vmax=vmax)
    ax.set_title("MR Head DICOM Sample Image")
    ax.set_xlabel("column")
    ax.set_ylabel("row")
    fig.colorbar(im, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "dicom_mr_head_image",
        "MR Head DICOM Sample Image",
        "Grayscale image of the 256 by 256 pixel MR-MONO2-16-head test file opened through the NetCDF UDF interface, with 16-bit pixel values windowed for display.",
        "Grayscale plot of the MR-MONO2-16-head 16-bit MR DICOM image.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

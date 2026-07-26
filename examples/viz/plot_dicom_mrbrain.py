"""Plot the single-frame MRBRAIN DICOM image through the NetCDF UDF interface.

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


def _normalize_to_uint8(data):
    """Scale a signed or unsigned 16-bit array to 8-bit with percentile contrast stretch."""
    data = np.asarray(data)
    low = np.percentile(data, 2)
    high = np.percentile(data, 98)
    if high == low:
        return np.zeros(data.shape, dtype=np.uint8)
    scaled = (data.astype(np.float64) - low) / (high - low)
    return np.clip(scaled * 255, 0, 255).astype(np.uint8)


def main():
    parser = argparse.ArgumentParser(
        description="Plot the MRBRAIN DICOM image through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_DICOM_MRBRAIN_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "a DICOM MRBRAIN file path is required (or set NEP_VIZ_DICOM_MRBRAIN_FILE)"
        )

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    data = read_pixel_data(path)[0, :, :]

    image = _normalize_to_uint8(data)

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    im = ax.imshow(image, origin="lower", cmap="viridis")
    ax.set_title("MRBRAIN DICOM Sample Image")
    ax.set_xlabel("column")
    ax.set_ylabel("row")
    fig.colorbar(im, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "dicom_mrbrain_image",
        "MRBRAIN DICOM Sample Image",
        "Grayscale image of the 512 by 512 pixel MRBRAIN.DCM test file opened through the NetCDF UDF interface, with 16-bit pixel values normalized to 8-bit grayscale.",
        "Grayscale plot of the MRBRAIN 16-bit MR DICOM image.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

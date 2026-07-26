"""Plot a montage of all frames from the 0003.DCM DICOM file.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import math
import os
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from _dicom_udf import read_pixel_data
from plot_common import save_with_metadata


def _to_luminance(frame):
    """Convert an RGB frame to grayscale; pass through existing 2-D frames."""
    frame = np.asarray(frame)
    if frame.ndim == 2:
        return frame
    if frame.ndim == 3 and frame.shape[2] == 3:
        return (
            0.299 * frame[..., 0]
            + 0.587 * frame[..., 1]
            + 0.114 * frame[..., 2]
        )
    raise ValueError(f"unsupported frame shape: {frame.shape}")


def _build_montage(frames):
    """Arrange a sequence of 2-D frames into a compact rectangular grid."""
    nframes = len(frames)
    ncols = math.ceil(math.sqrt(nframes))
    nrows = math.ceil(nframes / ncols)

    height, width = frames[0].shape
    montage = np.zeros((nrows * height, ncols * width), dtype=frames[0].dtype)

    for idx, frame in enumerate(frames):
        row = idx // ncols
        col = idx % ncols
        montage[row * height : (row + 1) * height, col * width : (col + 1) * width] = frame

    return montage, nrows, ncols


def main():
    parser = argparse.ArgumentParser(
        description="Plot a montage of DICOM XA frames through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_DICOM_XA_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "a DICOM XA file path is required (or set NEP_VIZ_DICOM_XA_FILE)"
        )

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    data = read_pixel_data(path)
    nframes = data.shape[0]
    frames = [_to_luminance(data[i, :, :]) for i in range(nframes)]

    montage, nrows, ncols = _build_montage(frames)

    fig, ax = plt.subplots(figsize=(8.0, 6.1))
    im = ax.imshow(montage, origin="lower", cmap="viridis")
    ax.set_title("DICOM XA Frame Montage")
    ax.set_xlabel("columns of frames")
    ax.set_ylabel("rows of frames")
    ax.set_xticks([])
    ax.set_yticks([])
    fig.colorbar(im, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "dicom_xa_frame_montage",
        "DICOM XA Frame Montage",
        f"Montage of all {nframes} frames from the 0003.DCM test file opened through the NetCDF UDF interface, arranged in a {nrows} by {ncols} grayscale grid.",
        f"Grayscale montage of {nframes} X-ray angiography DICOM frames.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

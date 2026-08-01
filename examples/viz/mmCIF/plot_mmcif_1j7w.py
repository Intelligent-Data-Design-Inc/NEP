"""Plot a 3D scatter of the 1J7W.cif X-ray crystal structure through the NetCDF UDF interface.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import os
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  (registers 3D projection)

from _mmcif_udf import read_structure

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from plot_common import save_with_metadata


def main():
    parser = argparse.ArgumentParser(
        description="Plot a 3D scatter of the 1J7W.cif X-ray crystal structure."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_MMCIF_1J7W_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "an mmCIF X-ray structure file path is required (or set NEP_VIZ_MMCIF_1J7W_FILE)"
        )

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    structure = read_structure(path)
    x = structure["x"][0, :]
    y = structure["y"][0, :]
    z = structure["z"][0, :]
    group = np.asarray(structure["group"])

    is_atom = group == "ATOM"
    is_hetatm = ~is_atom

    fig = plt.figure(figsize=(6.5, 6.0))
    ax = fig.add_subplot(111, projection="3d")
    ax.scatter(x[is_atom], y[is_atom], z[is_atom], s=2, c="black", marker=".", label="ATOM")
    ax.scatter(x[is_hetatm], y[is_hetatm], z[is_hetatm], s=8, c="0.6", marker="^", label="HETATM")
    ax.set_title("1J7W Structure")
    ax.set_xlabel("x (\u00c5)")
    ax.set_ylabel("y (\u00c5)")
    ax.set_zlabel("z (\u00c5)")
    ax.legend(loc="upper right", fontsize="small")

    save_with_metadata(
        fig,
        "mmcif_1j7w_structure",
        "1J7W Structure",
        "3D scatter of ATOM and HETATM Cartesian coordinates from the 1J7W.cif deoxy haemoglobin beta-Y-Q mutant crystal structure, opened through the NetCDF mmCIF UDF interface.",
        "3D scatter plot of the 1J7W haemoglobin mutant crystal structure atom coordinates.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

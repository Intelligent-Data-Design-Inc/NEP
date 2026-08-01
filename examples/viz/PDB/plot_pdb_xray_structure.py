"""Plot a 3D scatter of the 4HHB.pdb X-ray crystal structure through the NetCDF UDF interface.

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

from _pdb_udf import read_structure

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from plot_common import save_with_metadata


def main():
    parser = argparse.ArgumentParser(
        description="Plot a 3D scatter of the 4HHB.pdb X-ray crystal structure."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_PDB_XRAY_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "a PDB X-ray structure file path is required (or set NEP_VIZ_PDB_XRAY_FILE)"
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
    ax.set_title("4HHB X-ray Structure")
    ax.set_xlabel("x (\u00c5)")
    ax.set_ylabel("y (\u00c5)")
    ax.set_zlabel("z (\u00c5)")
    ax.legend(loc="upper right", fontsize="small")

    save_with_metadata(
        fig,
        "pdb_xray_structure",
        "4HHB X-ray Structure",
        "3D scatter of ATOM and HETATM Cartesian coordinates from the 4HHB.pdb hemoglobin X-ray crystal structure, opened through the NetCDF UDF interface.",
        "3D scatter plot of the 4HHB hemoglobin X-ray crystal structure atom coordinates.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

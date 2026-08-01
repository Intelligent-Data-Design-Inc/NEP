"""Plot a multi-model overlay of the 1GAB.pdb NMR ensemble through the NetCDF UDF interface.

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
        description="Plot a multi-model overlay of the 1GAB.pdb NMR ensemble."
    )
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("NEP_VIZ_PDB_NMR_FILE"),
    )
    args = parser.parse_args()
    if not args.path:
        parser.error(
            "a PDB NMR ensemble file path is required (or set NEP_VIZ_PDB_NMR_FILE)"
        )

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    structure = read_structure(path)
    x = structure["x"]
    y = structure["y"]
    z = structure["z"]
    nmodels = x.shape[0]

    fig = plt.figure(figsize=(6.5, 6.0))
    ax = fig.add_subplot(111, projection="3d")
    shades = np.linspace(0.15, 0.85, nmodels) if nmodels > 1 else [0.0]
    for m in range(nmodels):
        gray = str(shades[m])
        ax.plot(x[m, :], y[m, :], z[m, :], color=gray, linewidth=0.5, alpha=0.7)
    ax.set_title(f"1GAB NMR Ensemble ({nmodels} models)")
    ax.set_xlabel("x (\u00c5)")
    ax.set_ylabel("y (\u00c5)")
    ax.set_zlabel("z (\u00c5)")

    save_with_metadata(
        fig,
        "pdb_nmr_ensemble",
        "1GAB NMR Ensemble",
        "Overlay of backbone atom coordinates across all 20 MODEL blocks of the 1GAB.pdb NMR ensemble, opened through the NetCDF UDF interface, showing conformational variation between models.",
        "3D overlay plot of 20 NMR models from the 1GAB protein structure.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

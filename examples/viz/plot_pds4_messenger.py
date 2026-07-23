"""Plot the MESSENGER Mercury thermal neutron map through the NetCDF UDF interface.

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
        description="Plot the MESSENGER thermal neutron map through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_PDS4_MESSENGER_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a PDS4 MESSENGER file path is required (or set NEP_VIZ_PDS4_MESSENGER_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as ds:
        if "thermal_neutron_map.img" not in ds.groups:
            raise ValueError("MESSENGER dataset does not contain the thermal_neutron_map.img group")
        grp = ds.groups["thermal_neutron_map.img"]

        if "Mercury Thermal Neutron Map" not in grp.variables:
            raise ValueError("MESSENGER group does not contain the Mercury Thermal Neutron Map variable")
        data_var = grp.variables["Mercury Thermal Neutron Map"]
        if data_var.ndim != 2:
            raise ValueError(f"MESSENGER data must be two-dimensional, got {data_var.ndim}")

        data = np.ma.masked_invalid(np.ma.asarray(data_var[:, :]))
        data = np.ma.masked_where(np.ma.getdata(data) == 0, data)

    if data.size == 0 or np.ma.count(data) == 0:
        raise ValueError("MESSENGER thermal neutron map contains no valid data")

    fig, ax = plt.subplots(figsize=(6.0, 5.0))
    image = ax.imshow(data, origin="upper")
    ax.set_title("MESSENGER Mercury Thermal Neutron Map")
    ax.set_xlabel("Sample")
    ax.set_ylabel("Line")
    fig.colorbar(image, ax=ax, shrink=0.7)

    save_with_metadata(
        fig,
        "pds4_messenger_tnmap",
        "MESSENGER Mercury Thermal Neutron Map",
        "Grayscale map of the MESSENGER thermal neutron map opened through the NetCDF PDS4 UDF interface.",
        "Grayscale image of Mercury's thermal neutron absorption from the MESSENGER GRS ACS data.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

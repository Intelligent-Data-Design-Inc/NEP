"""Plot MAVEN NGIMS L3 scale-height temperature through the NetCDF UDF interface.

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
        description="Plot MAVEN NGIMS L3 temperature through the NetCDF UDF interface."
    )
    parser.add_argument(
        "path", nargs="?", default=os.environ.get("NEP_VIZ_PDS4_MAVEN_L3_FILE")
    )
    args = parser.parse_args()
    if not args.path:
        parser.error("a PDS4 MAVEN L3 file path is required (or set NEP_VIZ_PDS4_MAVEN_L3_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    group_name = "mvn_ngi_l3_res-sht-58942_20250101T010116_v06_r03.csv"

    with Dataset(str(path), mode="r") as ds:
        if group_name not in ds.groups:
            raise ValueError(f"MAVEN L3 dataset does not contain the {group_name} group")
        grp = ds.groups[group_name]

        if "T_UNIX" not in grp.variables:
            raise ValueError("MAVEN L3 group does not contain the T_UNIX variable")
        if "TEMPERATURE" not in grp.variables:
            raise ValueError("MAVEN L3 group does not contain the TEMPERATURE variable")

        time_var = grp.variables["T_UNIX"]
        temp_var = grp.variables["TEMPERATURE"]
        if time_var.ndim != 1 or temp_var.ndim != 1:
            raise ValueError("MAVEN L3 T_UNIX and TEMPERATURE must be one-dimensional")
        if time_var.shape[0] != temp_var.shape[0]:
            raise ValueError("MAVEN L3 T_UNIX and TEMPERATURE have incompatible lengths")

        time = np.ma.masked_invalid(np.ma.asarray(time_var[:]))
        temperature = np.ma.masked_invalid(np.ma.asarray(temp_var[:]))

    valid = ~(np.ma.getmaskarray(time) | np.ma.getmaskarray(temperature))
    if np.count_nonzero(valid) == 0:
        raise ValueError("MAVEN L3 dataset contains no valid TIME/TEMPERATURE pairs")

    time = np.ma.getdata(time)[valid]
    temperature = np.ma.getdata(temperature)[valid]

    fig, ax = plt.subplots(figsize=(6.0, 4.0))
    ax.plot(time, temperature, color="black", linestyle="-", marker="o")
    ax.set_title("MAVEN NGIMS L3 Temperature")
    ax.set_xlabel("Unix time (s)")
    ax.set_ylabel("Temperature (K)")

    save_with_metadata(
        fig,
        "pds4_maven_ngims_l3",
        "MAVEN NGIMS L3 Temperature",
        "Line plot of MAVEN NGIMS L3 derived temperature versus Unix time, opened through the NetCDF PDS4 UDF interface.",
        "A black line plots MAVEN NGIMS L3 temperature in Kelvin against Unix epoch seconds.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

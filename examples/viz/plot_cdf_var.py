"""Plot a CDF zVariable through the NetCDF UDF interface.

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
        description="Plot a CDF zVariable through the NetCDF UDF interface."
    )
    parser.add_argument("path", nargs="?", default=os.environ.get("NEP_VIZ_CDF_FILE"))
    args = parser.parse_args()
    if not args.path:
        parser.error("a CDF file path is required (or set NEP_VIZ_CDF_FILE)")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as ds:
        temperature = ds.variables["temperature"][0, :]

    fig, ax = plt.subplots(figsize=(6.0, 4.0))
    ax.plot(np.arange(len(temperature)), temperature, color="black", linestyle="-")
    ax.set_title("CDF Temperature Sample")
    ax.set_xlabel("Index")
    ax.set_ylabel("Temperature")

    save_with_metadata(
        fig,
        "cdf_temperature",
        "CDF Temperature Sample",
        "Line plot of the temperature zVariable from the tst_cdf_simple.cdf test file opened through the NetCDF UDF interface.",
        "A black line plots ten temperature values from 20.0 to 24.5 against their array index.",
    )
    plt.close(fig)


if __name__ == "__main__":
    main()

import argparse
import os
from pathlib import Path

from netCDF4 import Dataset


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs="?", default=os.environ.get("NEP_VIZ_UDF_FILE"))
    args = parser.parse_args()
    if not args.path:
        parser.error("a UDF file path is required")

    path = Path(args.path)
    if not path.is_file():
        raise FileNotFoundError(path)

    with Dataset(str(path), mode="r") as dataset:
        if not dataset.variables:
            raise RuntimeError(f"UDF dataset contains no variables: {path}")


if __name__ == "__main__":
    main()

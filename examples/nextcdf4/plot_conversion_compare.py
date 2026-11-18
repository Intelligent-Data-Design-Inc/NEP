"""Plot paired NEXTCDF-4 conversion measurements without relabeling history.

Usage:
    plot_conversion_compare.py OFF_CSV ON_CSV OFF_IO_CSV ON_IO_CSV PREFIX
    plot_conversion_compare.py --numeric OFF_CSV ON_CSV PREFIX

Outputs PREFIX_typed.png and PREFIX_io.png, or PREFIX_numeric.png.
Times include write fsync and reads after a cache-eviction request; the
headline result is the minimum of ten repetitions. Throughput uses the
memory data volume in each benchmark, not compressed file bytes.

Author: Edward Hartnett, Intelligent Data Design, Inc.
"""
import csv
import math
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

FIELDS = ("tmp2m", "prate_ave", "soilw1", "land")
STORAGE = ("contiguous", "deflate1")
CASES = ("int_short", "double_float", "float_double", "double_float_bitgroom",
         "double_float_granularbr", "double_float_bitround")
METRICS = ("write_s", "read_s", "write_MBps", "read_MBps")


def load(path, keys, expected, engine=None, numeric=False):
    """Read a complete, unique matrix with positive finite timing values."""
    rows = {}
    required = set(keys) | set(METRICS) | {"copies", "file_bytes"}
    if numeric:
        required |= {"engine", "nvalues", "memory_bytes"}
    else:
        required |= {"uncompressed_bytes", "ratio_vs_float"}
    with open(path, newline="") as stream:
        reader = csv.DictReader(stream)
        if not required.issubset(reader.fieldnames or ()):
            raise ValueError(f"{path}: missing required columns")
        for row in reader:
            key = tuple(row[name] for name in keys)
            if key not in expected or key in rows:
                raise ValueError(f"{path}: unexpected or duplicate row {key}")
            if numeric and row["engine"] != engine:
                raise ValueError(f"{path}: expected engine {engine}")
            for name in METRICS:
                value = float(row[name])
                if not math.isfinite(value) or value <= 0:
                    raise ValueError(f"{path}: invalid {name} at {key}")
            if int(row["copies"]) != 10 or int(row["file_bytes"]) <= 0:
                raise ValueError(f"{path}: invalid copies or file size at {key}")
            if numeric:
                nvalues = int(row["nvalues"])
                memory_bytes = int(row["memory_bytes"])
                size = 4 if row["case"] in ("int_short", "float_double") else 8
                if nvalues != 10 * 768 * 1536 or memory_bytes != nvalues * size:
                    raise ValueError(f"{path}: invalid memory volume at {key}")
                for operation in ("write", "read"):
                    calculated = memory_bytes / float(row[f"{operation}_s"]) / 1e6
                    if not math.isclose(calculated, float(row[f"{operation}_MBps"]), rel_tol=1e-5):
                        raise ValueError(f"{path}: inconsistent throughput at {key}")
            elif int(row["uncompressed_bytes"]) <= 0:
                raise ValueError(f"{path}: invalid data volume at {key}")
            rows[key] = row
    if set(rows) != expected:
        raise ValueError(f"{path}: missing rows {sorted(expected - set(rows))}")
    return rows


def chart(series, labels, title, output):
    """Draw write/read throughput panels, validating equal series lengths."""
    if not labels or any(len(rows) != len(labels) for _, rows in series):
        raise ValueError("inconsistent plot series")
    fig, axes = plt.subplots(2, 1, figsize=(max(10, len(labels) * 0.8), 9), sharex=True)
    width = 0.8 / len(series)
    for ax, metric, operation in zip(axes, ("write_MBps", "read_MBps"), ("Write", "Read")):
        for index, (name, rows) in enumerate(series):
            positions = [x - 0.4 + (index + 0.5) * width for x in range(len(labels))]
            ax.bar(positions, [float(row[metric]) for row in rows], width,
                   color="white", edgecolor="black", hatch=("", "///", "...", "xxx")[index],
                   label=name)
        ax.set_ylabel(f"{operation} MB/s (memory volume)")
        ax.grid(axis="y", linestyle="--", alpha=0.3)
        ax.set_axisbelow(True)
    axes[0].legend()
    axes[1].set_xticks(range(len(labels)), labels, rotation=35, ha="right")
    fig.suptitle(title + "\nMinimum of 10 runs; write fsync; read cache-eviction request")
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    fig.savefig(output, dpi=150)
    plt.close(fig)
    print(f"Saved {output}")


def numeric(off_path, on_path, prefix):
    """Plot all six standard conversion cases and both storage layouts."""
    keys = [(case, storage) for case in CASES for storage in STORAGE]
    off = load(off_path, ("case", "storage"), set(keys), "hdf5", True)
    on = load(on_path, ("case", "storage"), set(keys), "internal", True)
    chart([("HDF5 / legacy (OFF)", [off[key] for key in keys]),
           ("Internal (ON)", [on[key] for key in keys])],
          [f"{case}\n{storage}" for case, storage in keys],
          "NEXTCDF-4 standard numeric conversion", f"{prefix}_numeric.png")


def gdas(off_path, on_path, off_io_path, on_io_path, prefix):
    """Plot engine-labeled typed bfloat16 conversion and raw-I/O controls."""
    def matrix(types):
        return {(field, kind, storage) for field in FIELDS for kind in types for storage in STORAGE}
    off = load(off_path, ("field", "type", "storage"), matrix(("float", "bfloat16", "bfloat16_hdf5")))
    on = load(on_path, ("field", "type", "storage"), matrix(("float", "bfloat16", "bfloat16_internal")))
    off_io = load(off_io_path, ("field", "type", "storage"), matrix(("float", "bfloat16")))
    on_io = load(on_io_path, ("field", "type", "storage"), matrix(("float", "bfloat16")))
    keys = [(field, storage) for field in FIELDS for storage in STORAGE]
    labels = [f"{field}\n{storage}" for field, storage in keys]
    chart([("Typed HDF5 (OFF)", [off[field, "bfloat16_hdf5", storage] for field, storage in keys]),
           ("Typed internal (ON)", [on[field, "bfloat16_internal", storage] for field, storage in keys])],
          labels, "NEXTCDF-4 typed bfloat16 conversion", f"{prefix}_typed.png")
    chart([(f"{kind}, {engine}", [rows[field, kind, storage] for field, storage in keys])
           for kind in ("float", "bfloat16") for engine, rows in (("OFF", off_io), ("ON", on_io))],
          labels, "NEXTCDF-4 raw I/O controls (no conversion)", f"{prefix}_io.png")


def main(args):
    """Validate command-line inputs and return a process status."""
    try:
        if len(args) == 4 and args[0] == "--numeric":
            numeric(*args[1:])
        elif len(args) == 5 and args[0] != "--numeric":
            gdas(*args)
        else:
            print(__doc__, file=sys.stderr)
            return 2
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"conversion comparison: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

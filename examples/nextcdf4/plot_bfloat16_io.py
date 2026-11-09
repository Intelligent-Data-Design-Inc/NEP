"""Plot the NEXTCDF-4 bfloat16 no-conversion I/O benchmark results.

Usage:

    python3 plot_bfloat16_io.py <results.csv> <prefix>

Input CSV columns (written by examples/nextcdf4/bfloat16_io_perf):

    field,type,storage,copies,uncompressed_bytes,file_bytes,ratio_vs_float,
    write_s,read_s,write_MBps,read_MBps,write_speedup,read_speedup

Output:

    <prefix>_io.png  two stacked panels of write/read throughput
                     (MB/s of float32 data)

The figure is black and white with hatched bars, matching the style of
the other example plot scripts. Four configurations are plotted:
float32 and bfloat16, each contiguous and deflate1. The bfloat16 data
were already stored as bfloat16, so the times are pure I/O with no
conversion.

Author: Edward Hartnett, Intelligent Data Design, Inc.
"""

import csv
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

FIELDS = ["tmp2m", "prate_ave", "soilw1", "land"]
CONFIGS = [
    ("float", "contiguous"),
    ("bfloat16", "contiguous"),
    ("float", "deflate1"),
    ("bfloat16", "deflate1"),
]
LABELS = ["float32 contiguous", "bfloat16 contiguous",
          "float32 deflate1", "bfloat16 deflate1"]
HATCHES = ["", "///", "...", "xxx"]


def read_rows(path):
    """Read the CSV into {(field, type, storage): row}."""
    rows = {}
    with open(path, newline="") as fp:
        for row in csv.DictReader(fp):
            rows[(row["field"], row["type"], row["storage"])] = row
    return rows


def grouped_bars(ax, rows, column, scale=1.0):
    """Draw the 4-fields x 4-configs grouped bar chart on ax."""
    group_w = 0.8
    bar_w = group_w / len(CONFIGS)
    for c, (typ, storage) in enumerate(CONFIGS):
        vals = []
        for field in FIELDS:
            row = rows.get((field, typ, storage))
            vals.append(float(row[column]) * scale if row else 0.0)
        x = [i - group_w / 2 + bar_w * (c + 0.5) for i in range(len(FIELDS))]
        ax.bar(x, vals, bar_w, color="white", edgecolor="black",
               hatch=HATCHES[c], label=LABELS[c])
    ax.set_xticks(range(len(FIELDS)))
    ax.set_xticklabels(FIELDS)
    ax.yaxis.grid(True, linestyle="--", alpha=0.3)
    ax.set_axisbelow(True)


def main():
    if len(sys.argv) != 3:
        print("usage: plot_bfloat16_io.py <results.csv> <prefix>", file=sys.stderr)
        return 2
    csv_path, prefix = sys.argv[1], sys.argv[2]
    try:
        rows = read_rows(csv_path)
    except OSError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    # Write/read throughput chart, in MB/s of float32 data, with data
    # already in bfloat16.
    fig, (ax_w, ax_r) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)
    grouped_bars(ax_w, rows, "write_MBps")
    ax_w.set_ylabel("write (MB/s of float32 data)")
    grouped_bars(ax_r, rows, "read_MBps")
    ax_r.set_ylabel("read (MB/s of float32 data)")
    fig.suptitle("NEXTCDF-4 disk write/read throughput: float32 vs data already in bfloat16")
    handles, labels = ax_w.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=2,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.08, 1, 0.95))
    io_png = f"{prefix}_io.png"
    fig.savefig(io_png, dpi=150)
    print(f"Saved {io_png}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

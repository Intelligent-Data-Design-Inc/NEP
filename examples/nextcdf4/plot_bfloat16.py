"""Plot NEXTCDF-4 bfloat16 benchmark results.

Usage:

    python3 plot_bfloat16.py <results.csv> <prefix>

Input CSV columns (written by examples/nextcdf4/bfloat16_perf):

    field,type,storage,uncompressed_bytes,file_bytes,ratio_vs_float,
    write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,
    wconv_s,rconv_s

Outputs:

    <prefix>_size.png   grouped bar chart of file size (MB) per field
    <prefix>_speed.png  two stacked panels of write/read throughput (MB/s)

Both figures are black and white with hatched bars, matching the style
of the other example plot scripts.

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
COLORS = ["white", "white", "white", "white"]
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
        ax.bar(x, vals, bar_w, color=COLORS[c], edgecolor="black",
               hatch=HATCHES[c], label=LABELS[c])
    ax.set_xticks(range(len(FIELDS)))
    ax.set_xticklabels(FIELDS)
    ax.yaxis.grid(True, linestyle="--", alpha=0.3)
    ax.set_axisbelow(True)


def main():
    if len(sys.argv) != 3:
        print("usage: plot_bfloat16.py <results.csv> <prefix>", file=sys.stderr)
        return 2
    csv_path, prefix = sys.argv[1], sys.argv[2]
    try:
        rows = read_rows(csv_path)
    except OSError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    # File size chart.
    fig, ax = plt.subplots(figsize=(8, 5))
    grouped_bars(ax, rows, "file_bytes", scale=1e-6)
    ax.set_ylabel("file size (MB)")
    ax.set_title("NEXTCDF-4 file size: float32 vs bfloat16")
    handles, labels = ax.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=4,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.06, 1, 1))
    size_png = f"{prefix}_size.png"
    fig.savefig(size_png, dpi=150)
    print(f"Saved {size_png}")

    # Write/read throughput chart.
    fig, (ax_w, ax_r) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)
    grouped_bars(ax_w, rows, "write_MBps")
    ax_w.set_ylabel("write (MB/s of float32 data)")
    grouped_bars(ax_r, rows, "read_MBps")
    ax_r.set_ylabel("read (MB/s of float32 data)")
    fig.suptitle("NEXTCDF-4 write/read throughput: float32 vs bfloat16")
    handles, labels = ax_w.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=4,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.06, 1, 0.95))
    speed_png = f"{prefix}_speed.png"
    fig.savefig(speed_png, dpi=150)
    print(f"Saved {speed_png}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

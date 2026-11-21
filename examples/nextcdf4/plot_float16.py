"""Plot NEXTCDF-4 float16 benchmark results.

Usage:

    python3 plot_float16.py <results.csv> <prefix>

Input CSV columns (written by examples/nextcdf4/float16_perf):

    field,type,storage,copies,uncompressed_bytes,file_bytes,ratio_vs_float,
    write_s,read_s,write_MBps,read_MBps,max_abs_err,max_rel_err,
    wconv_s,rconv_s

Outputs:

    <prefix>_size.png        grouped bar chart of file size (MB) per field
                             (each file holds `copies` copies of the field)
    <prefix>_speed.png       two stacked panels of write/read throughput
                             (MB/s of float32 data)
    <prefix>_typed_speed.png  the same two panels with all six
                             configurations, adding float16 (typed)

All figures are black and white with hatched bars, matching the style
of the other example plot scripts. Six configurations are recorded in
the CSV: float32 and two float16 conversion paths (user code and the
internal engine via typed calls), each contiguous and deflate1. The
main charts (<prefix>_size.png and <prefix>_speed.png) leave out
float16_typed for comparability with the bfloat16 figures; the typed
chart shows all six. The float16_typed rows come from typed
nc_put_vara_float/nc_get_vara_float I/O, where the internal
conversion engine does the conversion.

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
    ("float16", "contiguous"),
    ("float16_typed", "contiguous"),
    ("float", "deflate1"),
    ("float16", "deflate1"),
    ("float16_typed", "deflate1"),
]
LABELS = ["float32 contiguous", "float16 contiguous", "float16 (typed) contiguous",
          "float32 deflate1", "float16 deflate1", "float16 (typed) deflate1"]
COLORS = ["white", "white", "white", "white", "white", "white"]
HATCHES = ["", "///", "\\\\\\", "...", "xxx", "+++"]
# Configurations shown in the main charts (float32/float16 x
# contiguous/deflate1); the float16_typed rows get their own chart.
MAIN = [0, 1, 3, 4]


def read_rows(path):
    """Read the CSV into {(field, type, storage): row}."""
    rows = {}
    with open(path, newline="") as fp:
        for row in csv.DictReader(fp):
            rows[(row["field"], row["type"], row["storage"])] = row
    return rows


def grouped_bars(ax, rows, column, scale=1.0, configs=range(len(CONFIGS))):
    """Draw the 4-fields x len(configs) grouped bar chart on ax."""
    group_w = 0.8
    bar_w = group_w / len(configs)
    for pos, c in enumerate(configs):
        typ, storage = CONFIGS[c]
        vals = []
        for field in FIELDS:
            row = rows.get((field, typ, storage))
            vals.append(float(row[column]) * scale if row else 0.0)
        x = [i - group_w / 2 + bar_w * (pos + 0.5) for i in range(len(FIELDS))]
        ax.bar(x, vals, bar_w, color=COLORS[c], edgecolor="black",
               hatch=HATCHES[c], label=LABELS[c])
    ax.set_xticks(range(len(FIELDS)))
    ax.set_xticklabels(FIELDS)
    ax.yaxis.grid(True, linestyle="--", alpha=0.3)
    ax.set_axisbelow(True)


def main():
    if len(sys.argv) != 3:
        print("usage: plot_float16.py <results.csv> <prefix>", file=sys.stderr)
        return 2
    csv_path, prefix = sys.argv[1], sys.argv[2]
    try:
        rows = read_rows(csv_path)
    except OSError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    copies = next(iter(rows.values()))["copies"]

    # File size chart (main configurations only).
    fig, ax = plt.subplots(figsize=(8, 5))
    grouped_bars(ax, rows, "file_bytes", scale=1e-6, configs=MAIN)
    ax.set_ylabel(f"file size (MB, {copies} copies of the field)")
    ax.set_title("NEXTCDF-4 file size: float32 vs float16")
    handles, labels = ax.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=2,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.10, 1, 1))
    size_png = f"{prefix}_size.png"
    fig.savefig(size_png, dpi=150)
    print(f"Saved {size_png}")

    # Write/read throughput chart (main configurations only), in MB/s
    # of float32 data, including any conversion.
    fig, (ax_w, ax_r) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)
    grouped_bars(ax_w, rows, "write_MBps", configs=MAIN)
    ax_w.set_ylabel("write (MB/s of float32 data)")
    grouped_bars(ax_r, rows, "read_MBps", configs=MAIN)
    ax_r.set_ylabel("read (MB/s of float32 data)")
    fig.suptitle("NEXTCDF-4 disk write/read throughput: float32 vs float16")
    handles, labels = ax_w.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=2,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.10, 1, 0.95))
    speed_png = f"{prefix}_speed.png"
    fig.savefig(speed_png, dpi=150)
    print(f"Saved {speed_png}")

    # The same chart with all six configurations, adding the typed calls
    # conversion path.
    fig, (ax_w, ax_r) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)
    grouped_bars(ax_w, rows, "write_MBps")
    ax_w.set_ylabel("write (MB/s of float32 data)")
    grouped_bars(ax_r, rows, "read_MBps")
    ax_r.set_ylabel("read (MB/s of float32 data)")
    fig.suptitle("float16 conversion: user code vs internal engine (disk write/read throughput)")
    handles, labels = ax_w.get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=3,
               frameon=True, edgecolor="black")
    fig.tight_layout(rect=(0, 0.10, 1, 0.95))
    typed_png = f"{prefix}_typed_speed.png"
    fig.savefig(typed_png, dpi=150)
    print(f"Saved {typed_png}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

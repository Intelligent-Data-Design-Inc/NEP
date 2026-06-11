"""Generate LZ4 performance plots from lz4.c CSV output (black and white).

Usage:
    ./lz4 > lz4_results.csv
    python3 plot_lz4.py

Reads:  lz4_results.csv
Writes: lz4_performance.jpg

Columns expected: lz4_level,shuffle,compressed_bytes,ratio,write_s,read_s
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

INPUT  = "lz4_results.csv"
OUTPUT = "lz4_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "level":   int(row["lz4_level"]),
                "shuffle": int(row["shuffle"]),
                "ratio":   float(row["ratio"]),
                "write_s": float(row["write_s"]),
                "read_s":  float(row["read_s"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './lz4 > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

levels = sorted(set(r["level"] for r in rows))

def series(shuffle_val, key):
    """Return values for the given shuffle setting, sorted by level."""
    return [r[key] for r in sorted(rows, key=lambda x: x["level"])
            if r["shuffle"] == shuffle_val]

ratio_no_shuffle  = series(0, "ratio")
ratio_shuffle     = series(1, "ratio")
write_no_shuffle  = series(0, "write_s")
write_shuffle     = series(1, "write_s")
read_no_shuffle   = series(0, "read_s")
read_shuffle      = series(1, "read_s")

# ---------------------------------------------------------------------------
# Plot: 3 subplots stacked vertically
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(3, 1, figsize=(9, 12), sharex=True)

line_styles = {
    "no_shuffle": {"linestyle": "-",  "marker": "o", "color": "black",  "label": "shuffle=off"},
    "shuffle":    {"linestyle": "--", "marker": "s", "color": "black",  "label": "shuffle=on",
                   "markerfacecolor": "white"},
}

# --- Subplot 1: Compression ratio ---
ax = axes[0]
ax.plot(levels, ratio_no_shuffle, **line_styles["no_shuffle"])
ax.plot(levels, ratio_shuffle,    **line_styles["shuffle"])
ax.set_ylabel("Compression ratio\n(uncompressed / compressed)", fontsize=10)
ax.set_title("NetCDF-4 LZ4 Compression: Ratio and Throughput vs. Level",
             fontsize=12)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 2: Write time ---
ax = axes[1]
ax.plot(levels, write_no_shuffle, **line_styles["no_shuffle"])
ax.plot(levels, write_shuffle,    **line_styles["shuffle"])
ax.set_ylabel("Write time (s)", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 3: Read time ---
ax = axes[2]
ax.plot(levels, read_no_shuffle, **line_styles["no_shuffle"])
ax.plot(levels, read_shuffle,    **line_styles["shuffle"])
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_xlabel("LZ4 level", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# Common x-axis ticks
axes[2].set_xticks(levels)

caption = "\n".join([
    "Dataset: 500\u00d7180\u00d7360 NC_FLOAT temperature (~129 MB uncompressed).",
    "Chunk shape: 10\u00d745\u00d790. Each point = one nc_put_var_float / nc_get_var_float call.",
    "",
    "shuffle=off: lz4 compresses raw float bytes.",
    "shuffle=on:  bytes are reordered by byte significance before compression,",
    "             increasing ratio at little cost (almost always beneficial for floats).",
    "             Shuffle enabled via nc_def_var_deflate(shuffle=1, deflate=0, level=0).",
    "",
    "Level 1: fastest compression, lowest ratio. Levels 6-9: better ratio, slower compression.",
    "Decompression speed is consistently fast across all levels.",
])
fig.text(0.5, 0.01, caption, ha="center", va="bottom", fontsize=8, color="#333333",
         multialignment="left", transform=fig.transFigure, fontfamily="monospace")

plt.tight_layout(rect=[0, 0.15, 1, 1])
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

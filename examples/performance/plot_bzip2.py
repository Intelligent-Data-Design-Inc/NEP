"""Generate BZIP2 performance plots from bzip2.c CSV output (black and white).

Usage:
    ./bzip2 > bzip2_results.csv
    python3 plot_bzip2.py

Reads:  bzip2_results.csv
Writes: bzip2_performance.jpg

Columns expected: bzip2_level,shuffle,compressed_bytes,ratio,write_s,read_s
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

INPUT  = "bzip2_results.csv"
OUTPUT = "bzip2_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "level":   int(row["bzip2_level"]),
                "shuffle": int(row["shuffle"]),
                "ratio":   float(row["ratio"]),
                "write_s": float(row["write_s"]),
                "read_s":  float(row["read_s"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './bzip2 > {INPUT}' first.",
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
fig, axes = plt.subplots(3, 1, figsize=(7, 6.1), sharex=True)

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
ax.set_title("NetCDF-4 BZIP2 Compression: Ratio and Throughput vs. Level",
             fontsize=12)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="black", alpha=0.3)
ax.set_axisbelow(True)

# --- Subplot 2: Write time ---
ax = axes[1]
ax.plot(levels, write_no_shuffle, **line_styles["no_shuffle"])
ax.plot(levels, write_shuffle,    **line_styles["shuffle"])
ax.set_ylabel("Write time (s)", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="black", alpha=0.3)
ax.set_axisbelow(True)

# --- Subplot 3: Read time ---
ax = axes[2]
ax.plot(levels, read_no_shuffle, **line_styles["no_shuffle"])
ax.plot(levels, read_shuffle,    **line_styles["shuffle"])
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_xlabel("BZIP2 level", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="black", alpha=0.3)
ax.set_axisbelow(True)

# Common x-axis ticks
axes[2].set_xticks(levels)

plt.tight_layout()
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

"""Generate endianness performance plots from endianness.c CSV output (black and white).

Usage:
    ./endianness > endianness_results.csv
    python3 plot_endianness.py

Reads:  endianness_results.csv
Writes: endianness_performance.jpg

Columns expected: endian_mode,write_s,read_s,file_bytes
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

INPUT  = "endianness_results.csv"
OUTPUT = "endianness_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "endian_mode": row["endian_mode"],
                "write_s":     float(row["write_s"]),
                "read_s":      float(row["read_s"]),
                "file_bytes":  int(row["file_bytes"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './endianness > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

# Extract data by endian mode
modes = ["native", "little", "big"]
x_pos = np.arange(len(modes))

write_times = [r["write_s"] for r in rows if r["endian_mode"] in modes]
read_times  = [r["read_s"] for r in rows if r["endian_mode"] in modes]

# ---------------------------------------------------------------------------
# Plot: 2 subplots (write time, read time)
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(1, 2, figsize=(10, 5))

bar_styles = {
    "native": {"color": "black",  "label": "NC_ENDIAN_NATIVE"},
    "little": {"color": "white",  "label": "NC_ENDIAN_LITTLE", "edgecolor": "black", "linewidth": 1.5, "hatch": "///"},
    "big":    {"color": "white",  "label": "NC_ENDIAN_BIG", "edgecolor": "black", "linewidth": 1.5, "hatch": "xxx"},
}

# Map modes to their styles
style_list = [bar_styles[m] for m in modes]

# --- Subplot 1: Write time ---
ax = axes[0]
for i, (mode, style) in enumerate(zip(modes, style_list)):
    val = [r["write_s"] for r in rows if r["endian_mode"] == mode][0]
    ax.bar(x_pos[i], val, 0.6, **style)
ax.set_ylabel("Write time (s)", fontsize=10)
ax.set_title("Write Performance", fontsize=11)
ax.set_xticks(x_pos)
ax.set_xticklabels(modes)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 2: Read time ---
ax = axes[1]
for i, (mode, style) in enumerate(zip(modes, style_list)):
    val = [r["read_s"] for r in rows if r["endian_mode"] == mode][0]
    ax.bar(x_pos[i], val, 0.6, **style)
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_title("Read Performance", fontsize=11)
ax.set_xticks(x_pos)
ax.set_xticklabels(modes)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# Overall title
fig.suptitle("NetCDF-4 Endianness Performance: Byte Order Effects",
             fontsize=12, fontweight="bold")

caption = "\n".join([
    "Dataset: 500x180x360 NC_FLOAT temperature (~129 MB uncompressed).",
    "native = platform native byte order. little = explicit little-endian.",
    "big = explicit big-endian. All tests use NetCDF-4/HDF5 format.",
    "",
    "NC_ENDIAN_NATIVE: Default byte order matching the platform.",
    "NC_ENDIAN_LITTLE: Forces little-endian storage (may require byte-swapping).",
    "NC_ENDIAN_BIG:    Forces big-endian storage (may require byte-swapping).",
    "",
    "Lower time is better. Performance impact depends on platform byte order.",
])
fig.text(0.5, -0.05, caption, ha="center", va="bottom", fontsize=8, color="#333333",
         multialignment="left", transform=fig.transFigure, fontfamily="monospace")

plt.tight_layout(rect=[0, 0.28, 1, 0.93])
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

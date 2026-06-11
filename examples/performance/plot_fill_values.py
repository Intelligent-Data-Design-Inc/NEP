"""Generate fill value performance plots from fill_values.c CSV output (black and white).

Usage:
    ./fill_values > fill_values_results.csv
    python3 plot_fill_values.py

Reads:  fill_values_results.csv
Writes: fill_values_performance.jpg

Columns expected: format,fill_mode,write_s,read_s,file_bytes
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

INPUT  = "fill_values_results.csv"
OUTPUT = "fill_values_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "format":     row["format"],
                "fill_mode":  int(row["fill_mode"]),
                "write_s":    float(row["write_s"]),
                "read_s":     float(row["read_s"]),
                "file_bytes": int(row["file_bytes"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './fill_values > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

# Extract data by format and fill mode
formats = ["classic", "nc4"]
x_pos = np.arange(len(formats))
width = 0.35  # Bar width

write_fill_on  = [r["write_s"] for r in rows if r["fill_mode"] == 1]
write_fill_off = [r["write_s"] for r in rows if r["fill_mode"] == 0]
read_fill_on   = [r["read_s"] for r in rows if r["fill_mode"] == 1]
read_fill_off  = [r["read_s"] for r in rows if r["fill_mode"] == 0]
file_bytes_on  = [r["file_bytes"] / (1024*1024) for r in rows if r["fill_mode"] == 1]  # MB
file_bytes_off = [r["file_bytes"] / (1024*1024) for r in rows if r["fill_mode"] == 0]  # MB

# ---------------------------------------------------------------------------
# Plot: 2 subplots (write time, read time)
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(1, 2, figsize=(10, 5))

bar_styles = {
    "fill_on":  {"color": "black",  "label": "NC_FILL"},
    "fill_off": {"color": "white",  "label": "NC_NOFILL", "edgecolor": "black", "linewidth": 1.5},
}

# --- Subplot 1: Write time ---
ax = axes[0]
bars1 = ax.bar(x_pos - width/2, write_fill_on, width, **bar_styles["fill_on"])
bars2 = ax.bar(x_pos + width/2, write_fill_off, width, **bar_styles["fill_off"])
ax.set_ylabel("Write time (s)", fontsize=10)
ax.set_title("Write Performance", fontsize=11)
ax.set_xticks(x_pos)
ax.set_xticklabels(formats)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 2: Read time ---
ax = axes[1]
bars1 = ax.bar(x_pos - width/2, read_fill_on, width, **bar_styles["fill_on"])
bars2 = ax.bar(x_pos + width/2, read_fill_off, width, **bar_styles["fill_off"])
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_title("Read Performance", fontsize=11)
ax.set_xticks(x_pos)
ax.set_xticklabels(formats)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# Overall title
fig.suptitle("NetCDF Fill Value Performance: NC_FILL vs NC_NOFILL",
             fontsize=12, fontweight="bold")

caption = "\n".join([
    "Dataset: 500\u00d7180\u00d7360 NC_FLOAT temperature (~129 MB uncompressed).",
    "classic = netCDF-3 64-bit offset format. nc4 = NetCDF-4/HDF5 format.",
    "",
    "NC_FILL:    Fill mode enabled (default). NetCDF writes fill values to",
    "            unwritten data locations automatically.",
    "NC_NOFILL:  Fill mode disabled. Faster writes, but unwritten data",
    "            contains undefined values.",
    "",
    "Lower time is better. File sizes are similar when all data is written.",
])
fig.text(0.5, -0.05, caption, ha="center", va="bottom", fontsize=8, color="#333333",
         multialignment="left", transform=fig.transFigure, fontfamily="monospace")

plt.tight_layout(rect=[0, 0.28, 1, 0.93])
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

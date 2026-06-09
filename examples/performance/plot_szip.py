"""Generate SZIP performance plots from szip.c CSV output (black and white).

Usage:
    ./szip > szip_results.csv
    python3 plot_szip.py

Reads:  szip_results.csv
Writes: szip_performance.jpg

Columns expected: pixels_per_block,compressed_bytes,ratio,write_s,read_s

All rows use NC_SZIP_NN coding (NC_SZIP_EC does not support NC_FLOAT).
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

INPUT  = "szip_results.csv"
OUTPUT = "szip_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "ppb":     int(row["pixels_per_block"]),
                "ratio":   float(row["ratio"]),
                "write_s": float(row["write_s"]),
                "read_s":  float(row["read_s"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './szip > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

rows.sort(key=lambda r: r["ppb"])
ppb_values = [r["ppb"]    for r in rows]
ratios     = [r["ratio"]  for r in rows]
write_s    = [r["write_s"] for r in rows]
read_s     = [r["read_s"]  for r in rows]

line_kw = {"linestyle": "-", "marker": "o", "color": "black",
           "label": "NC_SZIP_NN"}

# ---------------------------------------------------------------------------
# Plot: 3 subplots stacked vertically
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(3, 1, figsize=(9, 12), sharex=True)

# --- Subplot 1: Compression ratio ---
ax = axes[0]
ax.plot(ppb_values, ratios, **line_kw)
ax.set_ylabel("Compression ratio\n(uncompressed / compressed)", fontsize=10)
ax.set_title("NetCDF-4 SZIP Compression (NC_SZIP_NN): Ratio and Throughput vs. pixels_per_block",
             fontsize=11)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 2: Write time ---
ax = axes[1]
ax.plot(ppb_values, write_s, **line_kw)
ax.set_ylabel("Write time (s)", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# --- Subplot 3: Read time ---
ax = axes[2]
ax.plot(ppb_values, read_s, **line_kw)
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_xlabel("pixels_per_block", fontsize=10)
ax.legend(fontsize=9, frameon=True, edgecolor="black")
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)

# Share scale with write time but start from 0 so read times are visible
write_max = axes[1].get_ylim()[1]
axes[1].set_ylim(0, write_max)
axes[2].set_ylim(0, write_max)

# Common x-axis ticks
axes[2].set_xticks(ppb_values)

caption = "\n".join([
    "Dataset: 500\u00d7180\u00d7360 NC_FLOAT temperature (~129 MB uncompressed).",
    "Chunk shape: 10\u00d745\u00d790. Each point = one nc_put_var_float / nc_get_var_float call.",
    "",
    "NC_SZIP_NN: nearest-neighbor coding for floating-point scientific data.",
    "NC_SZIP_EC (entropy coding) is not used: it does not support NC_FLOAT.",
    "",
    "pixels_per_block: even divisors of CHUNK_X=90 {2, 6, 10, 18, 30}.",
    "Larger values give the encoder more context; higher ratio is better.",
])
fig.text(0.5, 0.01, caption, ha="center", va="bottom", fontsize=8, color="#333333",
         multialignment="left", transform=fig.transFigure, fontfamily="monospace")

plt.tight_layout(rect=[0, 0.14, 1, 1])
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

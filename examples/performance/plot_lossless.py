"""Generate lossless compression comparison plot from lossless.c CSV output (black and white).

Usage:
    ./lossless > lossless_results.csv
    python3 plot_lossless.py

Reads:  lossless_results.csv
Writes: lossless_performance.jpg

Columns expected: filter,level_or_pixels,compressed_bytes,ratio,write_s,read_s
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

INPUT  = "lossless_results.csv"
OUTPUT = "lossless_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "filter":  row["filter"],
                "level":   int(row["level_or_pixels"]),
                "ratio":   float(row["ratio"]),
                "write_s": float(row["write_s"]),
                "read_s":  float(row["read_s"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './lossless > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

# Sort by compression ratio (descending) for consistent display
rows.sort(key=lambda x: x["ratio"], reverse=True)

filters = [r["filter"] for r in rows]
ratios = [r["ratio"] for r in rows]
write_times = [r["write_s"] for r in rows]
read_times = [r["read_s"] for r in rows]

# Position filters on x-axis
x_pos = range(len(filters))

# ---------------------------------------------------------------------------
# Plot: 3 subplots stacked vertically
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(3, 1, figsize=(10, 12))

# Bar width
width = 0.6

# --- Subplot 1: Compression ratio ---
ax = axes[0]
bars1 = ax.bar(x_pos, ratios, width, color="white", edgecolor="black", linewidth=1.5)
ax.set_ylabel("Compression ratio\n(uncompressed / compressed)", fontsize=10)
ax.set_title("Compression Ratio", fontsize=11, fontweight="bold")
ax.set_xticks(x_pos)
ax.set_xticklabels(filters, rotation=45, ha="right", fontsize=9)
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)
# Add value labels on bars
for bar, ratio in zip(bars1, ratios):
    height = bar.get_height()
    ax.annotate(f"{ratio:.1f}x",
                xy=(bar.get_x() + bar.get_width() / 2, height),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center", va="bottom", fontsize=8)

# --- Subplot 2: Write time ---
ax = axes[1]
bars2 = ax.bar(x_pos, write_times, width, color="white", edgecolor="black", linewidth=1.5, hatch="///")
ax.set_ylabel("Write time (s)", fontsize=10)
ax.set_title("Write Performance", fontsize=11, fontweight="bold")
ax.set_xticks(x_pos)
ax.set_xticklabels(filters, rotation=45, ha="right", fontsize=9)
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)
# Add value labels on bars
for bar, wt in zip(bars2, write_times):
    height = bar.get_height()
    ax.annotate(f"{wt:.2f}s",
                xy=(bar.get_x() + bar.get_width() / 2, height),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center", va="bottom", fontsize=8)

# --- Subplot 3: Read time ---
ax = axes[2]
bars3 = ax.bar(x_pos, read_times, width, color="white", edgecolor="black", linewidth=1.5, hatch="\\\\\\")
ax.set_ylabel("Read time (s)", fontsize=10)
ax.set_title("Read Performance", fontsize=11, fontweight="bold")
ax.set_xticks(x_pos)
ax.set_xticklabels(filters, rotation=45, ha="right", fontsize=9)
ax.yaxis.grid(True, linestyle="--", color="gray", alpha=0.5)
ax.set_axisbelow(True)
# Add value labels on bars
for bar, rt in zip(bars3, read_times):
    height = bar.get_height()
    ax.annotate(f"{rt:.2f}s",
                xy=(bar.get_x() + bar.get_width() / 2, height),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center", va="bottom", fontsize=8)

caption = "\n".join([
    "Dataset: 500\u00d7180\u00d7360 NC_FLOAT temperature (~129 MB uncompressed).",
    "Chunk shape: 10\u00d745\u00d790. All tests use shuffle filter (determined optimal for floats).",
    "",
    "Filter settings: DEFLATE/gzip level 1, Zstandard level 1, SZIP NN p=2, LZ4 level 1, BZIP2 level 1.",
    "Higher compression ratio is better. Lower write/read times are better.",
    "BZIP2 achieves highest ratio (~44x) but with slower I/O. LZ4 offers best speed with good ratio (~27x).",
])
fig.text(0.5, 0.01, caption, ha="center", va="bottom", fontsize=9, color="#333333",
         multialignment="left", transform=fig.transFigure, fontfamily="monospace")

plt.tight_layout(rect=[0, 0.12, 1, 1])
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

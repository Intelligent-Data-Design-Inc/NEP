"""Generate a grouped bar chart of chunking.c CSV results (black and white)."""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

INPUT  = "chunking_results.csv"
OUTPUT = "chunking_performance.jpg"

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
with open(INPUT, newline="") as f:
    for row in csv.DictReader(f):
        rows.append(row)

# Build a dict: (chunk_shape, access_pattern) -> MB_per_s
data = {}
for r in rows:
    data[(r["chunk_shape"], r["access_pattern"])] = float(r["MB_per_s"])

shapes   = ["time_optimized", "column_optimized"]
patterns = ["time_slab", "column_profile"]

shape_labels   = {"time_optimized": "Time-optimized\n(1×36×72)",
                  "column_optimized": "Column-optimized\n(100×1×1)"}
pattern_labels = {"time_slab":       "Time slab\n(100 horiz. fields)",
                  "column_profile":  "Column profile\n(2,592 point series)"}

# 2×2 matrix: rows=chunk shapes, cols=access patterns
values = np.array([[data[(s, p)] for p in patterns] for s in shapes])

# ---------------------------------------------------------------------------
# Plot
# ---------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(10, 7))

x      = np.arange(len(patterns))
width  = 0.30
hatches = ["", "///"]   # solid vs hatched for the two chunk shapes

for i, (shape, hatch) in enumerate(zip(shapes, hatches)):
    bars = ax.bar(x + (i - 0.5) * width, values[i],
                  width=width,
                  label=shape_labels[shape],
                  color="white",
                  edgecolor="black",
                  hatch=hatch,
                  linewidth=1.2)
    # Annotate each bar with its value
    for bar, val in zip(bars, values[i]):
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + max(values.max() * 0.01, 1),
                f"{val:.1f}",
                ha="center", va="bottom", fontsize=9)

ax.set_yscale("log")
ax.set_ylabel("Read throughput (MB/s, log scale)", fontsize=11)
ax.set_xlabel("Access pattern", fontsize=11)
ax.set_title("NetCDF-4 Read Throughput: Chunk Shape vs. Access Pattern", fontsize=12)
ax.set_xticks(x)
ax.set_xticklabels([pattern_labels[p] for p in patterns], fontsize=10)
ax.legend(title="Chunk shape", fontsize=9, title_fontsize=9,
          loc="upper right", frameon=True, edgecolor="black")
ax.yaxis.grid(True, which="both", linestyle="--", color="black", alpha=0.3)
ax.set_axisbelow(True)

plt.tight_layout()
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

"""Compare BitGroom, GranularBitRound, and BitRound quantization algorithms
using zstandard + shuffle as the fixed lossless filter.

Usage:
    python3 plot_quantize_compare.py

Reads:  quantize_results.csv
Writes: quantize_compare_performance.jpg

Columns expected:
    quantize_alg,nsd_or_nsb,filter,compressed_bytes,ratio,write_s,read_s,max_abs_err
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

INPUT  = "quantize_results.csv"
OUTPUT = "quantize_compare_performance.jpg"
FILTER = "zstandard"

NSB_TO_NSD = {3: 1, 6: 2, 9: 3, 13: 4, 16: 5, 19: 6, 23: 7}

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
rows = []
try:
    with open(INPUT, newline="") as f:
        for row in csv.DictReader(f):
            rows.append({
                "alg":         row["quantize_alg"],
                "nsd_or_nsb":  int(row["nsd_or_nsb"]),
                "filter":      row["filter"],
                "ratio":       float(row["ratio"]),
                "write_s":     float(row["write_s"]),
                "read_s":      float(row["read_s"]),
                "max_abs_err": float(row["max_abs_err"]),
            })
except FileNotFoundError:
    print(f"Error: {INPUT} not found. Run './quantize > {INPUT}' first.",
          file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------------------
# Extract zstandard series per algorithm, x mapped to NSD
# ---------------------------------------------------------------------------
def get_series(alg, key):
    subset = [r for r in rows
              if r["alg"] == alg and r["filter"] == FILTER]
    subset.sort(key=lambda r: r["nsd_or_nsb"])
    if alg == "bitround":
        x = [NSB_TO_NSD[r["nsd_or_nsb"]] for r in subset
             if r["nsd_or_nsb"] in NSB_TO_NSD]
        subset = [r for r in subset if r["nsd_or_nsb"] in NSB_TO_NSD]
    else:
        x = [r["nsd_or_nsb"] for r in subset]
    return x, [r[key] for r in subset]

def get_baseline(key):
    for r in rows:
        if r["alg"] == "none" and r["filter"] == FILTER:
            return r[key]
    return None

ALGS = ["bitgroom", "granularbr", "bitround"]
ALG_LABELS = {
    "bitgroom":   "BitGroom",
    "granularbr": "GranularBitRound",
    "bitround":   "BitRound",
}

LINE_STYLES = {
    "bitgroom":   {"linestyle": "-",   "marker": "o", "color": "black",
                   "label": "BitGroom"},
    "granularbr": {"linestyle": "--",  "marker": "s", "color": "black",
                   "markerfacecolor": "white", "label": "GranularBitRound"},
    "bitround":   {"linestyle": "-.",  "marker": "^", "color": "black",
                   "label": "BitRound"},
}

# ---------------------------------------------------------------------------
# Plot: 3 stacked subplots
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(3, 1, figsize=(7, 6.1), sharex=True)
fig.suptitle(
    "Quantization Algorithm Comparison\n(zstandard + shuffle, level 1)",
    fontsize=11)

METRICS = [
    ("ratio",       "Compression ratio\n(uncompressed / compressed)"),
    ("write_s",     "Write time (s)"),
    ("max_abs_err", "Max absolute error (K)"),
]

for row_idx, (metric, ylabel) in enumerate(METRICS):
    ax = axes[row_idx]

    for alg in ALGS:
        x, y = get_series(alg, metric)
        if not x:
            continue
        ax.plot(x, y, **LINE_STYLES[alg])

    # Lossless-only reference line on ratio chart
    if metric == "ratio":
        base = get_baseline("ratio")
        if base is not None:
            ax.axhline(base, linestyle=":", color="black", alpha=0.4,
                       linewidth=1.0, label="lossless only")

    ax.set_ylabel(ylabel, fontsize=9)
    ax.yaxis.grid(True, linestyle="--", color="black", alpha=0.3)
    ax.set_axisbelow(True)

    if metric == "max_abs_err":
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.4f"))

    if row_idx == 0:
        ax.legend(fontsize=8, frameon=True, edgecolor="black",
                  loc="upper right")

axes[2].set_xlabel("NSD  (decimal significant digits)\n"
                   "BitRound NSB remapped: 3=1, 6=2, 9=3, 13=4, 16=5, 19=6",
                   fontsize=8)
axes[2].set_xticks(list(range(1, 7)))

plt.tight_layout()
plt.savefig(OUTPUT, dpi=150, format="jpeg")
print(f"Saved {OUTPUT}")

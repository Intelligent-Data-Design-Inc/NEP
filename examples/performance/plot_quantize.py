"""Generate quantization + compression performance plots from quantize.c CSV output.

Usage:
    ./quantize > quantize_results.csv
    python3 plot_quantize.py

Reads:  quantize_results.csv
Writes: quantize_performance.jpg

Columns expected:
    quantize_alg,nsd_or_nsb,filter,compressed_bytes,ratio,write_s,read_s,max_abs_err

Rows:
    quantize_alg == "none"       -> baseline (no quantization)
    quantize_alg == "bitgroom"   -> BitGroom NSD 1-7
    quantize_alg == "granularbr" -> GranularBitRound NSD 1-7
    quantize_alg == "bitround"   -> BitRound NSB 3,6,9,13,16,19,23

The NSB values for BitRound correspond to the same worst-case decimal
precision as BitGroom/GranularBitRound NSD 1-7:
    NSD: 1  2  3   4   5   6   7
    NSB: 3  6  9  13  16  19  23
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

INPUT  = "quantize_results.csv"
OUTPUT = "quantize_performance.jpg"

# NSB -> equivalent NSD for x-axis labelling on BitRound plots
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

FILTERS = ["deflate", "zstandard", "szip", "lz4", "bzip2"]
ALGS    = ["bitgroom", "granularbr", "bitround"]
NSD_VALS = list(range(1, 7))   # 1..6 (NSD=7 excluded: NC_FLOAT boundary bug in BitGroom)

# Line style per filter (black-and-white friendly)
FILTER_STYLES = {
    "deflate":   {"linestyle": "-",   "marker": "o", "color": "black"},
    "zstandard": {"linestyle": "--",  "marker": "s", "color": "black",
                  "markerfacecolor": "white"},
    "szip":      {"linestyle": "-.",  "marker": "^", "color": "black"},
    "lz4":       {"linestyle": ":",   "marker": "D", "color": "black",
                  "markerfacecolor": "white"},
    "bzip2":     {"linestyle": (0,(5,2,1,2)), "marker": "v", "color": "black"},
}

ALG_TITLES = {
    "bitgroom":   "BitGroom",
    "granularbr": "GranularBitRound",
    "bitround":   "BitRound",
}

# ---------------------------------------------------------------------------
# Helper: extract series for one alg × filter
# ---------------------------------------------------------------------------
def get_series(alg, flt, key):
    """Return (x_vals, y_vals) sorted by NSD for a given alg and filter.

    For BitRound, nsd_or_nsb holds NSB values; these are remapped to their
    NSD equivalents so all three algorithm columns share the same x-axis scale.
    """
    subset = [r for r in rows if r["alg"] == alg and r["filter"] == flt]
    subset.sort(key=lambda r: r["nsd_or_nsb"])
    if alg == "bitround":
        x = [NSB_TO_NSD[r["nsd_or_nsb"]] for r in subset]
    else:
        x = [r["nsd_or_nsb"] for r in subset]
    return x, [r[key] for r in subset]

def get_baseline(flt, key):
    """Return the scalar baseline value for a filter (alg == 'none')."""
    for r in rows:
        if r["alg"] == "none" and r["filter"] == flt:
            return r[key]
    return None

METRICS = [
    ("ratio",       "Compression ratio\n(uncompressed / compressed)"),
    ("write_s",     "Write time (s)"),
    ("max_abs_err", "Max absolute error (K)"),
]

# ---------------------------------------------------------------------------
# One figure per algorithm, 3 stacked subplots (ratio, write time, max error)
# ---------------------------------------------------------------------------
output_files = []
for alg in ALGS:
    fig, axes = plt.subplots(3, 1, figsize=(7, 6.1), sharex=True)
    fig.suptitle(
        f"NetCDF-4 Quantization + Compression: {ALG_TITLES[alg]}",
        fontsize=12)

    for row_idx, (metric, ylabel) in enumerate(METRICS):
        ax = axes[row_idx]

        for flt in FILTERS:
            x, y = get_series(alg, flt, metric)
            if not x:
                continue
            style = dict(FILTER_STYLES[flt])
            style["label"] = flt
            ax.plot(x, y, **style)

        # Horizontal reference lines: baseline compression ratio
        if metric == "ratio":
            for flt in FILTERS:
                base = get_baseline(flt, "ratio")
                if base is not None:
                    ax.axhline(base, linestyle=":", color="black", alpha=0.3,
                               linewidth=0.8)

        ax.set_ylabel(ylabel, fontsize=9)
        ax.yaxis.grid(True, linestyle="--", color="black", alpha=0.3)
        ax.set_axisbelow(True)

        if metric == "max_abs_err":
            ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.4f"))

    # x-axis label and ticks on bottom subplot
    if alg == "bitround":
        axes[2].set_xlabel("NSD equivalent  (NSB remapped to NSD 1–7)", fontsize=9)
    else:
        axes[2].set_xlabel("NSD  (decimal significant digits)", fontsize=9)
    axes[2].set_xticks(NSD_VALS)

    # Legend inside top subplot
    handles, labels = axes[0].get_legend_handles_labels()
    axes[0].legend(handles, labels, ncol=3, fontsize=8, frameon=True,
                   edgecolor="black", loc="upper right")

    plt.tight_layout()
    outfile = f"quantize_{alg}_performance.jpg"
    plt.savefig(outfile, dpi=150, format="jpeg")
    plt.close(fig)
    output_files.append(outfile)
    print(f"Saved {outfile}")

print("Done:", output_files)

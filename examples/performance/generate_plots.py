#!/usr/bin/env python3
"""
Generate plot images for cache_tuning.c example output.

This script creates JPG images showing the relationship between
chunk cache size and performance, based on the CSV output format
produced by the cache_tuning.c example program.

Usage:
    python3 generate_plots.py

Output:
    cache_performance_time.jpg - Time vs Cache Size plot
    cache_performance_speedup.jpg - Speedup vs Cache Size plot
    cache_thrashing.jpg - Thrashing detection comparison plot
"""

import matplotlib.pyplot as plt
import numpy as np

# Representative data based on expected cache_tuning.c output
# Cache sizes in MB
cache_sizes = [1.0, 4.0, 16.0, 64.0, 128.0]

# Simulated timing data (seconds) - inverse relationship with cache size
# These are representative values showing the expected trend
times = [12.345, 10.234, 8.123, 4.567, 3.456]

# Calculate speedup relative to baseline (1.0 MB cache)
baseline_time = times[0]
speedups = [baseline_time / t for t in times]

# Thrashing detection data (10-chunk working set)
thrash_cache = [4.0, 13.2]  # 4MB (too small) vs 13.2MB (fits working set)
thrash_times = [5.678, 0.234]
thrash_speedups = [thrash_times[0] / thrash_times[0], thrash_times[0] / thrash_times[1]]

def create_time_plot():
    """Create plot showing time vs cache size."""
    fig, ax = plt.subplots(figsize=(10, 7))

    ax.plot(cache_sizes, times, 'b-o', linewidth=2, markersize=8, label='Total Read Time')
    ax.set_xlabel('Chunk Cache Size (MB)', fontsize=12)
    ax.set_ylabel('Total Read Time (seconds)', fontsize=12)
    ax.set_title('NetCDF-4 Chunk Cache Performance\nTotal Read Time vs Cache Size',
                fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(loc='upper right')

    # Add annotations for each data point
    for i, (size, time) in enumerate(zip(cache_sizes, times)):
        ax.annotate(f'{time:.2f}s',
                   xy=(size, time),
                   xytext=(10, 10),
                   textcoords='offset points',
                   fontsize=9,
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.7))

    plt.tight_layout()
    plt.savefig('/home/ed/NEP/examples/performance/cache_performance_time.jpg',
               format='jpg', dpi=150)
    plt.close()
    print("Created: cache_performance_time.jpg")

def create_speedup_plot():
    """Create plot showing speedup vs cache size."""
    fig, ax = plt.subplots(figsize=(10, 7))

    ax.plot(cache_sizes, speedups, 'g-s', linewidth=2, markersize=8, label='Speedup vs Default')
    ax.set_xlabel('Chunk Cache Size (MB)', fontsize=12)
    ax.set_ylabel('Read Performance Speedup (× times faster)', fontsize=12)
    ax.set_title('NetCDF-4 Chunk Cache Performance\nRead Speedup vs Cache Size',
                fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(loc='lower right')

    # Add horizontal reference line at speedup=1.0
    ax.axhline(y=1.0, color='r', linestyle='--', alpha=0.5, label='Baseline (1x)')

    # Add annotations
    for i, (size, speedup) in enumerate(zip(cache_sizes, speedups)):
        ax.annotate(f'{speedup:.1f}x',
                   xy=(size, speedup),
                   xytext=(10, 10),
                   textcoords='offset points',
                   fontsize=9,
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='lightgreen', alpha=0.7))

    # Add explanatory text
    ax.text(0.05, 0.95, 'Baseline: Default 1MB cache\nSpeedup = Time_baseline / Time_cache_size',
           transform=ax.transAxes, fontsize=9, verticalalignment='top',
           bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    plt.tight_layout()
    plt.savefig('/home/ed/NEP/examples/performance/cache_performance_speedup.jpg',
               format='jpg', dpi=150)
    plt.close()
    print("Created: cache_performance_speedup.jpg")

def create_thrashing_plot():
    """Create plot showing thrashing detection comparison."""
    fig, ax = plt.subplots(figsize=(10, 7))

    x_labels = ['4 MB Cache\n(Undersized)', '13.2 MB Cache\n(Fits Working Set)']
    colors = ['red', 'green']

    bars = ax.bar(x_labels, thrash_times, color=colors, alpha=0.7, edgecolor='black', linewidth=1.5)

    ax.set_ylabel('Total Read Time (seconds)', fontsize=12)
    ax.set_title('NetCDF-4 Cache Thrashing Detection\nRead Time: Small vs. Properly-Sized Cache',
                fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3, axis='y')

    # Add value labels on bars
    for bar, time, speedup in zip(bars, thrash_times, thrash_speedups):
        height = bar.get_height()
        ax.annotate(f'{time:.3f}s\n({speedup:.1f}x)',
                   xy=(bar.get_x() + bar.get_width() / 2, height),
                   xytext=(0, 3),
                   textcoords="offset points",
                   ha='center', va='bottom',
                   fontsize=11, fontweight='bold',
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.8))

    # Add explanatory text
    explanation = ('Cache thrashing: Working set (10 chunks)\n'
                  'exceeds cache capacity → constant re-reading')
    ax.text(0.95, 0.95, explanation,
           transform=ax.transAxes, fontsize=9, verticalalignment='top', horizontalalignment='right',
           bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # Add speedup annotation
    speedup_factor = thrash_times[0] / thrash_times[1]
    ax.text(0.5, 0.5, f'{speedup_factor:.1f}x Speedup!',
           transform=ax.transAxes, fontsize=20, fontweight='bold',
           ha='center', va='center',
           bbox=dict(boxstyle='round', facecolor='gold', alpha=0.8),
           color='darkgreen')

    plt.tight_layout()
    plt.savefig('/home/ed/NEP/examples/performance/cache_thrashing.jpg',
               format='jpg', dpi=150)
    plt.close()
    print("Created: cache_thrashing.jpg")

def create_combined_plot():
    """Create a combined plot showing both time and speedup."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 7))

    # Left plot: Time vs Cache Size
    ax1.plot(cache_sizes, times, 'b-o', linewidth=2, markersize=8)
    ax1.set_xlabel('Chunk Cache Size (MB)', fontsize=11)
    ax1.set_ylabel('Total Read Time (seconds)', fontsize=11)
    ax1.set_title('Read Time vs Cache Size', fontsize=12, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log', base=2)

    # Right plot: Speedup vs Cache Size
    ax2.plot(cache_sizes, speedups, 'g-s', linewidth=2, markersize=8)
    ax2.set_xlabel('Chunk Cache Size (MB)', fontsize=11)
    ax2.set_ylabel('Read Speedup (× times)', fontsize=11)
    ax2.set_title('Read Speedup vs Cache Size', fontsize=12, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log', base=2)
    ax2.axhline(y=1.0, color='r', linestyle='--', alpha=0.5)

    fig.suptitle('NetCDF-4 Chunk Cache Performance Analysis\n'
                'Effect of Cache Size on Read Performance',
                fontsize=14, fontweight='bold')

    plt.tight_layout()
    plt.savefig('/home/ed/NEP/examples/performance/cache_performance_combined.jpg',
               format='jpg', dpi=150)
    plt.close()
    print("Created: cache_performance_combined.jpg")

if __name__ == '__main__':
    print("Generating cache tuning performance plots...")
    print()

    create_time_plot()
    create_thrashing_plot()

    print()
    print("Plots generated successfully!")
    print("Files created in: /home/ed/NEP/examples/performance/")
    print("  - cache_performance_time.jpg")
    print("  - cache_thrashing.jpg")

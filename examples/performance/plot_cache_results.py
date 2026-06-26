#!/usr/bin/env python3
"""
Plot cache tuning results from cache_tuning.c output.

This script can either:
1. Read actual CSV output from cache_tuning
2. Generate simulated data matching expected output

Usage:
    python3 plot_cache_results.py              # Use simulated data
    python3 plot_cache_results.py results.csv  # Use actual output file
"""

import matplotlib.pyplot as plt
import sys
import os

def parse_csv_data(filename=None):
    """Parse CSV data from file or generate simulated data."""
    
    if filename and os.path.exists(filename):
        # Read actual data from file
        with open(filename, 'r') as f:
            lines = f.readlines()
    else:
        # Generate simulated data matching cache_tuning.c output
        # Based on typical performance characteristics
        lines = [
            "# CSV data for plotting cache size vs performance",
            "CacheSizeMB,TimeSeconds,Speedup",
            "# Test 1: Default cache - 5 full passes through dataset",
            "64.0,8.234,1.00",
            "# Test 2: Cache size scaling - testing 256MB, 512MB, 1GB, 2GB",
            "256.0,6.543,1.26",
            "512.0,4.321,1.91",
            "1024.0,2.876,2.86",
            "2048.0,2.123,3.88",
            "# Test 3: Cache thrashing detection (50-chunk working set, 100 iterations)",
            "16.0,8.901,1.00",
            "52.8,0.345,25.80",
        ]
    
    # Parse the data
    cache_sizes = []
    times = []
    speedups = []
    test_sections = []
    current_section = None
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            # Track which test section we're in
            if 'Test 1' in line:
                current_section = 'baseline'
            elif 'Test 2' in line:
                current_section = 'scaling'
            elif 'Test 3' in line:
                current_section = 'thrashing'
            continue
        
        parts = line.split(',')
        if len(parts) >= 3:
            try:
                cache_sizes.append(float(parts[0]))
                times.append(float(parts[1]))
                speedups.append(float(parts[2]))
                test_sections.append(current_section)
            except ValueError:
                pass
    
    return cache_sizes, times, speedups, test_sections

def create_plot(cache_sizes, times, speedups, test_sections):
    """Create a two-panel plot showing cache performance."""
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10))
    
    # Separate data by test section
    scaling_indices = [i for i, s in enumerate(test_sections) if s == 'scaling']
    baseline_indices = [i for i, s in enumerate(test_sections) if s == 'baseline']
    thrashing_indices = [i for i, s in enumerate(test_sections) if s == 'thrashing']
    
    # Plot 1: Cache Size vs Time
    if scaling_indices:
        sizes = [cache_sizes[i] for i in scaling_indices]
        t = [times[i] for i in scaling_indices]
        ax1.plot(sizes, t, 'k-o', linewidth=2, markersize=8, label='Test 2: Scaling')
    
    if baseline_indices:
        sizes = [cache_sizes[i] for i in baseline_indices]
        t = [times[i] for i in baseline_indices]
        ax1.plot(sizes, t, 'k-s', linewidth=2, markersize=8, markerfacecolor='white', label='Test 1: Baseline (64MB)')
    
    ax1.set_xlabel('Cache Size (MB)', fontsize=11)
    ax1.set_ylabel('Time (seconds)', fontsize=11)
    ax1.set_title('Cache Size vs Read Time (5 passes through 800 chunks)', fontsize=12)
    ax1.legend(loc='upper right')
    ax1.grid(True, alpha=0.3)
    ax1.set_xticks([64, 256, 512, 1024, 2048])
    ax1.set_xticklabels(['64', '256', '512', '1024', '2048'])
    
    # Plot 2: Speedup
    if scaling_indices:
        sizes = [cache_sizes[i] for i in scaling_indices]
        sp = [speedups[i] for i in scaling_indices]
        ax2.plot(sizes, sp, 'k-o', linewidth=2, markersize=8, label='Test 2: Speedup')
    
    if baseline_indices:
        sizes = [cache_sizes[i] for i in baseline_indices]
        sp = [speedups[i] for i in baseline_indices]
        ax2.plot(sizes, sp, 'k-s', linewidth=2, markersize=8, markerfacecolor='white', label='Test 1: Baseline')
    
    ax2.set_xlabel('Cache Size (MB)', fontsize=11)
    ax2.set_ylabel('Speedup (relative to default)', fontsize=11)
    ax2.set_title('Performance Improvement vs Cache Size', fontsize=12)
    ax2.legend(loc='upper left')
    ax2.grid(True, alpha=0.3)
    ax2.set_xticks([64, 256, 512, 1024, 2048])
    ax2.set_xticklabels(['64', '256', '512', '1024', '2048'])
    ax2.axhline(y=1.0, color='black', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    
    return fig

def main():
    input_file = sys.argv[1] if len(sys.argv) > 1 else None
    
    cache_sizes, times, speedups, test_sections = parse_csv_data(input_file)
    
    if not cache_sizes:
        print("No data found")
        return 1
    
    fig = create_plot(cache_sizes, times, speedups, test_sections)
    
    # Save the plot
    output_file = 'cache_tuning_plot.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Plot saved to: {os.path.abspath(output_file)}")
    
    # Also show the data
    print("\nData summary:")
    print("-" * 50)
    print(f"{'Cache (MB)':<12} {'Time (s)':<12} {'Speedup':<10}")
    print("-" * 50)
    for i in range(len(cache_sizes)):
        section = test_sections[i] if i < len(test_sections) else 'unknown'
        print(f"{cache_sizes[i]:<12.1f} {times[i]:<12.3f} {speedups[i]:<10.2f} ({section})")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())

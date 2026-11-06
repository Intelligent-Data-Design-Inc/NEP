#!/usr/bin/env bash
# record_machine.sh - record the machine and library context of a
# bfloat16_perf benchmark run, in the format of nep-compress
# performance/reference_machine.txt.
#
# Usage: HDF5_PLUGIN_PATH=<zstd plugin dir> record_machine.sh BUILD_DIR OUTPUT
#
# Author: Edward Hartnett, Intelligent Data Design, Inc.

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "usage: record_machine.sh BUILD_DIR OUTPUT" >&2
    exit 1
fi

BUILD_DIR="$(cd "$1" && pwd)"
OUTPUT="$2"
SRC_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PERF="$BUILD_DIR/examples/nextcdf4/bfloat16_perf"

if [ ! -x "$PERF" ]; then
    echo "error: $PERF not found" >&2
    exit 1
fi
if [ -z "${HDF5_PLUGIN_PATH:-}" ]; then
    echo "error: HDF5_PLUGIN_PATH is not set" >&2
    exit 1
fi

netcdf_lib=$(ldd "$PERF" | awk '/libnetcdf\.so/ {print $3; exit}')
nc_config=$(command -v nc-config || echo "${netcdf_lib%/lib/*}/bin/nc-config")
hdf5_lib=$(ldd "$BUILD_DIR/src/libncnextcdf4.so" | awk '/libhdf5\.so/ {print $3; exit}')
cpu=$(awk -F: '/model name/ {sub(/^ /, "", $2); print $2; exit}' /proc/cpuinfo)
py_ver=$(python3 -c 'import sys; print("%d.%d.%d" % sys.version_info[:3])')
mpl_ver=$(python3 -c 'import matplotlib; print(matplotlib.__version__)')
sample_sha=$(sha256sum "$SRC_DIR/test/data/netCDF-4/gdas_sfc_sample.nc" | awk '{print $1}')
git_rev=$(git -C "$SRC_DIR" rev-parse --short HEAD)
git -C "$SRC_DIR" diff --quiet HEAD || git_rev="$git_rev-dirty"

{
    echo "Run date: $(date -u '+%Y-%m-%d %H:%M UTC')"
    echo "NEP: $(cat "$SRC_DIR/version.txt") ($git_rev)"
    echo "CPU: $cpu"
    echo "Cores: $(nproc)"
    echo "OS: $(uname -sr)"
    echo "Compiler: $(cc --version | head -1)"
    echo "NetCDF-C: $("$nc_config" --version) ($netcdf_lib)"
    echo "HDF5: $hdf5_lib"
    for so in "$HDF5_PLUGIN_PATH"/*.so; do
        sha=$(sha256sum "$so" | awk '{print $1}')
        target=$(readlink -f "$so")
        link=""
        [ "$so" != "$target" ] && link=" -> $target"
        hdf5_link=$(ldd "$target" | awk '/libhdf5\.so/ {print "libhdf5.so => " $3; exit}')
        echo "zstd provider: $so$link sha256 $sha links $hdf5_link"
    done
    echo "NREPS: 3"
    echo "Sample: test/data/netCDF-4/gdas_sfc_sample.nc sha256 $sample_sha"
    echo "Plotting: Python $py_ver, matplotlib $mpl_ver"
} > "$OUTPUT"

echo "Wrote $OUTPUT"

#!/usr/bin/env bash
# record_machine.sh - record the machine and library context of a
# bfloat16_perf and bfloat16_io_perf benchmark run, in the format of
# nep-compress performance/reference_machine.txt.
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
IO_PERF="$BUILD_DIR/examples/nextcdf4/bfloat16_io_perf"

if [ ! -x "$PERF" ]; then
    echo "error: $PERF not found" >&2
    exit 1
fi
if [ ! -x "$IO_PERF" ]; then
    echo "error: $IO_PERF not found" >&2
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
bf16_sample_sha=$(sha256sum "$SRC_DIR/test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc" | awk '{print $1}')
cache="$BUILD_DIR/CMakeCache.txt"
build_type=$(awk -F= '/^CMAKE_BUILD_TYPE:/ {print $2; exit}' "$cache")
cflags=$(awk -F= '/^CMAKE_C_FLAGS:/ {print $2; exit}' "$cache")
type_flags=""
if [ -n "$build_type" ]; then
    type_upper=$(printf '%s' "$build_type" | tr '[:lower:]' '[:upper:]')
    type_flags=$(awk -F= -v name="CMAKE_C_FLAGS_$type_upper" '$0 ~ "^" name ":" {print $2; exit}' "$cache")
fi
cflags_line=$(echo "$cflags $type_flags" | tr -s '[:space:]' ' ' | sed 's/^ //; s/ $//')
fstype=$(df --output=fstype "$BUILD_DIR" | tail -1 | tr -d ' ')
fsdev=$(df --output=source "$BUILD_DIR" | tail -1 | tr -d ' ')
disk="$fsdev"
parent=$(lsblk -no PKNAME "$fsdev" 2>/dev/null | head -1 | tr -d ' ' || true)
[ -n "$parent" ] && disk="/dev/$parent"
disk_model=$(lsblk -dno MODEL "$disk" 2>/dev/null | sed 's/ *$//' || true)
disk_rota=$(lsblk -dno ROTA "$disk" 2>/dev/null | tr -d ' ' || true)
[ "$disk_rota" = "1" ] && disk_kind=HDD || disk_kind=SSD
git_rev=$(git -C "$SRC_DIR" rev-parse --short HEAD)
git -C "$SRC_DIR" diff --quiet HEAD || git_rev="$git_rev-dirty"

{
    echo "Run date: $(date -u '+%Y-%m-%d %H:%M UTC')"
    echo "NEP: $(cat "$SRC_DIR/version.txt") ($git_rev)"
    echo "CPU: $cpu"
    echo "Cores: $(nproc)"
    echo "OS: $(uname -sr)"
    echo "Storage: $fstype on $fsdev ($disk_model, $disk_kind)"
    echo "Timing: write includes fsync; reads after posix_fadvise(DONTNEED)"
    echo "Compiler: $(cc --version | head -1)"
    echo "Build type: ${build_type:-(none)}"
    echo "C flags: $cflags_line"
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
    echo "NREPS: $(awk '/^#define NREPS/ {print $3; exit}' "$SRC_DIR/examples/nextcdf4/bfloat16_perf.c")"
    echo "NCOPIES: $(awk '/^#define NCOPIES/ {print $3; exit}' "$SRC_DIR/examples/nextcdf4/bfloat16_perf.c")"
    echo "Sample: test/data/netCDF-4/gdas_sfc_sample.nc sha256 $sample_sha"
    echo "Sample: test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc sha256 $bf16_sample_sha"
    echo "Plotting: Python $py_ver, matplotlib $mpl_ver"
} > "$OUTPUT"

echo "Wrote $OUTPUT"

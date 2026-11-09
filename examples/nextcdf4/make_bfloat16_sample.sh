#!/usr/bin/env bash
# make_bfloat16_sample.sh - build a NEXTCDF-4 NC_BFLOAT16 copy of the
# GDAS sample: run make_bfloat16_sample for the conversion, then
# h5repack to apply the 1x768x1536 chunks, shuffle and zstd level 19
# layout of the source file (NEXTCDF-4 cannot define those filters
# itself).
#
# Usage: HDF5_PLUGIN_PATH=<zstd plugin dir> \
#        [H5REPACK=<h5repack path>] \
#        make_bfloat16_sample.sh BUILD_DIR INPUT OUTPUT
#
# Author: Edward Hartnett, Intelligent Data Design, Inc.

set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "usage: make_bfloat16_sample.sh BUILD_DIR INPUT OUTPUT" >&2
    exit 1
fi

BUILD_DIR="$1"
INPUT="$2"
OUTPUT="$3"
PROG="$BUILD_DIR/examples/nextcdf4/make_bfloat16_sample"
H5REPACK="${H5REPACK:-h5repack}"

if [ ! -x "$PROG" ]; then
    echo "error: $PROG not found" >&2
    exit 1
fi
if [ -z "${HDF5_PLUGIN_PATH:-}" ]; then
    echo "error: HDF5_PLUGIN_PATH is not set" >&2
    exit 1
fi
if [ ! -x "$H5REPACK" ]; then
    echo "error: $H5REPACK is not executable" >&2
    exit 1
fi
version=$("$H5REPACK" -V 2>&1 | grep -o '[0-9][0-9.]*' | head -1 || true)
if [ -z "$version" ] || \
   [ "$(printf '2.1.1\n%s\n' "$version" | sort -V | head -1)" != "2.1.1" ]; then
    echo "error: $H5REPACK version ${version:-unknown} is older than 2.1.1 (bfloat16 support)" >&2
    exit 1
fi

tmp=$(mktemp --suffix=.nc)
trap 'rm -f "$tmp"' EXIT

NCRCENV_RC="$BUILD_DIR/.ncrc" NETCDF_RC="$BUILD_DIR" \
    "$PROG" "$INPUT" "$tmp"

# Shuffle is listed before the zstd user-defined filter so it runs
# first in the pipeline.
"$H5REPACK" \
    -l land,prate_ave,soilw1,tmp2m:CHUNK=1x768x1536 \
    -f land,prate_ave,soilw1,tmp2m:SHUF \
    -f land,prate_ave,soilw1,tmp2m:UD=32015,0,1,19 \
    "$tmp" "$OUTPUT"

bytes=$(stat -c %s "$OUTPUT")
echo "Wrote $OUTPUT ($bytes bytes)"

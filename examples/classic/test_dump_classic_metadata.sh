#!/bin/bash
# Test dump_classic_metadata by running coord_vars first, then comparing output.
# Edward Hartnett 2/14/26
set -e

# coord_vars must run first to produce coord_vars.nc
./coord_vars

# Run the metadata dumper and capture output
./dump_classic_metadata coord_vars.nc > dump_classic_metadata_output.txt

# Compare with expected output
diff -u ../expected_output/dump_classic_metadata_coord_vars_expected.txt dump_classic_metadata_output.txt
echo "PASS: dump_classic_metadata output matches expected"
rm -f dump_classic_metadata_output.txt

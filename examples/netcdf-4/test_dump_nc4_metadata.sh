#!/bin/bash
# Test dump_nc4_metadata by running user_types first, then comparing output.
# Edward Hartnett 2/14/26
set -e

# user_types must run first to produce user_types.nc
ASAN_OPTIONS=detect_leaks=0 ./user_types

# Run the metadata dumper and capture output
./dump_nc4_metadata user_types.nc > dump_nc4_metadata_output.txt

# Compare with expected output
diff -u ../expected_output/dump_nc4_metadata_user_types_expected.txt dump_nc4_metadata_output.txt
echo "PASS: dump_nc4_metadata output matches expected"
rm -f dump_nc4_metadata_output.txt

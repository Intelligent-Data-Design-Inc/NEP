#!/bin/bash
# Test f_dump_classic_metadata by running f_coord_vars first, then comparing output.
# Edward Hartnett 2/14/26
set -e

# f_coord_vars must run first to produce f_coord_vars.nc
./f_coord_vars

# Run the metadata dumper and capture output
./f_dump_classic_metadata f_coord_vars.nc > f_dump_classic_metadata_output.txt

# Compare with expected output
diff -u ../expected_output/f_dump_classic_metadata_coord_vars_expected.txt f_dump_classic_metadata_output.txt
echo "PASS: f_dump_classic_metadata output matches expected"
rm -f f_dump_classic_metadata_output.txt

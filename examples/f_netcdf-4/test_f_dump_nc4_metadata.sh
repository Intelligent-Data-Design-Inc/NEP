#!/bin/bash
# Test f_dump_nc4_metadata by running f_user_types first, then comparing output.
# Edward Hartnett 2/14/26
set -e

# f_user_types must run first to produce f_user_types.nc
./f_user_types

# Run the metadata dumper and capture output
./f_dump_nc4_metadata f_user_types.nc > f_dump_nc4_metadata_output.txt

# Compare with expected output
diff -u ../expected_output/f_dump_nc4_metadata_user_types_expected.txt f_dump_nc4_metadata_output.txt
echo "PASS: f_dump_nc4_metadata output matches expected"
rm -f f_dump_nc4_metadata_output.txt

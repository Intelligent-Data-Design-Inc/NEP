#!/bin/bash
set -e
./coord
bash ../validate_cdl.sh coord coord.nc ../expected_output/coord_expected.cdl

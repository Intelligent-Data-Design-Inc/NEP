#!/bin/bash
set -e
./f_simple_nc4
bash ../validate_cdl.sh f_simple_nc4 f_simple_nc4.nc ../expected_output/f_simple_nc4_expected.cdl

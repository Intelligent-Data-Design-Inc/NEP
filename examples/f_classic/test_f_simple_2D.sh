#!/bin/bash
set -e
./f_simple_2D
bash ../validate_cdl.sh f_simple_2D f_simple_2D.nc ../expected_output/f_simple_2D_expected.cdl

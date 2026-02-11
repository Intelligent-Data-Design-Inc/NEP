#!/bin/bash
set -e
./f_coord
bash ../validate_cdl.sh f_coord f_coord.nc ../expected_output/f_coord_expected.cdl

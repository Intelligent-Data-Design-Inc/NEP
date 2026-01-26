#!/bin/bash
set -e
./f_coord_vars
bash ../validate_cdl.sh f_coord_vars f_coord_vars.nc ../expected_output/f_coord_vars_expected.cdl

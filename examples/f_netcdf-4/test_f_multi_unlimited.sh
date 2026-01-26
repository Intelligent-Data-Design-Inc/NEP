#!/bin/bash
set -e
./f_multi_unlimited
bash ../validate_cdl.sh f_multi_unlimited f_multi_unlimited.nc ../expected_output/f_multi_unlimited_expected.cdl

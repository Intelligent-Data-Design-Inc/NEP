#!/bin/bash
set -e
./f_unlimited_dim
bash ../validate_cdl.sh f_unlimited_dim f_unlimited_dim.nc ../expected_output/f_unlimited_dim_expected.cdl

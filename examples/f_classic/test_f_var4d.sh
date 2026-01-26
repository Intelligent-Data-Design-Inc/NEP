#!/bin/bash
set -e
./f_var4d
bash ../validate_cdl.sh f_var4d f_var4d.nc ../expected_output/f_var4d_expected.cdl

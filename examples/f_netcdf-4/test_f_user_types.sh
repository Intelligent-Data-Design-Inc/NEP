#!/bin/bash
set -e
./f_user_types
bash ../validate_cdl.sh f_user_types f_user_types.nc ../expected_output/f_user_types_expected.cdl

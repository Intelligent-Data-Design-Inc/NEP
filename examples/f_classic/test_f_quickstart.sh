#!/bin/bash
set -e
./f_quickstart
bash ../validate_cdl.sh f_quickstart f_quickstart.nc ../expected_output/f_quickstart_expected.cdl

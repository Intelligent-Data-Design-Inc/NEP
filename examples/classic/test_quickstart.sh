#!/bin/bash
set -e
./quickstart
bash ../validate_cdl.sh quickstart quickstart.nc ../expected_output/quickstart_expected.cdl

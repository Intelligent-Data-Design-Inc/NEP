#!/bin/bash
set -e
./f_size_limits
bash ../validate_cdl.sh f_size_limits f_size_limits_classic.nc ../expected_output/f_size_limits_classic_expected.cdl
bash ../validate_cdl.sh f_size_limits f_size_limits_64bit_offset.nc ../expected_output/f_size_limits_64bit_offset_expected.cdl
bash ../validate_cdl.sh f_size_limits f_size_limits_64bit_data.nc ../expected_output/f_size_limits_64bit_data_expected.cdl

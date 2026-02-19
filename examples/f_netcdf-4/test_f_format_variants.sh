#!/bin/bash
set -e
./f_format_variants
bash ../validate_cdl.sh f_format_variants f_format_classic.nc ../expected_output/f_format_variants_classic_expected.cdl
bash ../validate_cdl.sh f_format_variants f_format_64bit_offset.nc ../expected_output/f_format_variants_64bit_offset_expected.cdl
bash ../validate_cdl.sh f_format_variants f_format_64bit_data.nc ../expected_output/f_format_variants_64bit_data_expected.cdl
bash ../validate_cdl.sh f_format_variants f_format_netcdf4.nc ../expected_output/f_format_variants_netcdf4_expected.cdl
bash ../validate_cdl.sh f_format_variants f_format_netcdf4_classic.nc ../expected_output/f_format_variants_netcdf4_classic_expected.cdl

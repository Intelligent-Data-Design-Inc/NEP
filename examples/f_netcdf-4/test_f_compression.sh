#!/bin/bash
set -e
./f_compression
bash ../validate_cdl.sh f_compression f_compress_none.nc ../expected_output/f_compression_none_expected.cdl
bash ../validate_cdl.sh f_compression f_compress_deflate1.nc ../expected_output/f_compression_deflate1_expected.cdl
bash ../validate_cdl.sh f_compression f_compress_deflate5.nc ../expected_output/f_compression_deflate5_expected.cdl
bash ../validate_cdl.sh f_compression f_compress_deflate9.nc ../expected_output/f_compression_deflate9_expected.cdl
bash ../validate_cdl.sh f_compression f_compress_shuffle.nc ../expected_output/f_compression_shuffle_expected.cdl
bash ../validate_cdl.sh f_compression f_compress_shuffle_deflate5.nc ../expected_output/f_compression_shuffle_deflate5_expected.cdl

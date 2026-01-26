#!/bin/bash
set -e
./f_chunking_performance
bash ../validate_cdl.sh f_chunking_performance f_chunk_contiguous.nc ../expected_output/f_chunking_performance_contiguous_expected.cdl
bash ../validate_cdl.sh f_chunking_performance f_chunk_time_optimized.nc ../expected_output/f_chunking_performance_time_optimized_expected.cdl
bash ../validate_cdl.sh f_chunking_performance f_chunk_spatial_optimized.nc ../expected_output/f_chunking_performance_spatial_optimized_expected.cdl
bash ../validate_cdl.sh f_chunking_performance f_chunk_balanced.nc ../expected_output/f_chunking_performance_balanced_expected.cdl

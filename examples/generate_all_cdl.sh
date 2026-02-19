#!/bin/bash
# Script to generate all expected CDL files for example validation
# Edward Hartnett 1/26/26

set -e

# Set up library path
export LD_LIBRARY_PATH=/usr/local/hdf5-1.14.6_cmake/lib:/usr/local/netcdf-c-4.9.3_cmake/lib:/usr/local/netcdf-fortran/lib:/usr/local/cdf-3.9.1/lib:$LD_LIBRARY_PATH

# Create output directory
mkdir -p expected_output

echo "Generating CDL files for C classic examples..."

# C Classic Examples
cd classic

# simple_2D
./simple_2D
ncdump simple_2D.nc > ../expected_output/simple_2D_expected.cdl
echo "  ✓ simple_2D_expected.cdl"

# coord_vars
./coord_vars
ncdump coord_vars.nc > ../expected_output/coord_vars_expected.cdl
echo "  ✓ coord_vars_expected.cdl"

# format_variants (creates 3 files)
./format_variants
ncdump format_classic.nc > ../expected_output/format_variants_classic_expected.cdl
ncdump format_64bit_offset.nc > ../expected_output/format_variants_64bit_offset_expected.cdl
ncdump format_64bit_data.nc > ../expected_output/format_variants_64bit_data_expected.cdl
echo "  ✓ format_variants_classic_expected.cdl"
echo "  ✓ format_variants_64bit_offset_expected.cdl"
echo "  ✓ format_variants_64bit_data_expected.cdl"

# size_limits (creates 3 files)
./size_limits
ncdump size_limits_classic.nc > ../expected_output/size_limits_classic_expected.cdl
ncdump size_limits_64bit_offset.nc > ../expected_output/size_limits_64bit_offset_expected.cdl
ncdump size_limits_64bit_data.nc > ../expected_output/size_limits_64bit_data_expected.cdl
echo "  ✓ size_limits_classic_expected.cdl"
echo "  ✓ size_limits_64bit_offset_expected.cdl"
echo "  ✓ size_limits_64bit_data_expected.cdl"

# unlimited_dim
./unlimited_dim
ncdump unlimited_dim.nc > ../expected_output/unlimited_dim_expected.cdl
echo "  ✓ unlimited_dim_expected.cdl"

# var4d
./var4d
ncdump var4d.nc > ../expected_output/var4d_expected.cdl
echo "  ✓ var4d_expected.cdl"

cd ..

echo ""
echo "Generating CDL files for C NetCDF-4 examples..."

# C NetCDF-4 Examples
cd netcdf-4

# simple_nc4
./simple_nc4
ncdump simple_nc4.nc > ../expected_output/simple_nc4_expected.cdl
echo "  ✓ simple_nc4_expected.cdl"

# compression (creates 6 files)
./compression
ncdump compress_none.nc > ../expected_output/compression_none_expected.cdl
ncdump compress_deflate1.nc > ../expected_output/compression_deflate1_expected.cdl
ncdump compress_deflate5.nc > ../expected_output/compression_deflate5_expected.cdl
ncdump compress_deflate9.nc > ../expected_output/compression_deflate9_expected.cdl
ncdump compress_shuffle.nc > ../expected_output/compression_shuffle_expected.cdl
ncdump compress_shuffle_deflate1.nc > ../expected_output/compression_shuffle_deflate1_expected.cdl
echo "  ✓ compression_none_expected.cdl"
echo "  ✓ compression_deflate1_expected.cdl"
echo "  ✓ compression_deflate5_expected.cdl"
echo "  ✓ compression_deflate9_expected.cdl"
echo "  ✓ compression_shuffle_expected.cdl"
echo "  ✓ compression_shuffle_deflate1_expected.cdl"

# chunking_performance (creates 4 files)
./chunking_performance
ncdump chunk_contiguous.nc > ../expected_output/chunking_performance_contiguous_expected.cdl
ncdump chunk_time_optimized.nc > ../expected_output/chunking_performance_time_optimized_expected.cdl
ncdump chunk_spatial_optimized.nc > ../expected_output/chunking_performance_spatial_optimized_expected.cdl
ncdump chunk_balanced.nc > ../expected_output/chunking_performance_balanced_expected.cdl
echo "  ✓ chunking_performance_contiguous_expected.cdl"
echo "  ✓ chunking_performance_time_optimized_expected.cdl"
echo "  ✓ chunking_performance_spatial_optimized_expected.cdl"
echo "  ✓ chunking_performance_balanced_expected.cdl"

# multi_unlimited
./multi_unlimited
ncdump multi_unlimited.nc > ../expected_output/multi_unlimited_expected.cdl
echo "  ✓ multi_unlimited_expected.cdl"

# user_types
./user_types
ncdump user_types.nc > ../expected_output/user_types_expected.cdl
echo "  ✓ user_types_expected.cdl"

cd ..

echo ""
echo "Generating CDL files for Fortran classic examples..."

# Fortran Classic Examples
cd f_classic

# f_simple_2D
./f_simple_2D
ncdump f_simple_2D.nc > ../expected_output/f_simple_2D_expected.cdl
echo "  ✓ f_simple_2D_expected.cdl"

# f_coord_vars
./f_coord_vars
ncdump f_coord_vars.nc > ../expected_output/f_coord_vars_expected.cdl
echo "  ✓ f_coord_vars_expected.cdl"

# f_format_variants (creates 3 files)
./f_format_variants
ncdump f_format_classic.nc > ../expected_output/f_format_variants_classic_expected.cdl
ncdump f_format_64bit_offset.nc > ../expected_output/f_format_variants_64bit_offset_expected.cdl
ncdump f_format_64bit_data.nc > ../expected_output/f_format_variants_64bit_data_expected.cdl
echo "  ✓ f_format_variants_classic_expected.cdl"
echo "  ✓ f_format_variants_64bit_offset_expected.cdl"
echo "  ✓ f_format_variants_64bit_data_expected.cdl"

# f_size_limits (creates 3 files)
./f_size_limits
ncdump f_size_limits_classic.nc > ../expected_output/f_size_limits_classic_expected.cdl
ncdump f_size_limits_64bit_offset.nc > ../expected_output/f_size_limits_64bit_offset_expected.cdl
ncdump f_size_limits_64bit_data.nc > ../expected_output/f_size_limits_64bit_data_expected.cdl
echo "  ✓ f_size_limits_classic_expected.cdl"
echo "  ✓ f_size_limits_64bit_offset_expected.cdl"
echo "  ✓ f_size_limits_64bit_data_expected.cdl"

# f_unlimited_dim
./f_unlimited_dim
ncdump f_unlimited_dim.nc > ../expected_output/f_unlimited_dim_expected.cdl
echo "  ✓ f_unlimited_dim_expected.cdl"

# f_var4d
./f_var4d
ncdump f_var4d.nc > ../expected_output/f_var4d_expected.cdl
echo "  ✓ f_var4d_expected.cdl"

cd ..

echo ""
echo "Generating CDL files for Fortran NetCDF-4 examples..."

# Fortran NetCDF-4 Examples
cd f_netcdf-4

# f_simple_nc4
./f_simple_nc4
ncdump f_simple_nc4.nc > ../expected_output/f_simple_nc4_expected.cdl
echo "  ✓ f_simple_nc4_expected.cdl"

# f_compression (creates 6 files)
./f_compression
ncdump f_compress_none.nc > ../expected_output/f_compression_none_expected.cdl
ncdump f_compress_deflate1.nc > ../expected_output/f_compression_deflate1_expected.cdl
ncdump f_compress_deflate5.nc > ../expected_output/f_compression_deflate5_expected.cdl
ncdump f_compress_deflate9.nc > ../expected_output/f_compression_deflate9_expected.cdl
ncdump f_compress_shuffle.nc > ../expected_output/f_compression_shuffle_expected.cdl
ncdump f_compress_shuffle_deflate1.nc > ../expected_output/f_compression_shuffle_deflate1_expected.cdl
echo "  ✓ f_compression_none_expected.cdl"
echo "  ✓ f_compression_deflate1_expected.cdl"
echo "  ✓ f_compression_deflate5_expected.cdl"
echo "  ✓ f_compression_deflate9_expected.cdl"
echo "  ✓ f_compression_shuffle_expected.cdl"
echo "  ✓ f_compression_shuffle_deflate1_expected.cdl"

# f_chunking_performance (creates 4 files)
./f_chunking_performance
ncdump f_chunk_contiguous.nc > ../expected_output/f_chunking_performance_contiguous_expected.cdl
ncdump f_chunk_time_optimized.nc > ../expected_output/f_chunking_performance_time_optimized_expected.cdl
ncdump f_chunk_spatial_optimized.nc > ../expected_output/f_chunking_performance_spatial_optimized_expected.cdl
ncdump f_chunk_balanced.nc > ../expected_output/f_chunking_performance_balanced_expected.cdl
echo "  ✓ f_chunking_performance_contiguous_expected.cdl"
echo "  ✓ f_chunking_performance_time_optimized_expected.cdl"
echo "  ✓ f_chunking_performance_spatial_optimized_expected.cdl"
echo "  ✓ f_chunking_performance_balanced_expected.cdl"

# f_multi_unlimited
./f_multi_unlimited
ncdump f_multi_unlimited.nc > ../expected_output/f_multi_unlimited_expected.cdl
echo "  ✓ f_multi_unlimited_expected.cdl"

# f_user_types
./f_user_types
ncdump f_user_types.nc > ../expected_output/f_user_types_expected.cdl
echo "  ✓ f_user_types_expected.cdl"

cd ..

echo ""
echo "CDL generation complete!"
echo "Total files generated: 50"

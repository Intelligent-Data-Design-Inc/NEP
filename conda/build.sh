#!/bin/bash
set -ex

echo "=== Conda build environment ==="
echo "PREFIX=${PREFIX}"
echo "BUILD_PREFIX=${BUILD_PREFIX}"
echo "SRC_DIR=${SRC_DIR}"
echo "CPU_COUNT=${CPU_COUNT}"
echo ""
echo "=== Checking for key libraries ==="
ls -la ${PREFIX}/lib/libcfitsio* 2>/dev/null || echo "libcfitsio NOT found in PREFIX/lib"
ls -la ${PREFIX}/lib/libxml2* 2>/dev/null || echo "libxml2 NOT found in PREFIX/lib"
ls -la ${PREFIX}/lib/liblz4* 2>/dev/null || echo "liblz4 NOT found in PREFIX/lib"
ls -la ${PREFIX}/lib/libbz2* 2>/dev/null || echo "libbz2 NOT found in PREFIX/lib"
echo ""
echo "=== Checking for key headers ==="
find ${PREFIX}/include -name "fitsio.h" 2>/dev/null || echo "fitsio.h NOT found"
find ${PREFIX}/include -name "libxml*" -type d 2>/dev/null || echo "libxml dir NOT found"
echo ""

cmake -B build \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DCMAKE_PREFIX_PATH="${PREFIX}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_FIND_DEBUG_MODE=ON \
  -DNEP_BUILD_LZ4=ON \
  -DNEP_BUILD_BZIP2=ON \
  -DNEP_ENABLE_FORTRAN=ON \
  -DNEP_ENABLE_GEOTIFF=OFF \
  -DNEP_ENABLE_GRIB2=OFF \
  -DNEP_ENABLE_FITS=ON \
  -DNEP_ENABLE_PDS4=ON \
  -DNEP_ENABLE_CDF=OFF \
  -DNEP_BUILD_DOCUMENTATION=OFF \
  -DNEP_BUILD_EXAMPLES=OFF \
  -DNEP_ENABLE_BENCHMARKS=OFF \
  -DNEP_ENABLE_PARALLEL_TESTS=OFF

cmake --build build -- -j${CPU_COUNT}
ctest --test-dir build --output-on-failure || true
cmake --install build

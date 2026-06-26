#!/bin/bash
# Run nczarr_simple and validate output CDL
# Edward Hartnett, Intelligent Data Design, Inc.
# 2026-06-26

if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
    export LD_LIBRARY_PATH=/home/ed/NEP/src/.libs:/usr/local/hdf5-2.1.0/lib:/usr/local/netcdf-c-4.10.0/lib:/usr/local/netcdf-fortran/lib:$LD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=/home/ed/NEP/src/.libs:/usr/local/hdf5-2.1.0/lib:/usr/local/netcdf-c-4.10.0/lib:/usr/local/netcdf-fortran/lib
fi

./nczarr_simple
bash ${srcdir}/../validate_cdl.sh nczarr_simple 'file://simple_nczarr.zarr#mode=nczarr' ${srcdir}/../expected_output/nczarr_simple_expected.cdl

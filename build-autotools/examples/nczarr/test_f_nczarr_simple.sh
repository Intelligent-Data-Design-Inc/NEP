#!/bin/bash
# Run f_nczarr_simple and validate output CDL
# Edward Hartnett, Intelligent Data Design, Inc.
# 2026-06-26

if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
    export LD_LIBRARY_PATH=/home/ed/NEP/build-autotools/src/.libs:/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:/usr/local/netcdf-fortran-4.6.2/lib:$LD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=/home/ed/NEP/build-autotools/src/.libs:/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:/usr/local/netcdf-fortran-4.6.2/lib
fi

./f_nczarr_simple
bash ${srcdir}/../validate_cdl.sh f_nczarr_simple 'file://f_simple_nczarr.zarr#mode=nczarr' ${srcdir}/../expected_output/f_nczarr_simple_expected.cdl

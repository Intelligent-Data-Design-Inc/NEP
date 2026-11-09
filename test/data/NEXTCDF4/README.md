# NEXTCDF-4 test data

`gdas_sfc_sample_bfloat16.nc` is `gdas_sfc_sample.nc` from
`../netCDF-4/` with the four data fields (`land`, `prate_ave`,
`soilw1`, `tmp2m`) stored as `NC_BFLOAT16` in a NEXTCDF-4 file. Values
were converted with `nc_get_var_bfloat16` (round to nearest, ties to
even).

It can only be read with the NEXTCDF-4 backend (`NC_NEXTCDF4`) and a
zstd HDF5 plugin on `HDF5_PLUGIN_PATH`, like the source file.

Differences from the source:

- `time` is a fixed dimension of length 1 (unlimited in the source).
- The data variables have no `_FillValue` or `missing_value`
  attributes (issue #377); fill points hold the bfloat16 rounding of
  `9.99e+20`.
- The coordinate variables stay `double`.

It is used by `examples/nextcdf4/bfloat16_io_perf.c`.

Regenerate with:

```sh
HDF5_PLUGIN_PATH=$HOME/plugins/zstd-only \
H5REPACK=/usr/local/hdf5-2.1.1/bin/h5repack \
  examples/nextcdf4/make_bfloat16_sample.sh build \
  test/data/netCDF-4/gdas_sfc_sample.nc \
  test/data/NEXTCDF4/gdas_sfc_sample_bfloat16.nc
```

Author: Edward Hartnett, Intelligent Data Design, Inc.

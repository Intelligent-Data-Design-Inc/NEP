# Test data

`gdas_sfc_sample.nc` is a small sample of the NOAA GFS/GDAS file
`gdas.t00z.sfcf003.nc` (FV3GFS surface forecast, 3-hour lead, grid
1536 x 768). It contains four full-grid fields plus the `grid_xt`,
`grid_yt` and `time` coordinate variables:

- `tmp2m` — 2 m temperature: smooth, typical meteorological field.
- `prate_ave` — average precipitation rate: noisy, hard to compress.
- `soilw1` — soil wetness: defined over land only, fill value over
  ocean.
- `land` — land-sea mask: categorical values.

The fields are stored chunked (1 x 768 x 1536) with the HDF5 shuffle
filter plus zstd level 19 (filter 32015). Reading the file requires a
zstd HDF5 plugin on `HDF5_PLUGIN_PATH`; without one, the file opens and
its metadata is readable, but reading the data fields fails.

Regenerate it from the full source file with:

```sh
mkdir -p "$HOME/plugins/zstd-only"
ln -sf /usr/local/netcdf-c/hdf5/lib/plugin/lib__nch5zstd.so "$HOME/plugins/zstd-only/"
HDF5_PLUGIN_PATH=$HOME/plugins/zstd-only \
  performance/make_gdas_sample.sh /home/ed/Downloads/gdas.t00z.sfcf003.nc test/data/gdas_sfc_sample.nc
```

Author: Edward Hartnett, Intelligent Data Design, Inc.

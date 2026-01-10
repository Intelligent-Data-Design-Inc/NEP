---
trigger: model_decision
---
# Local Build Command for NEP
## Machine-Specific Paths
- HDF5: [/usr/local/hdf5-1.14.6/](cci:7://file:///usr/local/hdf5-1.14.6:0:0-0:0) (custom build location)
- NetCDF-C: [/usr/local/netcdf-c/](cci:7://file:///usr/local/netcdf-c:0:0-0:0) (custom build location)
## Build Command
```bash
autoreconf -i && \
CFLAGS="-g -O0" \
CPPFLAGS="-I/usr/local/hdf5-1.14.6/include -I/usr/local/netcdf-c/include" \
LDFLAGS="-L/usr/local/hdf5-1.14.6/lib -L/usr/local/netcdf-c/lib" \
./configure --enable-geotiff --disable-fortran --disable-shared --disable-bzip2 --disable-lz4 && \
make clean && make -j && make check
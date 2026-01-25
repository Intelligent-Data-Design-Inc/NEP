---
trigger: model_decision
---
# Local Build Commands for NEP

## When to Use This Rule
Use these paths for **local development builds on Ed's machine**. 
For CI/GitHub Actions, different paths are used (see `.github/workflows/`).

## Machine-Specific Dependency Paths
- **HDF5**: `/usr/local/hdf5-1.14.6_cmake/`
- **NetCDF-C**: `/usr/local/netcdf-c-4.9.3_cmake/`
- **NetCDF-Fortran**: `/usr/local/netcdf-fortran/` (if Fortran enabled)
- **CDF**: `/usr/local/cdf-3.9.1/` (if CDF enabled)
- **GeoTIFF**: System packages (`libgeotiff-dev`, `libtiff-dev`)

## Runtime Environment
Before running tests or executables:
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-1.14.6_cmake/lib:/usr/local/netcdf-c-4.9.3_cmake/lib:/usr/local/netcdf-fortran/lib:/usr/local/cdf-3.9.1/lib:$LD_LIBRARY_PATH
```

## Build System Options

### Autotools (Primary)
Working directory: `/home/ed/NEP`

**Common configure flags:**
- `--enable-geotiff` - Enable GeoTIFF reader
- `--enable-cdf` - Enable NASA CDF reader
- `--disable-lz4` - Disable LZ4 compression
- `--disable-bzip2` - Disable bzip2 compression
- `--disable-fortran` - Disable Fortran wrapper library
- `--disable-shared` - Build static libraries only

**Full build command:**
```bash
autoreconf -i && \
CFLAGS="-g -O0" \
CPPFLAGS="-I/usr/local/hdf5-1.14.6_cmake/include -I/usr/local/netcdf-c-4.9.3_cmake/include -I/usr/local/cdf-3.9.1/include" \
LDFLAGS="-L/usr/local/hdf5-1.14.6_cmake/lib -L/usr/local/netcdf-c-4.9.3_cmake/lib -L/usr/local/cdf-3.9.1/lib -Wl,-rpath,/usr/local/hdf5-1.14.6_cmake/lib -Wl,-rpath,/usr/local/netcdf-c-4.9.3_cmake/lib" \
./configure --enable-geotiff --enable-cdf --disable-fortran --disable-shared --disable-bzip2 --disable-lz4 && \
make clean && make -j$(nproc) && make check
```

### CMake (Alternative)
**IMPORTANT**: All CMake builds must use the `build` directory, which is git-ignored.

Working directory: `/home/ed/NEP`

```bash
mkdir -p build && cd build
cmake .. \
  -DCMAKE_PREFIX_PATH="/usr/local/hdf5-1.14.6_cmake;/usr/local/netcdf-c-4.9.3_cmake;/usr/local/cdf-3.9.1" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_GEOTIFF=ON \
  -DENABLE_CDF=ON \
  -DENABLE_FORTRAN=OFF
make -j$(nproc) && ctest
```

**Never create CMake build artifacts outside the `build` directory** to avoid cluttering the repository with untracked files.

## Troubleshooting
- **"library not found" errors**: Check `LD_LIBRARY_PATH` is set
- **"header not found" errors**: Verify `CPPFLAGS` includes correct paths
- **Link errors**: Ensure `LDFLAGS` includes all dependency lib directories
- **Test failures**: Run `make check VERBOSE=1` for detailed output

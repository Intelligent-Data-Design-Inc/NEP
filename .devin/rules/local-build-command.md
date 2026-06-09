---
trigger: model_decision
---
# Local Build Commands for NEP

## When to Use This Rule
Use these paths for **local development builds on Ed's machine**. 
For CI/GitHub Actions, different paths are used (see `.github/workflows/`).

## Machine-Specific Dependency Paths
- **HDF5**: `/usr/local/hdf5-2.1.0/`
- **NetCDF-C**: `/usr/local/netcdf-c-4.10.0/`
- **NetCDF-Fortran**: `/usr/local/netcdf-fortran/` (if Fortran enabled)
- **CDF**: `/usr/local/cdf-3.9.1/` (if CDF enabled)
- **GeoTIFF**: System packages (`libgeotiff-dev`, `libtiff-dev`)

## Runtime Environment
Before running tests or executables:
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-2.1.0/lib:/usr/local/netcdf-c-4.10.0/lib:/usr/local/netcdf-fortran/lib:/usr/local/cdf-3.9.1/lib:$LD_LIBRARY_PATH
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
CPPFLAGS="-I/usr/local/hdf5-2.1.0/include -I/usr/local/netcdf-c-4.10.0/include -I/usr/local/netcdf-fortran/include -I/usr/local/cdf-3.9.1/include" \
LDFLAGS="-L/usr/local/hdf5-2.1.0/lib -L/usr/local/netcdf-c-4.10.0/lib -L/usr/local/netcdf-fortran/lib -L/usr/local/cdf-3.9.1/lib -Wl,-rpath,/usr/local/hdf5-2.1.0/lib -Wl,-rpath,/usr/local/netcdf-c-4.10.0/lib -Wl,-rpath,/usr/local/netcdf-fortran/lib" \
./configure --enable-geotiff --enable-cdf --disable-fortran --disable-shared --disable-bzip2 --disable-lz4 && \
make clean && make -j$(nproc) && make check
```

### CMake (Alternative)
**IMPORTANT**: All CMake builds must use the `build` directory, which is git-ignored.

Working directory: `/home/ed/NEP`

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/usr/local/netcdf-c-4.10.0;/usr/local/hdf5-2.1.0;/usr/local/cdf-3.9.1;/usr/local/NCEPLIBS-g2c-2.3.0;/usr/local/jasper-3.0.3" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_GEOTIFF=ON \
  -DENABLE_CDF=ON \
  -DENABLE_FORTRAN=OFF \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,/usr/local/hdf5-2.1.0/lib -Wl,-rpath,/usr/local/netcdf-c-4.10.0/lib"
make -j$(nproc) -C build && ctest --test-dir build
```

Add `-DENABLE_BENCHMARKS=ON -DBUILD_EXAMPLES=ON` to also build and run the performance examples.

**Runtime environment for tests** (if rpath not embedded):
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-2.1.0/lib:/usr/local/netcdf-c-4.10.0/lib:$LD_LIBRARY_PATH
```

**Never create CMake build artifacts outside the `build` directory** to avoid cluttering the repository with untracked files.

## Parallel Build (v1.9.0 Sprint 3)

Use these paths for local parallel I/O builds with MPICH and `--enable-parallel-tests`.

**Dependency chain**: HDF5 → NetCDF-C → NetCDF-Fortran, all MPICH-enabled:
- **HDF5 (MPICH)**: `/usr/local/hdf5-2.1.0_mpich/`
- **NetCDF-C (MPICH, parallel)**: `/usr/local/netcdf-c-4.10.0_mpich/`
- **NetCDF-Fortran (MPICH)**: `/usr/local/netcdf-fortran-4.6.2_mpich/`
- **MPI compilers**: `/usr/bin/mpicc`, `/usr/bin/mpif90` (system MPICH)

**Runtime LD_LIBRARY_PATH for parallel builds:**
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-2.1.0_mpich/lib:/usr/local/netcdf-c-4.10.0_mpich/lib:/usr/local/netcdf-fortran-4.6.2_mpich/lib:$LD_LIBRARY_PATH
```

**Autotools parallel build command:**
```bash
autoreconf -i && \
CFLAGS="-g -O0" \
CPPFLAGS="-I/usr/local/hdf5-2.1.0_mpich/include -I/usr/local/netcdf-c-4.10.0_mpich/include -I/usr/local/netcdf-fortran-4.6.2_mpich/include" \
LDFLAGS="-L/usr/local/hdf5-2.1.0_mpich/lib -L/usr/local/netcdf-c-4.10.0_mpich/lib -L/usr/local/netcdf-fortran-4.6.2_mpich/lib -Wl,-rpath,/usr/local/hdf5-2.1.0_mpich/lib -Wl,-rpath,/usr/local/netcdf-c-4.10.0_mpich/lib -Wl,-rpath,/usr/local/netcdf-fortran-4.6.2_mpich/lib" \
CC=mpicc FC=mpif90 \
./configure --enable-fortran --enable-parallel-tests --disable-geotiff --disable-cdf --disable-grib2 --disable-shared --disable-bzip2 --disable-lz4 && \
make clean && make -j$(nproc) && make check
```

**CMake parallel build command:**
```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=mpicc \
  -DCMAKE_Fortran_COMPILER=mpif90 \
  -DCMAKE_PREFIX_PATH="/usr/local/hdf5-2.1.0_mpich;/usr/local/netcdf-c-4.10.0_mpich;/usr/local/netcdf-fortran-4.6.2_mpich" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_FORTRAN=ON \
  -DENABLE_PARALLEL_TESTS=ON \
  -DENABLE_GEOTIFF=OFF \
  -DENABLE_CDF=OFF \
  -DENABLE_GRIB2=OFF \
  -DBUILD_LZ4=OFF \
  -DBUILD_BZIP2=OFF \
  -DBUILD_EXAMPLES=ON \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,/usr/local/hdf5-2.1.0_mpich/lib -Wl,-rpath,/usr/local/netcdf-c-4.10.0_mpich/lib -Wl,-rpath,/usr/local/netcdf-fortran-4.6.2_mpich/lib"
make -j$(nproc) -C build && ctest --test-dir build --verbose
```

## Troubleshooting
- **"library not found" errors**: Check `LD_LIBRARY_PATH` is set
- **"header not found" errors**: Verify `CPPFLAGS` includes correct paths
- **Link errors**: Ensure `LDFLAGS` includes all dependency lib directories
- **Test failures**: Run `make check VERBOSE=1` for detailed output

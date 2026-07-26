---
trigger: model_decision
---
# Local Build Commands for NEP

## When to Use This Rule
Use these paths for **local development builds on Ed's machine**. 
For CI/GitHub Actions, different paths are used (see `.github/workflows/`).

## Machine-Specific Dependency Paths
- **HDF5**: `/usr/local/hdf5-2.1.1/`
- **NetCDF-C**: `/usr/local/netcdf-c/`
- **NetCDF-Fortran**: `/usr/local/netcdf-fortran/` (if Fortran enabled)
- **CDF**: `/usr/local/cdf-3.9.1/` (if CDF enabled)
- **GeoTIFF**: System packages (`libgeotiff-dev`, `libtiff-dev`)
- **CFITSIO**: `/usr/local/cfitsio-4.6.4/` (FITS reader support)

## Runtime Environment
Before running tests or executables:
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:/usr/local/netcdf-fortran/lib:/usr/local/cdf-3.9.1/lib:$LD_LIBRARY_PATH
```

## Build System Options

### CMake
**IMPORTANT**: All CMake builds must use the `build` directory, which is git-ignored.

Working directory: `/home/ed/NEP`

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/usr/local/hdf5-2.1.1;/usr/local/netcdf-c;/usr/local/cdf-3.9.1;/usr/local/NCEPLIBS-g2c-2.3.0;/usr/local/jasper-3.0.3" \
  -DHDF5_ROOT=/usr/local/hdf5-2.1.1 \
  -DCMAKE_POLICY_DEFAULT_CMP0074=NEW \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNEP_ENABLE_GEOTIFF=ON \
  -DNEP_ENABLE_CDF=ON \
  -DNEP_ENABLE_GRIB2=OFF \
  -DNEP_ENABLE_FORTRAN=OFF \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,/usr/local/hdf5-2.1.1/lib -Wl,-rpath,/usr/local/netcdf-c/lib"
make -j$(nproc) -C build && ctest --test-dir build
```

**IMPORTANT**: `-DHDF5_ROOT=/usr/local/hdf5-2.1.1` and `-DCMAKE_POLICY_DEFAULT_CMP0074=NEW`
are required. Without them, CMake may select a different HDF5 than the one used to build NetCDF-C, causing `NetCDF: HDF error` at runtime.


**IMPORTANT**: If CMakeCache.txt has stale HDF5 entries, delete it before reconfiguring:
```bash
rm -f build/CMakeCache.txt
```

Add `-DNEP_ENABLE_BENCHMARKS=ON -DNEP_BUILD_EXAMPLES=ON` to also build and run the performance examples.

**Running performance benchmarks** (after building with NEP_ENABLE_BENCHMARKS=ON):
```bash
LD_LIBRARY_PATH=/home/ed/NEP/build/src:/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:/usr/local/jasper-3.0.3/lib:$LD_LIBRARY_PATH \
HDF5_PLUGIN_PATH=/usr/local/hdf5/lib/plugin \
./build/examples/performance/<example>
```

**Runtime environment for tests** (if rpath not embedded):
```bash
export LD_LIBRARY_PATH=/usr/local/hdf5-2.1.1/lib:/usr/local/netcdf-c/lib:$LD_LIBRARY_PATH
```

**Never create CMake build artifacts outside the `build` directory** to avoid cluttering the repository with untracked files.

## Parallel Build (v1.9.0 Sprint 3)

Use these paths for local parallel I/O builds with MPICH and `--enable-parallel-tests`.

**Dependency chain**: HDF5 → NetCDF-C → NetCDF-Fortran, all MPICH-enabled:
- **HDF5 (MPI)**: an MPI-enabled HDF5 2.1.1+ installation
- **NetCDF-C (MPI, parallel)**: an MPI-enabled NetCDF-C 4.10.1+ installation built against that HDF5
- **NetCDF-Fortran (MPI)**: an MPI-enabled installation built against that NetCDF-C
- **MPI compilers**: `/usr/bin/mpicc`, `/usr/bin/mpif90` (system MPICH)

**Runtime LD_LIBRARY_PATH for parallel builds:**
```bash
export LD_LIBRARY_PATH=$HDF5_ROOT/lib:$NETCDF_C_ROOT/lib:$NETCDF_FORTRAN_ROOT/lib:$LD_LIBRARY_PATH
```

**CMake parallel build command:**
```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=mpicc \
  -DCMAKE_Fortran_COMPILER=mpif90 \
  -DCMAKE_PREFIX_PATH="$HDF5_ROOT;$NETCDF_C_ROOT;$NETCDF_FORTRAN_ROOT" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNEP_ENABLE_FORTRAN=ON \
  -DNEP_ENABLE_PARALLEL_TESTS=ON \
  -DNEP_ENABLE_GEOTIFF=OFF \
  -DNEP_ENABLE_CDF=OFF \
  -DNEP_ENABLE_GRIB2=OFF \
  -DNEP_BUILD_LZ4=OFF \
  -DNEP_BUILD_BZIP2=OFF \
  -DNEP_BUILD_EXAMPLES=ON \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,$HDF5_ROOT/lib -Wl,-rpath,$NETCDF_C_ROOT/lib -Wl,-rpath,$NETCDF_FORTRAN_ROOT/lib"
make -j$(nproc) -C build && ctest --test-dir build --verbose
```

## Troubleshooting
- **"library not found" errors**: Check `LD_LIBRARY_PATH` is set
- **"header not found" errors**: Verify `CPPFLAGS` includes correct paths
- **Link errors**: Ensure `LDFLAGS` includes all dependency lib directories
- **Test failures**: Run `ctest --test-dir build --output-on-failure` for detailed output

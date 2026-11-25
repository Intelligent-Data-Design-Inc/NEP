# NEP (NetCDF Expansion Pack) - Press Release / FAQ

## About NEP at v4.3.0

NEP (NetCDF Expansion Pack) is an open-source framework that extends
NetCDF-C so existing C, Fortran and Python programs can read additional
scientific data formats through the standard NetCDF API, with no code
changes:

- **Space**: NASA CDF space physics, FITS astronomy, NASA/ESA PDS4
  planetary archives
- **Earth**: GeoTIFF geospatial rasters, GRIB2 NWP model output
- **Medical**: DICOM imaging
- **Bio**: legacy PDB and PDBx/mmCIF macromolecular structures

NEP also provides **NEXTCDF-4**, an experimental, backward-compatible
drop-in replacement for the netCDF-4/HDF5 backend. NEXTCDF-4 adds 16-bit
floating-point types, HDF5 reference types, the latest HDF5 superblock
support, and performance improvements, while its compatibility modes
write files readable by upstream netcdf-c.

LZ4/BZIP2 compression filters moved to the separate
[nep-compress](https://github.com/captainkirk99/nep-compress) project in
v4.2.0; install it alongside NEP for compression APIs and plugins.

Installation: CMake from source, `spack install nep` for HPC
environments, or `conda install -c conda-forge nep` (Linux).
Documentation: https://intelligent-data-design-inc.github.io/NEP/

---

## Frequently Asked Questions

### General Questions

#### Q: What is NEP?
**A:** NEP (NetCDF Expansion Pack) extends NetCDF-C with:
- User Defined Format (UDF) handlers for NASA CDF, FITS, PDS4, GeoTIFF, GRIB2, DICOM, legacy PDB and PDBx/mmCIF files
- The optional NEXTCDF-4 backend, a drop-in replacement for netCDF-4/HDF5 with new atomic types, reference types and the latest HDF5 superblock support
- Spack package manager and conda-forge support for simplified deployment
- Comprehensive example programs as companion code to *The NetCDF Developer's Handbook, Second Edition*

LZ4/BZIP2 compression lives in [nep-compress](https://github.com/captainkirk99/nep-compress).

#### Q: Who should use NEP?
**A:** NEP is designed for:
- Scientific researchers working with NetCDF-4/HDF5, NASA CDF, GeoTIFF, GRIB2, FITS, or PDS4 datasets
- Space physics researchers analyzing CDF satellite and mission data (IMAP, MMS, Van Allen Probes)
- Geospatial analysts working with GeoTIFF satellite imagery and raster data
- Meteorologists and oceanographers processing NWP model output in GRIB2 format
- Astronomers accessing FITS data from HST, Chandra, JWST, and other observatories
- Planetary scientists working with NASA/ESA PDS4 archive products
- Data center operators optimizing storage efficiency
- HPC system administrators optimizing I/O performance
- Software developers building scientific applications

#### Q: What problem does NEP solve?
**A:** NEP addresses key challenges in scientific data management:
1. **Format fragmentation**: Scientists must convert between NetCDF, CDF, GeoTIFF, GRIB2, FITS, and PDS4 formats
2. **Complex workflows**: Multiple tools required for different scientific data formats
3. **Backend limitations**: NEXTCDF-4 (experimental) extends type and metadata support

### Technical Questions

#### Q: What compression algorithms does NEP support?
**A:** NEP no longer bundles compression plugins or LZ4/BZIP2 wrappers.
Install [nep-compress](https://github.com/captainkirk99/nep-compress) for
LZ4 (speed-optimized, 2-3x faster than DEFLATE) and BZIP2 (ratio-optimized
for archival storage), including its C and Fortran wrapper APIs and HDF5
plugins. Through NEXTCDF-4, NEP itself supports generic HDF5 filters via
`nc_def_var_filter()` — including deflate, fletcher32, szip, bzip2 and
zstd plugins — but does not ship filter-specific convenience APIs.

#### Q: Do I need to modify my existing NetCDF code?
**A:** Format readers work with standard NetCDF applications once enabled. For the LZ4/BZIP2 wrapper APIs, update the include/module and link settings to use nep-compress:
- **Compression**: Install nep-compress and set `HDF5_PLUGIN_PATH` to its plugin directory (and the external BZIP2 plugin directory if used)
- **CDF files**: Use `nc_open()` to read CDF files directly
- **GeoTIFF files**: Use `nc_open()` to read GeoTIFF files directly
- **GRIB2 files**: Use `nc_open()` to read GRIB2 files directly
- **FITS files**: Call `NC_FITS_initialize()` (or use `.ncrc` autoload), then `nc_open()` reads FITS files
- **PDS4 files**: Call `NC_PDS4_initialize()` (or use `.ncrc` autoload), then `nc_open()` reads PDS4 XML labels
- **NEXTCDF-4 files**: Pass `NC_NEXTCDF4` to `nc_create()` or `nc_open()` to use the optional NEXTCDF-4 backend
All features integrate seamlessly with the standard NetCDF API.

#### Q: Does NEP provide example programs to help me learn?
**A:** Yes! NEP's example programs exercise NEP features and run as tests:
- **NEXTCDF-4 examples**: float16/bfloat16 I/O, sample generators, and conversion benchmarks (`examples/nextcdf4/`)
- **Performance examples**: Disk-bound NetCDF benchmarks (`examples/performance/`, `NEP_ENABLE_BENCHMARKS`); LZ4/BZIP2 benchmarks moved to nep-compress
- **UDF reader examples**: DICOM and PDB readers (`examples/dicom/`, `examples/pdb/`)
- **Built and tested**: Examples build with NEP and run as tests
- **Location**: All examples in the `examples/` directory with detailed README

The generic NetCDF teaching examples (classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and all Fortran versions) moved to the [NetCDF Developer's Handbook](https://github.com/captainkirk99/NetCDF_Developers_Handbook) repository in v4.3.0, where they remain companion code for *The NetCDF Developer's Handbook, Second Edition*.

#### Q: Does NEP provide visualization examples?
**A:** Yes. Optional Python examples in `examples/viz/` open enabled FITS, CDF, GeoTIFF, GRIB2, and PDS4 UDF files directly through `netCDF4.Dataset` and create static grayscale PNG plots with companion metadata files. Enable them with `-DNEP_ENABLE_VIZ_EXAMPLES=ON` or `--enable-viz-examples`. Their source-root `.venv` must build `netCDF4` from source against the same NetCDF-C library as NEP; bundled-libnetcdf wheels cannot load the NEP UDF handlers. Tests use the build-tree `.ncrc` configuration and validate output metadata, grayscale pixels, and publication size limits. The plots and metadata remain in the build tree and are not installed.

#### Q: What is NEXTCDF-4?
**A:** NEXTCDF-4 is a clean-room rewrite of the NetCDF-4/HDF5 backend delivered as a NEP UDF expansion pack. It implements the full enhanced NetCDF-4 data model, supports new atomic types (`NC_FLOAT16`, small floating-point types, complex numbers, bitfields, and HDF5 references), and fixes long-standing renaming issues, while coexisting with netcdf-c's built-in HDF5 backend. Files are selected explicitly with `NC_NEXTCDF4`.

#### Q: How do I create or open a NEXTCDF-4 file?
**A:** Pass `NC_NEXTCDF4` to `nc_create()` or `nc_open()`:
```c
nc_create("file.nc", NC_NEXTCDF4 | NC_CLOBBER, &ncid);
nc_open("file.nc", NC_NEXTCDF4 | NC_NOWRITE, &ncid);
```
There is no magic-number dispatch for NEXTCDF-4 because HDF5 files are already owned by the built-in NetCDF-4 backend; you must request it with the mode flag.

#### Q: What new atomic types does NEXTCDF-4 support?
**A:** NEXTCDF-4 adds small floating-point types (`NC_FLOAT16`, `NC_BFLOAT16`, `NC_FLOAT8_E4M3`, `NC_FLOAT8_E5M2`, `NC_FLOAT6_E2M3`, `NC_FLOAT6_E3M2`, `NC_FLOAT4_E2M1`), complex numbers (`NC_COMPLEX`, `NC_DOUBLECOMPLEX`), bitfields (`NC_BITFIELD8/16/32/64`), and HDF5 references (`NC_STD_REF`, covering object, region, and attribute targets). These types require native NEXTCDF-4 mode and are not available in `NC_NETCDF4_MODEL` compatibility mode.

#### Q: Can NEXTCDF-4 files be read by upstream netcdf-c?
**A:** Files created with the `NC_NETCDF4_MODEL` compatibility flag are written with HDF5 Superblock v1 and can be read by upstream netcdf-c linked against HDF5 1.10.x or later. Native NEXTCDF-4 files use HDF5 Superblock v3 and require HDF5 1.14.x or later; whether upstream netcdf-c can read them depends on the HDF5 version it was built with.

#### Q: Why is NEXTCDF-4 explicit selection only?
**A:** HDF5 files share a single fixed magic number that is already registered by netcdf-c's built-in NetCDF-4 backend. To avoid modifying netcdf-c's dispatch or model-inference code, NEXTCDF-4 registers only the UDF9 slot and leaves magic-number dispatch unchanged. Applications therefore pass `NC_NEXTCDF4` when they want the NEXTCDF-4 backend.

#### Q: What are `NC_CLASSIC_MODEL` and `NC_NETCDF4_MODEL` compatibility modes?
**A:** `NC_CLASSIC_MODEL` restricts the file to the NetCDF-3 data model (root group only, one unlimited dimension, classic atomic types). `NC_NETCDF4_MODEL` allows the full enhanced NetCDF-4 data model but forbids NEXTCDF-4-specific atomic types so the file remains readable by upstream netcdf-c. Both modes can be combined with `NC_NEXTCDF4` at create time but are mutually exclusive with each other.

### Installation and Usage

#### Q: What are the system requirements?
**A:** 
- **Operating System**: Linux or Unix
- **Core Libraries**: NetCDF-C (v4.10.1+), HDF5 (v1.14+)
- **Fortran Support (optional)**: NetCDF-Fortran (v4.6.2+ required for the NEXTCDF-4 binding)
- **CDF Support (optional)**: NASA CDF Library (v3.9.x)
- **GeoTIFF Support (optional)**: libgeotiff, libtiff
- **GRIB2 Support (optional)**: NOAA NCEPLIBS-g2c (≥ 2.1.0), libjasper (≥ 3.0.0)
- **FITS Support (optional)**: CFITSIO (≥ 3.0; locally `/usr/local/cfitsio-4.6.4/`; CI: apt `libcfitsio-dev`)
- **PDS4 Support (optional)**: libxml2 (`libxml2-dev` on Ubuntu)
- **DICOM Support (optional)**: libdicom, libjpeg/libjpeg-turbo, libgdcm
- **NEXTCDF-4 Support (optional)**: HDF5 2.1.1 (required); netcdf-c with the 64-slot UDF scheme (v4.10.2 or current main) and `NC_UDF9` support
- **Build Tools**: CMake (v3.19+)
- **Documentation (optional)**: Doxygen and Graphviz

#### Q: How do I install NEP?
**A:** Multiple installation methods:
- **Spack** (recommended for HPC): `spack install nep`
- **Conda**: `conda install -c conda-forge nep` (Linux; PDB, mmCIF, FITS and PDS4 readers; compression is in `nep-compress`)
- **CMake**: `cmake -B build && cmake --build build && cmake --install build`
See the README for detailed instructions.

#### Q: Can I enable only specific features?
**A:** Yes! Use build options to control which features are compiled. All extended format readers default to **OFF** (v2.2.0+); enable explicitly as needed:
- **CMake**: `-DNEP_ENABLE_FORTRAN=ON/OFF`, `-DNEP_ENABLE_CDF=ON/OFF`, `-DNEP_ENABLE_GEOTIFF=ON/OFF`, `-DNEP_ENABLE_GRIB2=ON/OFF`, `-DNEP_ENABLE_FITS=ON/OFF`, `-DNEP_ENABLE_PDS4=ON/OFF`, `-DNEP_ENABLE_DICOM=ON/OFF`, `-DNEP_ENABLE_NEXTCDF4=ON/OFF`, `-DNEP_BUILD_EXAMPLES=ON/OFF`
- All five format readers (CDF, GeoTIFF, GRIB2, FITS, PDS4) can be enabled simultaneously — there are no mutual-exclusivity restrictions as of v2.2.0
- **Spack**: Reader variants control formats; install nep-compress separately for LZ4/BZIP2

#### Q: How do I use LZ4 compression in my NetCDF files?
**A:** Install [nep-compress](https://github.com/captainkirk99/nep-compress), set `HDF5_PLUGIN_PATH` to its LZ4 plugin directory, include `<nep_compress.h>` and link `libnepcompress` (or use `module nep_compress` and `libnepcompressf` in Fortran). NEP v4.2 no longer provides these wrapper functions:
```c
nc_def_var_lz4(ncid, varid, level);  // C API
nf90_def_var_lz4(ncid, varid, level) ! Fortran API
```

#### Q: How do I read CDF files with NEP?
**A:** Enable CDF support during build (`-DNEP_ENABLE_CDF=ON`), then use standard NetCDF API:
```c
nc_open("data.cdf", NC_NOWRITE, &ncid);
nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid);
nc_get_var_double(ncid, varid, data);
```

#### Q: How do I read GeoTIFF files with NEP?
**A:** Enable GeoTIFF support during build (`-DNEP_ENABLE_GEOTIFF=ON`), then use standard NetCDF API:
```c
nc_open("image.tif", NC_NOWRITE, &ncid);
nc_inq_dimlen(ncid, 0, &bands);
nc_get_var_float(ncid, varid, raster_data);
```

#### Q: How do I read GRIB2 files with NEP?
**A:** Enable GRIB2 support during build (`-DNEP_ENABLE_GRIB2=ON`), supply NCEPLIBS-g2c path, then use standard NetCDF API:
```c
nc_open("forecast.grib2", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "WIND", &varid);
nc_get_var_float(ncid, varid, data);  /* full [ny][nx] grid, land = _FillValue */
```
Or use `ncdump forecast.grib2` directly after installing the `.ncrc` file.

#### Q: How do I read FITS files with NEP?
**A:** Enable FITS support during build (`-DNEP_ENABLE_FITS=ON`), ensure CFITSIO is installed, then register the handler and use the standard NetCDF API:
```c
NC_FITS_initialize();                  /* register UDF3 handler */
nc_open("image.fits", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "image", &varid);
nc_get_vara_float(ncid, varid, start, count, pixels);
```
Extension HDUs appear as child groups; use `nc_inq_grp_ncid()` to navigate them.

#### Q: How do I read PDS4 files with NEP?
**A:** Enable PDS4 support during build (`-DNEP_ENABLE_PDS4=ON`), ensure libxml2 is installed, then register the handler and open the XML label:
```c
NC_PDS4_initialize();                           /* register UDF5 handler */
nc_open("product.xml", NC_NOWRITE, &ncid);
nc_inq_ncid(ncid, "datafile.img", &grpid);     /* child group per file area */
nc_inq_varid(grpid, "image", &varid);
nc_get_vara_float(grpid, varid, start, count, data);
```

### Compatibility and Performance

#### Q: Is NEP compatible with existing NetCDF applications?
**A:** Format-reader and NEXTCDF-4 APIs remain available. Applications using the former NEP LZ4/BZIP2 wrapper APIs must migrate to nep-compress.

#### Q: What is the performance overhead?
**A:** 
- NEP's performance depends on the enabled format reader and workload.
- LZ4/BZIP2 compression performance benchmarks are maintained by nep-compress.

#### Q: Does NEP support parallel I/O?
**A:** NEP has MPI test programs using upstream NetCDF (`NEP_ENABLE_PARALLEL_TESTS`); the parallel I/O example programs moved to the NetCDF Developer's Handbook repository in v4.3.0. The experimental NEXTCDF-4 backend gained file-level parallel I/O in v4.3.0: with `-DNEP_ENABLE_NEXTCDF4_PARALLEL=ON` (MPI + parallel HDF5 + parallel NetCDF-C), `nc_create_par`/`nc_open_par` work on NEXTCDF-4 files via the MPI-IO driver — `nc_create_par` takes `NC_NEXTCDF4|NC_NETCDF4`, `nc_open_par` takes bare `NC_NEXTCDF4`. `nc_var_par_access` selects independent (default) or collective data transfers per variable. All ranks participate in collective transfers, even with zero elements; unlimited growth requires collective access (`NC_ECANTEXTEND` for independent growth). Filters defined in parallel force collective access; after reopening, select collective access explicitly before filtered writes (independent filtered reads are permitted, matching upstream). Fixed-size types, including small floats and fixed-size user types, are supported; parallel string/dynamic/reference variable I/O returns `NC_ENOTBUILT`. Four-rank CI covers both Open MPI (`ci-parallel.yml`) and MPICH (`ci-parallel-mpich.yml`, running the `nextcdf4_parallel` label — eight ported upstream `nc_test4` parallel tests in two modes plus interchange and external-plugin tests); the deprecated `NC_MPIIO`/`NC_MPIPOSIX` flags remain rejected. Memory-image files, SWMR and sharing modes are also unsupported; memory/sharing flags are rejected rather than silently ignored.

#### Q: Can I control NEXTCDF-4 fill, cache settings and group names?
**A:** Yes, starting with v4.3.0 Sprint 1O. `nc_set_fill` sets the session default for newly defined eligible variables; existing variables and explicit per-variable overrides stay unchanged, and reopen resets the default to `NC_FILL`. `nc_set_var_chunk_cache` applies actual HDF5 cache settings even in read-only/data mode; overrides last only for the session, and `nc_get_var_chunk_cache` reports effective chunked settings. Group creation/rename work in native and `NC_NETCDF4_MODEL` modes, preserving descendants and data; classic-model groups remain unsupported. See [NEXTCDF-4](nextcdf4.md) for validation, fill exclusions and cache details.

#### Q: Can NEXTCDF-4 store strings and nested string data?
**A:** Yes, in native and NetCDF-4-model files, including strings in compound/array members and vlen bases. Values and attributes survive reopen; model-mode output interchanges with upstream netcdf-c, and NEXTCDF-4 can update upstream-created string files. Writes borrow your buffers, while reads and fill inquiries return owned payloads: use `nc_free_string` for standalone arrays and recursive `nc_reclaim_data` for nested values before closing the file. NULL standalone inputs/default fills read as empty strings; standalone string no-fill is rejected. New strings are UTF-8-tagged without transcoding, and legacy ASCII tags remain supported. Dynamic filled datasets allocate storage early to make read-only fills safe under HDF5 2.1.1, so sparse extensions may cost more storage. Wider fixed-length HDF5 strings, classic-model strings and reference nesting remain unsupported. See [NEXTCDF-4](nextcdf4.md) for layouts, ownership and selected test coverage.

#### Q: Can I use NEXTCDF-4 from Fortran?
**A:** Enable both NEXTCDF-4 and Fortran and use `netcdf` plus `nep_nextcdf4`.
NetCDF-Fortran >=4.6.2 and compatible compiler modules are required. The installed
`NEP::nepnextcdf4f` target provides initialization, mode/new-type constants and
scalar–rank-7 float16/bfloat16 bit-buffer I/O with one-based Fortran selections.
Standard real32/real64 NetCDF-Fortran I/O performs numeric half conversion; the new
wrappers exchange raw `integer(c_int16_t)` encodings. Reference wrappers and general
enhanced-type value interfaces are deferred beyond v4.3.0; constants alone do not
supply them. See [NEXTCDF-4 Fortran](nextcdf4_fortran.md) for linkage and an example.

#### Q: Can I use NEP on Windows?
**A:** Currently, NEP targets Linux and Unix platforms. Windows support may be added in future releases based on community demand.

### Development and Community

#### Q: Is NEP open source?
**A:** Yes! NEP is released under an open-source license. Contributions from the community are welcome.

#### Q: How can I contribute to NEP?
**A:** Contributions are welcome in several forms:
- Code contributions (bug fixes, new features, UDF handlers)
- Documentation improvements
- Testing and bug reports
- Performance benchmarking
- Community support

#### Q: Where can I get help?
**A:** Check the project documentation, GitHub issues, and community forums. File bug reports and feature requests through the GitHub issue tracker.

#### Q: Does NEP support Fortran applications?
**A:** Yes. NEP's format handlers work with NetCDF-Fortran applications. The old `module nep` compression wrappers moved to `module nep_compress`; install nep-compress for `nf90_def_var_lz4`, `nf90_inq_var_lz4`, `nf90_def_var_bzip2`, and `nf90_inq_var_bzip2`.

#### Q: What is the current version?
**A:** NEP v4.3.0 is in development (release tag pending), providing:
- Format readers for NASA CDF (UDF4), FITS (UDF3), PDS4 (UDF5), GeoTIFF (UDF0/UDF1), GRIB2 (UDF2), DICOM (UDF6), legacy PDB (UDF7) and PDBx/mmCIF (UDF8)
- The experimental NEXTCDF-4 backend (UDF9): full enhanced data model including strings and nested user types, 16-bit floats with NEP round-to-nearest-even conversion and typed `nc_*_float16`/`nc_*_bfloat16` APIs, `NC_STD_REF` reference types, generic `nc_def_var_filter` HDF5 filter support, provenance attributes, `nc_set_fill`/chunk-cache/group-rename support, `NC_NETCDF4_MODEL` and `NC_CLASSIC_MODEL` compatibility modes, and the optional `nep_nextcdf4` Fortran module
- CMake >= 3.19 with improved HDF5 detection; Spack and conda-forge packaging
- LZ4/BZIP2 compression via the separate nep-compress project
- Comprehensive documentation, CI testing (including sanitizer and coverage jobs), and example programs.

#### Q: What formats does NEP support?
**A:** NEP supports multiple scientific data formats through the NetCDF UDF system:
- **NetCDF-4/HDF5**: Native format; optional NEXTCDF-4 backend selected with `NC_NEXTCDF4` (LZ4/BZIP2 plugins come from nep-compress)
- **GeoTIFF** (v1.5.0, UDF0/UDF1): Geospatial raster imagery via UDF handler
- **GRIB2** (v1.7.0, UDF2): Meteorological and oceanographic NWP model output via UDF handler
- **FITS** (v2.0.0, UDF3): Astronomical images and tables from HST, JWST, Chandra via UDF handler
- **NASA CDF** (v1.3.0, UDF4): Space physics and satellite data via UDF handler
- **NASA/ESA PDS4** (v2.2.0, UDF5): Planetary science arrays and tables via UDF handler
- **DICOM** (v3.0.0, UDF6): Medical imaging via UDF handler
- **Legacy PDB** (v3.3.0, UDF7) and **PDBx/mmCIF** (v3.4.0, UDF8): Macromolecular structures via UDF handlers
- **NEXTCDF-4** (v4.0.0, UDF9): Optional NetCDF-4/HDF5 backend with new atomic types and enhanced metadata model
All formats accessible through the standard NetCDF API.

#### Q: How stable is NEP?
**A:** NEP is production-ready and thoroughly tested:
- **Format readers**: stable across v1.x–v4.x releases
- **NEXTCDF-4**: experimental in v4.3.0 — functional for the enhanced data model, but some upstream edge cases remain unimplemented and parallel/memory/SWMR modes are rejected
- **v4.2.0** moved LZ4/BZIP2 APIs and plugins to nep-compress; applications using those APIs must update their build configuration
- **Breaking change notes**: v2.2.0 changed GeoTIFF/GRIB2/FITS defaults from ON to OFF; v4.3.0 removed the deprecated `NC_REF_OBJECT`/`NC_REF_REGION` constants in favor of `NC_STD_REF`
- **Comprehensive testing**: Extensive test suites and CI validation (ci.yml, ci-fits.yml, ci-formats.yml, ci-parallel.yml, ci-parallel-mpich.yml, nextcdf4.yml with normal, sanitizer and coverage jobs)
- **Real-world data**: Tested with NASA IMAP MAG, MODIS imagery, NOAA GDAS GRIB2, HST WFPC2 FITS, PDS4 MRO CRISM data, and NEXTCDF-4 round-trip files

### Use Cases

#### Q: What are typical use cases for NEP?
**A:** 
- **Space physics research**: Direct access to NASA CDF satellite and mission data (IMAP, MMS, Van Allen Probes)
- **Geospatial analysis**: Read GeoTIFF satellite imagery, land cover, and digital elevation models
- **Remote sensing workflows**: Process MODIS, Landsat, and aerial photography in GeoTIFF format
- **Ocean modeling**: Direct access to NOAA GDAS wave forecast GRIB2 files (e.g., `gdaswave.t00z.wcoast.0p16.f000.grib2`)
- **Operational meteorology**: Read NWP model output (GFS, NAM, HRRR) in GRIB2 format through the NetCDF API
- **Astronomical data analysis**: Read HST, Chandra, and JWST FITS files with standard NetCDF API; no CFITSIO application code required
- **Planetary science**: Access NASA/ESA PDS4 planetary data archives (e.g., MRO CRISM, Cassini, New Horizons, Voyager) through the NetCDF API
- **Medical imaging**: Read DICOM CT/MR/XA/US objects, including multi-frame pixel data, through the NetCDF API
- **Structural biology**: Read PDB and mmCIF macromolecular structure files through the NetCDF API
- **NEXTCDF-4 workflows**: Write NetCDF-4 files with modern HDF5 Superblock v3, new small floating-point and complex types, `NC_STD_REF` references, and corrected dimension/variable renaming
- **Multi-format analysis**: Unified NetCDF API for NetCDF-4, NEXTCDF-4, CDF, GeoTIFF, GRIB2, FITS, PDS4, DICOM, PDB and mmCIF data

#### Q: Is NEP suitable for real-time or archival data storage?
**A:** For compression-heavy workflows, pair NEP with nep-compress — its LZ4 plugin suits real-time/low-latency processing and BZIP2 suits archival storage. NEP's own contribution is direct format access and the NEXTCDF-4 backend.

#### Q: How does NEP benefit HPC environments?
**A:** NEP supports scientific format access and parallel I/O test programs. For fast LZ4 compression use nep-compress alongside NEP.

---

## Contact and Resources

For more information:
- **Documentation**: See docs/ directory in the repository
- **Installation Guide**: README.md
- **Technical Details**: docs/design.md and docs/prd.md
- **Release Notes**: docs/releases/
- **Roadmap**: docs/roadmap.md

---

## Release History

- **v0.1.3** (Nov 2025): Architecture shift from HDF5 VOL to NetCDF UDF, Doxygen documentation
- **v1.0.0**: LZ4 and BZIP2 compression filters
- **v1.1.0**: Fortran wrappers for compression functions
- **v1.2.0**: Documentation improvements and GitHub Pages deployment
- **v1.3.0**: NASA CDF format support via UDF handler (UDF slot 2, later moved to UDF4)
- **v1.4.0**: Spack package manager support for NEP and CDF
- **v1.5.0** (Jan 2026): GeoTIFF read support via UDF handler (UDF0/UDF1)
- **v1.7.0** (Mar 2026): GRIB2 read support via UDF handler (UDF2)
- **v1.8.0** (May 2026): Simplified example validation patterns; example programming rules documented
- **v1.9.0** (Jun 2026): Parallel I/O examples (C and Fortran) and CI pipeline with OpenMPI
- **v1.10.0** (Jun 2026): Performance benchmark examples (deflate, zstandard, szip, lz4, bzip2, lossless comparison)
- **v1.11.0** (Jun 2026): NcZarr examples (C and Fortran) covering basic I/O, chunking, compression, enhanced data model
- **v2.0.0** (Jun 2026): FITS read support via UDF handler (UDF3); CFITSIO integration; primary and extension HDU metadata and data I/O
- **v2.1.0** (Jul 2026): Example programs updated as companion code for *The NetCDF Developer's Handbook, Second Edition*; Zstandard compression examples added
- **v2.2.0** (Jul 2026): NASA/ESA PDS4 read support via UDF handler (UDF5); CDF moved to UDF4; all five readers can be enabled simultaneously; GeoTIFF/GRIB2/FITS default to OFF
- **v2.3.0** (Jul 2026): PDS4 reader documentation and Doxygen integration; Cassini, MESSENGER, and LCS-9P mission test data; minimum NetCDF-C raised to 4.10.1
- **v2.4.0** (Jul 2026): Spack release packaging for v2.3.0; all project-specific CMake options renamed with `NEP_` prefix; netcdf-c dependency aligned to >= 4.10.1 in Spack
- **v2.5.0** (Jul 2026): Complete Spack variant coverage for all optional format readers (GeoTIFF, GRIB2, CDF, FITS, PDS4, parallel, examples, benchmarks); all-variants CI integration testing; comprehensive README Spack installation section
- **v2.6.0** (Jul 2026): *In preparation* — further PDS4 reader testing with MAVEN NGIMS delimited tables, MAVEN IUVS `Group_Field_Binary` repeated fields (depth-2 nesting), Perseverance Mastcam-Z `Array_3D_Image` products (Sol 1738 and Sol 1737), and `scaling_factor`/`value_offset` attribute preservation
- **v2.7.1** (Jul 2026): Documentation cleanup — per-format reference pages extracted from `docs/formats.md`, README PDS4 Tests section moved to `docs/pds4.md`, and design/prd/prfaq updated with New Horizons and v2.7.1 metadata
- **v2.8.0** (Jul 2026): Optional Python visualization examples with validated grayscale PNG and metadata outputs
- **v3.0.0** (Jul 2026): DICOM UDF reader for uncompressed, JPEG Baseline, and multi-frame imaging objects
- **v4.0.0** (Aug 2026): NEXTCDF-4 optional NetCDF-4/HDF5 backend (UDF9) with full enhanced data model, new atomic types (float16, bfloat16, FP8/FP6/FP4, complex, bitfield, reference), Superblock v3 default, and `NC_NETCDF4_MODEL` compatibility mode
- **v4.1.0**: NEXTCDF-4 documentation suite and NEP logging (`ELOG`/`nep_set_log_level`); standalone satellite-mission examples moved to Earth_Observation_in_Practice
- **v4.2.0**: LZ4/BZIP2 compression APIs, plugins, tests and benchmarks moved to the nep-compress project; NEP is format-readers + NEXTCDF-4 only; first conda-forge package
- **v4.3.0** (in development): NEXTCDF-4 maturation — strings and full user-type nesting, 16-bit float typed I/O and conversion engine, `NC_STD_REF` references, generic HDF5 filters, provenance attributes, fill/cache/group-rename, Fortran binding; CMake >= 3.19 with improved HDF5 detection; GeoTIFF in the conda package

---

*Last Updated: November 2026 (v4.3.0)*

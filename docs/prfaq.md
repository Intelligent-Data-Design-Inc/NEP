# NEP (NetCDF Expansion Pack) - Press Release / FAQ

## Press Release

**FOR IMMEDIATE RELEASE**

### NetCDF Expansion Pack (NEP) v2.2.0 Adds NASA/ESA PDS4 Planetary Data Support

*Open-source framework now provides high-performance compression (LZ4/BZIP2) and transparent access to six scientific data formats — GeoTIFF, GRIB2, FITS, NASA CDF, and NASA/ESA PDS4 — through the standard NetCDF API*

**July 2026** - The scientific computing community gains powerful new capabilities with NEP (NetCDF Expansion Pack) v2.2.0, an open-source framework that extends NetCDF-4 with high-performance compression and seamless access to multiple scientific data formats — now including NASA/ESA Planetary Data System version 4 (PDS4) planetary science data.

#### The Challenge

Scientific researchers work with diverse data formats — NetCDF-4, NASA CDF space physics data, GeoTIFF geospatial imagery, GRIB2 meteorological model output, FITS astronomical data, and NASA/ESA PDS4 planetary science archives — often requiring format conversion before analysis. Large-scale data volumes demand efficient compression balancing storage savings with processing speed. Traditional workflows force scientists to convert data between formats, adding complexity and storage overhead.

#### The Solution

NEP provides a comprehensive solution with two key capabilities:

**High-Performance Compression (v1.0.0, v1.1.0):**
- **LZ4**: 2-3x faster compression than DEFLATE for real-time workflows and HPC environments
- **BZIP2**: Superior compression ratios for archival storage and bandwidth-constrained scenarios
- **C and Fortran APIs**: Full support for both languages with `nc_*` and `nf90_*` functions

**Multi-Format UDF Handlers:**

*GeoTIFF Support (v1.5.0, UDF0/UDF1):*
- **Geospatial Data Access**: Read GeoTIFF raster data through standard NetCDF API
- **Format Detection**: Automatic TIFF/GeoTIFF identification and validation
- **Metadata Preservation**: Georeferencing, coordinate systems, and GeoTIFF tags accessible as NetCDF attributes
- **Multi-Band Support**: Handle single and multi-band imagery with proper dimension mapping

*GRIB2 Support (v1.7.0, UDF2):*
- **Meteorological/Oceanographic Data Access**: Read GRIB2 NWP model output through the standard NetCDF API
- **Full Grid Expansion**: Bitmap-aware data reading substitutes `_FillValue` for land-masked points
- **Product Inventory**: All GRIB2 products exposed as named NC_FLOAT variables with `[y, x]` dimensions
- **`.ncrc` Autoload**: Zero-code-change access — place `.ncrc` in home directory and `nc_open()` works on `.grib2` files

*FITS Support (v2.0.0, UDF3):*
- **Astronomical Data Access**: Read FITS files from HST, Chandra, JWST, and other instruments through the standard NetCDF API
- **HDU Mapping**: Primary HDU in root group; each extension HDU becomes a named child group
- **Full Data I/O**: `NC_FITS_get_vara()` reads pixel hyperslabs and table column data via CFITSIO
- **Keyword Mapping**: Standard FITS keywords (`BUNIT`, `BZERO`, `BSCALE`, `BLANK`) mapped to NetCDF attributes

*NASA CDF Support (v1.3.0, UDF4):*
- **Transparent Access**: Read CDF space physics and satellite data through standard NetCDF API
- **No Conversion Required**: Direct access to CDF files without format conversion
- **Complete Metadata**: Full mapping of CDF variables, attributes, and types to NetCDF equivalents

*NASA/ESA PDS4 Support (v2.2.0, UDF5):*
- **Planetary Science Data Access**: Read PDS4 XML-labeled data products through the standard NetCDF API
- **Array and Table Support**: Array images, binary tables, character tables, and delimited tables fully supported
- **Metadata Mapping**: `Identification_Area` and `Observation_Area` become global attributes; each `File_Area_Observational` becomes a child group
- **Full Data I/O**: `NC_PDS4_get_vara()` reads array hyperslabs and table field values with automatic byte-order conversion

**Example Programs (v2.1.0+):**
- **Learning Resources**: Comprehensive example programs in C and Fortran demonstrating NetCDF API usage
- **Companion Code**: Examples accompany *The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition* (https://www.amazon.com/dp/B0H7Q1Z75L)
- **Multiple Categories**: Classic NetCDF, NetCDF-4, NcZarr, OPeNDAP, parallel I/O, and performance examples

This comprehensive framework enables scientists to:

- **Eliminate format conversion**: Access CDF, GeoTIFF, GRIB2, FITS, and PDS4 files directly through the NetCDF API
- **Optimize compression**: Choose LZ4 for speed or BZIP2 for compression ratio
- **Process data faster**: LZ4 delivers 2-3x faster compression/decompression than DEFLATE
- **Simplify workflows**: Use familiar NetCDF API for six scientific data formats
- **Maintain data integrity**: Lossless compression and accurate metadata mapping
- **Deploy easily**: Spack package manager support for HPC environments

"NEP has grown into a truly multi-format scientific data framework," said Ed Hartnett, Principal Architect. "Version 2.2.0 completes our planetary science support with full PDS4 reading — array images, binary tables, and character tables are now directly accessible through the same NetCDF API scientists already use for CDF, GeoTIFF, GRIB2, and FITS data. Six formats, one API, no conversion."


#### Availability

NEP v2.2.0 is available now as open-source software. Installation options include:
- **Source Build**: CMake or Autotools build systems
- **Spack Package Manager**: `spack install nep` for HPC environments
- **Documentation**: Complete API documentation at https://intelligent-data-design-inc.github.io/NEP/

---

## Frequently Asked Questions

### General Questions

#### Q: What is NEP?
**A:** NEP (NetCDF Expansion Pack) is an open-source framework that extends NetCDF-4 with:
- High-performance compression (LZ4 and BZIP2) for HDF5/NetCDF-4 files
- User Defined Format (UDF) handlers for GeoTIFF, GRIB2, FITS, NASA CDF, and NASA/ESA PDS4 files
- C and Fortran APIs for all features
- Spack package manager support for simplified HPC deployment
- Comprehensive example programs as companion code to *The NetCDF Developer's Handbook, Second Edition*

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
1. **Limited compression options**: NetCDF-4 traditionally relies on DEFLATE, which is slow for large datasets
2. **Format fragmentation**: Scientists must convert between NetCDF, CDF, GeoTIFF, GRIB2, FITS, and PDS4 formats
3. **Speed vs ratio trade-offs**: No easy way to choose between fast compression and high compression ratios
4. **I/O bottlenecks**: Compression overhead can limit performance in HPC environments
5. **Complex workflows**: Multiple tools required for different scientific data formats

### Technical Questions

#### Q: What compression algorithms does NEP support?
**A:** NEP supports two complementary lossless compression algorithms:

**LZ4 Compression:**
- **Purpose**: Speed-optimized for real-time and HPC workflows
- **Speed**: 2-3x faster compression/decompression than DEFLATE
- **Ratio**: Good compression ratios, slightly lower than DEFLATE
- **Use Case**: Real-time data processing, HPC I/O optimization, speed-critical workflows

**BZIP2 Compression:**
- **Purpose**: Compression ratio-optimized for archival storage
- **Ratio**: Better compression ratios than DEFLATE
- **Speed**: Slower than LZ4 but faster than most high-ratio algorithms
- **Use Case**: Long-term archival, bandwidth-constrained transfers, storage optimization

#### Q: When should I use LZ4 vs BZIP2?
**A:**
- **Use LZ4 when**: I/O speed is critical, working with real-time data, running on HPC systems, need fast decompression
- **Use BZIP2 when**: Storage space is limited, archiving data long-term, transferring over slow networks, compression ratio matters most
- **Both are lossless**: Choose based on your workflow priorities—speed or compression ratio

#### Q: Do I need to modify my existing NetCDF code?
**A:** No! NEP features work transparently with existing NetCDF applications:
- **Compression**: Set HDF5_PLUGIN_PATH and use standard NetCDF compression APIs
- **CDF files**: Use `nc_open()` to read CDF files directly
- **GeoTIFF files**: Use `nc_open()` to read GeoTIFF files directly
- **GRIB2 files**: Use `nc_open()` to read GRIB2 files directly
- **FITS files**: Call `NC_FITS_initialize()` (or use `.ncrc` autoload), then `nc_open()` reads FITS files
- **PDS4 files**: Call `NC_PDS4_initialize()` (or use `.ncrc` autoload), then `nc_open()` reads PDS4 XML labels
All features integrate seamlessly with the standard NetCDF API.

#### Q: Does NEP provide example programs to help me learn?
**A:** Yes! NEP includes comprehensive example programs in both C and Fortran, updated for v2.1.0 as companion code for *The NetCDF Developer's Handbook: The Authoritative Guide to Writing High-Performance Programs for Scientific Data Management, Second Edition*:
- **Classic NetCDF examples**: Basic 2D arrays, coordinate variables, format variants, size limits, unlimited dimensions, 4D variables
- **NetCDF-4 examples**: NetCDF-4 files, compression, chunking, multiple unlimited dimensions, user-defined types
- **NcZarr examples**: Local NcZarr stores, chunking, compression, enhanced data model with hierarchical groups
- **OPeNDAP examples**: Remote data access with constraint expressions and timing instrumentation
- **Parallel I/O examples**: MPI collective I/O with domain decomposition (C and Fortran)
- **Performance examples**: Benchmarking compression algorithms (gated by `NEP_ENABLE_BENCHMARKS`)
- **Dual language support**: All examples available in both C and Fortran
- **Built and tested**: Examples build with NEP and run as tests
- **Location**: All examples in the `examples/` directory with detailed README

### Installation and Usage

#### Q: What are the system requirements?
**A:** 
- **Operating System**: Linux or Unix
- **Core Libraries**: NetCDF-C (v4.9+), HDF5 (v1.12+)
- **Compression (optional)**: LZ4, BZIP2
- **Fortran Support (optional)**: NetCDF-Fortran (v4.6.2+; zstd API requires 4.6.0+)
- **CDF Support (optional)**: NASA CDF Library (v3.9.x)
- **GeoTIFF Support (optional)**: libgeotiff, libtiff
- **GRIB2 Support (optional)**: NOAA NCEPLIBS-g2c (≥ 2.1.0), libjasper (≥ 3.0.0)
- **FITS Support (optional)**: CFITSIO (≥ 3.0; locally `/usr/local/cfitsio-4.6.4/`; CI: apt `libcfitsio-dev`)
- **PDS4 Support (optional)**: libxml2 (`libxml2-dev` on Ubuntu)
- **Build Tools**: CMake (v3.9+) or Autotools
- **Documentation (optional)**: Doxygen and Graphviz

#### Q: How do I install NEP?
**A:** Multiple installation methods:
- **Spack** (recommended for HPC): `spack install nep`
- **CMake**: `cmake -B build && cmake --build build && cmake --install build`
- **Autotools**: `./configure && make && make install`
See the README for detailed instructions.

#### Q: Can I enable only specific features?
**A:** Yes! Use build options to control which features are compiled. All extended format readers default to **OFF** (v2.2.0+); enable explicitly as needed:
- **CMake**: `-DNEP_BUILD_LZ4=ON/OFF`, `-DNEP_BUILD_BZIP2=ON/OFF`, `-DNEP_ENABLE_FORTRAN=ON/OFF`, `-DNEP_ENABLE_CDF=ON/OFF`, `-DNEP_ENABLE_GEOTIFF=ON/OFF`, `-DNEP_ENABLE_GRIB2=ON/OFF`, `-DNEP_ENABLE_FITS=ON/OFF`, `-DNEP_ENABLE_PDS4=ON/OFF`, `-DNEP_BUILD_EXAMPLES=ON/OFF`
- **Autotools**: `--enable-lz4`, `--enable-bzip2`, `--enable-fortran`, `--enable-cdf`, `--enable-geotiff`, `--enable-grib2`, `--enable-fits`, `--enable-pds4`, `--disable-examples`
- All five format readers (CDF, GeoTIFF, GRIB2, FITS, PDS4) can be enabled simultaneously — there are no mutual-exclusivity restrictions as of v2.2.0
- **Spack**: `spack install nep+lz4+bzip2+fortran` (variants control features)

#### Q: How do I use LZ4 compression in my NetCDF files?
**A:** LZ4 compression is available as an HDF5 filter. Once NEP is installed, set HDF5_PLUGIN_PATH and use the NEP compression API:
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
**A:** Yes! NEP maintains 100% API compatibility with NetCDF-C. Existing applications work without modification.

#### Q: What is the performance overhead?
**A:** 
- **LZ4 Compression**: 2-3x faster than DEFLATE—actually improves I/O performance
- **BZIP2 Compression**: Slower than LZ4 but provides better compression ratios
- **Overall**: Performance improvements from reduced I/O time typically outweigh any overhead

#### Q: Does NEP support parallel I/O?
**A:** Yes, NEP is designed to work in HPC environments with parallel I/O operations. LZ4 compression is particularly well-suited for parallel workflows.

#### Q: Can I use NEP on Windows?
**A:** Currently, NEP targets Linux and Unix platforms. Windows support may be added in future releases based on community demand.

### Compression Algorithms

#### Q: What compression algorithms does NEP v1.1.0 support?
**A:** NEP v1.1.0 supports two lossless compression algorithms:
- **LZ4**: High-speed compression for real-time and HPC workflows
- **BZIP2**: High-ratio compression for archival storage and bandwidth-constrained scenarios

#### Q: Can I use both LZ4 and BZIP2 in the same application?
**A:** Yes! You can choose different compression algorithms for different variables or files based on your specific requirements.

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
**A:** Yes. NEP v1.1.0 added Fortran 90 wrappers (module `nep`) for compression functions. Fortran applications can call `nf90_def_var_lz4`, `nf90_inq_var_lz4`, `nf90_def_var_bzip2`, and `nf90_inq_var_bzip2` to enable and query compression.

#### Q: What is the current version?
**A:** NEP v2.6.0 is the current release (July 2026), providing:
- LZ4 and BZIP2 compression for HDF5/NetCDF-4 files (C and Fortran APIs)
- GeoTIFF geospatial raster support via UDF handler (UDF0/UDF1)
- GRIB2 meteorological/oceanographic data support via UDF handler (UDF2)
- FITS astronomical data support via UDF handler (UDF3)
- NASA CDF space physics data support via UDF handler (UDF4)
- NASA/ESA PDS4 planetary science data support via UDF handler (UDF5), including New Horizons Alice mission data
- All five format readers can be enabled simultaneously
- Complete Spack variants for every optional reader and utility
- Comprehensive documentation, CI testing, and example programs

NEP v2.7.1 is in preparation and focuses on documentation cleanup and reorganization.

#### Q: What formats does NEP support?
**A:** NEP supports multiple scientific data formats through the NetCDF UDF system:
- **NetCDF-4/HDF5**: Native format with LZ4/BZIP2 compression
- **GeoTIFF** (v1.5.0, UDF0/UDF1): Geospatial raster imagery via UDF handler
- **GRIB2** (v1.7.0, UDF2): Meteorological and oceanographic NWP model output via UDF handler
- **FITS** (v2.0.0, UDF3): Astronomical images and tables from HST, JWST, Chandra via UDF handler
- **NASA CDF** (v1.3.0, UDF4): Space physics and satellite data via UDF handler
- **NASA/ESA PDS4** (v2.2.0, UDF5): Planetary science arrays and tables via UDF handler
All formats accessible through the standard NetCDF API.

#### Q: How stable is NEP?
**A:** NEP is production-ready and thoroughly tested:
- **v1.0.0-v2.5.0**: All releases maintain backward compatibility
- **v2.6.0** is in preparation; no breaking changes are planned
- **Breaking change note**: v2.2.0 changed GeoTIFF, GRIB2, and FITS defaults from ON to OFF; explicit enable flags required for these formats
- **Comprehensive testing**: Extensive test suites and CI validation (ci.yml, ci-fits.yml, ci-formats.yml, ci-parallel.yml)
- **Real-world data**: Tested with NASA IMAP MAG, MODIS imagery, NOAA GDAS GRIB2, HST WFPC2 FITS, and PDS4 MRO CRISM data

### Use Cases

#### Q: What are typical use cases for NEP?
**A:** 
- **Weather forecasting**: Fast compression of large meteorological datasets with LZ4; direct GRIB2 NWP output access
- **Climate modeling**: Efficient storage and access to simulation outputs with BZIP2
- **Space physics research**: Direct access to NASA CDF satellite and mission data (IMAP, MMS, Van Allen Probes)
- **Geospatial analysis**: Read GeoTIFF satellite imagery, land cover, and digital elevation models
- **Remote sensing workflows**: Process MODIS, Landsat, and aerial photography in GeoTIFF format
- **Ocean modeling**: Direct access to NOAA GDAS wave forecast GRIB2 files (e.g., `gdaswave.t00z.wcoast.0p16.f000.grib2`)
- **Operational meteorology**: Read NWP model output (GFS, NAM, HRRR) in GRIB2 format through the NetCDF API
- **Astronomical data analysis**: Read HST, Chandra, and JWST FITS files with standard NetCDF API; no CFITSIO application code required
- **Planetary science**: Access NASA/ESA PDS4 planetary data archives (e.g., MRO CRISM, Cassini, New Horizons, Voyager) through the NetCDF API
- **HPC workflows**: Improve I/O performance with fast LZ4 compression
- **Data archival**: Maximize storage efficiency with BZIP2 compression
- **Multi-format analysis**: Unified NetCDF API for NetCDF-4, CDF, GeoTIFF, GRIB2, FITS, and PDS4 data

#### Q: Can NEP handle real-time data streams?
**A:** Yes! LZ4's high-speed compression makes it ideal for real-time or near-real-time data processing where low latency is critical.

#### Q: Is NEP suitable for archival storage?
**A:** Yes! BZIP2 provides excellent compression ratios ideal for archival storage. LZ4 provides good compression ratios suitable for many scenarios. NEP allows you to choose the right algorithm based on your priorities—speed or compression ratio.

#### Q: How does NEP benefit HPC environments?
**A:** NEP reduces I/O bottlenecks through fast compression, supports parallel I/O operations, and is designed for exascale computing environments. The performance improvements can significantly reduce job completion times.

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

---

*Last Updated: July 2026 (v2.6.0, v2.7.1 release preparation)*

# NEP (NetCDF Extension Pack) - Press Release / FAQ

## Press Release

**FOR IMMEDIATE RELEASE**

### NetCDF Extension Pack (NEP) v1.5.0 Adds GeoTIFF Support for Geospatial Data Access

*Open-source framework now provides high-performance compression (LZ4/BZIP2), NASA CDF support, and transparent GeoTIFF file access through NetCDF API*

**January 2026** - The scientific computing community gains powerful new capabilities with NEP (NetCDF Extension Pack) v1.5.0, an open-source framework that extends NetCDF-4 with high-performance compression and seamless access to multiple scientific data formats including GeoTIFF geospatial raster data.

#### The Challenge

Scientific researchers work with diverse data formats—NetCDF-4, NASA CDF space physics data, and GeoTIFF geospatial imagery—often requiring format conversion before analysis. Large-scale data volumes demand efficient compression balancing storage savings with processing speed. Traditional workflows force scientists to convert data between formats, adding complexity and storage overhead.

#### The Solution

NEP provides a comprehensive solution with three key capabilities:

**High-Performance Compression (v1.0.0, v1.1.0):**
- **LZ4**: 2-3x faster compression than DEFLATE for real-time workflows and HPC environments
- **BZIP2**: Superior compression ratios for archival storage and bandwidth-constrained scenarios
- **C and Fortran APIs**: Full support for both languages with `nc_*` and `nf90_*` functions

**NASA CDF Support (v1.3.0):**
- **Transparent Access**: Read CDF space physics and satellite data through standard NetCDF API
- **No Conversion Required**: Direct access to CDF files without format conversion
- **Complete Metadata**: Full mapping of CDF variables, attributes, and types to NetCDF equivalents

**GeoTIFF Support (v1.5.0):**
- **Geospatial Data Access**: Read GeoTIFF raster data through standard NetCDF API
- **Format Detection**: Automatic TIFF/GeoTIFF identification and validation
- **Metadata Preservation**: Georeferencing, coordinate systems, and GeoTIFF tags accessible as NetCDF attributes
- **Multi-Band Support**: Handle single and multi-band imagery with proper dimension mapping

This comprehensive framework enables scientists to:

- **Eliminate format conversion**: Access CDF and GeoTIFF files directly through NetCDF API
- **Optimize compression**: Choose LZ4 for speed or BZIP2 for compression ratio
- **Process data faster**: LZ4 delivers 2-3x faster compression/decompression than DEFLATE
- **Simplify workflows**: Use familiar NetCDF API for multiple scientific data formats
- **Maintain data integrity**: Lossless compression and accurate metadata mapping
- **Deploy easily**: Spack package manager support for HPC environments

"NEP transforms how scientists handle diverse scientific data," said Ed Hartnett, Principal Architect. "Version 1.5.0 brings together high-performance compression with transparent access to NASA CDF and GeoTIFF formats. Researchers can now read space physics data and geospatial imagery using the same NetCDF API they already know, eliminating the need for format conversion and streamlining their analysis workflows."


#### Availability

NEP v1.5.0 is available now as open-source software. Installation options include:
- **Source Build**: CMake or Autotools build systems
- **Spack Package Manager**: `spack install nep` for HPC environments
- **Documentation**: Complete API documentation at https://intelligent-data-design-inc.github.io/NEP/

---

## Frequently Asked Questions

### General Questions

#### Q: What is NEP?
**A:** NEP (NetCDF Extension Pack) is an open-source framework that extends NetCDF-4 with:
- High-performance compression (LZ4 and BZIP2) for HDF5/NetCDF-4 files
- User Defined Format (UDF) handlers for NASA CDF and GeoTIFF files
- C and Fortran APIs for all features
- Spack package manager support for simplified HPC deployment

#### Q: Who should use NEP?
**A:** NEP is designed for:
- Scientific researchers working with NetCDF-4/HDF5, NASA CDF, or GeoTIFF datasets
- Space physics researchers analyzing CDF satellite and mission data
- Geospatial analysts working with GeoTIFF satellite imagery and raster data
- Data center operators optimizing storage efficiency
- HPC system administrators optimizing I/O performance
- Software developers building scientific applications
- Anyone needing flexible compression or multi-format access for scientific data

#### Q: What problem does NEP solve?
**A:** NEP addresses key challenges in scientific data management:
1. **Limited compression options**: NetCDF-4 traditionally relies on DEFLATE, which is slow for large datasets
2. **Format fragmentation**: Scientists must convert between NetCDF, CDF, and GeoTIFF formats
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
All features integrate seamlessly with the standard NetCDF API.

### Installation and Usage

#### Q: What are the system requirements?
**A:** 
- **Operating System**: Linux or Unix
- **Core Libraries**: NetCDF-C (v4.9+), HDF5 (v1.12+)
- **Compression (optional)**: LZ4, BZIP2
- **Fortran Support (optional)**: NetCDF-Fortran (v4.5.4+)
- **CDF Support (optional)**: NASA CDF Library (v3.9.x)
- **GeoTIFF Support (optional)**: libgeotiff, libtiff
- **Build Tools**: CMake (v3.9+) or Autotools
- **Documentation (optional)**: Doxygen and Graphviz

#### Q: How do I install NEP?
**A:** Multiple installation methods:
- **Spack** (recommended for HPC): `spack install nep`
- **CMake**: `cmake -B build && cmake --build build && cmake --install build`
- **Autotools**: `./configure && make && make install`
See the README for detailed instructions.

#### Q: Can I enable only specific features?
**A:** Yes! Use build options to control which features are compiled:
- **CMake**: `-DBUILD_LZ4=ON/OFF`, `-DBUILD_BZIP2=ON/OFF`, `-DENABLE_FORTRAN=ON/OFF`, `-DENABLE_CDF=ON/OFF`, `-DENABLE_GEOTIFF=ON/OFF`
- **Autotools**: `--enable-lz4`, `--enable-bzip2`, `--enable-fortran`, `--enable-cdf`, `--enable-geotiff`
- **Spack**: `spack install nep+lz4+bzip2+fortran` (variants control features)

#### Q: How do I use LZ4 compression in my NetCDF files?
**A:** LZ4 compression is available as an HDF5 filter. Once NEP is installed, set HDF5_PLUGIN_PATH and use the NEP compression API:
```c
nc_def_var_lz4(ncid, varid, level);  // C API
nf90_def_var_lz4(ncid, varid, level) ! Fortran API
```

#### Q: How do I read CDF files with NEP?
**A:** Enable CDF support during build (`-DENABLE_CDF=ON`), then use standard NetCDF API:
```c
nc_open("data.cdf", NC_NOWRITE, &ncid);
nc_inq(ncid, &ndims, &nvars, &natts, &unlimdimid);
nc_get_var_double(ncid, varid, data);
```

#### Q: How do I read GeoTIFF files with NEP?
**A:** Enable GeoTIFF support during build (`-DENABLE_GEOTIFF=ON`), then use standard NetCDF API:
```c
nc_open("image.tif", NC_NOWRITE, &ncid);
nc_inq_dimlen(ncid, 0, &bands);
nc_get_var_float(ncid, varid, raster_data);
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
**A:** Yes. NEP v1.1.0 added Fortran 90 wrappers (module `ncsqueeze`) for compression functions. Fortran applications can call `nf90_def_var_lz4`, `nf90_inq_var_lz4`, `nf90_def_var_bzip2`, and `nf90_inq_var_bzip2` to enable and query compression.

#### Q: What is the current version?
**A:** NEP v1.5.0 is the current release (January 2026), providing:
- LZ4 and BZIP2 compression for HDF5/NetCDF-4 files (C and Fortran APIs)
- NASA CDF format support via UDF handler
- GeoTIFF format support via UDF handler
- Spack package manager support
- Comprehensive documentation and CI testing

#### Q: What formats does NEP support?
**A:** NEP supports multiple scientific data formats:
- **NetCDF-4/HDF5**: Native format with LZ4/BZIP2 compression
- **NASA CDF** (v1.3.0): Space physics and satellite data via UDF handler
- **GeoTIFF** (v1.5.0): Geospatial raster imagery via UDF handler
All formats accessible through standard NetCDF API.

#### Q: How stable is NEP?
**A:** NEP is production-ready and thoroughly tested:
- **v1.0.0-v1.5.0**: All releases maintain backward compatibility
- **Zero breaking changes**: Stable API across all versions
- **Comprehensive testing**: Extensive test suites and CI validation
- **Real-world data**: Tested with NASA IMAP MAG, MODIS imagery, and production datasets

### Use Cases

#### Q: What are typical use cases for NEP?
**A:** 
- **Weather forecasting**: Fast compression of large meteorological datasets with LZ4
- **Climate modeling**: Efficient storage and access to simulation outputs with BZIP2
- **Space physics research**: Direct access to NASA CDF satellite and mission data (IMAP, MMS, Van Allen Probes)
- **Geospatial analysis**: Read GeoTIFF satellite imagery, land cover, and digital elevation models
- **Remote sensing workflows**: Process MODIS, Landsat, and aerial photography in GeoTIFF format
- **Satellite data processing**: Optimize storage for large observation datasets
- **Ocean modeling**: Reduce I/O bottlenecks in simulation workflows
- **HPC workflows**: Improve I/O performance with fast LZ4 compression
- **Data archival**: Maximize storage efficiency with BZIP2 compression
- **Multi-format analysis**: Unified NetCDF API for NetCDF-4, CDF, and GeoTIFF data

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
- **v1.3.0**: NASA CDF format support via UDF handler
- **v1.4.0**: Spack package manager support for NEP and CDF
- **v1.5.0** (Jan 2026): GeoTIFF read support via UDF handler

---

*Last Updated: January 2026 (v1.5.0)*

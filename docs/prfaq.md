# NEP (NetCDF Extension Pack) - Press Release / FAQ

## Press Release

**FOR IMMEDIATE RELEASE**

### NetCDF Extension Pack (NEP) v1.1.0 Brings High-Performance Compression to Fortran Applications

*Open-source framework provides flexible compression options (LZ4 and BZIP2) for HDF5/NetCDF-4 files, now with Fortran 90 wrappers*

**November 2025** - The scientific computing community today gains access to NEP (NetCDF Extension Pack) v1.1.0, a powerful open-source framework that extends high-performance compression options for HDF5/NetCDF-4 files to both C and Fortran applications.

#### The Challenge

Large-scale scientific data producers face mounting challenges with data storage and I/O performance. Massive data volumes require efficient compression that balances storage savings with processing speed. Traditional DEFLATE compression is often too slow for real-time workflows, while offering limited flexibility in choosing optimal compression strategies.

#### The Solution

NEP introduces high-performance compression for HDF5/NetCDF-4 files with two complementary algorithms, and with v1.1.0 adds first-class Fortran support through thin wrappers around the existing C APIs:

**LZ4**: Blazing-fast compression delivering 2-3x faster speeds than DEFLATE, ideal for real-time workflows and HPC environments.

**BZIP2**: Superior compression ratios exceeding DEFLATE, perfect for archival storage and bandwidth-constrained scenarios.

This dual-algorithm approach, now available from both C and Fortran, enables scientists to:

- **Choose the right tool**: Select LZ4 for speed or BZIP2 for compression ratio based on workflow needs
- **Process data faster**: LZ4 compression/decompression is 2-3x faster than DEFLATE
- **Maximize storage efficiency**: BZIP2 achieves better compression ratios than DEFLATE
- **Reduce I/O bottlenecks**: Speed-optimized LZ4 ideal for HPC environments
- **Optimize archival storage**: BZIP2 ideal for long-term data retention
- **Maintain data integrity**: Both algorithms provide lossless compression
- **Work seamlessly**: Transparent integration with existing NetCDF-4 applications

"NEP transforms how scientists handle large-scale data," said Ed Hartnett, Principal Architect. "With both LZ4 and BZIP2 compression, researchers can choose the optimal algorithm for their workflow—LZ4 for speed-critical operations or BZIP2 for storage optimization. With v1.1.0, those same capabilities are now directly available from Fortran codes via simple `nf90_*` functions. This flexibility enables scientists to optimize their data workflows based on specific requirements, regardless of whether they use C or Fortran."


#### Availability

NEP v1.1.0 is available now as open-source software. Visit the project repository for documentation and installation instructions.

---

## Frequently Asked Questions

### General Questions

#### Q: What is NEP?
**A:** NEP (NetCDF Extension Pack) is an open-source framework that provides high-performance compression (LZ4 and BZIP2) for HDF5/NetCDF-4 files through HDF5 filter plugins, with both C and Fortran APIs.

#### Q: Who should use NEP?
**A:** NEP is designed for:
- Scientific researchers working with large NetCDF-4/HDF5 datasets
- Data center operators optimizing storage efficiency
- HPC system administrators optimizing I/O performance
- Software developers building scientific applications
- Anyone needing flexible compression options for NetCDF-4 files

#### Q: What problem does NEP solve?
**A:** NEP addresses key compression challenges:
1. **Limited compression options**: NetCDF-4 traditionally relies on DEFLATE, which is slow for large datasets
2. **Speed vs ratio trade-offs**: No easy way to choose between fast compression and high compression ratios
3. **I/O bottlenecks**: Compression overhead can limit performance in HPC environments

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
**A:** No! LZ4 and BZIP2 compression work transparently with existing NetCDF-4 applications. Simply set the HDF5_PLUGIN_PATH environment variable and use standard NetCDF-4 compression APIs.

### Installation and Usage

#### Q: What are the system requirements?
**A:** 
- **Operating System**: Linux or Unix
- **Required Libraries (C)**: NetCDF-C (v4.9+), HDF5 (v1.12+), LZ4, BZIP2
- **Additional Fortran Support**: NetCDF-Fortran (v4.5.4+) when building the Fortran wrappers
- **Build Tools**: CMake (v3.9+) or Autotools
- **Optional**: Doxygen (for building documentation)

#### Q: How do I install NEP?
**A:** Installation is straightforward with either CMake or Autotools. See the README for detailed instructions.

#### Q: Can I enable only specific features?
**A:** Yes! Use build options to control which compression algorithms are compiled. Both CMake and Autotools support enable/disable flags for LZ4 and BZIP2.

#### Q: How do I use LZ4 compression in my NetCDF files?
**A:** LZ4 compression is available as an HDF5 filter. Once NEP is installed, the LZ4 filter is automatically available to NetCDF-4 applications through the HDF5_PLUGIN_PATH mechanism.

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
**A:** Yes. NEP v1.1.0 adds Fortran 90 wrappers (module `ncsqueeze`) around the existing C compression APIs. Fortran applications can call `nf90_def_var_lz4`, `nf90_inq_var_lz4`, `nf90_def_var_bzip2`, and `nf90_inq_var_bzip2` to enable and query compression, using the standard NetCDF Fortran API alongside NEP.

#### Q: What is the current version?
**A:** NEP v1.1.0 is the current release, providing LZ4 and BZIP2 compression support for HDF5/NetCDF-4 files for both C and Fortran applications.

#### Q: How stable is NEP?
**A:** NEP v1.0.0 is production-ready and has undergone thorough testing. Both LZ4 and BZIP2 compression filters are stable and ready for use in production environments.

### Use Cases

#### Q: What are typical use cases for NEP?
**A:** 
- **Weather forecasting**: Fast compression of large meteorological datasets
- **Climate modeling**: Efficient storage and access to simulation outputs
- **Satellite data processing**: Optimizing storage for large observation datasets
- **Ocean modeling**: Reducing I/O bottlenecks in simulation workflows
- **HPC workflows**: Improving I/O performance with fast compression
- **Data archival**: Maximizing storage efficiency with high-ratio compression

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

*Last Updated: November 2025 (v1.1.0)*

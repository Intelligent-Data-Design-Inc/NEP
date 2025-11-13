# NEP Development Roadmap


### v1.0 (Week 6)
- Full production release
- Comprehensive documentation
- Performance validation
- Community release

### v0.4 (Week 5)
- CDF format reader implementation
- Performance optimizations

### v0.3 (Week 3)
- GeoTIFF format reader implementation
- Enhanced format detection system

### v0.2 (Week 2)
- GRIB2 format reader implementation

#### Sprint 3: GeoTIFF File Open/Close
- Add a small, simple GeoTIFF test file in test/data.
- Add file open/close to the GeoTIFF UDF handler.
- Add a test which opens/closes the file.
- Ensure test data files are copied to build directory for both CMake and Autotools builds.

#### Sprint 2: cdf File Open/Close
- Add a small, simple cdf test file in test/data.
- Add file open/close to the cdf UDF handler.
- Add a test which opens/closes the file.
- Ensure test data files are copied to build directory for both CMake and Autotools builds.


### v1.0.0: GRIB2
#### Sprint 1: GRIB2 File Open/Close
- **NC_Dispatch Implementation**: Populate file_open and file_close function pointers in GRIB2 UDF handler NC_Dispatch structure
- **Glue Code Functions**: Create wrapper functions with proper NetCDF dispatch API signatures that interface with NCEPLIBS-g2 library
- **Test Enhancement**: Modify existing `test/test_grib2_udf` test to open and close actual GRIB2 file using NetCDF API
- **Test Data**: Use existing `test/data/gdaswave.t00z.wcoast.0p16.f000.grib2` file for file operations testing
- **Test Data Build Integration**: Ensure test data files are copied from `test/data/` to build directory for both CMake and Autotools builds so tests can find them
- **g2c Logging Integration**: Simple logging - call g2c_set_log_level(3) at test start, g2c_set_log_level(0) at test end
- **Error Handling**: Return appropriate NetCDF error codes without printing error messages
- **Resource Management**: Proper memory management and cleanup in file close operations

### v0.1.3 Documentation
#### Sprint 1: Documentation with Doxygen
- **Doxygen Build Integration**: Add Doxygen documentation generation to both CMake and Autotools build systems
  - CMake: `doc` target using `find_package(Doxygen)` with conditional building
  - Autotools: `make doc` target with Doxygen detection in configure.ac
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
- **Doxygen Configuration**: Create comprehensive Doxyfile.in template with variable substitution
  - Project metadata (version, description, author) automatically populated
  - Input/output paths configured relative to build directory
  - Warning levels set to maximum with warnings treated as errors
  - HTML output generation enabled, other formats optional
- **API Documentation Standards**: Establish and implement documentation requirements
  - All public API functions documented with Doxygen comments
  - Parameter descriptions, return values, and usage examples required
  - Code examples must be compilable and tested
  - Architecture overview and installation guides included
- **Documentation Quality Assurance**: Implement zero-warning documentation builds
  - Documentation warnings treated as build failures
  - Clean documentation generation for all existing source files in `src/`
  - Generated documentation placed in `docs/html/` directory

### v0.1.2 Add Compression Plugins
#### Sprint 1: Add LZ4 Compression Plugin
- Add hdf5_plugin subdirectory, and a hdf5_plugin/LZ4 directory.
 - The hdf5_plugin/LZ4 directory will contain contents of /home/ed/hdf5_plugins/LZ4.
 - The hdf5_plugin directory will contain the build files from /home/ed/hdf5_plugins.
 - The build files in hdf5_plugin will be modified so that only the LZ4 plugin is built. All other plugin code will be removed.

#### Sprint 2: CI 
- **CI Documentation Integration**: CI pipeline enhanced with documentation build matrix
  - Documentation build steps added for both CMake and Autotools build systems
  - Zero-warning enforcement implemented (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - Build matrix optimized to test documentation builds with all_enabled UDF configuration only
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
- **Documentation Updates**: Project documentation updated with new architecture
  - Updated `docs/prd.md` with FR-012: CI Documentation Integration requirements
  - Updated `docs/design.md` with comprehensive CI and documentation architecture section
- **GitHub Pages Deployment**: Removed from this release, planned for future versionment

### v0.1.1 Build System(s) Improvement
#### Sprint 1: Add New UDF Handlers to Build Systems
- **UDF Handler Integration**: Add BUFR, GeoTIFF, and CDF UDF handlers to both CMake and Autotools build systems
  - Each handler compiles as a separate shared library (.so/.dll)
  - Dynamic loading at runtime via NetCDF's nc_def_user_format() API
  - Isolated dependency management per handler
- **Dependencies Integration**:
  - GRIB2: NCEPLIBS-g2 (NOAA/NCEP libraries)
  - BUFR: NCEPLIBS-bufr (NOAA/NCEP libraries) 
  - GeoTIFF: libgeotiff (OSGeo project)
  - CDF: NASA CDF library from https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/cdf39_1-dist-all.tar.gz
- **Build Configuration Options**:
  - CMake: `-DENABLE_GRIB2/BUFR/GEOTIFF/CDF=ON/OFF` (default: ON)
  - Autotools: `--enable/disable-grib2/bufr/geotiff/cdf` (default: enabled)
  - Automatic dependency detection, if dependency is missing, configuration errors out.
  - Clear error messages for missing dependencies
- **Documentation Updates**: Updated docs/prd.md and docs/design.md with shared library architecture and dependency specifications

#### Sprint 2: Installation System
- **Install Targets**: Complete implementation of install targets for both CMake and Autotools build systems
- **Shared Library Installation**: UDF handler shared libraries (.so files) installed to configurable path
- **Installation Path Configuration**: Single installation path for all components, configurable via:
  - CMake: `CMAKE_INSTALL_PREFIX` variable
  - Autotools: `--prefix` configure option
- **CMake Integration**: Generate and install CMake config files for `find_package()` support
- **Platform Support**: Linux/Unix platforms only
- **Uninstall Support**: Implement uninstall targets in both build systems (no manifest tracking)
- **Dependency Assumptions**: No dependency verification during installation - assumes all required libraries are already present


### v0.1.0 
- Initial NEP framework setup
- Empty GRIB UDF handler
- Build system implementation, Cmake and autotools
- Unit tests
- CI testing


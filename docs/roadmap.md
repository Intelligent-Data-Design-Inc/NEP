# NFEP Development Roadmap

| Version | Project Month | Notes |
|---------|---------------|-------|
| v0.1    | 1             | NFEP framework, build system. |
| v0.2    | 2             | GRIB2 reader. |
| v0.3    | 3             | Add BUFR reader. |
| v0.4    | 4             | Add GeoTIFF reader. |
| v0.5    | 5             | Add CDF reader. |
| v1.0    | 6             | Full release. |

## Version Details

### v1.0 (Month 6)
- Full production release
- Comprehensive documentation
- Performance validation
- Community release

### v0.4 (Month 5)
- CDF format reader implementation
- Performance optimizations

### v0.3 (Month 3)
- GeoTIFF format reader implementation
- Enhanced format detection system

### v0.2 (Month 2)
- GRIB2 format reader implementation

### v0.1.6 BUFR File Open/Close
- Add a small, simple BUFR test file in test/data.
- Add file open/close to the BUFR vol connector.
- Add a test which opens/closes the file.

### v0.1.5 GeoTIFF File Open/Close
- Add a small, simple GeoTIFF test file in test/data.
- Add file open/close to the GeoTIFF vol connector.
- Add a test which opens/closes the file.

### v0.1.4 cdf File Open/Close
- Add a small, simple cdf test file in test/data.
- Add file open/close to the cdf vol connector.
- Add a test which opens/closes the file.

### v0.1.3 GRIB2 File Open/Close
- We have existing test test/test_grib2_vol. But it only creates a HDF5 file.
- Instead it should open and close our test GRIB2 file.
- Logging should be tried.

### v0.1.2 Documentation
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

#### Sprint 2: CI 
- **CI Documentation Integration**: CI pipeline enhanced with documentation build matrix
  - Documentation build steps added for both CMake and Autotools build systems
  - Zero-warning enforcement implemented (documentation warnings treated as build failures)
  - Documentation artifacts uploaded and preserved for 30 days
  - Build matrix optimized to test documentation builds with all_enabled VOL configuration only
  - Build options: `-DBUILD_DOCUMENTATION=ON/OFF` (CMake), `--enable/disable-docs` (Autotools)
- **Documentation Updates**: Project documentation updated with new architecture
  - Updated `docs/prd.md` with FR-012: CI Documentation Integration requirements
  - Updated `docs/design.md` with comprehensive CI and documentation architecture section
- **GitHub Pages Deployment**: Removed from this release, planned for future versionment

### v0.1.1 Build System(s) Improvement
#### Sprint 1: Add New VOLs to Build Systems
- **VOL Connector Integration**: Add BUFR, GeoTIFF, and CDF VOL connectors to both CMake and Autotools build systems
  - Each connector compiles as a separate shared library (.so/.dll)
  - Dynamic loading at runtime using dlopen() or platform equivalent
  - Isolated dependency management per connector
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
- **Shared Library Installation**: VOL connector shared libraries (.so files) installed to configurable path
- **Installation Path Configuration**: Single installation path for all components, configurable via:
  - CMake: `CMAKE_INSTALL_PREFIX` variable
  - Autotools: `--prefix` configure option
- **CMake Integration**: Generate and install CMake config files for `find_package()` support
- **Platform Support**: Linux/Unix platforms only
- **Uninstall Support**: Implement uninstall targets in both build systems (no manifest tracking)
- **Dependency Assumptions**: No dependency verification during installation - assumes all required libraries are already present


### v0.1.0 
- Initial NFEP framework setup
- Empty GRIB vol connector
- Build system implementation, Cmake and autotools
- Unit tests
- CI testing


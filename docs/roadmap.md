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

### v0.1.1
#### Sprint 1: Add New VOLs to Build Systems
- **VOL Connector Integration**: Add BUFR, GeoTIFF, and CDF VOL connectors to both CMake and Autotools build systems
  - Each connector compiles as a separate shared library (.so/.dll)
  - Dynamic loading at runtime using dlopen() or platform equivalent
  - Isolated dependency management per connector
- **Dependencies Integration**:
  - GRIB2: NCEPLIBS-g2 (NOAA/NCEP libraries)
  - BUFR: NCEPLIBS-bufr (NOAA/NCEP libraries) 
  - GeoTIFF: libgeotiff (OSGeo project)
  - CDF: NASA CDF library from https://cdf.gsfc.nasa.gov/html/sw_and_docs.html
- **Build Configuration Options**:
  - CMake: `-DENABLE_GRIB2/BUFR/GEOTIFF/CDF=ON/OFF` (default: ON)
  - Autotools: `--enable/disable-grib2/bufr/geotiff/cdf` (default: enabled)
  - Automatic dependency detection with graceful fallback
  - Clear error messages for missing dependencies
- **Documentation Updates**: Updated docs/prd.md and docs/design.md with shared library architecture and dependency specifications

#### Sprint 2: Installs
- The install targets work for both build systems.
- docs/prd.md and docs/design.md are updated.

#### Sprint 3: Documentation with Doxygen
- Doxygen is add to the builds.
- An initial Doxygen configuration file is added.
- gh-pages is used to display the main branch doxygen build.
- docs/prd.md and docs/design.md are updated.

### v0.1.0 
- Initial NFEP framework setup
- Empty GRIB vol connector
- Build system implementation, Cmake and autotools
- Unit tests
- CI testing


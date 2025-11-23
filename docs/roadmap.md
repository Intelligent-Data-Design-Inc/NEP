# NEP Development Roadmap

### v4.0 GeoTIFF
#### Sprint 1: GeoTIFF File Open/Close
- Add a small, simple GeoTIFF test file in test/data.
- Add file open/close to the GeoTIFF UDF handler.
- Add a test which opens/closes the file.
- Ensure test data files are copied to build directory for both CMake and Autotools builds.

### v2.0.0: GRIB2
#### Sprint 1: GRIB2 File Open/Close
- **NC_Dispatch Implementation**: Populate file_open and file_close function pointers in GRIB2 UDF handler NC_Dispatch structure
- **Glue Code Functions**: Create wrapper functions with proper NetCDF dispatch API signatures that interface with NCEPLIBS-g2 library
- **Test Enhancement**: Modify existing `test/test_grib2_udf` test to open and close actual GRIB2 file using NetCDF API
- **Test Data**: Use existing `test/data/gdaswave.t00z.wcoast.0p16.f000.grib2` file for file operations testing
- **Test Data Build Integration**: Ensure test data files are copied from `test/data/` to build directory for both CMake and Autotools builds so tests can find them
- **g2c Logging Integration**: Simple logging - call g2c_set_log_level(3) at test start, g2c_set_log_level(0) at test end
- **Error Handling**: Return appropriate NetCDF error codes without printing error messages
- **Resource Management**: Proper memory management and cleanup in file close operations

#### Sprint 2: File Metadata

#### Sprint 3: Reading Data

#### Sprint 4: Var Metadata

### v1.4.0 Spack Support
#### Sprint 1: Spack CI Testing and Spack Integration
- Set up testing of the spack file just like NCEPLIBS-g2c has.
- Submit spack file to spack repo.

### v1.3.0 CDF Support
#### Sprint 1: Add CDF Library Detection to Build Systems
- **NASA CDF Library Integration**: Add NASA CDF library v3.9.x to CI
  - Download and build from source: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/
  - CI installation path: `$GITHUB_WORKSPACE/cdf-install`
  - Cache CDF build in GitHub Actions for faster CI runs
- **Build System Configuration**: Add CDF library detection to both CMake and Autotools
  - CMake: Add `-DENABLE_CDF=ON/OFF` option (default: OFF)
  - Autotools: Add `--enable-cdf/--disable-cdf` configure option (default: disabled)
  - When enabled, locate CDF library and headers
  - Dependency detection mirrors GRIB2 pattern: AC_CHECK_HEADERS + AC_SEARCH_LIBS in configure.ac
  - CMake uses find_library for CDF detection
  - Error message if CDF enabled but library not found
  - Set HAVE_CDF macro when library found
  - No UDF implementation in this sprint - only library detection
- **Environment Configuration**: Set up CDF paths in CI
  - Add CDF include path to CPPFLAGS: `-I$GITHUB_WORKSPACE/cdf-install/include`
  - Add CDF library path to LDFLAGS: `-L$GITHUB_WORKSPACE/cdf-install/lib`
  - Update LD_LIBRARY_PATH: `$GITHUB_WORKSPACE/cdf-install/lib`
  - Update CMAKE_PREFIX_PATH to include CDF installation
- **CI Integration**: Exercise CDF detection in both build systems
  - Add CDF build step with caching (key: `cdf-3.9.x-${{ runner.os }}`)
  - Test CDF-enabled builds for both CMake and Autotools
  - Verify library detection succeeds when enabled
  - Verify builds succeed when disabled (default)
  - Reuse existing dependency setup pattern from HDF5/NetCDF-C
- **Configuration Files**: Update config headers for CDF support
  - Add `HAVE_CDF` macro to `config.h.cmake.in` and `config.h.in`
  - Add HAVE_CDF to `nep_config.h.in`
  - Add CDF component to `NEPConfig.cmake.in` when enabled
  - Document CDF as optional dependency in build documentation

#### Sprint 2: Add UDF for CDF
- Add UDF for CDF
- Hello-world functionality.

#### Sprint 3: Open the Test File
- Write a test which opens the test CDF file.
- Check netCDF metadata for correctness.
- Check data for correctness.


### v1.2.0 Documentation Improvements
#### Sprint 1: Documentation Fixes
- Fix GitHub Pages documentation deployment so that docs are deployed from the `main` branch (not from release tags), keeping the existing environment protection rules intact.
- Ensure the docs deployment workflow always uses the project version from `version.txt` (currently `1.2.0`) as the single source of truth for displayed version numbers.
- Update the top-level docs and Doxygen main page so the product title no longer refers to "NetCDF4/HDF5 Format Extension Pack"; HDF5 may be mentioned only in technical sections, not in the main product tagline.
- Remove all references to HDF5 VOL or VOLs from the documentation set, reflecting that VOLs are no longer used.
- Fix all visible version strings on the main page so they match the current release version (e.g., v1.2.0) and do not show stale versions like 0.1.1 or 1.0.0.
- Remove the "future releases" section entirely from the published documentation; no promises are made beyond the most recent release.
- Define the "API Documentation" section to point to the versioned C/Fortran API docs (for example, per-release URLs like `/NEP/v1.2.0/api/` with a "latest" alias), and ensure the link target is consistently updated on release.
- On the main page, add a concise "Features and options" bullet list that explicitly mentions:
  - The ability to enable/disable LZ4 and BZIP2 compression support.
  - The ability to enable/disable the Fortran build.
  - That when Fortran is enabled, `netcdf-fortran` is a required dependency.
  Detailed flags and dependency tables live on a dedicated build/options page, not on the main page.



### v1.1.0 Fortran Wrappers
#### Sprint 1: Add Fortran Wrappers for Compression Functions
- Add Fortran-callable wrappers for existing v1.0.0 compression functions without changing C API behavior.
- Integrate Fortran wrappers into both build systems:
  - CMake: build Fortran wrappers by default; add `-DENABLE_FORTRAN=ON/OFF` option.
  - Autotools: build Fortran wrappers by default; add `--enable-fortran/--disable-fortran` options.
  - When Fortran is disabled, do not build/install any Fortran libraries or modules and do not build/run Fortran tests.
- Keep Fortran source code in `fsrc/` and Fortran tests in `ftest/`.
- Add smoke tests in `ftest/` for each exposed Fortran wrapper and hook them into CI when Fortran is enabled.
- Ensure enabling Fortran wrappers does not change behavior of existing C-only workflows, and disabling Fortran has no impact on C builds or tests.

### v1.0.0 LZ4 Compression
#### Sprint 1: GitHub Pages Documentation Deployment
- **GitHub Actions Workflow**: Create automated workflow for Doxygen documentation deployment to GitHub Pages
  - Trigger on releases and manual workflow dispatch
  - Build documentation using Doxygen from main branch
  - Deploy generated HTML documentation to gh-pages branch
  - Configure GitHub Pages to serve from gh-pages branch
- **Documentation URL Configuration**: Set up custom documentation URL at https://intelligent-data-design-inc.github.io/NEP/
- **Workflow Integration**: Integrate with existing CI pipeline
  - Separate workflow file for documentation deployment
  - Use GitHub Actions checkout and deployment actions
  - Configure proper permissions for gh-pages deployment
- **Version Management**: Documentation versioned with releases
  - Latest documentation always reflects current main branch
  - Release-specific documentation snapshots preserved
- **Verification**: Ensure documentation is accessible and properly formatted on GitHub Pages

#### Sprint 2: CI and Build System Improvements
- **CMake Build System Simplification**: Comment out GRIB2, CDF, and GeoTIFF format handler options
  - Comment out `option(ENABLE_GRIB2)`, `option(ENABLE_CDF)`, `option(ENABLE_GEOTIFF)` in CMakeLists.txt
  - Comment out UDF handler library builds in src/CMakeLists.txt
  - Add clear version-specific comments (v2.0.0, v3.0, v4.0) for future restoration
  - Preserve all code structure for future re-enablement
- **Autotools Build System Simplification**: Comment out format handler configuration
  - Comment out `AC_ARG_ENABLE` sections for GRIB2, CDF, and GeoTIFF in configure.ac
  - Comment out dependency checks for NCEPLIBS-g2, NASA CDF, and libgeotiff
  - Comment out library build rules in src/Makefile.am
  - Maintain code structure with restoration markers
- **CI Pipeline Optimization**: Reduce build matrix and dependencies
  - Simplify build matrix from 10 configurations to 2 (cmake/autotools with LZ4 only)
  - Remove NCEPLIBS-g2, NASA CDF, and libgeotiff installation steps
  - Keep core dependencies: HDF5, NetCDF-C, NetCDF-Fortran, LZ4, Doxygen
  - Update environment variables (LD_LIBRARY_PATH, CPPFLAGS, LDFLAGS) to remove format handler paths
  - Remove format handler test execution steps
  - Add v1.0.0 scope comment to workflow file
- **Configuration File Updates**: Update config headers for v1.0.0 scope
  - Comment out HAVE_GRIB2, HAVE_CDF, HAVE_GEOTIFF in config.h.cmake.in
  - Comment out format handler macros in nep_config.h.in
  - Update NEPConfig.cmake.in to show only LZ4 component
  - Add version-specific comments throughout
- **Documentation Updates**: Clarify v1.0.0 scope and future roadmap
  - Update README.md to emphasize LZ4-only scope for v1.0.0
  - Update docs/design.md with current implementation status
  - Update docs/prd.md with v1.0.0 deliverables and postponed features
  - Create docs/releases/v1.0.0.md with release notes
- **Expected Benefits**: Reduced complexity and faster builds
  - 30-40% reduction in build time (fewer dependencies)
  - 50% reduction in CI execution time (simplified matrix)
  - Clear v1.0.0 scope focusing on LZ4 compression
  - All format handler infrastructure preserved for future versions

#### Sprint 3: Spack Support
- **Spack Package Creation**: Create package.py file for NEP following Spack package conventions
  - Define package metadata: name, homepage, URL, description, maintainers
  - Specify version information with checksums for releases
  - Support both CMake and Autotools build systems
  - Define package variants for build options
- **Dependency Specification**: Declare all required and optional dependencies
  - Required: netcdf-c, hdf5, lz4, bzip2
  - Optional: doxygen (for documentation builds)
  - Specify version constraints for each dependency
- **Build System Integration**: Configure both CMake and Autotools build methods
  - CMake: Define cmake_args() method with proper flags
  - Autotools: Define configure_args() method with enable/disable options
  - Handle installation prefix and library paths
- **Variant Configuration**: Define Spack variants for NEP features
  - +docs/-docs: Enable/disable documentation building
  - +lz4/-lz4: Enable/disable LZ4 compression support
  - +bzip2/-bzip2: Enable/disable BZIP2 compression support
- **Testing Integration**: Add install and smoke tests
  - Verify installed libraries exist
  - Check HDF5 plugin installation
  - Test basic functionality similar to NCEPLIBS-g2c testing approach
- **Documentation**: Add package documentation and usage examples
  - Installation instructions via spack install nep
  - Variant usage examples
  - Integration with other Spack packages

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


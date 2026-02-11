# NEP Development Roadmap

### V1.6.0
Implement Phase 4 of the GeoTIFF read layer for NEP v1.5.0: Extract and expose coordinate reference system (CRS) and georeferencing information following CF conventions. (see closed GitHub issue 59)

### V1.5.4: NetCDF Self-Loading UDF Support

NetCDF-C now supports self-loading UDFs via RC file configuration. This version adds initialization functions for GeoTIFF and CDF format handlers to enable automatic loading through the new UDF plugin system.

#### Sprint 1: Master UDF Slot Allocation Header and Configure-Time Detection

**Objective**: Create centralized UDF slot allocation and detect NetCDF-C UDF self-registration capability at configure time.

**Background**: NEP must work with both old and new versions of NetCDF-C. Newer versions support UDF self-registration, while older versions require manual `nc_def_user_format()` calls. We detect capability by checking for `NC_HAS_UDF_SELF_LOAD` in `netcdf_meta.h`.

**Tasks**:
- **Configure-time detection** (CMake and Autotools):
  - Check for `NC_HAS_UDF_SELF_LOAD` in `netcdf_meta.h` during configure
  - Define `HAVE_NETCDF_UDF_SELF_REGISTRATION` if `NC_HAS_UDF_SELF_LOAD` is 1
  - Add to `config.h` for use in source code
  - CMake: Use `check_symbol_exists(NC_HAS_UDF_SELF_LOAD netcdf_meta.h HAVE_NETCDF_UDF_SELF_REGISTRATION)`
  - Autotools: Use `AC_CHECK_DECLS([NC_HAS_UDF_SELF_LOAD], [], [], [[#include <netcdf_meta.h>]])`
- Create `include/NEP.h` header file with master UDF slot mapping
- Define constants for each format's UDF slot assignment:
  - `NEP_UDF_GEOTIFF_STANDARD` → `NC_UDF0` (standard TIFF little-endian, magic: "II*")
  - `NEP_UDF_GEOTIFF_BIGTIFF` → `NC_UDF1` (BigTIFF little-endian, magic: "II+")
  - `NEP_UDF_CDF` → `NC_UDF2` (NASA CDF format, magic: "\xCD\xF3\x00\x01")
  - `NEP_UDF_GRIB2` → `NC_UDF3` (GRIB2 format, reserved for future use)
  - `NEP_UDF_RESERVED_4` through `NEP_UDF_RESERVED_9` for future formats
- Add comprehensive Doxygen documentation explaining:
  - UDF slot allocation strategy
  - Magic number format for each slot
  - How to add new format handlers
  - NetCDF-C UDF system overview
  - Conditional compilation strategy for UDF support
- Update `include/geotiffdispatch.h` to include NEP.h and reference slot constants
- Update `include/cdfdispatch.h` to include NEP.h and reference slot constants
- Add NEP.h to build systems (CMake and Autotools)

**Definition of Done**:
- Configure detects `NC_HAS_UDF_SELF_LOAD` and sets `HAVE_NETCDF_UDF_SELF_REGISTRATION` appropriately
- NEP.h created with all slot constants defined
- Both dispatch headers updated to use NEP.h constants
- Header compiles without errors
- Documentation clearly explains slot allocation and conditional compilation

#### Sprint 2: GeoTIFF Initialization Function

**Objective**: Implement GeoTIFF UDF initialization function with conditional compilation for backward compatibility.

**Implementation Strategy**:
- **With UDF self-registration** (`HAVE_NETCDF_UDF_SELF_REGISTRATION` defined):
  - NetCDF-C will call `NC_GEOTIFF_initialize()` automatically via RC file or dlopen
  - Function should NOT call `nc_def_user_format()` - NetCDF handles registration
  - Function can perform any other initialization needed
- **Without UDF self-registration** (older NetCDF-C):
  - `NC_GEOTIFF_initialize()` must call `nc_def_user_format()` to register dispatch table
  - Applications must explicitly call `NC_GEOTIFF_initialize()` at startup

**Tasks**:
- Implement `NC_GEOTIFF_initialize()` function in `src/geotiffdispatch.c`
- Function must be exported (not static) for dynamic loading via dlopen/LoadLibrary
- Use conditional compilation:
  ```c
  #ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
  /* Old NetCDF-C: manually register dispatch table */
  nc_def_user_format(NEP_UDF_GEOTIFF_STANDARD | NC_NETCDF4, &GEOTIFF_dispatch_table, "II*");
  nc_def_user_format(NEP_UDF_GEOTIFF_BIGTIFF | NC_NETCDF4, &GEOTIFF_dispatch_table, "II+");
  #endif
  /* New NetCDF-C: registration handled by NetCDF, no nc_def_user_format() needed */
  ```
- Register both TIFF variants when using old NetCDF-C:
  - **UDF0 slot**: Standard TIFF little-endian (magic: "II*")
  - **UDF1 slot**: BigTIFF little-endian (magic: "II+")
- Same dispatch table handles both TIFF variants (format detection in open function)
- Return `NC_NOERR` on success, appropriate error code on failure
- Add comprehensive Doxygen documentation explaining conditional behavior
- Update all GeoTIFF test programs to call `NC_GEOTIFF_initialize()` at startup
- Verify existing GeoTIFF tests continue to pass with both old and new NetCDF-C

**Pattern Reference**: See `test_geotiff/tst_*.c` files for current registration pattern

**Definition of Done**:
- `NC_GEOTIFF_initialize()` implemented with conditional compilation
- Works correctly with both old and new NetCDF-C versions
- Both TIFF variants registered correctly (when needed)
- All existing GeoTIFF tests pass
- Function documented with Doxygen explaining both code paths

#### Sprint 3: CDF Initialization Function and Integration Testing

**Objective**: Implement CDF UDF initialization function with conditional compilation and validate complete UDF self-loading system.

**Implementation Strategy**:
- **With UDF self-registration** (`HAVE_NETCDF_UDF_SELF_REGISTRATION` defined):
  - NetCDF-C will call `NC_CDF_initialize()` automatically via RC file or dlopen
  - Function should NOT call `nc_def_user_format()` - NetCDF handles registration
  - Function can perform any other initialization needed
- **Without UDF self-registration** (older NetCDF-C):
  - `NC_CDF_initialize()` must call `nc_def_user_format()` to register dispatch table
  - Applications must explicitly call `NC_CDF_initialize()` at startup

**Tasks**:
- Implement `NC_CDF_initialize()` function in `src/cdfdispatch.c`
- Function must be exported (not static) for dynamic loading
- Use conditional compilation:
  ```c
  #ifndef HAVE_NETCDF_UDF_SELF_REGISTRATION
  /* Old NetCDF-C: manually register dispatch table */
  nc_def_user_format(NEP_UDF_CDF | NC_NETCDF4, &CDF_dispatch_table, "\xCD\xF3\x00\x01");
  #endif
  /* New NetCDF-C: registration handled by NetCDF, no nc_def_user_format() needed */
  ```
- Register CDF format when using old NetCDF-C:
  - **UDF2 slot**: NASA CDF format with CDF3 magic number (0xCDF30001)
  - Magic bytes: `\xCD\xF3\x00\x01` for automatic format detection
  - Note: CDF files can have magic bytes 0xCDF30001 (CDF3) or 0xCDF26002 (CDF2.6)
- Return `NC_NOERR` on success, appropriate error code on failure
- Add comprehensive Doxygen documentation explaining conditional behavior
- Update CDF test programs to call `NC_CDF_initialize()` at startup
- Create integration test validating all initialization functions work with both old and new NetCDF-C
- Add test for programmatic UDF registration (old NetCDF-C path)
- Document RC file configuration in user guide (new NetCDF-C only):
  ```ini
  # Example .ncrc configuration for GeoTIFF (requires HAVE_NETCDF_UDF_SELF_REGISTRATION)
  NETCDF.UDF0.LIBRARY=/usr/local/lib/libnep.so
  NETCDF.UDF0.INIT=NC_GEOTIFF_initialize
  NETCDF.UDF0.MAGIC=II*
  
  # Example .ncrc configuration for CDF
  NETCDF.UDF2.LIBRARY=/usr/local/lib/libnep.so
  NETCDF.UDF2.INIT=NC_CDF_initialize
  NETCDF.UDF2.MAGIC=\xCD\xF3\x00\x01
  ```
- Update build documentation with UDF self-loading examples
- Reference NetCDF-C UDF documentation for configuration details
- Document backward compatibility strategy

**Pattern Reference**: See `test/tst_cdf_udf.c:195-200` for current registration pattern

**Definition of Done**:
- `NC_CDF_initialize()` implemented with conditional compilation
- Works correctly with both old and new NetCDF-C versions
- CDF format registered correctly in UDF2 slot (when needed)
- All existing CDF tests pass
- Integration test validates all initialization functions with both NetCDF-C versions
- Documentation updated with RC file examples and backward compatibility notes
- No slot conflicts between GeoTIFF and CDF

#### Sprint 4: More Examples
**Detailed Plan**: See `docs/plan/v1.5.4-sprint4-more-examples.md`

- Add C example `classic/coord.c` and Fortran example `f_classic/f_coord.f90`
- Create a classic-format NetCDF file with three fixed dimensions: lat(4), lon(5), time(3)
- Create coordinate variables for lat, lon, time with full CF convention attributes
  - Time attributes: `units = "hours since 2026-01-01"`, `standard_name`, `long_name`, `axis`, `calendar`
  - Lat/lon attributes: `units`, `standard_name`, `long_name`, `axis`
- Create a variable called "sfc_temp" with dimensions time, lat, lon
  - Add `coordinates = "time lat lon"` attribute (CF best practice)
  - Add `_FillValue`, `units`, `standard_name`, `long_name` attributes
- Fill coordinate vars with reasonable data (lat: -45 to 45, lon: -120 to 120, time: 0/6/12 hours)
- Generate surface temperature data using formula: `280.0 + lat_idx * 2.0 + lon_idx * 0.5 + time_idx * 1.0`
- Write all coordinate and sfc_temp variables
- Close the file
- Re-open the file and validate all contents (dimensions, variables, attributes, data)
- Generate CDL expected output files and add CDL validation tests
- Integrate into CMake and Autotools build systems with test scripts


### V1.5.3
#### Sprint 1: Add C Quickstart Example
**Detailed Plan**: See `docs/plan/v1.5.3-sprint1-c-quickstart.md`

- Add example C program `classic/quickstart.c`
- Very short intro to NetCDF demonstrating basic operations
- Define two dimensions: "X" (size 2) and "Y" (size 3)
- Define one variable of type NC_INT called "data" with dimensions X and Y
- Create array to hold generated data (simple sequential values)
- Write array data to file
- Add global attribute "description" = "a quickstart example"
- Add variable attribute "units" = "m/s"
- Close the file
- Reopen and validate all contents (dimensions, variables, attributes, data)
- Integrate into CMake and Autotools build systems
- Add comprehensive Doxygen documentation

#### Sprint 2: Add Fortran Quickstart Example
**Detailed Plan**: See `docs/plan/v1.5.3-sprint2-fortran-quickstart.md`

- Fortran program `f_classic/f_quickstart.f90`
- Equivalent to C version using NetCDF-Fortran API (nf90_* functions)
- Same structure: 2 dimensions, 1 variable, 2 attributes
- Handle Fortran-specific considerations (column-major arrays, 1-based indexing)
- Conditional build integration (only when ENABLE_FORTRAN=ON)
- Cross-language validation ensuring identical output to C version
- Comprehensive Doxygen documentation

#### Sprint 3: CDL Output Validation
**Detailed Plan**: See `docs/plan/v1.5.3-sprint3-cdl-validation.md`

- Generate expected CDL baselines for both quickstart examples
- Store in `examples/expected_output/` directory
- Implement automated CDL comparison tests using ncdump + diff
- CMake test integration with add_test()
- Autotools test integration with shell scripts
- Document regeneration process for intentional changes
- CI integration for regression testing
- Clear error messages and troubleshooting guidance

### V1.5.2
#### Sprint 1: C Groups Example Implementation
- **C Example Program (groups.c)**: Create comprehensive groups demonstration in `examples/netcdf-4/`
  - **Group Structure**: Root group + 2 sub-groups (SubGroup1, SubGroup2), with SubGroup2 containing NestedGroup
  - **Dimension Definitions**: 
    - Root level: 2 dimensions (e.g., `x=3`, `y=4`)
    - NestedGroup: 1 additional dimension (e.g., `z=2`)
  - **Variable Definitions**: 2D variables using all 5 new integer types
    - NC_UBYTE variable in root group
    - NC_USHORT variable in SubGroup1
    - NC_UINT variable in SubGroup2
    - NC_INT64 variable in NestedGroup (using root dimensions)
    - NC_UINT64 variable in NestedGroup (3D using root dims + z dimension)
  - **Dimension Visibility Validation**: Use `nc_inq_dimid()` to explicitly verify root dimensions are accessible in all groups
  - **Data Operations**: Write minimal test data (3x4 arrays with simple sequential values), close file, reopen, validate all metadata with `nc_inq_*` functions, print data
  - **Doxygen Documentation**: Comprehensive documentation explaining groups, nested groups, dimension visibility, and new integer types
- **CMake Build Integration**: Add to `examples/netcdf-4/CMakeLists.txt`
  - Build groups.c as executable linked against NetCDF-C
  - Register as CTest test when `BUILD_EXAMPLES=ON`
  - Follow existing example build patterns
- **Autotools Build Integration**: Add to `examples/netcdf-4/Makefile.am`
  - Add to `make check` test suite
  - Follow existing example build patterns
- **Test Execution**: Program runs as automated test
  - Exit code 0 on success
  - Creates NetCDF file demonstrating groups and new types
  - No CDL validation in Sprint 1 (deferred to Sprint 3)

#### Sprint 2: Fortran Groups Example Implementation
- **Fortran Example Program (f_groups.f90)**: Fortran equivalent in `examples/f_netcdf-4/`
  - Parallel implementation matching C example structure from Sprint 1
  - Same group hierarchy, dimensions, variables, and data
  - Fortran-90 API calls (`nf90_def_grp`, `nf90_inq_grp_ncid`, etc.)
  - Equivalent Doxygen documentation
- **CMake Build Integration**: Add to `examples/f_netcdf-4/CMakeLists.txt`
  - Build f_groups.f90 when `ENABLE_FORTRAN=ON` and netcdf-fortran available
  - Register as CTest test when `BUILD_EXAMPLES=ON`
  - Follow existing example build patterns
- **Autotools Build Integration**: Add to `examples/f_netcdf-4/Makefile.am`
  - Conditional Fortran build using `AM_CONDITIONAL([BUILD_FORTRAN])`
  - Add to `make check` test suite
  - Follow existing example build patterns
- **Test Execution**: Program runs as automated test
  - Exit code 0 on success
  - Creates NetCDF file demonstrating groups and new types
  - No CDL validation in Sprint 2 (deferred to Sprint 3)

#### Sprint 3: CDL Output Validation
- **CDL Reference Files**: Generate expected output baselines
  - Run groups.c and f_groups.f90 to create NetCDF files
  - Use `ncdump` to generate CDL representations
  - Store in `examples/expected_output/` as `groups_expected.cdl` and `f_groups_expected.cdl`
  - CDL files serve as regression test baselines
- **Automated Validation**: Integrate CDL comparison into test suite
  - CMake: Custom test commands with `add_test()` that run example + ncdump + comparison
  - Autotools: Shell scripts performing execute → ncdump → compare workflow
  - Use `diff` or text comparison to validate output matches expected CDL
  - Clear error messages showing differences on validation failures
- **Test Enhancement**: Update test scripts
  - Capture example program output
  - Run `ncdump` on generated NetCDF files
  - Compare against stored CDL baselines
  - Test fails if output differs (indicates regression)
- **Documentation Updates**: Document validation approach
  - Update `examples/README.md` with CDL validation explanation
  - Document CDL regeneration process for intentional changes
  - Add troubleshooting section for validation failures
- **CI Integration**: Validation runs automatically in CI pipeline
  - Examples run as tests when BUILD_EXAMPLES=ON (default)
  - CDL validation catches regressions
  - No additional CI workflow changes needed
  
### V1.5.1
#### Sprint 1: Add Examples Directory and Build Integration
- **Source Integration**: Copy example programs from `/home/ed/writing_netcdf_programs` to new `examples/` directory in NEP
  - Create `examples/` directory structure matching source layout
  - Copy C examples: `classic/` (6 programs), `netcdf-4/` (5 programs)
  - Copy Fortran examples: `f_classic/` (6 programs), `f_netcdf-4/` (5 programs)
  - Copy empty directories: `parallelIO/`, `performance/` (placeholders for future work)
  - Preserve directory structure for consistency and future expansion
- **CMake Build System Integration**: Add examples to CMake build
  - Add `option(BUILD_EXAMPLES "Build example programs" ON)` to root CMakeLists.txt
  - Create `examples/CMakeLists.txt` as top-level build file
  - Create subdirectory CMakeLists.txt for each example category (classic, netcdf-4, f_classic, f_netcdf-4)
  - Reference `/home/ed/writing_netcdf_programs/CMakeLists.txt` for build patterns
  - Link examples against installed NetCDF-C library (not internal NEP code)
  - Configure RPATH for runtime library discovery
  - Conditional Fortran builds: Only build f_classic/ and f_netcdf-4/ when `ENABLE_FORTRAN=ON` and netcdf-fortran is available
  - Register examples as CTest tests when `BUILD_EXAMPLES=ON`
- **Autotools Build System Integration**: Add examples to Autotools build
  - Add `AC_ARG_ENABLE([examples])` to configure.ac with default enabled
  - Create `examples/Makefile.am` and subdirectory Makefile.am files
  - Reference `/home/ed/writing_netcdf_programs/configure.ac` and Makefile.am files for patterns
  - Conditional Fortran builds: Use `AM_CONDITIONAL([BUILD_FORTRAN])` to control f_classic/ and f_netcdf-4/
  - Add examples to `make check` test suite when enabled
  - Update AC_CONFIG_FILES to include examples/Makefile and subdirectories
- **Test Integration**: Examples execute as validation tests
  - Each example program runs during `make check` (Autotools) or `ctest` (CMake)
  - Test success criteria: Program executes without errors (exit code 0)
  - Test output: Programs create NetCDF files demonstrating various features
  - No output validation in Sprint 1 (deferred to Sprint 2)
- **Doxygen Documentation Updates**: Add examples section to API documentation
  - Update `docs/Doxyfile.in` to include `examples/` directory in INPUT paths
  - Create `examples/README.md` with overview of example categories
  - Add examples section to Doxygen main page with:
    - Brief description of each example category (classic, netcdf-4, Fortran variants)
    - Links to source files with descriptions of what each demonstrates
    - Build instructions for examples (how to enable/disable)
  - Document example programs with Doxygen comments explaining their purpose
- **Build Configuration**: Flexible example building
  - CMake: `-DBUILD_EXAMPLES=ON/OFF` (default: ON)
  - Autotools: `--enable-examples/--disable-examples` (default: enabled)
  - When disabled: Examples not built, not installed, not tested
  - Fortran examples automatically skip if ENABLE_FORTRAN=OFF or netcdf-fortran unavailable

#### Sprint 2: Example Output Validation with CDL Comparison
- **CDL Reference Files**: Generate and store expected output
  - Run each example program to generate NetCDF output files
  - Use `ncdump` to create CDL (Common Data Language) representation of each output file
  - Store CDL files in `examples/expected_output/` directory
  - CDL files serve as regression test baselines
  - Naming convention: `<example_name>_expected.cdl` (e.g., `simple_2D_expected.cdl`)
- **Test Enhancement**: Automated output validation
  - Modify test scripts to capture example program output
  - Run `ncdump` on generated NetCDF files after each example executes
  - Compare ncdump output against stored CDL files using text comparison
  - Test fails if output differs from expected CDL (indicates regression)
  - Provide clear error messages showing differences when validation fails
- **CMake Test Updates**: Integrate CDL validation into CTest
  - Add custom test commands that run example + ncdump + comparison
  - Use CMake's `add_test()` with comparison logic
  - Store generated files in build directory for inspection
  - Clean up temporary files after successful tests
- **Autotools Test Updates**: Integrate CDL validation into make check
  - Create shell scripts for each example that perform: execute → ncdump → compare
  - Add scripts to TESTS variable in Makefile.am
  - Use `diff` or similar tool for CDL comparison
  - Report failures with helpful diagnostics
- **Documentation Updates**: Document validation approach
  - Update `examples/README.md` with explanation of CDL validation
  - Document how to regenerate expected CDL files if intentional changes occur
  - Add troubleshooting section for common validation failures
  - Explain purpose of regression testing for examples
- **CI Integration**: Ensure examples run in continuous integration
  - Examples already run as tests when BUILD_EXAMPLES=ON (default)
  - CDL validation catches regressions in CI pipeline
  - No additional CI workflow changes needed (examples use existing test infrastructure)

  #### Sprint 3: Further Development of Examples
  - The fortran example f_simple2d.f90 needs to have a similar structure to the C program classic/simple_2D.c. The fortran program has a subprogram do all the work, instead, move that code to the main program so it looks similar to the C program.
  - The Fortran program f_simple_2D.f90 needs to define the exact same attributes as the C program simple_2d.c. The output of the two programs should be identical, and this can be tested by comparing their CDF output with ncdump.

  #### Sprint 4: Example Documented with Doxygen
  - We need doxygen documentation on all the examples, C and Fortran.
  - The documentation should explain the example, assuming the reader does not know much about netCDF.

  
### v1.5.0 GeoTIFF Read Support (Released: January 2026)
#### Sprint 1: GeoTIFF File Open/Close and Data Reading
- [x] Add GeoTIFF test files in test/data (MODIS NRT Global Flood Product samples)
- [x] Implement GeoTIFF UDF handler (`src/geotifffile.c`, `src/geotiffdispatch.c`)
- [x] File open/close operations (`NC_GEOTIFF_open()`, `NC_GEOTIFF_close()`)
- [x] Automatic TIFF magic number validation and GeoTIFF tag detection
- [x] Metadata extraction (dimensions, variables, attributes from GeoTIFF structure)
- [x] Data reading (`NC_GEOTIFF_get_vara()`) with type conversion
- [x] Endianness support (little-endian and big-endian TIFF files)
- [x] Security hardening with validation against malformed files
- [x] Dimension mapping (bands, rows, columns to NetCDF dimensions)
- [x] Build system integration (CMake and Autotools with `ENABLE_GEOTIFF` option)
- [x] Comprehensive test suite (10 test programs covering basic functionality, errors, edge cases, performance)
- [x] Test data copied to build directory for both CMake and Autotools builds
- [x] CI integration with GeoTIFF tests in GitHub Actions workflow
- [x] Documentation updates (API docs, design.md, build-options.md)
- [x] libgeotiff and libtiff dependency integration

**Status**: Released January 1, 2026. Provides read-only access to GeoTIFF files through standard NetCDF API.

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
- **Spack CI Workflow**: Create separate `.github/workflows/spack.yml` workflow
  - Run on PRs and pushes to main branch
  - Isolate Spack testing from main CI matrix
  - Follow NCEPLIBS patterns for workflow structure
- **Spack CI Testing Scope**: Implement two-tier testing approach
  - Lint checks on every PR: Use `spack audit` and `spack style` to validate package.py syntax
  - Full install test on main branch: Clone spack, add NEP as local package, run `spack install nep@develop`
  - Validate dependency resolution and build completion
- **Add Fortran Variant**: Update `spack/package.py` with Fortran support
  - Add `variant("fortran", default=True, description="Build Fortran wrappers")`
  - Add `depends_on("netcdf-fortran", when="+fortran", type=("build", "link"))`
  - Update `cmake_args()` with `ENABLE_FORTRAN` based on variant
- **CMake-Only Build System**: Simplify package.py to CMake only
  - Remove Autotools support from package.py (inherit only from CMakePackage)
  - Remove `configure_args()` method and build_system variant
  - Aligns with roadmap intent; Autotools users build directly from source
- **Version and Checksum Updates**: Prepare for spack repo submission
  - Add released version (v1.4.0) with valid SHA256 checksum
  - Keep `develop` branch pointing to `main` for testing
  - Update version in package.py after release is tagged
- **Spack Repository Submission**: Submit PR to spack/spack repository
  - Submit after CI is passing and at least one version has valid checksum
  - Follow spack contribution guidelines and package conventions
  - Iterate based on spack maintainer feedback
- **Definition of Done**:
  - Spack CI workflow passing on PRs and main branch
  - package.py updated with Fortran variant and CMake-only build
  - At least one released version with valid checksum
  - PR submitted to spack/spack repository

  #### Sprint 2: Spack Package File for CDF
  - **Package Structure**: Create `spack/cdf/package.py` following standard Spack lowercase naming convention
    - Package class name: `Cdf`
    - Inherits from `MakefilePackage` base class
    - Override `edit()`, `build()`, and `install()` methods for custom build system
  - **Version and Dependencies**: 
    - Add version 3.9.1 with calculated SHA256 checksum from: `https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/cdf39_1/cdf39_1-dist-cdf.tar.gz`
    - No variants in initial implementation (can add shared/fortran variants in future sprint)
    - No external dependencies beyond system compiler
  - **Build System Implementation**:
    - CDF uses custom Makefile requiring `OS=linux ENV=gnu` parameters
    - Build command: `make OS=linux ENV=gnu all`
    - Install command: `make INSTALLDIR=<prefix> install`
    - Reference `.github/workflows/ci.yml` lines 74-86 for build details
  - **Testing Workflow**: Create `.github/workflows/spack-cdf.yml`
    - Include spec check: `spack spec cdf`
    - Include install attempt: `spack install -v cdf || true`
    - Use `|| true` to exit successfully even on failure for manual debugging
    - Review logs to identify and fix issues iteratively
  - **Submission**: Submit CDF package to spack/spack-packages repository
    - Follow same PR process as NEP package submission
    - Ensure `spack audit packages cdf` passes
    - Ensure `spack style` checks pass
  - **Definition of Done**:
    - [x] `spack/cdf/package.py` created with MakefilePackage implementation
    - [x] Version 3.9.1 with valid SHA256 checksum
    - [x] `.github/workflows/spack-cdf.yml` workflow created and running
    - [x] Package includes explicit URL for version to pass Spack CI verification
    - [x] PR submitted to spack/spack-packages repository
    - [ ] PR approved and merged by Spack maintainers

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

#### Sprint 2: CDF Data File and Program
- Find small CDF data file, put it in repo in test/data
- Write a small test program to open the file and print metadata, using NASA CDF library. Only build if CDF is enabled.
- Add to CI.

#### Sprint 3: Add UDF for CDF
- **CDF UDF Handler Integration**: Integrate existing CDF source files into both build systems
  - Source files to integrate: `src/cdfdispatch.c`, `src/cdffile.c`, `src/cdffunc.c`, `src/cdfvar.c`
  - Header file to integrate: `include/cdfdispatch.h`
  - Reference implementation: `/home/ed/netcdf-c/libhdf4` for build pattern
  - UDF documentation: https://docs.unidata.ucar.edu/netcdf/NUG/user_defined_formats.html
- **CMake Build System Integration**:
  - Add CDF UDF handler library build when `ENABLE_CDF=ON` and `HAVE_CDF=TRUE`
  - Library name: `libnccdf` (shared library)
  - Source files: `cdfdispatch.c`, `cdffile.c`, `cdffunc.c`, `cdfvar.c`
  - Link against: HDF5, NetCDF-C, and NASA CDF library (`${CDF_LIBRARY}`)
  - Set library version to match project version with SOVERSION 1
  - Install to `${CMAKE_INSTALL_LIBDIR}` with RPATH configuration
  - Export as part of NEPTargets for CMake package integration
  - Add to `src/CMakeLists.txt` following GRIB2 pattern (currently commented out)
- **Autotools Build System Integration**:
  - Add conditional CDF library build when `HAVE_CDF` is true
  - Library name: `libnccdf.la` (libtool library)
  - Source files: `cdfdispatch.c`, `cdffile.c`, `cdffunc.c`, `cdfvar.c`
  - Link flags: `$(HDF5_LIBS) $(CDF_LIBS)` via `libnccdf_la_LIBADD`
  - Version info: `-version-info 1:0:0` via `libnccdf_la_LDFLAGS`
  - Add to `src/Makefile.am` following GRIB2 pattern (currently commented out)
  - Conditional build: `if HAVE_CDF ... endif` block
- **Code Preparation**:
  - Comment out main function bodies in all CDF source files (cdfdispatch.c, cdffile.c, cdffunc.c, cdfvar.c)
  - Leave function signatures, declarations, and dispatch table structure intact
  - Preserve all header includes and type definitions
  - Keep NC_Dispatch structure initialization in cdfdispatch.c
  - Ensure code compiles but functions return placeholder/error codes
- **Code Attribution Updates**:
  - Change all `@author` tags to: `Edward Hartnett` (consistent spelling)
  - Update or add `@date` tags to current date (2025-11-23) in all CDF files
  - Add copyright line to all CDF source files: `@copyright Intelligent Data Design, Inc. All rights reserved.`
  - Maintain existing Doxygen comment structure and formatting
- **Header File Updates**:
  - Verify `include/cdfdispatch.h` has proper include guards and prototypes
  - Ensure header is compatible with both C and C++ (extern "C" blocks)
  - Update header documentation to reflect CDF UDF handler purpose
- **Build Verification**:
  - Verify builds succeed with `-DENABLE_CDF=ON` (CMake) when CDF library is available
  - Verify builds succeed with `--enable-cdf` (Autotools) when CDF library is available
  - Verify builds succeed with CDF disabled (default behavior)
  - Confirm `libnccdf.so` (or `libnccdf.la`) is created and installed when enabled
  - Check that library links properly against all dependencies (HDF5, NetCDF-C, CDF)
- **CI Integration**:
  - CDF UDF library build exercised in existing CI CDF-enabled configurations
  - Verify library installation and linking in CI environment
  - No functional tests yet - only build/install verification
- **Documentation**:
  - Update build documentation to reflect CDF UDF handler availability
  - Note that CDF UDF is build-only in this sprint (no functional implementation)
  - Document that Sprint 4 will add functional tests and implementation

#### Sprint 4: Open the CDF Test File
- **NC_CDF_open() Implementation**: Complete implementation of file open with full metadata mapping
  - Open CDF file using CDFopen() from NASA CDF library
  - Store CDF file ID in NC_CDF_FILE_INFO_T structure
  - Query CDF file metadata (number of variables, dimensions, attributes)
  - Build complete internal NetCDF metadata structures (NC_FILE_INFO_T, NC_GRP_INFO_T)
  - Map CDF dimensions to NetCDF dimensions
  - Map CDF variables to NetCDF variables with correct types
  - Map CDF attributes to NetCDF attributes (global and variable)
  - Reference implementation pattern: `/home/ed/netcdf-c/libhdf4` for NetCDF internal metadata structures
- **CDF Type Mapping**: Implement cdf_type_info() function for type conversion
  - Create lookup table mapping CDF types to NetCDF types:
    - CDF_INT1/CDF_BYTE → NC_BYTE
    - CDF_INT2 → NC_SHORT
    - CDF_INT4 → NC_INT
    - CDF_INT8 → NC_INT64
    - CDF_UINT1 → NC_UBYTE
    - CDF_UINT2 → NC_USHORT
    - CDF_UINT4 → NC_UINT
    - CDF_REAL4/CDF_FLOAT → NC_FLOAT
    - CDF_REAL8/CDF_DOUBLE → NC_DOUBLE
    - CDF_CHAR/CDF_UCHAR → NC_CHAR
  - Handle endianness detection and conversion
  - Set type size correctly for each mapped type
  - Support all common CDF types from the start
- **Metadata Helper Functions**: Implement stubbed functions in cdffile.c
  - `cdf_rec_grp_del()`: Recursively delete group metadata (for cleanup)
  - `cdf_type_info()`: Map CDF types to NetCDF types with size and endianness
  - `nc4_set_var_type()`: Set NetCDF variable type information
  - Additional helper functions as needed for dimension/variable/attribute creation
- **NC_CDF_close() Implementation**: Complete file close with proper cleanup
  - Close CDF file using CDFclose()
  - Free all allocated metadata structures
  - Clean up NC_CDF_FILE_INFO_T and NC_VAR_CDF_INFO_T structures
  - Ensure no memory leaks
- **Minimal Data Reading**: Implement basic NC_CDF_get_vara() for validation
  - Support reading scalar values and simple arrays
  - Use CDFgetVarData() or CDFhyperGetVarData() from NASA CDF library
  - Handle type conversion from CDF to NetCDF types
  - Validate data path works correctly
  - Full array slicing and advanced features can be deferred to future sprints
- **Error Handling**: Standard error handling for robustness
  - Validate file exists and is accessible
  - Check CDFopen() return status, return NC_ENOTNC4 if not a valid CDF file
  - Detect and reject unsupported CDF features (e.g., rVariables if not supported)
  - Map CDF error codes to appropriate NetCDF error codes
  - Return NC_NOERR on success
  - No error message printing (return codes only)
- **Test Program**: Create tst_cdf_udf.c to validate UDF functionality
  - Open CDF test file using NetCDF API (nc_open) via UDF dispatch
  - Query and validate file metadata using nc_inq functions:
    - Number of dimensions (nc_inq_ndims)
    - Number of variables (nc_inq_nvars)
    - Number of global attributes (nc_inq_natts)
    - Dimension names and sizes (nc_inq_dim)
    - Variable names, types, and dimensions (nc_inq_var)
    - Attribute names, types, and values (nc_inq_att, nc_get_att)
  - Use assertion-based validation with specific expected values
  - Read minimal data from each variable using nc_get_var or nc_get_vara
  - Validate data values match expected values from CDF file
  - Close file using nc_close
  - Return 0 on success, non-zero on failure
  - Test builds conditionally when CDF enabled in both CMake and Autotools
  - Integrate into CI test suites (CTest and make check)
- **Metadata Validation Strategy**: Assertion-based testing for automated validation
  - Test asserts specific metadata values (e.g., assert(ndims == 3))
  - Validate dimension names and sizes match CDF file
  - Validate variable names, types, and shapes match CDF file
  - Validate attribute names, types, and values match CDF file
  - Provide clear error messages on assertion failures
  - Automated pass/fail criteria for CI integration
- **Build System Integration**: Ensure test builds and runs in both systems
  - CMake: Add tst_cdf_udf to test/CMakeLists.txt when ENABLE_CDF=ON
  - Autotools: Add tst_cdf_udf to test/Makefile.am when HAVE_CDF=yes
  - Link test against NetCDF-C library (uses UDF, not direct CDF library)
  - Register test with CTest and make check
  - Test uses existing CDF test file from Sprint 2 (test/data/)
- **CI Integration**: Validate UDF works in CI environment
  - CI runs tst_cdf_udf for CDF-enabled builds
  - Test execution for both CMake and Autotools builds
  - Verify test passes and produces expected output
  - No changes to CI workflow needed (test runs automatically when CDF enabled)


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

 
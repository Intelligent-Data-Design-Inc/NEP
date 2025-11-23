# NEP – NetCDF Extension Pack v1.3.0
## Project Overview

NEP v1.3.0 extends the NetCDF Extension Pack to support NASA Common Data Format (CDF) through a User Defined Format (UDF) handler, enabling transparent access to CDF files through the standard NetCDF API.

## Architecture

The v1.3.0 architecture adds:
- CDF UDF handler using NC_Dispatch interface
- Integration with NASA CDF library v3.9.x
- Automatic CDF format detection via magic numbers
- Runtime registration via nc_def_user_format()

## Key Components

1. **CDF UDF Handler**: Translation layer between NetCDF API and NASA CDF library
2. **Format Detection**: Magic number-based CDF file identification
3. **NC_Dispatch Implementation**: File operations, metadata access, data reading

## Technical Requirements

- File open/close operations
- Metadata extraction from CDF files
- Data reading functionality
- Variable information access
- Integration with existing v1.0.0 and v2.0.0 features

## Dependencies

- All v1.0.0 and v2.0.0 dependencies
- NASA CDF library v3.9.x (built from source)
  - Source: https://spdf.gsfc.nasa.gov/pub/software/cdf/dist/latest/
  - Installation: Separate prefix directory (`$GITHUB_WORKSPACE/cdf-install` in CI)
  - Detection: AC_CHECK_HEADERS + AC_SEARCH_LIBS pattern (Autotools), find_package/find_library (CMake)

## v1.3.0 Preparatory Architecture

### Build System Integration
- **Optional Dependency**: CDF support disabled by default in v1.3.0
  - CMake: `-DENABLE_CDF=ON/OFF` (default: OFF)
  - Autotools: `--enable-cdf/--disable-cdf` (default: disabled)
- **Dependency Detection**: Mirrors GRIB2 pattern for consistency
  - Check for CDF headers (cdf.h)
  - Search for CDF library (libcdf)
  - Error if enabled but not found
- **Configuration Macros**: HAVE_CDF defined when enabled and found

### CI Integration
- **CDF Build Step**: Download and build NASA CDF library from source
- **Caching**: GitHub Actions cache for CDF build artifacts
- **Environment Setup**: CPPFLAGS, LDFLAGS, LD_LIBRARY_PATH, CMAKE_PREFIX_PATH
- **Build Matrix**: Single configuration with CDF enabled for validation

### Test Data
- **Location**: `test/data/` directory
- **File Type**: Small (<100KB) space physics data from NASA SPDF
- **Build Integration**: Copied to build directory for both CMake and Autotools

---

*Version: 1.3.0*
*Status: In planning*

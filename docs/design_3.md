# NEP – NetCDF Extension Pack v3.0.0
## Project Overview

NEP v3.0.0 will extend the NetCDF Extension Pack to support NASA Common Data Format (CDF) through a User Defined Format (UDF) handler, enabling transparent access to CDF files through the standard NetCDF API.

## Architecture (Planned)

The v3.0.0 architecture will add:
- CDF UDF handler using NC_Dispatch interface
- Integration with NASA CDF library
- Automatic CDF format detection via magic numbers
- Runtime registration via nc_def_user_format()

## Key Components (To Be Developed)

1. **CDF UDF Handler**: Translation layer between NetCDF API and NASA CDF library
2. **Format Detection**: Magic number-based CDF file identification
3. **NC_Dispatch Implementation**: File operations, metadata access, data reading

## Technical Requirements (Planned)

- File open/close operations
- Metadata extraction from CDF files
- Data reading functionality
- Variable information access
- Integration with existing v1.0.0 and v2.0.0 features

## Dependencies

- All v1.0.0 and v2.0.0 dependencies
- NASA CDF library

---

*Version: 3.0.0 (Planning)*
*Status: Not yet implemented*

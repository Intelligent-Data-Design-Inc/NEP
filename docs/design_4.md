# NEP – NetCDF Extension Pack v4.0.0
## Project Overview

NEP v4.0.0 will extend the NetCDF Extension Pack to support GeoTIFF geospatial data format through a User Defined Format (UDF) handler, enabling transparent access to GeoTIFF files through the standard NetCDF API.

## Architecture (Planned)

The v4.0.0 architecture will add:
- GeoTIFF UDF handler using NC_Dispatch interface
- Integration with libgeotiff library
- Automatic GeoTIFF format detection via magic numbers
- Runtime registration via nc_def_user_format()

## Key Components (To Be Developed)

1. **GeoTIFF UDF Handler**: Translation layer between NetCDF API and libgeotiff
2. **Format Detection**: Magic number-based GeoTIFF file identification
3. **NC_Dispatch Implementation**: File operations, metadata access, data reading
4. **Geospatial Metadata**: Projection and coordinate information handling

## Technical Requirements (Planned)

- File open/close operations
- Metadata and projection information extraction
- Data reading functionality
- Geospatial coordinate information access
- Integration with existing v1.0.0, v2.0.0, and v3.0.0 features

## Dependencies

- All v1.0.0, v2.0.0, and v3.0.0 dependencies
- libgeotiff library

---

*Version: 4.0.0 (Planning)*
*Status: Not yet implemented*

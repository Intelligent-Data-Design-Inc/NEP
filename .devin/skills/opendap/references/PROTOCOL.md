# OPeNDAP Protocol Details

This document provides detailed information about the Data Access Protocol (DAP) versions 2 and 4.

## DAP2 Protocol

DAP2 is the original OPeNDAP protocol, widely supported and stable.

### DAP2 Responses

**Dataset Descriptor Structure (DDS)**:
- Describes the "shape" of the data
- C-like syntax showing variables, dimensions, and types
- Accessed via `.dds` suffix

Example DDS:
```
Dataset {
    Grid {
        Array:
            Int16 sst[time = 1857][lat = 89][lon = 180];
        Maps:
            Float64 time[time = 1857];
            Float64 lat[lat = 89];
            Float64 lon[lon = 180];
    } sst;
    Float64 time_bnds[time = 1857][nbnds = 2];
} sst.mnmean.nc;
```

**Data Attribute Structure (DAS)**:
- Contains metadata about variables
- Includes units, descriptions, valid ranges, etc.
- Accessed via `.das` suffix

Example DAS:
```
Attributes {
    sst {
        String long_name "Monthly Means of Sea Surface Temperature";
        String units "degC";
        Float32 scale_factor 0.01;
        Float32 add_offset 0.0;
        Int16 missing_value 32767;
        String valid_range "-500, 4500";
    }
    NC_GLOBAL {
        String title "COADS 1-degree Equatorial Enhanced";
        String history "Created by NOAA/PMEL";
    }
}
```

**Binary Data (.dods)**:
- XDR-encoded binary data
- Includes DDS followed by data values
- Most efficient for data transfer

### DAP2 Data Types

**Simple Types**:
- Byte, Int16, Int32
- UInt16, UInt32
- Float32, Float64
- String, URL

**Constructor Types**:
- Array - Multi-dimensional arrays
- Structure - Named collection of variables
- Sequence - Ordered list of structures
- Grid - Array with coordinate maps

### DAP2 Constraint Expression Format

**Projection** (what to return):
```
?var1,var2,var3
?structure.field1,structure.field2
?array[start:stop]
```

**Selection** (filtering):
```
?var&var>value
?sequence&sequence.field<100
```

## DAP4 Protocol

DAP4 is the enhanced protocol with improved features and performance.

### DAP4 Responses

**Dataset Metadata Response (DMR)**:
- Unified XML document combining structure and attributes
- Accessed via `.dmr.xml` suffix
- Supports groups (hierarchical organization)

Example DMR:
```xml
<Dataset name="sst.mnmean.nc">
    <Dimension name="time" size="1857"/>
    <Dimension name="lat" size="89"/>
    <Dimension name="lon" size="180"/>
    
    <Int16 name="sst">
        <Dim name="/time"/>
        <Dim name="/lat"/>
        <Dim name="/lon"/>
        <Attribute name="long_name" type="String">
            <Value>Monthly Means of Sea Surface Temperature</Value>
        </Attribute>
        <Attribute name="units" type="String">
            <Value>degC</Value>
        </Attribute>
        <Attribute name="scale_factor" type="Float32">
            <Value>0.01</Value>
        </Attribute>
    </Int16>
    
    <Float64 name="time">
        <Dim name="/time"/>
        <Attribute name="units" type="String">
            <Value>days since 1800-1-1 00:00:00</Value>
        </Attribute>
    </Float64>
</Dataset>
```

**Binary Data (.dap)**:
- More efficient encoding than DAP2
- Chunked transfer for large datasets
- Better compression support

### DAP4 Enhancements

**Groups**:
- Hierarchical organization like HDF5
- Namespace management
- Example: `/group1/subgroup/variable`

**Enhanced Types**:
- Opaque - Binary blobs
- Enum - Enumerated types
- 64-bit integers (Int64, UInt64)

**Improved Constraint Expressions**:
```
?dap4.ce=/variable[0:10]
?dap4.ce=/group/variable&/group/variable>100
```

### DAP4 Constraint Expression Format

**Projection with namespace**:
```
?dap4.ce=/sst
?dap4.ce=/gt3r/heights/delta_time,/gt3r/heights/lon_ph
```

**Filters**:
```
?dap4.ce=/sst[0:100][0:50][0:80]
?dap4.ce=/sequence{field1,field2|field1>100}
```

## Protocol Comparison

| Feature | DAP2 | DAP4 |
|---------|------|------|
| Metadata | Separate DDS/DAS | Unified DMR (XML) |
| Groups | No | Yes |
| 64-bit integers | No | Yes |
| Encoding | XDR | More efficient |
| Chunking | Limited | Full support |
| Constraint syntax | Simple | Enhanced |
| Adoption | Universal | Growing |

## Choosing DAP Version

**Use DAP2 when**:
- Maximum compatibility needed
- Working with older servers
- Simple data structures
- Established workflows

**Use DAP4 when**:
- Working with grouped data (HDF5-like)
- Need 64-bit integer support
- Large dataset performance critical
- Server supports DAP4

## HTTP Details

### Request Methods

**GET requests** for all operations:
```
GET /path/to/dataset.nc.dds HTTP/1.1
Host: server.domain
```

### Response Headers

```
Content-Type: application/vnd.opendap.dds
Content-Type: application/vnd.opendap.das
Content-Type: application/vnd.opendap.data
Content-Type: text/xml (for DMR)
```

### Authentication

**Basic HTTP Auth**:
```
http://username:password@server.org/data.nc
```

**.netrc file** (recommended):
```
machine server.org
login username
password mypassword
```

**OAuth/Bearer tokens** (server-dependent):
```
Authorization: Bearer <token>
```

## Error Handling

### DAP2 Errors

Errors returned as text with HTTP error codes:
```
Error {
    code = 404;
    message = "Variable 'xyz' not found";
};
```

### DAP4 Errors

XML error responses:
```xml
<Error>
    <Message>Variable not found: xyz</Message>
    <Context>Dataset: /data/file.nc</Context>
    <OtherInformation>Check variable name spelling</OtherInformation>
</Error>
```

### Common HTTP Status Codes

- **200 OK** - Success
- **400 Bad Request** - Invalid constraint expression
- **401 Unauthorized** - Authentication required
- **404 Not Found** - Dataset or variable not found
- **500 Internal Server Error** - Server-side error

## Performance Considerations

### Bandwidth Optimization

1. **Use constraint expressions** to subset at server
2. **Request only needed variables** in projection
3. **Use appropriate stride** for sampling
4. **Leverage server functions** for processing

### Caching

Servers may cache:
- Metadata responses (DDS, DAS, DMR)
- Constraint expression results
- Aggregated datasets

Clients should cache:
- Metadata for repeated access
- Frequently accessed subsets

### Compression

DAP4 supports:
- Chunked transfer encoding
- Gzip compression
- Server-side compression filters

Request compressed responses:
```
Accept-Encoding: gzip, deflate
```

## Server Capabilities

### Version Detection

Query server version:
```
http://server.org/opendap/version
```

### Function Discovery

List available server functions:
```
http://server.org/data.nc?version()
```

Get function help:
```
http://server.org/data.nc?function_name()
```

### Service Endpoints

Standard endpoints:
- `.dds` - DAP2 structure
- `.das` - DAP2 attributes
- `.dods` - DAP2 binary data
- `.dmr.xml` - DAP4 metadata
- `.dap` - DAP4 binary data
- `.ascii` - ASCII representation
- `.html` - Web form
- `.info` - Combined info page
- `.ver` - Version information

## Protocol Extensions

### Server-Side Processing

Some servers support:
- **Aggregation** - Combine multiple files
- **Subsetting** - Spatial/temporal subsetting
- **Transformation** - Unit conversion, reprojection
- **Statistics** - Mean, min, max calculations

### Custom Functions

Servers can implement custom functions:
```
?custom_function(variable, param1, param2)
```

Check server documentation for available functions.

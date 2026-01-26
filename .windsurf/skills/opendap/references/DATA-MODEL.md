# OPeNDAP Data Model

This document provides comprehensive documentation of the OPeNDAP data model and type system.

## Overview

The OPeNDAP data model is designed to be general enough to represent data from various storage formats (NetCDF, HDF5, relational databases, etc.) while being specific enough to preserve the structure and relationships in the data.

## Base Types

Base types represent atomic data values.

### Numeric Types

**Byte** (8-bit signed integer):
```
Byte temperature;
Range: -128 to 127
```

**Int16** (16-bit signed integer):
```
Int16 elevation;
Range: -32,768 to 32,767
```

**Int32** (32-bit signed integer):
```
Int32 station_id;
Range: -2,147,483,648 to 2,147,483,647
```

**Int64** (64-bit signed integer, DAP4 only):
```
Int64 timestamp;
Range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
```

**UInt16** (16-bit unsigned integer):
```
UInt16 count;
Range: 0 to 65,535
```

**UInt32** (32-bit unsigned integer):
```
UInt32 pixel_value;
Range: 0 to 4,294,967,295
```

**UInt64** (64-bit unsigned integer, DAP4 only):
```
UInt64 file_size;
Range: 0 to 18,446,744,073,709,551,615
```

**Float32** (32-bit floating point):
```
Float32 temperature;
IEEE 754 single precision
```

**Float64** (64-bit floating point):
```
Float64 latitude;
IEEE 754 double precision
```

### String Types

**String** (variable-length character string):
```
String station_name;
String comment;
```

**URL** (Uniform Resource Locator):
```
URL data_source;
```

## Constructor Types

Constructor types build complex data structures from base types and other constructors.

### Array

Multi-dimensional arrays of any base or constructor type.

**Syntax**:
```
Type name[dim1][dim2]...[dimN];
```

**Examples**:
```
Float32 temperature[time=100][lat=50][lon=80];
Int16 elevation[y=1000][x=1000];
String station_names[stations=25];
```

**Characteristics**:
- Fixed dimensions at creation
- Homogeneous (all elements same type)
- Zero-indexed
- Rectangular (not ragged)

**Subsetting**:
```
temperature[0:10][20:30][40:50]  # Hyperslab
temperature[5][25][60]            # Single element
temperature[0:2:100]              # With stride
```

### Structure

Named collection of variables (like a C struct).

**Syntax**:
```
Structure {
    Type1 field1;
    Type2 field2;
    ...
} name;
```

**Example**:
```
Structure {
    Float64 latitude;
    Float64 longitude;
    Int32 elevation;
    String name;
} station;
```

**Characteristics**:
- Heterogeneous (different types)
- Named fields
- Accessed via dot notation: `station.latitude`

**Use Cases**:
- Grouping related metadata
- Station information
- Coordinate pairs
- Complex data records

### Sequence

Ordered collection of instances (like database rows).

**Syntax**:
```
Sequence {
    Type1 field1;
    Type2 field2;
    ...
} name;
```

**Example**:
```
Sequence {
    Float64 time;
    Float64 depth;
    Float32 temperature;
    Float32 salinity;
} cast;
```

**Characteristics**:
- Variable length (number of instances unknown)
- Each instance is a Structure
- Can be filtered with selection expressions
- Accessed one instance at a time

**Nested Sequences**:
```
Sequence {
    Int32 station_id;
    Float64 latitude;
    Float64 longitude;
    Sequence {
        Float64 depth;
        Float32 temperature;
        Float32 salinity;
    } measurements;
} station;
```

**Filtering**:
```
?cast&cast.temperature>20
?cast.depth,cast.temperature&cast.depth<100
```

### Grid

Array with coordinate map vectors (georeferenced data).

**Syntax**:
```
Grid {
    Array:
        Type array_name[dim1][dim2]...[dimN];
    Maps:
        Type1 map1[dim1];
        Type2 map2[dim2];
        ...
} name;
```

**Example**:
```
Grid {
    Array:
        Int16 sst[time=1857][lat=89][lon=180];
    Maps:
        Float64 time[time=1857];
        Float64 lat[lat=89];
        Float64 lon[lon=180];
} sst;
```

**Characteristics**:
- Combines array with coordinate variables
- Maps provide independent variable values
- Common for geospatial data
- Subsetting returns both array and corresponding maps

**Use Cases**:
- Gridded climate data
- Satellite imagery
- Model output
- Any regularly or irregularly spaced data

## DAP4 Extensions

DAP4 adds several enhancements to the data model.

### Groups

Hierarchical organization of variables (like HDF5 groups).

**Example**:
```
Group {
    Float64 time[time=100];
    
    Group heights {
        Float32 delta_time[time=100];
        Float64 lat_ph[time=100];
        Float64 lon_ph[time=100];
    }
    
    Group quality {
        Int32 qa_flag[time=100];
    }
} gt3r;
```

**Access**:
```
?/gt3r/heights/lat_ph
?/gt3r/quality/qa_flag
```

### Opaque

Binary data of unknown structure.

**Syntax**:
```
Opaque name;
```

**Use Cases**:
- Embedded images
- Compressed data
- Proprietary formats
- Binary metadata

### Enumeration

Named integer constants (like C enum).

**Syntax**:
```
Enum quality_flag {
    good = 0,
    questionable = 1,
    bad = 2,
    missing = 3
};
```

**Usage**:
```
quality_flag qa[time=100];
```

## Data Model Translation

### From NetCDF-3 to OPeNDAP

**NetCDF-3 Dimensions** → **OPeNDAP Array dimensions**:
```
NetCDF: float temp(time, lat, lon);
OPeNDAP: Float32 temp[time][lat][lon];
```

**NetCDF-3 Variables** → **OPeNDAP Grid** (if coordinate variables exist):
```
NetCDF:
  float sst(time, lat, lon);
  double time(time);
  float lat(lat);
  float lon(lon);

OPeNDAP:
  Grid {
    Array: Float32 sst[time][lat][lon];
    Maps: Float64 time[time];
          Float32 lat[lat];
          Float32 lon[lon];
  } sst;
```

**NetCDF-3 Attributes** → **OPeNDAP DAS**:
```
NetCDF: sst:units = "degC";
OPeNDAP DAS:
  sst {
    String units "degC";
  }
```

### From HDF5 to OPeNDAP

**HDF5 Groups** → **DAP4 Groups**:
```
HDF5: /group1/subgroup/dataset
DAP4: /group1/subgroup/dataset
```

**HDF5 Datasets** → **OPeNDAP Arrays**:
```
HDF5: Dataset "temperature" [100][50][80]
DAP4: Float32 temperature[100][50][80];
```

**HDF5 Compound Types** → **OPeNDAP Structures**:
```
HDF5 Compound:
  {
    "lat": H5T_NATIVE_DOUBLE,
    "lon": H5T_NATIVE_DOUBLE,
    "elev": H5T_NATIVE_INT
  }

OPeNDAP:
  Structure {
    Float64 lat;
    Float64 lon;
    Int32 elev;
  }
```

### From Relational Database to OPeNDAP

**Database Table** → **OPeNDAP Sequence**:
```
SQL Table:
  CREATE TABLE stations (
    id INT,
    lat DOUBLE,
    lon DOUBLE,
    name VARCHAR(50)
  );

OPeNDAP:
  Sequence {
    Int32 id;
    Float64 lat;
    Float64 lon;
    String name;
  } stations;
```

**SQL WHERE** → **OPeNDAP Selection**:
```
SQL: SELECT * FROM stations WHERE lat > 0;
OPeNDAP: ?stations&stations.lat>0
```

## Data Type Sizes

| Type | Size (bytes) | Notes |
|------|--------------|-------|
| Byte | 1 | Signed |
| Int16 | 2 | Signed |
| Int32 | 4 | Signed |
| Int64 | 8 | Signed, DAP4 only |
| UInt16 | 2 | Unsigned |
| UInt32 | 4 | Unsigned |
| UInt64 | 8 | Unsigned, DAP4 only |
| Float32 | 4 | IEEE 754 |
| Float64 | 8 | IEEE 754 |
| String | Variable | Null-terminated |
| URL | Variable | Null-terminated |

## Attributes

Attributes provide metadata about variables and datasets.

### Attribute Types

All base types can be used as attributes:
```
String long_name "Sea Surface Temperature";
String units "degrees_C";
Float32 valid_min -5.0;
Float32 valid_max 45.0;
Int32 missing_value -9999;
```

### Attribute Containers

**Variable attributes**:
```
sst {
    String long_name "Sea Surface Temperature";
    String units "degC";
    Float32 scale_factor 0.01;
}
```

**Global attributes**:
```
NC_GLOBAL {
    String title "COADS 1-degree Enhanced";
    String institution "NOAA/PMEL";
    String history "Created 2020-01-15";
}
```

### Standard Attributes

**CF Conventions**:
- `units` - Physical units
- `long_name` - Descriptive name
- `standard_name` - CF standard name
- `valid_min`, `valid_max` - Valid range
- `_FillValue` - Missing data indicator
- `scale_factor`, `add_offset` - Packing parameters

## Complex Data Examples

### Time Series Station Data

```
Dataset {
    Sequence {
        Int32 station_id;
        Float64 latitude;
        Float64 longitude;
        String station_name;
        Sequence {
            Float64 time;
            Float32 temperature;
            Float32 salinity;
            Float32 pressure;
            Int32 quality_flag;
        } measurements;
    } stations;
} ocean_observations;
```

### Satellite Imagery with Metadata

```
Dataset {
    Structure {
        String satellite_name;
        String sensor_type;
        Float64 acquisition_time;
        Structure {
            Float64 upper_left_lat;
            Float64 upper_left_lon;
            Float64 lower_right_lat;
            Float64 lower_right_lon;
        } bounds;
    } metadata;
    
    Grid {
        Array:
            UInt16 radiance[line=1000][pixel=1000];
        Maps:
            Float32 latitude[line=1000];
            Float32 longitude[pixel=1000];
    } image;
} satellite_scene;
```

### Climate Model Output

```
Dataset {
    Grid {
        Array:
            Float32 temperature[time=365][level=50][lat=180][lon=360];
        Maps:
            Float64 time[time=365];
            Float32 level[level=50];
            Float32 lat[lat=180];
            Float32 lon[lon=360];
    } air_temperature;
    
    Grid {
        Array:
            Float32 u_wind[time=365][level=50][lat=180][lon=360];
        Maps:
            Float64 time[time=365];
            Float32 level[level=50];
            Float32 lat[lat=180];
            Float32 lon[lon=360];
    } eastward_wind;
} climate_model;
```

## Best Practices

### Choosing Data Types

1. **Use Grid for georeferenced data** with coordinate variables
2. **Use Sequence for variable-length data** (stations, profiles)
3. **Use Structure for related metadata** (location, time, etc.)
4. **Use appropriate numeric precision** (Float32 vs Float64)
5. **Use String for text data** (names, comments)

### Data Organization

1. **Group related variables** in Structures
2. **Use meaningful names** for variables and fields
3. **Include coordinate variables** for arrays
4. **Add comprehensive attributes** for metadata
5. **Follow conventions** (CF, COARDS, etc.)

### Performance Considerations

1. **Array layout matters** - row-major vs column-major
2. **Sequence access is sequential** - can't random access
3. **Grid subsetting is efficient** - server-side processing
4. **Structure access is fast** - all fields together
5. **String data is variable-length** - can impact performance

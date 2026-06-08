# NetCDF-Java Library

## Overview
The netCDF-Java library is a 100% Java framework for reading and writing scientific data formats. It implements the Common Data Model (CDM), which is an abstract data model that merges netCDF, OPeNDAP, and HDF5 data models to create a unified API for accessing many types of scientific data.

**Key Capabilities:**
- Read netCDF-3, netCDF-4, HDF5, GRIB, BUFR, and many other scientific data formats
- Write netCDF-3 files natively
- Write netCDF-4 files via JNI to netCDF-C library
- Access remote datasets via OPeNDAP and other protocols
- Support for NcML (NetCDF Markup Language) for metadata manipulation and aggregation
- Coordinate system identification and georeferencing
- Scientific feature type support (grids, point data, radial data, etc.)

## Documentation and Resources
- **GitHub Repository:** https://github.com/Unidata/netcdf-java
- **Main Documentation:** https://docs.unidata.ucar.edu/netcdf-java/current/userguide/
- **API Reference:** Available through Maven artifacts
- **License:** BSD-3 (as of version 5.0)
- **Maven Repository:** https://artifacts.unidata.ucar.edu/

## Common Data Model (CDM) Architecture

The CDM has three layers that build on each other:

### 1. Data Access Layer (Syntactic Layer)
Handles data reading and writing through:
- **NetcdfFile:** Read-only access to datasets
- **NetcdfFiles:** Static methods for opening files
- **IOServiceProvider:** Interface for format-specific implementations
- **Variable, Dimension, Attribute, Group, Structure:** Metadata objects

### 2. Coordinate System Layer
Identifies coordinates of data arrays:
- General coordinate concepts for scientific data
- Specialized georeferencing coordinate systems for Earth Science
- CoordinateAxis and CoordinateSystem objects

### 3. Scientific Feature Types Layer
Specialized methods for specific data types:
- Grids
- Point data
- Radial data (radar, lidar)
- Station data
- Trajectory data

## Basic Usage Patterns

### Opening and Reading Files

```java
// Open a NetCDF file
try (NetcdfFile ncfile = NetcdfFiles.open(pathToFile)) {
    // File is automatically closed when try block exits
    
    // Find a variable by name
    Variable v = ncfile.findVariable("temperature");
    if (v == null) {
        System.err.println("Variable not found");
        return;
    }
    
    // Read all data from the variable
    Array data = v.read();
    
    // Read a subset using section specification
    // Format: "dim1_start:dim1_end:dim1_stride, dim2_start:dim2_end, ..."
    Array subset = v.read("0:10:2, :, 5");
    
} catch (IOException e) {
    e.printStackTrace();
}
```

### Working with Metadata

```java
try (NetcdfFile ncfile = NetcdfFiles.open(pathToFile)) {
    // List all variables
    for (Variable var : ncfile.getVariables()) {
        System.out.println("Variable: " + var.getFullName());
        System.out.println("  Type: " + var.getDataType());
        System.out.println("  Shape: " + Arrays.toString(var.getShape()));
        
        // Get attributes
        for (Attribute attr : var.attributes()) {
            System.out.println("  Attribute: " + attr.getFullName() + " = " + attr.getValue());
        }
    }
    
    // List dimensions
    for (Dimension dim : ncfile.getDimensions()) {
        System.out.println("Dimension: " + dim.getFullName() + " = " + dim.getLength());
    }
    
    // Get global attributes
    for (Attribute attr : ncfile.getGlobalAttributes()) {
        System.out.println("Global: " + attr.getFullName() + " = " + attr.getValue());
    }
    
} catch (IOException e) {
    e.printStackTrace();
}
```

### Reading Data Arrays

```java
// Read scalar data
Variable scalarVar = ncfile.findVariable("scalar_value");
double scalarValue = scalarVar.readScalarDouble();

// Read 1D array
Variable var1d = ncfile.findVariable("time");
Array timeData = var1d.read();
int[] shape = timeData.getShape();
for (int i = 0; i < shape[0]; i++) {
    double value = timeData.getDouble(i);
    System.out.println("time[" + i + "] = " + value);
}

// Read multidimensional array
Variable var3d = ncfile.findVariable("temperature");
Array tempData = var3d.read();
Index index = tempData.getIndex();
int[] shape3d = tempData.getShape();
for (int t = 0; t < shape3d[0]; t++) {
    for (int y = 0; y < shape3d[1]; y++) {
        for (int x = 0; x < shape3d[2]; x++) {
            double value = tempData.getDouble(index.set(t, y, x));
        }
    }
}
```

### Array Section Syntax

NetCDF-Java uses Fortran 90 array section syntax with zero-based indexing:
- `":"` - all elements in dimension
- `"start:end"` - elements from start to end (inclusive)
- `"start:end:stride"` - elements with stride
- Example: `"0:10:2, :, 5"` means first dimension 0-10 with stride 2, all of second dimension, element 5 of third dimension

## NetCDF Markup Language (NcML)

NcML is an XML representation of netCDF metadata that can:
- Describe netCDF file structure (similar to CDL)
- Modify existing datasets (add/change attributes, variables)
- Create virtual datasets through aggregation
- Define coordinate systems

### Basic NcML Example

```xml
<?xml version="1.0" encoding="UTF-8"?>
<netcdf xmlns="http://www.unidata.ucar.edu/namespaces/netcdf/ncml-2.2">
  <attribute name="title" value="Modified Dataset" />
  
  <variable name="temperature">
    <attribute name="units" value="Celsius" />
    <attribute name="long_name" value="Air Temperature" />
  </variable>
</netcdf>
```

### NcML Aggregation

NcML supports several aggregation types:
- **joinExisting:** Concatenate along existing dimension
- **joinNew:** Create new dimension for aggregation
- **union:** Combine variables from multiple files
- **tiled:** Aggregate multidimensional tiles

```xml
<netcdf xmlns="http://www.unidata.ucar.edu/namespaces/netcdf/ncml-2.2">
  <aggregation dimName="time" type="joinExisting">
    <scan location="/data/model/" suffix=".nc" />
  </aggregation>
</netcdf>
```

## Advanced Features

### Opening Remote Files

```java
// OPeNDAP URL
NetcdfFile ncfile = NetcdfFiles.open("https://server.org/dods/dataset");

// HTTP Server
NetcdfFile ncfile = NetcdfFiles.open("https://server.org/data/file.nc");

// AWS S3
NetcdfFile ncfile = NetcdfFiles.open("cdms3://bucket-name/path/to/file.nc");
```

### Using NetcdfDataset for Enhanced Features

```java
// NetcdfDataset provides coordinate system support and NcML processing
try (NetcdfDataset ncd = NetcdfDatasets.openDataset(pathToFile)) {
    // Access coordinate systems
    for (CoordinateSystem cs : ncd.getCoordinateSystems()) {
        System.out.println("Coordinate System: " + cs.getName());
        for (CoordinateAxis axis : cs.getCoordinateAxes()) {
            System.out.println("  Axis: " + axis.getFullName());
        }
    }
}
```

### Disk Caching

NetCDF-Java automatically handles compressed files (.Z, .zip, .gzip, .gz, .bz2) by uncompressing them to a disk cache before opening.

## File Format Support

The library can read many formats through IOServiceProvider implementations:
- NetCDF-3 (classic and 64-bit offset)
- NetCDF-4 (HDF5-based)
- HDF4 and HDF5
- GRIB (GRIB-1 and GRIB-2)
- BUFR
- NEXRAD Level 2 and Level 3
- OPeNDAP (DAP2 and DAP4)
- Many others

**Note:** Some formats require optional modules to be included as Maven/Gradle artifacts.

## Maven/Gradle Integration

### Maven Example

```xml
<dependency>
  <groupId>edu.ucar</groupId>
  <artifactId>cdm-core</artifactId>
  <version>5.5.3</version>
</dependency>

<!-- For all formats in one artifact -->
<dependency>
  <groupId>edu.ucar</groupId>
  <artifactId>netcdfAll</artifactId>
  <version>5.5.3</version>
</dependency>
```

### Gradle Example

```gradle
dependencies {
    implementation 'edu.ucar:cdm-core:5.5.3'
    // or for all formats
    implementation 'edu.ucar:netcdfAll:5.5.3'
}
```

## ToolsUI Application

ToolsUI is a graphical application for browsing and debugging NetCDF files:
- Download: `toolsUI.jar` from netCDF-Java downloads page
- Run: `java -Xmx1g -jar toolsUI.jar`
- Features: Browse metadata, view data, test coordinate systems, debug IOSPs

## Best Practices

1. **Always use try-with-resources** to ensure files are properly closed
2. **Read metadata first** - structural metadata is loaded at open time, data is lazy-loaded
3. **Use section specifications** to read subsets of large arrays
4. **Check for null** when finding variables or attributes
5. **Use NetcdfDataset** when you need coordinate system support
6. **Cache remote files** for better performance with repeated access
7. **Use appropriate data types** - Array provides type-specific getters (getDouble, getFloat, etc.)

## Common Pitfalls

- **Zero-based indexing:** Unlike Fortran, Java uses zero-based array indexing
- **Write limitations:** Native Java can only write netCDF-3; netCDF-4 requires C library via JNI
- **Module dependencies:** Some file formats require additional Maven artifacts
- **Memory management:** Large arrays can consume significant memory; use sections when possible
- **Thread safety:** NetcdfFile objects are not thread-safe; use one per thread or synchronize access

## Integration with THREDDS

The THREDDS Data Server (TDS) is built on top of netCDF-Java and provides:
- Remote data access via OPeNDAP, WCS, WMS, HTTP
- Catalog services for dataset discovery
- Aggregation and virtual dataset support
- Metadata services

## Version History

- **Version 5.x:** Decoupled from TDS, BSD-3 license, modular architecture
- **Version 4.6 and earlier:** Combined with TDS in single repository
- **Current target:** Java 8 (community feedback being gathered for future versions)

## When to Use NetCDF-Java

Use netCDF-Java when you need to:
- Read scientific data in Java applications
- Support multiple file formats with a single API
- Work with remote datasets (OPeNDAP, HTTP, S3)
- Manipulate metadata without rewriting files (NcML)
- Aggregate multiple files into virtual datasets
- Access coordinate system information
- Build web services for scientific data (with THREDDS)

## Related Technologies

- **netCDF-C:** C library for netCDF, can be called via JNI for netCDF-4 writing
- **THREDDS Data Server:** Web server built on netCDF-Java
- **OPeNDAP:** Protocol for remote data access
- **CF Conventions:** Metadata conventions for climate and forecast data
- **NcML:** XML language for netCDF metadata and aggregation

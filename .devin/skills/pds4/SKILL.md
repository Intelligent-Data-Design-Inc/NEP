# PDS4 Skill

## Overview

**PDS4** (Planetary Data System version 4) is the data standard maintained by
NASA's Planetary Data System (PDS) and the European Space Agency (ESA).  It is
used to archive and distribute solar-system science data: spacecraft images,
spectrometer tables, radar cubes, telemetry, and more.

PDS4 superseded the older PDS3 standard.  The PDS4 schema and tools are managed
at https://pds.nasa.gov/ and the formal Information Model is published at
https://pds.nasa.gov/datastandards/documents/current-version/.

---

## Key Concept: Label / Data File Separation

Every PDS4 data product consists of two parts:

1. **XML label file** (`*.xml`) — human- and machine-readable metadata that
   fully describes the product, its structure, and its provenance.  The label is
   always a well-formed XML document conforming to the PDS4 schema namespace
   `http://pds.nasa.gov/pds4/pds/v1`.

2. **Data file(s)** — the raw binary or ASCII payload referenced by the label.
   Common extensions: `.img`, `.dat`, `.tab`, `.fits`, `.qub`.  A label may
   reference more than one data file (multi-file products).

The label is the entry point.  NEP opens the XML label and then resolves the
referenced data files relative to the label's directory.

---

## PDS4 Information Model — Core Classes

| PDS4 Class | Description | netCDF mapping |
|---|---|---|
| `Array` | N-dimensional numeric array | Variable + dimensions |
| `Array_2D_Image` | 2-D image (special case of Array) | 2-D variable |
| `Table_Binary` | Fixed-record binary table | Group with one variable per Field |
| `Table_Character` | Fixed-record ASCII table | Group with one variable per Field |
| `Table_Delimited` | CSV-like table | Group with one variable per Field |
| `File_Area_*` | Container that links a data class to a file | — |
| `Observation_Area` | Provenance, target, instrument metadata | Global attributes |
| `Product_Observational` | Root label element for science products | Root group |

---

## XML Label Structure

A minimal PDS4 label looks like:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<?xml-model href="https://pds.nasa.gov/pds4/pds/v1/PDS4_PDS_1N00.sch"
    schematypens="http://purl.oclc.org/dsdl/schematron"?>
<Product_Observational
    xmlns="http://pds.nasa.gov/pds4/pds/v1"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://pds.nasa.gov/pds4/pds/v1
        https://pds.nasa.gov/pds4/pds/v1/PDS4_PDS_1N00.xsd">

  <Identification_Area>
    <logical_identifier>urn:nasa:pds:bundle:collection:product</logical_identifier>
    <version_id>1.0</version_id>
    <title>My Product</title>
    <information_model_version>1.23.0.0</information_model_version>
    <product_class>Product_Observational</product_class>
  </Identification_Area>

  <Observation_Area>
    <!-- target, instrument, time info -->
  </Observation_Area>

  <File_Area_Observational>
    <File>
      <file_name>image.img</file_name>
    </File>
    <Array_2D_Image>
      <axes>2</axes>
      <axis_index_order>Last Index Fastest</axis_index_order>
      <Element_Array>
        <data_type>IEEE754MSBSingle</data_type>
      </Element_Array>
      <Axis_Array>
        <axis_name>Line</axis_name>
        <elements>256</elements>
        <sequence_number>1</sequence_number>
      </Axis_Array>
      <Axis_Array>
        <axis_name>Sample</axis_name>
        <elements>256</elements>
        <sequence_number>2</sequence_number>
      </Axis_Array>
    </Array_2D_Image>
  </File_Area_Observational>

</Product_Observational>
```

---

## Magic Number Detection

PDS4 label files are XML and always begin with the XML declaration:

```
<?xml
```

NEP uses `<?xml` (5 bytes, `NEP_MAGIC_PDS4`) as the magic number for automatic
format detection in the NetCDF dispatch layer.

**Caveat**: Any XML file will match this magic number.  The PDS4 handler must
further confirm that the root element belongs to the PDS4 namespace
(`http://pds.nasa.gov/pds4/pds/v1`) before accepting the file.  Files that
match the magic but fail this check should return `NC_ENOTNC`.

---

## Mapping PDS4 to the netCDF-4 Model

| PDS4 concept | netCDF-4 mapping |
|---|---|
| `Product_Observational` (root) | Root group |
| `Identification_Area` elements | Global attributes (string) |
| `Observation_Area` elements | Global attributes (string) |
| Each `File_Area_*` | Child group (named from `File/file_name`) |
| `Array` / `Array_2D_Image` | Variable in the containing group |
| `Axis_Array` | Dimension (name from `axis_name`, length from `elements`) |
| `Element_Array/data_type` | `nc_type` (see table below) |
| `Table_*/Record_*/Field_*` | Variable per field (name from `Field_*/name`) |
| Field `unit` | `units` attribute |

### Implementation details

- `Identification_Area` direct children are stored as root-level global string attributes.
- `Observation_Area` is flattened recursively: each leaf text element becomes a global string attribute; duplicate names are skipped to keep the mapping deterministic.
- `Axis_Array` entries are sorted by `sequence_number`; the resulting order is the netCDF dimension order (leftmost = slowest).
- `Array` / `Array_2D_Image` variables are named from the `name` element if present, otherwise default to `data`.
- Byte order is recorded from the MSB/LSB prefix of the PDS4 `data_type` and is used by the data reader.

### PDS4 data_type → nc_type mapping

| PDS4 `data_type` | `nc_type` | Notes |
|---|---|---|
| `IEEE754MSBSingle` | `NC_FLOAT` | Big-endian 32-bit float |
| `IEEE754MSBDouble` | `NC_DOUBLE` | Big-endian 64-bit float |
| `IEEE754LSBSingle` | `NC_FLOAT` | Little-endian 32-bit float |
| `IEEE754LSBDouble` | `NC_DOUBLE` | Little-endian 64-bit float |
| `UnsignedByte` | `NC_UBYTE` | 8-bit unsigned int |
| `SignedByte` | `NC_BYTE` | 8-bit signed int |
| `UnsignedMSB2` / `UnsignedLSB2` | `NC_USHORT` | 16-bit unsigned int |
| `SignedMSB2` / `SignedLSB2` | `NC_SHORT` | 16-bit signed int |
| `UnsignedMSB4` / `UnsignedLSB4` | `NC_UINT` | 32-bit unsigned int |
| `SignedMSB4` / `SignedLSB4` | `NC_INT` | 32-bit signed int |
| `UnsignedMSB8` / `UnsignedLSB8` | `NC_UINT64` | 64-bit unsigned int |
| `SignedMSB8` / `SignedLSB8` | `NC_INT64` | 64-bit signed int |
| `ASCII_Real` | `NC_DOUBLE` | ASCII table numeric field |
| `ASCII_Integer` | `NC_INT64` | ASCII table integer field |
| `ASCII_String` | `NC_CHAR` | ASCII table string field |

---

## Implementation Status

As of v2.2.0 Sprint 5:
- `Identification_Area` and `Observation_Area` are mapped to global string attributes.
- Each `File_Area_Observational` becomes a child group named from `File/file_name`.
- `Array` and `Array_2D_Image` metadata (dimensions, variable, and type) are read.
- `Table_Binary`, `Table_Character`, and `Table_Delimited` metadata are read:
  - A `record` dimension is created with length from `<records>`.
  - Each `Field_*` becomes a 1-D variable with dimension `[record]` (or 2-D `[record, strlen]` for NC_CHAR string fields).
  - Field `unit` is mapped to a `units` attribute on the variable.
  - Field names with spaces are sanitized (spaces replaced with underscores).
  - Unsupported field types are silently skipped.
- Data reading (`nc_get_vara`) is deferred to Sprint 6.

### Table mapping details

| PDS4 table element | Record element | Field element |
|---|---|---|
| `Table_Binary` | `Record_Binary` | `Field_Binary` |
| `Table_Character` | `Record_Character` | `Field_Character` |
| `Table_Delimited` | `Record_Delimited` | `Field_Delimited` |

### Additional ASCII type mappings (Sprint 5)

| PDS4 `data_type` | `nc_type` | Notes |
|---|---|---|
| `ASCII_NonNegative_Integer` | `NC_UINT64` | Unsigned integer |
| `ASCII_Boolean` | `NC_UBYTE` | Boolean flag |
| `ASCII_Date` | `NC_CHAR` | Date string |
| `ASCII_Date_Time_YMD` | `NC_CHAR` | Date-time string |
| `ASCII_Date_Time_YMD_UTC` | `NC_CHAR` | UTC date-time string |
| `UTF8_String` | `NC_CHAR` | Unicode string |

## libxml2 Usage in NEP

NEP uses **libxml2** (`libxml2-dev` on Ubuntu) to parse PDS4 XML labels.
Key API calls:

```c
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* Parse a file */
xmlDoc *doc = xmlReadFile(path, NULL, XML_PARSE_NONET);

/* Walk the tree */
xmlNode *root = xmlDocGetRootElement(doc);
const xmlChar *ns = root->ns ? root->ns->href : NULL;

/* Free */
xmlFreeDoc(doc);
xmlCleanupParser();
```

The PDS4 namespace URI is `http://pds.nasa.gov/pds4/pds/v1`.  The handler
should reject files whose root element namespace does not match.

---

## NEP UDF Slot Assignment

| Slot | Format |
|---|---|
| UDF0 | GeoTIFF BigTIFF |
| UDF1 | GeoTIFF standard TIFF |
| UDF2 | GRIB2 |
| UDF3 | FITS |
| UDF4 | NASA CDF |
| **UDF5** | **PDS4** |
| UDF6–UDF9 | Reserved |

---

## Standards References

- PDS4 Standards Reference: https://pds.nasa.gov/datastandards/documents/current-version/
- PDS4 Information Model: https://pds.nasa.gov/datastandards/schema/released/
- PDS4 Data Dictionary: https://pds.nasa.gov/datastandards/dictionaries/
- libxml2 API documentation: https://gnome.pages.gitlab.gnome.org/libxml2/devhelp/libxml2.devhelp2

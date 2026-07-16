# PDS4-to-NetCDF Mapping

This page describes how the NEP PDS4 reader maps the Planetary Data System
version 4 (PDS4) information model to the NetCDF-4 (enhanced) data model.

## Overview

A PDS4 `Product_Observational` label is an XML file that describes one or more
data objects (arrays, tables) stored in companion binary or text data files.
The NEP PDS4 UDF handler (`ncpds4`, registered as UDF5) parses the XML label
and constructs a read-only, in-memory NetCDF-4 group/variable/attribute
hierarchy that applications access through the standard `nc_open()` / 
`nc_get_vara_*()` API.

## Group Structure

```
root group (ncid)
├── global attributes (from Identification_Area, Observation_Area)
└── <data_file_name> group
    ├── group attributes (table_name, etc.)
    ├── dimensions (record, strlen, rep, array axes)
    └── variables (one per field or array)
```

- **Root group**: Contains global string attributes extracted from
  `Identification_Area` and `Observation_Area`.
- **File group**: One child group per `File_Area_Observational`, named from
  `<File><file_name>`. All arrays and tables within that file area share this
  group.

## Global Attributes

Extracted from the PDS4 XML label and stored as `NC_CHAR` string attributes
on the root group:

| PDS4 Element | NetCDF Attribute Name |
|---|---|
| `Identification_Area/logical_identifier` | `logical_identifier` |
| `Identification_Area/version_id` | `version_id` |
| `Identification_Area/title` | `title` |
| `Identification_Area/Citation_Information/author_list` | `author_list` |
| `Identification_Area/Citation_Information/description` | `description` |
| `Observation_Area/Target_Identification/name` | `target_name` |
| `Observation_Area/Investigation_Area/name` | `investigation_name` |
| `Observation_Area/Observing_System/name` | `observing_system` |
| `Observation_Area/Time_Coordinates/start_date_time` | `start_date_time` |
| `Observation_Area/Time_Coordinates/stop_date_time` | `stop_date_time` |

Additional label-level metadata may also be captured depending on the
label content.

## Array Objects

PDS4 array objects (`Array`, `Array_1D`, `Array_2D`, `Array_2D_Image`,
`Array_2D_Spectrum`, `Array_3D`, `Array_3D_Image`, etc.) are all dispatched to the same generic
array reader, which handles any number of axes:

| PDS4 | NetCDF |
|---|---|
| `<axes>` count | Number of dimensions |
| Each `<Axis_Array>` → `<axis_name>`, `<elements>` | Named dimension with size `elements` |
| `<Element_Array>/<data_type>` | Variable type (see Type Mapping below) |
| `<offset>` | Internal byte offset for `fseek()` data reading |
| `<Element_Array>/<scaling_factor>` | `scaling_factor` string attribute on variable |
| `<Element_Array>/<value_offset>` | `value_offset` string attribute on variable |

The variable is named from the array's `<name>` element, or defaults to
`"data"` if no `<name>` is provided. Axis ordering in the NetCDF variable
follows the `<sequence_number>` order of `<Axis_Array>` elements (ascending),
with sequence 1 as the slowest-varying (outermost) dimension in C order.

**Example**: An `Array_2D_Image` with `Line=512, Sample=256` becomes:
```
dimensions:
  Line = 512 ;
  Sample = 256 ;
variables:
  float data(Line, Sample) ;
```

**Example**: An `Array_3D_Image` with `Band=3, Line=1200, Sample=1648`,
`SignedMSB2` type and `scaling_factor=5.0e-06` becomes:
```
dimensions:
  Band = 3 ;
  Line = 1200 ;
  Sample = 1648 ;
variables:
  short data(Band, Line, Sample) ;
    data:scaling_factor = "5.0e-06" ;
    data:value_offset = "0.0" ;
```

Note: `scaling_factor` and `value_offset` are stored as raw `NC_CHAR` string
attributes preserving the literal value from the label. They are not
automatically applied during data reads (no CF `scale_factor` / `add_offset`
conversion).

## Table Objects

Tables (`Table_Binary`, `Table_Character`, `Table_Delimited`) are all
handled with a common pattern:

| PDS4 | NetCDF |
|---|---|
| `<records>` | `record` dimension (one per table) |
| Each `<Field_Binary>` / `<Field_Character>` / `<Field_Delimited>` | One variable |
| `<name>` | Variable name (spaces → underscores) |
| `<data_type>` | Variable type |
| `<unit>` | `units` attribute on the variable |

### Scalar Fields

Each scalar (non-grouped) field becomes a **1D variable** dimensioned by
`record`:

```
dimensions:
  record = 100 ;
variables:
  float TANGENT_ALT(record) ;
    TANGENT_ALT:units = "km" ;
```

### String Fields (NC_CHAR)

Fields with character/string data types (e.g., `ASCII_String`,
`ASCII_Date_Time`) become **2D variables** `[record, <name>_strlen]`:

```
dimensions:
  record = 1 ;
  PRODUCT_ID_strlen = 56 ;
variables:
  char PRODUCT_ID(record, PRODUCT_ID_strlen) ;
```

### Group Fields (Group_Field_Binary)

`Group_Field_Binary` elements contain repeated (vector) fields. NEP supports
nesting up to depth 2.

#### Depth-1 (flat group)

Each field inside a single-level `Group_Field_Binary` becomes a **2D variable**
`[record, <name>_rep]`:

| PDS4 Group Element | NetCDF Mapping |
|---|---|
| `<repetitions>` | Trailing dimension size |
| `<group_location>` | Internal byte offset of group within record |
| `<group_length>` | Total byte length of all repetitions |
| `<Field_Binary>/<name>` | Variable name |

```
dimensions:
  record = 100 ;
  COLUMN_rep = 2 ;
  V_TANGENT_rep = 3 ;
variables:
  float COLUMN(record, COLUMN_rep) ;
    COLUMN:units = "1/(cm**2)" ;
  double V_TANGENT(record, V_TANGENT_rep) ;
    V_TANGENT:units = "km/s" ;
```

The repetition dimension is named `<field_name>_rep`. Data is read by
computing the byte offset for each repetition within the record:

```
offset = table_offset + record_index * record_length
       + group_location + rep_index * (group_length / repetitions)
       + inner_field_offset
```

#### Depth-2 (nested group)

When an outer `Group_Field_Binary` contains one or more inner
`Group_Field_Binary` elements (indicated by `<groups>` > 0), each leaf field
becomes a **3D variable** `[record, <name>_outer_rep, <name>_inner_rep]`:

| PDS4 Element | NetCDF Mapping |
|---|---|
| Outer `<repetitions>` | Second dimension (outer repetitions) |
| Inner `<repetitions>` | Third dimension (inner repetitions) |
| `<Field_Binary>/<name>` | Variable name |

```
dimensions:
  record = 12 ;
  ALT_outer_rep = 19 ;
  ALT_inner_rep = 3 ;
variables:
  float ALT(record, ALT_outer_rep, ALT_inner_rep) ;
```

The byte seek formula for depth-2 fields is:

```
offset = table_offset + record_index * record_length
       + outer_group_location + outer_rep * outer_group_length
       + inner_group_location + inner_rep * inner_group_length
       + field_offset
```

Depth-3 and deeper nesting is not supported and fields at that level are
skipped silently.

#### Variable Name Collision Resolution

When multiple tables in the same file group define fields with the same name
(common in products with many tables), the second and subsequent occurrences
are prefixed with the table's `<local_identifier>` to avoid collision:

```
first occurrence:  ALT            (from data_DENSITY)
second occurrence: data_TEMPERATURE_ALT
```

### Multiple Tables in One File Area

When a single `File_Area_Observational` contains multiple tables (common in
FITS-backed PDS4 products), all tables share the same file group. Each table
adds its own `record` dimension (potentially with a different length) and its
own set of variables. When field names collide across tables, the duplicate
is prefixed with the table's `<local_identifier>` (see Variable Name Collision
Resolution above).

## Data Type Mapping

| PDS4 Data Type | NetCDF Type | Size | Notes |
|---|---|---|---|
| `IEEE754MSBSingle` | `NC_FLOAT` | 4 | Big-endian, byte-swapped on LE hosts |
| `IEEE754LSBSingle` | `NC_FLOAT` | 4 | Little-endian |
| `IEEE754MSBDouble` | `NC_DOUBLE` | 8 | Big-endian |
| `IEEE754LSBDouble` | `NC_DOUBLE` | 8 | Little-endian |
| `SignedMSB2` | `NC_SHORT` | 2 | Big-endian |
| `SignedLSB2` | `NC_SHORT` | 2 | Little-endian |
| `UnsignedMSB2` | `NC_USHORT` | 2 | Big-endian |
| `UnsignedLSB2` | `NC_USHORT` | 2 | Little-endian |
| `SignedMSB4` | `NC_INT` | 4 | Big-endian |
| `SignedLSB4` | `NC_INT` | 4 | Little-endian |
| `UnsignedMSB4` | `NC_UINT` | 4 | Big-endian |
| `UnsignedLSB4` | `NC_UINT` | 4 | Little-endian |
| `SignedMSB8` | `NC_INT64` | 8 | Big-endian |
| `SignedLSB8` | `NC_INT64` | 8 | Little-endian |
| `UnsignedMSB8` | `NC_UINT64` | 8 | Big-endian |
| `UnsignedLSB8` | `NC_UINT64` | 8 | Little-endian |
| `UnsignedByte` | `NC_UBYTE` | 1 | |
| `ASCII_String`, `UTF8_String` | `NC_CHAR` | 1 | 2D: `[record, strlen]` |
| `ASCII_Date_Time*` | `NC_CHAR` | 1 | All date/time variants → `NC_CHAR` |
| `ASCII_Real` | `NC_DOUBLE` | — | Parsed from text (Table_Character/Delimited) |
| `ASCII_Integer` | `NC_INT` | — | Parsed from text |
| `ASCII_NonNegative_Integer` | `NC_INT64` | — | Parsed from text |

Byte-swapping is performed automatically based on the endianness encoded in
the PDS4 type name and the host platform's native byte order.

## Data File Resolution

The data file path is resolved relative to the XML label file's directory.
The `<File><file_name>` element provides the filename, which is joined with
the label's directory path. This supports:

- Raw binary files (`.img`, `.IMG`, `.dat`)
- CSV text files (`.csv`, `.tab`)
- FITS files (`.fits`) used as binary containers with absolute byte offsets
- VICAR/ODL-prefixed binary files (`.IMG`) — the label's `<offset>` element
  points past any embedded headers, so the reader seeks directly to the pixel
  data without parsing the VICAR or ODL header content

The reader opens the data file with `fopen()` and uses `fseek()` to the
byte offsets specified in the label. This means **any file format can serve
as the data container** as long as the PDS4 label provides correct byte
offsets.

## Limitations

- **Read-only**: The PDS4 handler does not support `nc_create()` or write
  operations.
- **Nested Group_Field depth limit**: Nesting up to depth 2 is supported
  (outer group containing inner groups). Depth-3 and deeper nested groups
  are skipped silently.
- **No packed data**: PDS4 `Packed_Data_Fields` are not supported.
- **Single Product_Observational**: Only the first `Product_Observational`
  in a label is processed.
- **No schema validation**: The reader does not validate the XML against the
  PDS4 schema; it trusts the label structure.

## Tested Mission Data

The following real mission datasets have been validated:

| Mission | Instrument | Product | Data Container |
|---|---|---|---|
| Cassini-Huygens | HRD | Engineering on/off log | `.tab` (Table_Character) |
| MESSENGER | NS | Mercury thermal neutron map | `.img` (Array_2D_Map) |
| Deep Impact | LCS | Comet 9P photometry | `.tab` (Table_Character) |
| MAVEN | NGIMS | L1B housekeeping | `.csv` (Table_Delimited, 324 fields) |
| MAVEN | NGIMS | L3 science | `.csv` (Table_Delimited, 15 fields) |
| MAVEN | IUVS | L2 corona (FUV) | `.fits` (Table_Binary, 8 tables, depth-1 Group_Field_Binary) |
| MAVEN | IUVS | L2 periapse | `.fits` (Table_Binary, multiple tables, depth-2 nested Group_Field_Binary) |
| Mars 2020 / Perseverance | Mastcam-Z (ZCAM) | Sol 1738 calibrated radiance image | `.IMG` (Array_3D_Image, Band=3 × Line=1200 × Sample=1648, `SignedMSB2`) |
| Mars 2020 / Perseverance | Mastcam-Z (ZCAM) | Sol 1737 calibrated radiance image | `.IMG` (Array_3D_Image, Band=3 × Line=1200 × Sample=1648, `SignedMSB2`) |
| New Horizons | Alice ultraviolet imaging spectrograph | Jupiter encounter partially processed pixel-list product | `.lblx` label + `.fit` container (Array_2D_Spectrum and Table_Binary) |
| New Horizons | Alice ultraviolet imaging spectrograph | KEM1 calibrated histogram/PHD/housekeeping product | `.lblx` label + `.fit` container (Array_2D_Spectrum, Array_1D, and Table_Binary) |
| New Horizons | Alice ultraviolet imaging spectrograph | Pluto encounter compressed histogram/PHD/housekeeping product | `.lblx` label + `.fit` container (Array_2D_Spectrum, Array_1D, and Table_Binary) |
| New Horizons | Alice ultraviolet imaging spectrograph | Launch/partially processed histogram/PHD/wavelength/count-rate/housekeeping product | `.lblx` label + `.fit` container (Array_2D_Spectrum and Table_Binary) |

## PDS4 Tests

The PDS4 reader tests are in `test/tst_pds4_udf.c` and run when PDS4 support is enabled (`--enable-pds4` / `-DNEP_ENABLE_PDS4=ON`).

| Test Name | Description | Mission / Source | Data Type |
|---|---|---|---|
| `test_table_binary()` | Verifies metadata for a synthetic binary table: 5-record dimension, three variables (`Timestamp`, `Detector_Counts`, `Temperature`) with units | Synthetic test data | `Table_Binary` |
| `test_table_character()` | Verifies metadata for a fixed-width character table: 224-record dimension, three `NC_DOUBLE` variables (`Wavelength`, `Reflectance`, `Error`) | Synthetic test data | `Table_Character` |
| `test_array_data_read()` | Reads a 4×4 float32 image array and verifies all 16 values and a sub-hyperslab `[1:2,1:2]` | Synthetic test data | Array (`Array_2D_Image`) |
| `test_table_binary_data_read()` | Reads binary table field values: `Timestamp[0..4]`, `Detector_Counts[0..4]`, `Temperature[0..4]` | Synthetic test data | `Table_Binary` |
| `test_table_character_data_read()` | Reads fixed-width character table values: `Wavelength[0..2]`, `Reflectance[0..2]`, `Error[0..2]` | Synthetic test data | `Table_Character` |
| `test_mission_cassini_hrd()` | Opens real HRD engineering table; verifies 11 records, two `NC_CHAR` fields (`ON_OFF_TIME`, `ON_OFF_FLAG`) | Cassini-Huygens HRD dust detector | `Table_Character` |
| `test_mission_messenger_tnmap()` | Opens real thermal neutron map; verifies 360×720 `NC_UBYTE` array and reads a hyperslab | MESSENGER Mercury orbiter | Binary array (`Array_2D_Map`) |
| `test_mission_lcs_9p()` | Opens real comet photometry table; verifies 8 variables (`ASCII_Integer`, `ASCII_Real`) and first-record values | Deep Impact LCS, 9P/Tempel comet | `Table_Character` |
| `test_mission_maven_l1b()` | Opens real L1B housekeeping table; verifies 324 variables, 26121 records, `TIME` is `NC_DOUBLE`, and `TIME[0..1] > 0` | MAVEN NGIMS, Mars atmosphere | `Table_Delimited` (324 fields, 9.3 MB CSV) |
| `test_mission_maven_l3()` | Opens real L3 science table; verifies 15 variables, 2 records, `T_UTC` is `NC_CHAR` (`ASCII_Date_Time`), and known values for `T_UNIX[0]`, `SCALE_HEIGHT[0]`, `TEMPERATURE[0]` | MAVEN NGIMS, Mars atmosphere | `Table_Delimited` (15 fields, 446 B CSV) |
| `test_mission_maven_iuvs_metadata()` | Opens FITS-backed PDS4 file; verifies 116 variables (scalar + `Group_Field_Binary`), `COLUMN` is 2D with trailing dim=2, `V_TANGENT` trailing dim=3, `TANGENT_ALT` is 1D `NC_FLOAT` | MAVEN IUVS, Mars corona | `Table_Binary` (8 tables, FITS container, `Group_Field_Binary`) |
| `test_mission_maven_iuvs_data()` | Reads scalar field `TANGENT_ALT[0]` (finite > 0 km) and group field `RADIANCE[50,0..1]` (finite > 0 kR), confirming FITS-as-container I/O and 2D group field reading | MAVEN IUVS, Mars corona | `Table_Binary` (FITS container, 181 KB) |
| `test_mission_maven_periapse_metadata()` | Opens FITS-backed PDS4 file; verifies nested `Group_Field_Binary` (depth-2), table-prefixed field names, `DENSITY_ALT` dims `[12,19,3]`, and `GEOMETRY_RETRIEVAL` scalar doubles | MAVEN IUVS, Mars periapse | `Table_Binary` (FITS container, nested groups) |
| `test_mission_maven_periapse_data()` | Reads scalar `LAT[0]` from `GEOMETRY_RETRIEVAL` and 3D nested group field `DENSITY_ALT[0,0,0]` (finite > 0) | MAVEN IUVS, Mars periapse | `Table_Binary` (nested groups) |
| `test_mission_perseverance_mastcamz_metadata()` | Opens Perseverance Sol 1738 `Array_3D_Image`; verifies `data` is `NC_SHORT`, dims `[3,1200,1648]`, and `scaling_factor="5.0e-06"` | Mars 2020 / Perseverance | `Array_3D_Image` (VICAR/ODL `.IMG`) |
| `test_mission_perseverance_mastcamz_data()` | Reads pixel `[0,0,0]` as `NC_SHORT` and checks valid `SignedMSB2` range | Mars 2020 / Perseverance | `Array_3D_Image` |
| `test_mission_perseverance_mastcamz_1737_metadata()` | Same metadata checks for the second Perseverance Sol 1737 `Array_3D_Image` product | Mars 2020 / Perseverance | `Array_3D_Image` |
| `test_mission_perseverance_mastcamz_1737_data()` | Reads pixel `[0,0,0]` from Sol 1737 and checks valid `SignedMSB2` range | Mars 2020 / Perseverance | `Array_3D_Image` |
